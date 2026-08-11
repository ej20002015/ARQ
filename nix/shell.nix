{ pkgs }:

pkgs.mkShell {
  packages = with pkgs; [
    cmake
    ninja
    pkg-config
    gnumake
    libuuid
    jdk
    gcc
    clang
    (python3.withPackages (ps: with ps; [ jinja2 toml ]))
    gdb
  ];

  shellHook = ''
    echo "ARQ development environment loaded"
    echo "Use 'cmake -S . -B build' then 'cmake --build build'"
  '';
}
