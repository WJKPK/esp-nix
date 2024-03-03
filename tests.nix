{ pkgs, mockgen, compiler-path, ...}:
let
  getFetchContentFlags = file:
    let
      inherit (builtins) head elemAt match;
      parse = match "(.*)\nFetchContent_Declare\\(\n  ([^\n]*)\n([^)]*)\\).*"
        file;
      name = elemAt parse 1;
      content = elemAt parse 2;
      getKey = key: elemAt
        (match "(.*\n)?  ${key} ([^\n]*)(\n.*)?" content) 1; url = getKey "GIT_REPOSITORY";
      pkg = pkgs.fetchFromGitHub {
        owner = head (match ".*github.com/([^/]*)/.*" url);
        repo = head (match ".*/([^/]*)\\.git" url);
        rev = getKey "GIT_TAG";
        hash = getKey "# hash:";
      };
  in
    if (parse == null) then [ ] else
    ([ "-DFETCHCONTENT_SOURCE_DIR_${pkgs.lib.toUpper name}=${pkg}" ] ++
      getFetchContentFlags (head parse));
in
  with pkgs;
  stdenv.mkDerivation {
    name = "tests";
    src = ./.;

    nativeBuildInputs = [
      cmake
      mockgen
    ];

    buildInputs = [
      cpputest
    ];

    #probably good option to set while debuging
    #dontFixup = true;
    cmakeFlags = getFetchContentFlags
        (builtins.readFile ./CMakeLists.txt) ++ ["-DCMAKE_SKIP_BUILD_RPATH=ON"];

    installPhase = "mkdir -p $out/bin; cp tests $out/bin/.";

    env.RISCV_INCLUDE_PATH = "${compiler-path}";
}
