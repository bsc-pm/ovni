let
  pkgs = import (builtins.fetchTarball
    "https://pm.bsc.es/gitlab/rarias/bscpkgs/-/archive/master/bscpkgs-master.tar.gz");

  rWrapper = pkgs.rWrapper.override {
    packages = with pkgs.rPackages; [ tidyverse rjson jsonlite egg viridis ];
  };

  # Recursively set MPI
  bsc = pkgs.bsc.extend (last: prev: {
    mpi = last.impi;
    #mpi = last.openmpi;

    ovni = last.callPackage ./ovni.nix { };

    # Use a fixed version to compile Nanos6 and nOS-V, so we don't need to
    # rebuild them when ovni changes. We only need to maintain the
    # compatibility in the versions of ovni traces, which will be
    # checked by the emulator anyway.
    ovniFixed = last.ovni.overrideAttrs (old: {
      src = builtins.fetchGit {
        url = "ssh://git@bscpm03.bsc.es/rarias/ovni.git";
        ref = "master";
        rev = "e47cf8fe22f79d9315e057376b4166013c2217a4";
      };
    });

    nanos6 = (prev.nanos6Git.override {
      gitUrl = "ssh://git@bscpm03.bsc.es/nanos6/nanos6.git";
      gitBranch = "master";
      # Master at 2022-11-14
      #gitCommit = "a6f88173b7d849a453f35029cb0fbee73e8685da";
      extrae = null;
    }).overrideAttrs (old: {
      buildInputs = old.buildInputs ++ [ last.ovniFixed ];
      configureFlags = old.configureFlags ++ [
        "--with-ovni=${last.ovniFixed}"
      ];
    });

    ompss2 = {
      # We need a recent clang to avoid silent ABI incompatible changes...
      clangUnwrapped = prev.clangOmpss2Unwrapped.overrideAttrs (
        old:
        rec {
          src = builtins.fetchTree {
            type = "git";
            url = "ssh://git@bscpm03.bsc.es/llvm-ompss/llvm-mono.git";
            ref = "master";
            # Master at 2022-11-14
            rev = "58f311bf028d344ebd6072268bd21d33867d1466";
            shallow = true;
          };
          version = "58f311bf";
        }
      );

      clang = prev.clangOmpss2.override {
        clangOmpss2Unwrapped = last.ompss2.clangUnwrapped;
      };

      stdenv = pkgs.overrideCC pkgs.llvmPackages_11.stdenv bsc.ompss2.clang;
    };

    nosv = pkgs.stdenv.mkDerivation rec {
      pname = "nosv";
      version = src.shortRev;
      buildInputs = with pkgs; [ autoreconfHook pkg-config numactl last.ovniFixed ];
      configureFlags = [ "--with-ovni=${last.ovniFixed}" ];
      dontStrip = true;
      src = builtins.fetchGit {
        url = "ssh://git@gitlab-internal.bsc.es/nos-v/nos-v.git";
        ref = "master";
      };
    };

    nodes = (prev.nanos6Git.override {
      gitUrl = "ssh://git@gitlab-internal.bsc.es/nos-v/nodes.git";
      gitBranch = "master";
      #gitCommit = "8bfbc2dbd8b09b6578aa5c07e57bffedcf5568af";
      extrae = null;
    }).overrideAttrs (old: {
      name = "nodes";
      configureFlags = old.configureFlags ++ [
        "--with-ovni=${last.ovniFixed}"
        "--with-nosv=${last.nosv}"
      ];
    });

    # Build ovni with old gcc versions
    genOldOvni = stdenv: (last.ovni.override {
      stdenv = stdenv;
    }).overrideAttrs (old: {
      pname = old.name + "@" + stdenv.cc.cc.version;
      cmakeFlags = old.cmakeFlags ++ [
        "-DCMAKE_INTERPROCEDURAL_OPTIMIZATION=OFF"
      ];
    });

    oldCompilers = [
      #pkgs.gcc49Stdenv # broken
      pkgs.gcc6Stdenv
      pkgs.gcc7Stdenv
      pkgs.gcc8Stdenv
      pkgs.gcc9Stdenv
      pkgs.gcc10Stdenv
      pkgs.gcc11Stdenv
      pkgs.gcc12Stdenv
    ];

    oldOvnis = map last.genOldOvni last.oldCompilers;

    genOldOvniNoLTO = stdenv: (last.genOldOvni stdenv).overrideAttrs (old: {
      cmakeFlags = old.cmakeFlags ++ [
        "-DCMAKE_INTERPROCEDURAL_OPTIMIZATION=OFF"
      ];
    });

    oldOvnisNoLTO = map last.genOldOvniNoLTO last.oldCompilers;

    genOldOvniRelease = stdenv: (last.genOldOvni stdenv).overrideAttrs (old: {
      cmakeBuildType = "Release";
    });

    oldOvnisRelease = map last.genOldOvniRelease last.oldCompilers;

    # Now we rebuild ovni with the Nanos6 and nOS-V versions, which were
    # linked to the previous ovni. We need to be able to exit the chroot
    # to run Nanos6 tests, as they require access to /sys for hwloc
    ovni-rt = (last.ovni.override {
      stdenv = last.ompss2.stdenv;
    }).overrideAttrs (old: {
      __noChroot = true;
      buildInputs = old.buildInputs ++ [
        pkgs.gdb
        last.nosv
        last.nanos6
        last.nodes
        pkgs.strace
      ];
      preConfigure = ''
        export NODES_HOME="${last.nodes}"
      '';
    });
  });

in
  pkgs // { bsc = bsc; }
