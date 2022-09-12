let
  pkgs = import (builtins.fetchTarball
    "https://pm.bsc.es/gitlab/rarias/bscpkgs/-/archive/update-bscpkgs/bscpkgs-master.tar.gz");

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
        ref = "nanos6-emu-with-tests";
        rev = "3d39b8cc544140727c83a066a8fca785aff21965";
      };
    });

    nanos6 = (prev.nanos6Git.override {
      gitUrl = "ssh://git@bscpm03.bsc.es/nanos6/forks/nanos6-extern-001.git";
      gitBranch = "ovni_instr";
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
            # Master at 2022-07-26
            rev = "d3d4f2bf231b9461a5881c5bf56659516d45e670";
            shallow = true;
          };
          version = "d3d4f2bf";
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
        pkgs.strace
      ];
      cmakeFlags = old.cmakeFlags ++ [ "-DENABLE_TEST_RT=ON" ];
    });
  });

in
  bsc.ovni-rt
