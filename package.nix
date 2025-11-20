{ lib

, clangStdenv
, gitignore
}:

let
  inherit (gitignore.lib) gitignoreSource;
in
clangStdenv.mkDerivation {

  pname = "capsmorse";
  version = "0.0.0";

  src = gitignoreSource ./.;

  nativeBuildInputs = [ ];
  buildInputs = [ ];

  installPhase = ''
    install -D capsmorse $out/bin/capsmorse
  '';

  meta = {
    description = "A fun program to display pressed keys through the caps lock light";
    homepage = "https://github.com/celesteornato/capsmorse";
    # license = "";
    maintainers = [ "mrnossiom" ];
    mainProgram = "capsmorse";
  };
}
