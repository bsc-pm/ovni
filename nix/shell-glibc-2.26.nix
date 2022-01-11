{
  pkgs ? import (
    builtins.fetchTarball {
      # Use nixpkgs 18.03 with old glibc and cmake
      url = "https://github.com/NixOS/nixpkgs/archive/3e1be2206b4c1eb3299fb633b8ce9f5ac1c32898.tar.gz";
    }
  ) {}

# By default use debug
, enableDebug ? true
}:

with pkgs;

{ 
  ovni = stdenv.mkDerivation rec {
    name = "ovni";

    buildInputs = [ cmake openmpi ];

    cmakeBuildType = if (enableDebug) then "Debug" else "Release";
    cmakeFlags = [ "-DCMAKE_SKIP_BUILD_RPATH=OFF" ];
    buildFlags = [ "VERBOSE=1" ];
    checkFlags = [ "CTEST_OUTPUT_ON_FAILURE=1" ];
    dontStrip = true;
    doCheck = true;
    checkTarget = "test";

    src = ../.;
  };

  shell = pkgs.mkShell {
    nativeBuildInputs = [ cmake openmpi ];
    NIX_ENFORCE_PURITY = 0;
    shellHook = ''
      set -e
      curdir=$(pwd)
      rm -rf build install
      mkdir build install
      cd build
      cmake -DCMAKE_BUILD_TYPE=Debug \
        -DENABLE_DEBUG_LOG=ON \
        -DCMAKE_INSTALL_PREFIX="$curdir/install" ..
      make VERBOSE=1
      make test CTEST_OUTPUT_ON_FAILURE=1
      make install
      exit 0
    '';
  };
}
