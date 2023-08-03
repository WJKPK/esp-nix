{
  description = "ESP32c3 development";

  inputs = {
    nixpkgs.url = "nixpkgs/nixpkgs-unstable";
    flake-utils.url = "github:numtide/flake-utils";
    nixpkgs-esp-dev.url = "github:mirrexagon/nixpkgs-esp-dev";
  };

  outputs = { self, nixpkgs, flake-utils, nixpkgs-esp-dev }:
    flake-utils.lib.eachSystem [ "x86_64-linux" ] (system: let
      pkgs = import nixpkgs { inherit system; overlays = [(import "${nixpkgs-esp-dev}/overlay.nix")];};
      esp-idf = with pkgs; esp-idf-esp32c3.override {
            rev = "79b1379662b547f6eb0e5fed33df70587b03d99c";
            sha256 = "sha256-JNJ4wfkS6lEMNeaMf06ORzNPgHQ59s96zMlm9/lSS9A=";
          };
    in {
      devShell = pkgs.mkShell {
        buildInputs = [
          esp-idf
        ];
      };
      defaultPackage = pkgs.stdenv.mkDerivation {
        name = "esp32c3-project";
        src = ./code;

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
          cp -r * $out
        '';
      };
    });
}
