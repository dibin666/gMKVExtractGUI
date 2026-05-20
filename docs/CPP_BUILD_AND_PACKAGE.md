# C++/Qt Build And Package Notes

The repository now builds the native C++/Qt application as the active product.
It does not embed the Matroska command-line tools. On Linux, users need
`mkvmerge`, `mkvinfo`, and `mkvextract` available on `PATH` or can choose the
`mkvmerge` executable manually. On Windows, users need MKVToolNix installed so
the app can detect its installation directory and run the three `.exe` tools.

Record Linux and Windows evidence in
[CPP_MANUAL_PARITY_CHECKLIST.md](CPP_MANUAL_PARITY_CHECKLIST.md) before release.
On Linux, `scripts/cpp_manual_smoke.sh` can generate a tiny MKV fixture and
verify the local C++ build plus direct `mkvmerge`, `mkvinfo`, and `mkvextract`
invocation.

## Linux Debug Build

Prerequisites:

- CMake 3.21 or newer
- A C++17 compiler
- Qt 6.2 or newer with Core, Widgets, and Test modules
- `mkvmerge`, `mkvinfo`, and `mkvextract` installed separately for runtime
  analysis/extraction, normally through the distro's MKVToolNix package

```bash
cmake -S . -B build/cpp -DCMAKE_BUILD_TYPE=Debug
cmake --build build/cpp
/usr/bin/ctest --test-dir build/cpp --output-on-failure
cmake --install build/cpp --prefix build/cpp-install
```

The install step places `gMKVExtractGUIQt`, `gmkvextract-*.json`, and
`gMkvExtractGuiIcon.ico` under the install `bin` directory. Locale source files
live in `src/gMKVExtractGUI.Cpp/resources/locales`. Distributions should package
the required Qt runtime libraries through their normal dependency system. A
distro package can be layered on top of the install tree.

## Linux Release Packages

The release workflow builds Linux packages for:

- `amd64` / `x86_64`
- `arm64` / `aarch64`

Local package validation must run in Docker so package-test dependencies stay
out of the host system:

```bash
docker build -f packaging/linux/Dockerfile.package-test -t gmkvextractgui-package-test .
docker run --rm \
  -v "$PWD:/work" \
  -w /work \
  gmkvextractgui-package-test \
  bash packaging/linux/docker-package-test.sh
docker image rm gmkvextractgui-package-test:latest
```

The Docker validation script builds in `/tmp`, runs CTest, validates Linux
desktop/AppStream metadata, and creates `.deb` and `.rpm` packages with CPack.
The workflow runs the same CMake/CPack flow on clean GitHub-hosted runners.

The CPack generators create `.deb` and `.rpm` packages from the CMake install
rules. The `.deb` and `.rpm` metadata declare `mkvtoolnix` as a runtime
dependency where the format supports dependency metadata. The application still
resolves `mkvmerge`, `mkvinfo`, and `mkvextract` directly at runtime.

## Windows Debug Build

Prerequisites:

- Visual Studio 2022 with C++ desktop tools
- CMake 3.21 or newer
- Qt 6.2 or newer for MSVC
- MKVToolNix installed separately for runtime analysis/extraction

```powershell
cmake -S . -B build\cpp -G "Visual Studio 17 2022" -A x64
cmake --build build\cpp --config Debug
ctest --test-dir build\cpp -C Debug --output-on-failure
cmake --install build\cpp --config Debug --prefix build\cpp-install
```

After install, run Qt deployment from a Qt-enabled developer prompt:

```powershell
windeployqt build\cpp-install\bin\gMKVExtractGUIQt.exe
```

Keep the installed `gmkvextract-*.json` locale files beside the executable so
runtime localization and the Translation Editor can load and save the existing
JSON format without conversion.

## Windows Release Packages

The release workflow builds Windows x64 with Visual Studio 2022 and Qt for
MSVC, installs the app to `build/cpp-install`, and then runs:

```powershell
windeployqt build\cpp-install\bin\gMKVExtractGUIQt.exe
```

After Qt deployment, the workflow creates:

- `gMKVExtractGUI-<version>-windows-x64-portable.zip`
- `gMKVExtractGUI-<version>-windows-x64-installer.exe`

The installer is generated with NSIS from
`packaging/windows/gMKVExtractGUI.nsi`. Windows packages do not bundle
MKVToolNix; users install MKVToolNix separately and the app detects its
installation directory.

## GitHub Release Workflow

`.github/workflows/release-packages.yml` runs on:

- `workflow_dispatch` for test packaging runs
- `push` tags matching `v*` for release publishing

Tag builds create or update the matching GitHub Release and upload all package
assets. Manual dispatch builds still upload artifacts to the workflow run
without publishing a GitHub Release.
