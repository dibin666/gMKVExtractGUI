# C++/Qt Build And Package Notes

The C++ rewrite builds side-by-side with the existing C# application. It does
not embed the Matroska command-line tools. On Linux, users need `mkvmerge`,
`mkvinfo`, and `mkvextract` available on `PATH` or can choose the `mkvmerge`
executable manually. On Windows, users still need MKVToolNix installed so the
app can detect its installation directory and run the three `.exe` tools.

Before calling the rewrite feature-complete, record Linux and Windows evidence
in [CPP_MANUAL_PARITY_CHECKLIST.md](CPP_MANUAL_PARITY_CHECKLIST.md). On Linux,
`scripts/cpp_manual_smoke.sh` can generate a tiny MKV fixture and verify the
local C++ build plus direct `mkvmerge`, `mkvinfo`, and `mkvextract` invocation.

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
`gMkvExtractGuiIcon.ico` under the install `bin` directory. Distributions should
package the required Qt runtime libraries through their normal dependency
system. An AppImage or distro package can be layered on top of the install tree.

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
