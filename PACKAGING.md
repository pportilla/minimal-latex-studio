# Packaging Checklist

This project is structured so the upstream source tree can be packaged from
native Qt/CMake sources without generated assets or network access during build.

## Release Requirements

- Publish the upstream source in a real public repository.
- Create a `v0.1.0` release tag from a clean source tree.
- Prefer a signed tag or detached source signature for future releases.
- Keep the Arch package sources in `packaging/arch` under the explicit MIT
  package-source license metadata (`REUSE.toml` and `LICENSES/MIT.txt`).
- Run `scripts/prepare-release.sh` from the tagged upstream tree. It creates
  `dist-release/minimal-latex-studio-0.1.0.tar.gz`, updates the PKGBUILD
  checksum, and regenerates `.SRCINFO`.
- Upload that tarball as the `v0.1.0` release asset.
- Regenerate `packaging/arch/.SRCINFO` after every PKGBUILD change:

```bash
cd packaging/arch
makepkg --printsrcinfo > .SRCINFO
```

## Local Package Checks

```bash
cd packaging/arch
makepkg -Csr
namcap PKGBUILD
namcap minimal-latex-studio-*.pkg.tar.*
```

The package also runs CTest during `check()`. Current tests validate the desktop
file and AppStream metadata.

The repository-level check script runs the native build, CTest, clang-tidy,
desktop validation, AppStream validation, and `namcap PKGBUILD`:

```bash
scripts/check-package.sh
```

After the release checksum is real, export the AUR repository contents:

```bash
scripts/export-aur.sh
```

The export includes `PKGBUILD`, `.SRCINFO`, `.nvchecker.toml`, `README.md`, and
MIT package-source license metadata. Commit those files in the AUR package
repository.

## Clean Chroot Check

Install Arch devtools and build in a clean chroot before any AUR submission.
Run this on an Arch Linux system or Arch VM/container. Do not mix Arch package
repositories into Manjaro or another derivative just to install `devtools`.

```bash
sudo pacman -S --needed devtools
cd packaging/arch
extra-x86_64-build
```

If `pkgctl` is available, run its package-source license check/setup flow as
well before asking for official repository review.

## Official Repository Path

Upstream projects cannot directly publish into Arch official repositories. The
practical path is to maintain a clean upstream release and a healthy AUR package,
then have an Arch Package Maintainer adopt it if there is enough user interest
and it satisfies repository policy.

Use `packaging/official/maintainer-request.md` as the starting point when asking
for package maintainer review.
