{
  description = "CppUMockGen";

  inputs = {
    nixpkgs.url = "nixpkgs/release-21.05";
    nixpkgs-unstable.url = "nixpkgs/nixpkgs-unstable";
    flake-utils.url = "github:numtide/flake-utils";
    cppumock = {
      url = "github:jgonzalezdr/CppUMockGen";
      flake = false;
    };
  };

  outputs = { self, nixpkgs, nixpkgs-unstable, flake-utils, cppumock }:
    flake-utils.lib.eachSystem [ "x86_64-linux" ] (system: let
    pkgs = import nixpkgs { inherit system; };
    pkgs-unstable = import nixpkgs-unstable { inherit system; };
    cxxopts_custom = pkgs-unstable.cxxopts.overrideAttrs {
        version = "1.4.4";
        src = pkgs.fetchFromGitHub {
          owner = "jarro2783";
          repo = "cxxopts";
          rev = "d7b930846cdccfc8bcecc4d7150ddcbadffac360";
          sha256 = "sha256-972wX0guLw7dyru22wR6T0aDny5KF4sTHoDlA8CYzDA=";
        };
       postPatch = '''';
    };
    in {
      packages.default = pkgs.stdenv.mkDerivation {
        name = "CppUMockGen";
        src = cppumock;

        nativeBuildInputs = [
          pkgs.cmake
          cxxopts_custom
        ];

        buildInputs = [
          pkgs.libcxx
          pkgs.libclang
        ];

        cmakeFlags = ["-DLibClang_LIB_PATH=${pkgs.lib.getLib pkgs.libclang}/lib/libclang.so"];
      };
   });
}
