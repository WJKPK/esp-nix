{
  description = "ESP32C3 Thermostat";

  inputs = {
    nixpkgs.url = "nixpkgs/nixpkgs-unstable";
    nixpkgs-old.url = "nixpkgs/release-21.05";
    flake-utils.url = "github:numtide/flake-utils";
    nixpkgs-esp-dev.url = "github:mirrexagon/nixpkgs-esp-dev";
  };

  outputs = { self, nixpkgs, nixpkgs-old, flake-utils, nixpkgs-esp-dev }:
    flake-utils.lib.eachSystem [ "x86_64-linux" ] (system: let

    get-drv-by-name = name: derivations: builtins.head (builtins.filter (d: d.name == name) derivations);

    pkgs = import nixpkgs { inherit system; overlays = [(import "${nixpkgs-esp-dev}/overlay.nix")];};
    pkgs-old = import nixpkgs-old { inherit system; };

    idf-revision = "79b1379662b547f6eb0e5fed33df70587b03d99c";
    esp-idf = with pkgs; esp-idf-esp32c3.override {
      rev = idf-revision; sha256 = "sha256-JNJ4wfkS6lEMNeaMf06ORzNPgHQ59s96zMlm9/lSS9A=";
    };
    get-compiler-path = get-drv-by-name "riscv32-esp-elf-esp-idf-${idf-revision}";
    compiler-path = get-compiler-path esp-idf.propagatedBuildInputs;

    in {
      devShell = pkgs.callPackage ./shell.nix { inherit pkgs-old esp-idf compiler-path; };

      packages.test = pkgs.callPackage ./tests.nix { inherit pkgs-old compiler-path; };

      defaultPackage = pkgs.callPackage ./build.nix { inherit esp-idf; };
    });
}
