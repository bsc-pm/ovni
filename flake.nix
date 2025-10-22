{
  inputs.jungle.url = "git+https://jungle.bsc.es/git/rarias/jungle";

  nixConfig.bash-prompt = "\[nix-develop\]$ ";

  outputs = { self, jungle }:
  let
    # Set to true to replace all libovni in all runtimes with the current
    # source. Causes large rebuilds on changes of ovni.
    useLocalOvni = false;

    ovniOverlay = final: prev: {
      nosv = prev.nosv.override {
        useGit = true;
        gitBranch = "master";
        gitCommit = "badfdec9b438c2d4a0989c0a9ff5513223a8ed85";
      };
      nanos6 = prev.nanos6.override {
        useGit = true;
        gitBranch = "master";
        gitCommit = "f39ea57c67a613d098050e2bb251116a021e91e5";
      };
      nodes = prev.nodes.override {
        useGit = true;
        gitBranch = "master";
        gitCommit = "2fc2fd686a6c0f831323ff47c15480688f9b21ea";
      };
      clangOmpss2Unwrapped = prev.clangOmpss2Unwrapped.override {
        useGit = true;
        gitBranch = "master";
        gitCommit = "872ba63f86edaefc9787984ef3fae9f2f94e0124";
      };
      mpi = prev.mpich;
      bench6 = prev.bench6.overrideAttrs (old: rec {
        src = builtins.fetchGit {
          url = "ssh://git@bscpm04.bsc.es/rarias/bench6.git";
          ref = "master";
          rev = "14227b1aa5a17deb5b746eb9648de2eb89fe3521";
        };
        version = src.shortRev;
        cmakeFlags = [ "-DCMAKE_C_COMPILER=clang" "-DCMAKE_CXX_COMPILER=clang++" ];
        env = with final; {
          NANOS6_HOME = nanos6;
          NODES_HOME = nodes;
          NOSV_HOME = nosv;
        };
        buildInputs = with final; [ bigotes cmake clangOmpss2 openmp openmpv
          nanos6 nodes nosv mpi tampi tagaspi gpi-2 openblas openblas.dev ovni
        ];
        hardeningDisable = [ "all" ];
        dontStrip = true;
      });

      # Use a fixed commit for libovni
      ovniFixed = prev.ovni.override {
        useGit = true;
        gitBranch = "master";
        gitCommit = "3bbfe0f0ecdf58e3f46ebafdf2540680f990b76b";
      };
      # Build with the current source
      ovniLocal = prev.ovni.overrideAttrs (old: rec {
        pname = "ovni-local";
        version = if self ? shortRev then self.shortRev else "dirty";
        src = self;
        cmakeFlags = [ "-DOVNI_GIT_COMMIT=${version}" ];
      });
      # Select correct ovni for libovni
      ovni = if (useLocalOvni) then final.ovniLocal else final.ovniFixed;
    };
    pkgs = import jungle.inputs.nixpkgs {
      system = "x86_64-linux";
      overlays = [
        jungle.bscOverlay
        ovniOverlay
      ];
    };
    compilerList = with pkgs; [
      #gcc49Stdenv # broken
      #gcc6Stdenv # deprecated
      #gcc7Stdenv # deprecated
      #gcc8Stdenv # deprecated
      gcc9Stdenv
      gcc10Stdenv
      gcc11Stdenv
      gcc12Stdenv
      gcc13Stdenv
      gcc14Stdenv
    ];
    lib = pkgs.lib;
  in {
    packages.x86_64-linux.ovniPackages = {
      # Allow inspection of packages from the command line
      inherit pkgs;
    } // rec {
      # Build with the current source
      local = pkgs.ovniLocal;

      # Build in Debug mode
      debug = local.overrideAttrs (old: {
        pname = "ovni-debug";
        cmakeBuildType = "Debug";
      });

      # Without MPI
      nompi = local.overrideAttrs (old: {
        pname = "ovni-nompi";
        buildInputs = lib.filter (x: x != pkgs.mpi ) old.buildInputs;
        cmakeFlags = old.cmakeFlags ++ [ "-DUSE_MPI=OFF" ];
      });

      # Helper function to build with different compilers
      genOvniCC = stdenv: (local.override {
        stdenv = stdenv;
      }).overrideAttrs (old: {
        name = "ovni-gcc" + stdenv.cc.cc.version;
        cmakeFlags = old.cmakeFlags ++ [
          "-DCMAKE_INTERPROCEDURAL_OPTIMIZATION=OFF"
        ];
      });

      # Test several gcc versions
      compilers = let
        drvs = map genOvniCC compilerList;
      in pkgs.runCommand "ovni-compilers" { } ''
        printf "%s\n" ${builtins.toString drvs} > $out
      '';

      # Enable RT tests
      rt = (local.override {
        # Provide the clang compiler as default
        stdenv = pkgs.stdenvClangOmpss2;
      }).overrideAttrs (old: {
        pname = "ovni-rt";
        # We need to be able to exit the chroot to run Nanos6 tests, as they
        # require access to /sys for hwloc
        __noChroot = true;
        buildInputs = old.buildInputs ++ (with pkgs; [
          pkg-config nosv nanos6 nodes openmpv bench6
        ]);
        cmakeFlags = old.cmakeFlags ++ [ "-DENABLE_ALL_TESTS=ON" ];
        preConfigure = old.preConfigure or "" + ''
          export NOSV_HOME="${pkgs.nosv}"
          export NODES_HOME="${pkgs.nodes}"
          export NANOS6_HOME="${pkgs.nanos6}"
          export OPENMP_RUNTIME="libompv"
        '';
      });

      # Build with ASAN and pass RT tests
      asan = rt.overrideAttrs (old: {
        pname = "ovni-asan";
        cmakeFlags = old.cmakeFlags ++ [ "-DCMAKE_BUILD_TYPE=Asan" ];
        # Ignore leaks in tests for now, only check memory errors
        preCheck = old.preCheck + ''
          export ASAN_OPTIONS=detect_leaks=0
        '';
      });

      ubsan = rt.overrideAttrs (old: {
        pname = "ovni-ubsan";
        cmakeFlags = old.cmakeFlags ++ [ "-DCMAKE_BUILD_TYPE=Ubsan" ];
      });

      armv7 = (pkgs.pkgsCross.armv7l-hf-multiplatform.ovniLocal.overrideAttrs (old: {
        pname = "ovni-armv7";
        buildInputs = [];
        nativeBuildInputs = [ pkgs.pkgsCross.armv7l-hf-multiplatform.buildPackages.cmake ];
        cmakeFlags = old.cmakeFlags ++ [ "-DUSE_MPI=OFF" ];
      })).overrideDerivation (old: {
        doCheck = true;
      });

      aarch64 = (pkgs.pkgsCross.aarch64-multiplatform.ovniLocal.overrideAttrs (old: {
        pname = "ovni-aarch64";
        buildInputs = [];
        nativeBuildInputs = [ pkgs.pkgsCross.aarch64-multiplatform.buildPackages.cmake ];
        cmakeFlags = old.cmakeFlags ++ [ "-DUSE_MPI=OFF" ];
      })).overrideDerivation (old: {
        doCheck = true;
      });

      riscv64 = (pkgs.pkgsCross.riscv64.ovniLocal.overrideAttrs (old: {
        pname = "ovni-riscv64";
        buildInputs = [];
        nativeBuildInputs = [ pkgs.pkgsCross.riscv64.buildPackages.cmake ];
        cmakeFlags = old.cmakeFlags ++ [ "-DUSE_MPI=OFF" ];
      })).overrideDerivation (old: {
        doCheck = true;
      });
    };
  };
}
