{
  inputs.nixpkgs.url = "nixpkgs";
  inputs.bscpkgs.url = "git+https://git.sr.ht/~rodarima/bscpkgs";
  inputs.bscpkgs.inputs.nixpkgs.follows = "nixpkgs";

  nixConfig.bash-prompt = "\[nix-develop\]$ ";

  outputs = { self, nixpkgs, bscpkgs }:
  let
    # Set to true to replace all libovni in all runtimes with the current
    # source. Causes large rebuilds on changes of ovni.
    useLocalOvni = false;

    ovniOverlay = final: prev: {
      nosv = prev.nosv.override {
        useGit = true;
        gitBranch = "master";
        gitCommit = "af186f448d3b5675c107747f5c9237e7f4d31e4b";
      };
      nanos6 = prev.nanos6.override {
        useGit = true;
        gitBranch = "master";
        gitCommit = "21fccec383a4136daf5919093a6ffcdc8c139bfe";
      };
      nodes = prev.nodes.override {
        useGit = true;
        gitBranch = "master";
        gitCommit = "70ce0ed0a20842d8eb3124aa5db5916fb6fc238f";
      };
      clangOmpss2Unwrapped = prev.clangOmpss2Unwrapped.override {
        useGit = true;
        gitBranch = "master";
        gitCommit = "b813108e2810c235480688ed7d1b0f1faf76e804";
      };

      # Use a fixed commit for libovni
      ovniFixed = prev.ovni.override {
        useGit = true;
        gitBranch = "master";
        gitCommit = "68fc8b0eba299c3a7fa3833ace2c94933a26749e";
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
    pkgs = import nixpkgs {
      system = "x86_64-linux";
      overlays = [
        bscpkgs.bscOverlay
        ovniOverlay
      ];
    };
    compilerList = with pkgs; [
      #gcc49Stdenv # broken
      gcc6Stdenv
      gcc7Stdenv
      gcc8Stdenv
      gcc9Stdenv
      gcc10Stdenv
      gcc11Stdenv
      gcc12Stdenv
      gcc13Stdenv
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
        buildInputs = old.buildInputs ++ (with pkgs; [ pkg-config nosv nanos6 nodes openmpv ]);
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
    };
  };
}
