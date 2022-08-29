{
  stdenv
, cmake
, openmpi
}:

stdenv.mkDerivation rec {
  name = "ovni";

  buildInputs = [ cmake openmpi ];

  # Prevent accidental reutilization of previous builds, as we are taking the
  # current directory as-is
  preConfigure = ''
    rm -rf build install

    # There is no /bin/bash
    patchShebangs test/*.sh
  '';

  cmakeBuildType = "Debug";
  cmakeFlags = [ "-DCMAKE_SKIP_BUILD_RPATH=OFF" ];
  buildFlags = [ "VERBOSE=1" ];
  preCheck = ''
    export CTEST_OUTPUT_ON_FAILURE=1
  '';
  dontStrip = true;
  doCheck = true;
  checkTarget = "test";

  src = ../.;
}
