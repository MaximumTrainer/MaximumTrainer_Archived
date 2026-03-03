# Agent Rules: Qt Build Configuration

This file defines how Qt and its companion libraries must be configured per
target platform. Follow these rules exactly when modifying the build system,
CI workflow, or any code that uses platform-conditional Qt APIs.

---

## Platform Matrix

| Platform | Runner | Qt Version | Arch | Qt API Level |
|----------|--------|------------|------|--------------|
| Linux    | ubuntu-22.04 | 5.15.x (system) | x86_64 | Qt5 |
| Windows  | windows-latest | 5.15.2 (jurplel/install-qt-action@v3) | msvc2019_64 | Qt5 |
| macOS    | macos-latest (ARM64) | 6.7.3 (jurplel/install-qt-action@v3) | clang_64 | Qt6 |

macOS uses Qt6 because `macos-latest` is Apple Silicon (ARM64). Qt5 `clang_64`
is an x86_64 binary that runs under Rosetta, causing Clang ODR violations when
the same QWT headers are resolved through both a flat include path and a
framework path simultaneously. Qt6 with native ARM64 avoids Rosetta but does
**not** eliminate the QWT framework ODR issue — see the QWT rule below.

---

## Qt Modules Required

### Linux (Qt5 system packages)
```
qtbase5-dev
qtwebengine5-dev
qtconnectivity5-dev
```

### Windows (`jurplel/install-qt-action@v3`)
```
version: 5.15.2
modules: qtwebengine
```

### macOS (`jurplel/install-qt-action@v3`)
```
version: 6.7.3
host: mac
target: desktop
arch: clang_64
modules: qtwebengine qtconnectivity qtwebchannel qtpositioning
```
`qtwebchannel` and `qtpositioning` are transitive requirements of
`qtwebengine` on Qt6 — they must be listed explicitly.

---

## QWT Configuration

### Version
QWT **6.2.0** on all platforms.

### Linux
Installed via system package `libqwt-qt5-dev`. No build step needed.
`QMAKEFEATURES` is set to `/usr/share/qt5/mkspecs/features`.

### Windows
Built from source (SourceForge tarball). Install prefix set to
`$env:USERPROFILE\qwt` (user-profile-relative so it works both locally and
in CI without hard-coded workspace paths). `QMAKEFEATURES` set to
`$env:USERPROFILE\qwt\features`.

Use `curl.exe -L` (not `Invoke-WebRequest`) for the SourceForge download —
SourceForge uses multi-hop redirects that `Invoke-WebRequest` does not follow.

### macOS
Built from source. **CRITICAL:** Before calling `qmake qwt.pro`, apply:
```bash
sed -i.bak 's/QwtFramework//' qwtconfig.pri
```
This disables `.framework` bundle mode. Without it, QWT installs into
`qwt.framework/` and the qwt.prf feature file adds both a flat include path
(`-I.../qwt.framework/Headers`) and a framework search path
(`-F.../lib -framework qwt`). Clang resolves the same headers through two
different canonical paths and treats `QwtPlot` as two distinct types (ODR
violation), causing errors like:
```
cannot convert WorkoutPlotZoomer* to QwtPlot*
```
With `QwtFramework` disabled, QWT installs headers to
`/usr/local/qwt-6.2.0/include/` and links as a plain dylib — one canonical
path, no ODR issue.
`QMAKEFEATURES` is set to `/usr/local/qwt-6.2.0/features`.

---

## VLC-Qt Configuration

VLC-Qt 1.1.x is **Qt5-only**. It is not used on macOS (Qt6).

The `GC_HAVE_VLCQT` preprocessor define controls whether VLC-Qt code is
compiled. It must be set wherever VLC-Qt libraries are present:

| Platform | `GC_HAVE_VLCQT` set? | VLC-Qt source |
|----------|----------------------|---------------|
| Linux    | Yes (unconditional in `unix:!macx` block of `PowerVelo.pro`) | Built from source (tag 1.1.1) |
| Windows  | Yes (inside `!isEmpty(VLCQT_INSTALL)` block) | Pre-built 1.1.0 MSVC2015 x64 |
| macOS    | No (VLC-Qt omitted entirely) | — |

**Rule:** Never remove `DEFINES += GC_HAVE_VLCQT` from the Linux block.
**Rule:** `ui_workoutdialog.h` always promotes `MyVlcPlayer` — the header
`myvlcplayer.h` must always be includable. Only the `.cpp` compilation is
conditional on `GC_HAVE_VLCQT`.

---

## SFML Configuration

| Platform | Source |
|----------|--------|
| Linux    | `libsfml-dev` (apt) |
| Windows  | GitHub release `SFML/SFML@2.6.1` (vc17 64-bit zip), extracted to `C:\sfml` |
| macOS    | Homebrew `sfml` (`brew install sfml`), path via `$(brew --prefix sfml)` |

---

## Qt Version Guard Pattern

When a Qt API changed between Qt5 and Qt6, use version guards rather than
picking one version unconditionally:

```cpp
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    void enterEvent(QEnterEvent *event) override;
#else
    void enterEvent(QEvent *event) override;
#endif
```

---

## Known Qt5 → Qt6 Breaking Changes (already fixed)

| Removed API (Qt5) | Replacement (Qt6) | Files affected |
|-------------------|-------------------|----------------|
| `Qt::WindowFlags f = 0` | `Qt::WindowFlags f = Qt::WindowFlags()` | `clickablelabel.h` |
| `QFontMetrics::width()` | `horizontalAdvance()` | `fancytabbar.cpp`, `reportutil.cpp` |
| `QWidget::enterEvent(QEvent*)` | `enterEvent(QEnterEvent*)` | `fancytabbar.h/cpp` |
| `QPixmapCache::find(key, QPixmap&)` | `find(key, QPixmap*)` | `stylehelper.cpp` |
| `QLayout::setMargin(n)` | `setContentsMargins(n,n,n,n)` | `mydialbox.cpp`, `workoutplot.cpp`, `workoutdialog.cpp` |

---

## Windows Path Conventions

- Qt is installed by `jurplel/install-qt-action@v3` into `QT_HOME\Qt\<ver>\<arch>\`.
  `QT_HOME` defaults to `C:\Qt` in CI. For local builds, set `QT_HOME` to the
  parent of your Qt installation directory.
- QWT is installed to `$env:USERPROFILE\qwt` — always user-profile-relative,
  never a hard-coded workspace path.
- VLC-Qt is extracted to `C:\vlcqt`.
- SFML is extracted to `C:\sfml`.

---

## CI Workflow File

`.github/workflows/build.yml`  
Three jobs: `build_linux`, `build_windows`, `build_mac`.  
All triggered on every push to any branch.
