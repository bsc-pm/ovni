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

    nanos6 = (prev.nanos6Git.override {
      gitUrl = "ssh://git@bscpm03.bsc.es/nanos6/forks/nanos6-extern-001.git";
      gitBranch = "ovni_instr";
      extrae = null;
    }).overrideAttrs (old: {
      buildInputs = old.buildInputs ++ [ last.ovni ];
      patches = [ ./0001-Emit-a-fill-event-at-shutdown.patch ];
      configureFlags = old.configureFlags ++ [
        "--with-ovni=${last.ovni}"
      ];
    });

    # Quick hack, as we only need the libovni runtime to match ours
    nanos6' = /nix/store/zg989jl3mgdps7amdskna43hipb6snsq-nanos6-60fc5f2;

    ompss2 = {
      clangUnwrapped = prev.clangOmpss2Unwrapped.overrideAttrs (
        old:
        rec {
          src = ../kk/llvm-mono-d3d4f2bf231b9461a5881c5bf56659516d45e670.tar.bz2;
          #src = fetchTarball {
          #  url = ../kk/llvm-mono-d3d4f2bf231b9461a5881c5bf56659516d45e670.tar.bz2;
          #};
          #builtins.fetchTree {
          #  type = "git";
          #  url = "ssh://git@bscpm03.bsc.es/llvm-ompss/llvm-mono.git";
          #  ref = "master";
          #  # Master at 2022-07-26
          #  rev = "d3d4f2bf231b9461a5881c5bf56659516d45e670";
          #  shallow = true;
          #};
          version = "d3d4f2bf";
        }
      );

      #clangUnwrapped = /nix/store/fg621rqj50x85gnsbh1pj304049yqlaq-clang-ompss2-d3d4f2b;

      clang = prev.clangOmpss2.override {
        clangOmpss2Unwrapped = last.ompss2.clangUnwrapped;
      };

      #clang = /nix/store/qva7b665inxgg8wrfl2jf9dwzdp69sxq-clang-ompss2-wrapper-d3d4f2b;

      stdenv = pkgs.overrideCC pkgs.llvmPackages_11.stdenv bsc.ompss2.clang;
    };

    nosv = pkgs.stdenv.mkDerivation rec {
      pname = "nosv";
      version = src.shortRev;
      buildInputs = with pkgs; [ autoreconfHook pkg-config numactl last.ovni ];
      configureFlags = [ "--with-ovni=${last.ovni}" ];
      dontStrip = true;
      src = builtins.fetchGit {
        url = "ssh://git@gitlab-internal.bsc.es/nos-v/nos-v.git";
        ref = "master";
      };
    };

    # Quick fix to avoid rebuilding every time the ovni source changes.
    # Use this nosv' version below as dependency of ovni-rt
    nosv' = /nix/store/rvnrbc7ibpw06jdilz6mha7szzxcr2mi-nosv-8936f3e;

    ovni = last.callPackage ./ovni.nix { };

    ovni-rt = (last.ovni.override {
      stdenv = last.ompss2.stdenv;
    }).overrideAttrs (old: {
      __impure = true;
      __noChroot = true;
      buildInputs = old.buildInputs ++ [
        last.nosv'
        pkgs.strace
      ];
      cmakeFlags = old.cmakeFlags ++ [ "-DBUILD_RT_TESTING=ON" ];
    });
  });

in
  bsc.ovni-rt
  #bsc.ompss2.clang
