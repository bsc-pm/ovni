# Build with `nix-build nix/old-glibc.nix`
let
  # Pin the nixpkgs
  nixpkgsPath = builtins.fetchTarball {
    # Descriptive name to make the store path easier to identify
    name = "nixos-20.09";
    # Commit hash for nixos-20.09 as of 2021-01-11
    url = "https://github.com/nixos/nixpkgs/archive/41dddb1283733c4993cb6be9573d5cef937c1375.tar.gz";
    # Hash obtained using `nix-prefetch-url --unpack <url>`
    sha256 = "1blbidbmxhaxar2x76nz72bazykc5yxi0algsbrhxgrsvijs4aiw";
  };

  pkgs = import nixpkgsPath { };

  ovni = pkgs.stdenv.mkDerivation rec {
    name = "ovni";

    buildInputs = with pkgs; [ cmake openmpi ];

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
  };

  nosv = pkgs.stdenv.mkDerivation rec {
    pname = "nosv";
    version = src.shortRev;
    buildInputs = with pkgs; [ autoreconfHook pkg-config numactl ovni ];
    configureFlags = [ "--with-ovni=${ovni}" ];
    dontStrip = true;
    src = builtins.fetchGit {
      url = "ssh://git@gitlab-internal.bsc.es/nos-v/nos-v.git";
      ref = "master";
    };
  };

  # Quick fix to avoid rebuilding every time the ovni source changes.
  # Use this nosv' version below as dependency of ovni-rt
  nosv' = /nix/store/rvnrbc7ibpw06jdilz6mha7szzxcr2mi-nosv-8936f3e;

  ovni-rt = ovni.overrideAttrs (old: {
    __impure = true;
    __noChroot = true;
    buildInputs = old.buildInputs ++ [ nosv pkgs.strace ];
    cmakeFlags = old.cmakeFlags ++ [ "-DBUILD_RT_TESTING=ON" ];
  });

in

  ovni-rt
