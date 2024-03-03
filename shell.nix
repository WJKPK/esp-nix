{pkgs, esp-idf, mockgen, compiler-path, ...}:
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

