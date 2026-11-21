{
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-26.05";
    l4y3r = {
      url = "git+https://codeberg.org/us4tiyny4n/14y3r.git";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs = {
    nixpkgs,
    l4y3r,
    ...
  }: {
    devShells = l4y3r.lib.forAllSystems (system: let
      pkgs = nixpkgs.legacyPackages.${system};
    in {
      default = l4y3r.lib.mkComposedShell {
        inherit system;
        alias = "cyb3r14";
        bases = ["nix" "cpp-clang"];
        extraBuildInputs = with pkgs; [
          kdePackages.extra-cmake-modules

          # graphics
          wayland
          wayland-scanner
          wayland-protocols
          libxkbcommon
          libffi
          libglvnd

          # audio
          alsa-lib
          libpulseaudio
          pipewire

          # build
          just
          pkg-config
        ];
        extraShellHook = ''
        '';
      };
    });
  };
}
