{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-25.05";

    gitignore.url = "github:hercules-ci/gitignore.nix";
    gitignore.inputs.nixpkgs.follows = "nixpkgs";
  };

  outputs = { self, nixpkgs, gitignore }:
    let
      inherit (nixpkgs.lib) genAttrs getExe;

      forAllSystems = genAttrs [ "x86_64-linux" "aarch64-linux" "aarch64-darwin" ];
      forAllPkgs = function: forAllSystems (system: function pkgs.${system});

      mkApp = (program: { type = "app"; inherit program; });

      pkgs = forAllSystems (system: import nixpkgs {
        inherit system;
        overlays = [ ];
      });
    in
    {
      formatter = forAllPkgs (pkgs: pkgs.nixpkgs-fmt);

      packages = forAllPkgs (pkgs: rec {
        default = capsmorse;
        capsmorse = pkgs.callPackage ./package.nix { inherit gitignore; };
      });
      apps = forAllSystems (system: rec {
        default = capsmorse;
        capsmorse = mkApp (getExe self.packages.${system}.capsmorse);
      });

      devShells = forAllPkgs (pkgs:
        with pkgs.lib;
        let
          mkClangShell = pkgs.mkShell.override { stdenv = pkgs.clangStdenv; };
        in
        {
          default = mkClangShell rec {
            nativeBuildInputs = with pkgs; [
              clang-tools
            ] ++ (with llvmPackages; [ clang lldb ]);

            buildInputs = with pkgs; [ ];

            LD_LIBRARY_PATH = makeLibraryPath buildInputs;
          };
        });
    };
}
