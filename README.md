# LiTeX

LiTeX is a stripped-down native LaTeX editor inspired by TeXstudio. It includes:

- Qt 6 source editor with LaTeX highlighting and line numbers
- Local LaTeX builds through `latexmk`, `pdflatex`, `xelatex`, or `lualatex`
- Automatic rerun detection for cross references and bibliography workflows
- Manual force-rerun and single-pass build modes
- Build log tab and an embedded QTermWidget terminal tab in the lower output panel
- Poppler-based PDF preview with fit-width and zoom controls
- Resizable editor/PDF split, hideable PDF preview, and resizable lower panel
- Source file watching with reload-from-disk prompts for external edits
- Click-to-clean auxiliary LaTeX files
- JSON-backed themes with a small Options dialog for theme, font size, smart line wrap, column guide, sidebar visibility, and PDF visibility

## Requirements

- Qt 6
- Poppler Qt 6 bindings
- QTermWidget for Qt 6
- A LaTeX distribution with at least one supported compiler on `PATH`

On Arch Linux:

```bash
sudo pacman -S --needed qt6-base poppler-qt6 qtermwidget hicolor-icon-theme cmake pkgconf
```

For document builds and SyncTeX source/PDF navigation, install TeX Live command
line tools. `latexmk` from `texlive-binextra` is the recommended build driver:

```bash
sudo pacman -S --needed texlive-bin texlive-binextra
```

`biber` is optional and used automatically when a `biblatex` project needs it:

```bash
sudo pacman -S --needed biber
```

## Build

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
./build/minimal-latex-studio
```

## Install Locally

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build
sudo cmake --install build
```

## Themes

Themes are JSON files. The default theme is installed from `resources/themes/smart-modern.json`.
To create another theme, copy that file, give it a new `id` and `name`, then adjust
the semantic `colors`, `editor`, `syntax`, `terminal`, and `pdf` sections. Select
the JSON file from Options.

## Shortcuts

- `Ctrl+N`: new document
- `Ctrl+O`: open document
- `Ctrl+S`: save document
- `F1`: build PDF
- `Ctrl+B`: build PDF
- `Ctrl+Shift+K`: clean auxiliary files
- `Ctrl++` / `Ctrl+=`: increase text size in the focused editor or terminal
- `Ctrl+-`: decrease text size in the focused editor or terminal
- `Ctrl+0`: reset text size in the focused editor or terminal
- `Ctrl+Shift+[`: fold the current section or environment
- `Ctrl+Shift+]`: unfold all folded text
- `Ctrl` + left click in the editor: jump to the matching PDF location
- `Ctrl` + left click in the PDF preview: jump to the matching source line

## License

LiTeX is released under the MIT License. See `LICENSE` for details.
