# Copilot instructions - gMKVExtractGUI

## Build, Test, And Smoke Commands

This repository is now a native C++/Qt project built with CMake. The active app
target is `gMKVExtractGUIQt`; the reusable core library target is
`gMKVToolNixCpp`.

```bash
cmake -S . -B build/cpp -DCMAKE_BUILD_TYPE=Debug
cmake --build build/cpp
/usr/bin/ctest --test-dir build/cpp --output-on-failure
cmake --install build/cpp --prefix build/cpp-install
scripts/cpp_manual_smoke.sh
```

Windows builds should use Visual Studio 2022, Qt for MSVC, and `windeployqt`
after install:

```powershell
cmake -S . -B build\cpp -G "Visual Studio 17 2022" -A x64
cmake --build build\cpp --config Debug
ctest --test-dir build\cpp -C Debug --output-on-failure
cmake --install build\cpp --config Debug --prefix build\cpp-install
windeployqt build\cpp-install\bin\gMKVExtractGUIQt.exe
```

Lint: no repository lint command is configured. Run `git diff --check` and scan
for debug output before committing.

## High-Level Architecture

- `CMakeLists.txt` - root CMake entry point.
- `src/gMKVToolNix.Cpp` - QtCore-based library for MKVToolNix process planning,
  tool discovery, version parsing, segment models, settings, logging, jobs,
  filename patterns, extraction planning, and JSON localization.
- `src/gMKVExtractGUI.Cpp` - Qt Widgets desktop application. `main.cpp`
  initializes settings and localization, then opens `MainWindow`.
- `tests/gMKVToolNix.Cpp.Tests` - Qt Test coverage for core behavior and
  headless GUI smoke paths.
- `scripts/cpp_manual_smoke.sh` - Linux smoke script that creates a tiny MKV
  fixture and verifies direct `mkvmerge`, `mkvinfo`, and `mkvextract` usage.

File analysis is a merged-tool pipeline. `SegmentAnalyzer` uses `MkvMergeService`
as the primary structured source, supplements missing details with
`MkvInfoService`, then feeds segment models to the Qt main window.

Extraction uses planned command arguments from `MkvExtractPlanner` and runs
them through `MkvExtractRunner`. Immediate extraction and queued jobs share the
same segment and filename-pattern contracts.

## Platform Contracts

- Linux resolves direct command tools: `mkvmerge`, `mkvinfo`, and `mkvextract`.
  Empty settings use `PATH`; a manual value may point to the `mkvmerge`
  executable or a compatible directory.
- Windows resolves an MKVToolNix directory that contains `mkvmerge.exe`,
  `mkvinfo.exe`, and `mkvextract.exe`. Registry candidates are Windows-only.
- Do not introduce Linux dependencies on the MKVToolNix GUI directory.
- Windows smoke/build evidence must be gathered on a Windows host unless the
  project owner explicitly waives it.

## Key Conventions

- Text files use UTF-8 without BOM and CRLF line endings, except shell scripts,
  which stay LF and executable.
- Visible UI strings go through the JSON localization contract. Add English keys
  to `src/gMKVExtractGUI.Cpp/resources/locales/gmkvextract-en.json`; update
  Simplified Chinese in `gmkvextract-zh-cn.json` for user-facing C++ UI changes.
- Locale files are copied beside the executable during build/install. Missing
  locale folders must still fall back to built-in English strings.
- First launch defaults to Simplified Chinese for Chinese system locales and
  English otherwise. Saved `Culture:` settings still win.
- Use Qt containers and `QString`/`QStringList` consistently inside the Qt-based
  library and GUI.
- Keep tool discovery behavior centralized in `ToolLocator`; do not duplicate
  command lookup logic in UI code.
- Keep filename-pattern defaults, option persistence, UI placeholder menus, and
  extraction naming tests aligned when adding or changing placeholders.
- Job definitions are serialized to XML. If job fields or segment subtypes
  change, update save/load tests and the Job Manager flow together.
