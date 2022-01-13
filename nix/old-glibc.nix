# Build with `nix-build nix/old-glibc.nix`
let
  # Define the set of packages from the old 2018.03 nixos release, so we get a
  # glibc 2.26 which doesn't define the gettid() function, and one of the lowest
  # cmake versions supported (3.10.2).
  url = "https://github.com/NixOS/nixpkgs/archive/3e1be2206b4c1eb3299fb633b8ce9f5ac1c32898.tar.gz";
  pkgs = import (builtins.fetchTarball { inherit url; }) {};

in

  with pkgs;

  stdenv.mkDerivation rec {
    name = "ovni";

    buildInputs = [ cmake openmpi ];

    # Prevent accidental reutilization of previous builds, as we are taking the
    # current directory as-is
    preConfigure = ''
      rm -rf build install
    '';

    cmakeBuildType = "Debug";
    cmakeFlags = [ "-DCMAKE_SKIP_BUILD_RPATH=OFF" ];
    buildFlags = [ "VERBOSE=1" ];
    checkFlags = [ "CTEST_OUTPUT_ON_FAILURE=1" "ARGS=-V" ];
    dontStrip = true;
    doCheck = true;
    checkTarget = "test";

    src = ../.;
  }
