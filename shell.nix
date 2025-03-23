{pkgs, pkgs-old, esp-idf, compiler-path, ...}:
let
    mockgen = import ./tests/cppumockgen.nix { inherit pkgs pkgs-old; };
in
with pkgs;
  mkShell {
    buildInputs = [
      esp-idf
      uncrustify
      cpputest
      cmake
      mockgen
    ];
    env.RISCV_INCLUDE_PATH = "${compiler-path}";
  }

