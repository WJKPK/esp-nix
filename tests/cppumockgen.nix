{ pkgs, pkgs-old }:
let
  cxxopts_custom = pkgs.cxxopts.overrideAttrs (old: {
    version = "1.4.4";
    src = pkgs.fetchFromGitHub {
      owner = "jarro2783";
      repo = "cxxopts";
      rev = "d7b930846cdccfc8bcecc4d7150ddcbadffac360";
      sha256 = "sha256-972wX0guLw7dyru22wR6T0aDny5KF4sTHoDlA8CYzDA=";
    };
    postPatch = '''';
  });
  cppumock = pkgs.fetchFromGitHub {
    owner = "jgonzalezdr";
    repo = "CppUMockGen";
    rev = "master";
    sha256 = "sha256-ug6eM5WeX2w1vHzmP8yeJbpOSQNs/pYYC8XCGERPauI=";
  };
in
pkgs-old.stdenv.mkDerivation {
  name = "CppUMockGen";
  src = cppumock;
  nativeBuildInputs = [
    pkgs-old.cmake
    cxxopts_custom
  ];
  buildInputs = [
    pkgs-old.libcxx
    pkgs-old.libclang
  ];
  cmakeFlags = ["-DLibClang_LIB_PATH=${pkgs.lib.getLib pkgs-old.libclang}/lib/libclang.so"];
}
