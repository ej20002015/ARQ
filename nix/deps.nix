# Fetch and unpack each dependency into arq-deps, like: 
#   $out/openssl-v3.6.0/ 
#   $out/zlib-v1.3.1/

{ pkgs }:

let
  deps = [
    {
      name = "openssl-v3.6.0";
      url  = "https://github.com/ej20002015/ARQ_dependencies/releases/download/1.0.0-alpha/openssl-3.6.0-linux-x64.tar.gz";
      sha256 = "sha256-Zckn2LLNPI2vNAtZxy+KarX/eqgWRfWtZcS/nvUnsGs=";
    }
    {
      name = "zlib-v1.3.1";
      url  = "https://github.com/ej20002015/ARQ_dependencies/releases/download/1.0.0-alpha/zlib-v1.3.1-linux-x64.tar.gz";
      sha256 = "sha256-jqT3o1ZuoJu84BPGZxEoF/45+MPNOnFZd7mC+XJ7hm4=";
    }
    {
      name = "librdkafka_v2.12.0";
      url  = "https://github.com/ej20002015/ARQ_dependencies/releases/download/1.0.0-alpha/librdkafka-v2.12.0-linux-x64.tar.gz";
      sha256 = "sha256-heBgF70dTtrssWTsqRCMBK4Kd5apiHje7fcdAXkz5e4=";
    }
    {
      name = "SWIG_4.3.1";
      url  = "https://github.com/ej20002015/ARQ_dependencies/releases/download/1.0.0-alpha/SWIG-4.3.1-linux-x64.tar.gz";
      sha256 = "sha256-KIcjmP4UgpWsPBnRY+d4IXgyaY8p+x6R+h4GiKoZRZk=";
    }
    {
      name = "grpc-v1.76.0";
      url  = "https://github.com/ej20002015/ARQ_dependencies/releases/download/1.0.0-alpha/grpc-v1.76.0-linux-x64.tar.gz";
      sha256 = "sha256-1zphkRPvp6FL03OSypJ1pZ1kaJ+uTeznejH5+gkS6A0=";
    }
    {
      name = "natsc-v3.11.0";
      url  = "https://github.com/ej20002015/ARQ_dependencies/releases/download/1.0.0-alpha/nats.c-v3.11.0-linux-x64.tar.gz";
      sha256 = "sha256-UuQJ1gYRozp6hXmwIbVsib8K5SV2BqujuTYpijcwrbk=";
    }
  ];
in
pkgs.symlinkJoin {
  name = "arq-deps";
  paths = builtins.map (dep:
    pkgs.stdenv.mkDerivation {
      name = dep.name;
      src = pkgs.fetchurl {
        url = dep.url;
        sha256 = dep.sha256;
      };

      unpackPhase = ''
        mkdir -p tmp
        tar -xzf $src -C tmp
        dir="$(ls tmp)"
        mkdir -p $out/${dep.name}
        mv tmp/$dir/* $out/${dep.name}/
      '';

      installPhase = "true";
    }
  ) deps;
}