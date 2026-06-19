# Arch/AUR Packaging Notes

This directory prepares the project for an Arch package workflow. The package
builds the native Qt/CMake source tree directly.

## Before Publishing

- Ensure the `url` in `PKGBUILD` matches the real public upstream repository.
- Create a signed or checksummed source release tag `v0.1.0`.
- Run `scripts/prepare-release.sh` from the upstream repository.
- Upload `dist-release/minimal-latex-studio-0.1.0.tar.gz` as the GitHub release asset.
- Run `makepkg --printsrcinfo > .SRCINFO` after every `PKGBUILD` change.
- Keep `REUSE.toml` and `LICENSES/0BSD.txt` with the package source files.
- Run `makepkg -Csr` and `namcap` in a clean Arch chroot before submitting to the AUR.
- Run `extra-x86_64-build` from `devtools` on Arch Linux before asking an Arch
  maintainer to review it. Manjaro is fine for local development, but not for
  official Arch clean-chroot validation.
- For official repository consideration, keep the AUR package healthy and wait for
  an Arch package maintainer to adopt or request it. Upstream projects cannot
  directly submit packages to the official repositories.

## Current Package Shape

- Builds a native `/usr/bin/minimal-latex-studio` executable with CMake.
- Uses Qt 6 widgets for the application shell.
- Uses Poppler Qt 6 for PDF rendering.
- Uses QTermWidget for the embedded terminal.
- Detects installed LaTeX compilers at runtime.
- Treats `texlive-binextra`, `biber`, and larger TeX collections as optional dependencies.
