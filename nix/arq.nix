{ pkgs }:

let
  depsRoot = import ./deps.nix { inherit pkgs; };
in
pkgs.stdenv.mkDerivation {
  pname = "arq";
  version = "1.0.0";
  src = ../.;

  nativeBuildInputs = with pkgs; [ cmake ninja pkg-config gcc clang ];
  buildInputs = with pkgs; [ libuuid jdk ];

  cmakeFlags = [
    "-DCMAKE_BUILD_TYPE=Release"
    "-DARQ_DEPS_ROOT=${depsRoot}"
  ];
}
