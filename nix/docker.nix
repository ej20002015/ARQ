{ pkgs, arq }:

pkgs.dockerTools.buildImage {
  name = "arq";
  tag = "1.0.0";

  copyToRoot = pkgs.buildEnv {
    name = "image-root";
    paths = [
      arq
      pkgs.glibc
    ];
    pathsToLink = [ "/bin" "/lib" ];
  };

  config = {
    Cmd = [ "/bin/arq" ];
    WorkingDir = "/bin";
  };
}
