let
  pkgs = import (builtins.fetchTarball
    "https://pm.bsc.es/gitlab/rarias/bscpkgs/-/archive/master/bscpkgs-master.tar.gz");

  rWrapper = pkgs.rWrapper.override {
    packages = with pkgs.rPackages; [ tidyverse rjson jsonlite egg viridis ];
  };

  # Recursively set MPI
  bsc = pkgs.bsc.extend (last: prev: {

    include-what-you-use = let
      gcc = pkgs.gcc;
      targetConfig = pkgs.stdenv.targetPlatform.config;
    in pkgs.wrapCCWith rec {
      cc = pkgs.include-what-you-use;
      extraBuildCommands = ''
        echo "--gcc-toolchain=${gcc}" >> $out/nix-support/cc-cflags
        echo "-B${gcc.cc}/lib/gcc/${targetConfig}/${gcc.version}" >> $out/nix-support/cc-cflags
        echo "-isystem${gcc.cc}/lib/gcc/${targetConfig}/${gcc.version}/include" >> $out/nix-support/cc-cflags
        wrap include-what-you-use $wrapper $ccPath/include-what-you-use
        substituteInPlace "$out/bin/include-what-you-use" --replace 'dontLink=0' 'dontLink=1'
      '';
    };

    ovni-local = last.callPackage ./ovni.nix { };

    # Build ovni with old gcc versions
    genOldOvni = stdenv: (last.ovni-local.override {
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

    ovni-nompi = last.ovni-local.overrideAttrs (old: {
      buildInputs = pkgs.lib.filter (x: x != last.mpi ) old.buildInputs;
      cmakeFlags = old.cmakeFlags ++ [ "-DUSE_MPI=OFF" ];
    });

    # Now we rebuild ovni with the Nanos6 and nOS-V versions, which were
    # linked to the previous ovni. We need to be able to exit the chroot
    # to run Nanos6 tests, as they require access to /sys for hwloc
    ovni-rt = (last.ovni-local.override {
      stdenv = last.stdenvClangOmpss2;
    }).overrideAttrs (old: {
      __noChroot = true;
      buildInputs = old.buildInputs ++ [
        last.include-what-you-use
        pkgs.gdb
        last.nosv
        last.nanos6
        last.nodes
        pkgs.strace
      ];
      preConfigure = ''
        export NODES_HOME="${last.nodes}"
        export NANOS6_HOME="${last.nanos6}"
      '';
    });

    ovni-asan = last.ovni-rt.overrideAttrs (old: {
      cmakeFlags = old.cmakeFlags ++ [ "-DCMAKE_BUILD_TYPE=Asan" ];
      # Ignore leaks in tests for now, only check memory errors
      preCheck = old.preCheck + ''
        export ASAN_OPTIONS=detect_leaks=0
      '';
    });

  });

in
  pkgs // { bsc = bsc; }
