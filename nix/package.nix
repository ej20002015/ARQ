{ pkgs }:

pkgs.stdenv.mkDerivation {
  pname = "arq";
  version = "1.0.0";
  src = ../.;

  nativeBuildInputs = with pkgs; [
    cmake
    ninja
    pkg-config
    gcc
    clang
  ];

  buildInputs = with pkgs; [
    libuuid
    jdk
  ];

  cmakeFlags = [ "-DCMAKE_BUILD_TYPE=Release" ];

  buildPhase = ''
    cmake -S . -B build -G Ninja
    cmake --build build
  '';

  installPhase = ''
    mkdir -p $out/bin
    cp build/arq $out/bin/
  '';
}
