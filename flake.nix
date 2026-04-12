{
  description = "ARQ C++ project";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};

        shell  = import ./nix/shell.nix { inherit pkgs; };
        arq = import ./nix/arq.nix { inherit pkgs; };
        docker = import ./nix/docker.nix { inherit pkgs arq; };
      in {
        devShells.default = shell;
        packages = {
          inherit arq docker;
        };
      }
    );
}
