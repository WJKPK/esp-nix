{pkgs, esp-idf, ...}:
with pkgs;
stdenv.mkDerivation {
  name = "esp32c3";
  src = ./src;

  buildInputs = [
    esp-idf
  ];

  phases = [ "unpackPhase" "buildPhase" ];
  
  buildPhase = ''
    # The build system wants to create a cache directory somewhere in the home
    # directory, so we make up a home for it.
    mkdir temp-home
    export HOME=$(readlink -f temp-home)

    # idf-component-manager wants to access the network, so we disable it.
    export IDF_COMPONENT_MANAGER=0

    idf.py build

    mkdir $out
    cp -r build/* $out
  '';
}

