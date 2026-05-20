# gMKVExtractGUI C++/Qt

Native C++/Qt desktop application for inspecting Matroska/WebM files and
extracting tracks, timecodes, cues, chapters, tags, attachments, and CUE sheets
through MKVToolNix command-line tools.

## English

### Project Status

- Active development fork: `https://github.com/dibin666/gMKVExtractGUI.git`
- Active branch: `cpp-migration`
- Upstream project: `https://github.com/Gpower2/gMKVExtractGUI`
- Legacy C# source recovery point: branch `legacy-csharp-archive` and tag
  `legacy-csharp-before-cpp-cleanup`

This repository now treats the native C++/Qt application as the active product.
The original repository remains configured as `upstream` for future syncs.

### Features

- Analyze MKV/WebM files with `mkvmerge` and `mkvinfo`.
- Extract tracks, timecodes, cues, chapters, tags, attachments, and CUE sheets
  with `mkvextract`.
- Batch jobs, save/load job lists, progress display, logs, abort controls, and
  overwrite/source-directory options.
- Custom output filename patterns for video, audio, subtitles, chapters,
  attachments, tags, timecodes, cues, and CUE sheets.
- Runtime language switching, dark mode, and an in-app translation editor.
- First launch follows the system language: Chinese locales default to
  Simplified Chinese, all other locales default to English.

### Linux Usage

Linux does not use MKVToolNix directory detection. Install the command-line
tools and keep them on `PATH`, or choose the `mkvmerge` executable in the app.
The app resolves `mkvmerge`, `mkvinfo`, and `mkvextract` directly.

Build and run:

```bash
cmake -S . -B build/cpp -DCMAKE_BUILD_TYPE=Debug
cmake --build build/cpp
./build/cpp/src/gMKVExtractGUI.Cpp/gMKVExtractGUIQt
```

Install into a staging directory:

```bash
cmake --install build/cpp --prefix build/cpp-install
./build/cpp-install/bin/gMKVExtractGUIQt
```

Runtime prerequisites:

- CMake 3.21 or newer for building
- C++17 compiler
- Qt 6.2 or newer with Core, Widgets, and Test modules
- `mkvmerge`, `mkvinfo`, and `mkvextract`

Release packages:

- `.deb` for Debian/Ubuntu compatible systems on `amd64` and `arm64`
- `.rpm` for RPM compatible systems on `x86_64` and `aarch64`
- AppImage for `x86_64` and `aarch64`

Linux packages do not bundle MKVToolNix. Install the distro's `mkvtoolnix`
package or otherwise provide `mkvmerge`, `mkvinfo`, and `mkvextract`.

### Windows Usage

Install MKVToolNix normally. On Windows, the app detects an MKVToolNix
installation directory and validates that `mkvmerge.exe`, `mkvinfo.exe`, and
`mkvextract.exe` are present.

Build from a Visual Studio and Qt enabled developer prompt:

```powershell
cmake -S . -B build\cpp -G "Visual Studio 17 2022" -A x64
cmake --build build\cpp --config Debug
ctest --test-dir build\cpp -C Debug --output-on-failure
cmake --install build\cpp --config Debug --prefix build\cpp-install
windeployqt build\cpp-install\bin\gMKVExtractGUIQt.exe
```

Release packages:

- Windows x64 installer `.exe`
- Windows x64 portable `.zip`

Windows packages do not bundle MKVToolNix. Install MKVToolNix separately so the
app can detect its directory.

### Language Files

Locale JSON files live in
`src/gMKVExtractGUI.Cpp/resources/locales/gmkvextract-*.json` and are copied
beside the executable during build/install. English and Simplified Chinese are
the primary maintained languages for this migration pass, while the existing
locale set remains available.

### Documentation

- User manual: [docs/README.md](docs/README.md)
- Build and package notes:
  [docs/CPP_BUILD_AND_PACKAGE.md](docs/CPP_BUILD_AND_PACKAGE.md)
- Manual parity checklist:
  [docs/CPP_MANUAL_PARITY_CHECKLIST.md](docs/CPP_MANUAL_PARITY_CHECKLIST.md)
- Translator guide: [docs/TRANSLATOR_GUIDE.md](docs/TRANSLATOR_GUIDE.md)

## 中文

### 项目状态

- 当前开发 fork：`https://github.com/dibin666/gMKVExtractGUI.git`
- 当前开发分支：`cpp-migration`
- 原作者仓库：`https://github.com/Gpower2/gMKVExtractGUI`
- 旧 C# 源码恢复点：分支 `legacy-csharp-archive`，标签
  `legacy-csharp-before-cpp-cleanup`

本仓库当前以原生 C++/Qt 程序作为主产品继续开发。原作者仓库保留为
`upstream`，便于以后同步。

### 功能

- 使用 `mkvmerge` 和 `mkvinfo` 分析 MKV/WebM 文件。
- 使用 `mkvextract` 提取轨道、时间码、Cue、章节、标签、附件和 CUE
  Sheet。
- 支持批量任务、任务保存/加载、进度显示、日志、终止任务、覆盖输出和使用源
  目录输出。
- 支持视频、音频、字幕、章节、附件、标签、时间码、Cue、CUE Sheet 的自定义
  输出文件名模式。
- 支持运行时切换语言、深色模式和内置翻译编辑器。
- 首次启动会跟随系统语言：中文系统默认简体中文，其他系统默认英文。

### Linux 使用方式

Linux 下不使用 MKVToolNix 目录检测，也不依赖 `mkvtoolnix` GUI 目录。请安装
`mkvmerge`、`mkvinfo` 和 `mkvextract`，并将它们放到 `PATH`；也可以在程序中手动
选择 `mkvmerge` 可执行文件。程序会直接解析这三个命令行工具。

构建并运行：

```bash
cmake -S . -B build/cpp -DCMAKE_BUILD_TYPE=Debug
cmake --build build/cpp
./build/cpp/src/gMKVExtractGUI.Cpp/gMKVExtractGUIQt
```

安装到临时目录后运行：

```bash
cmake --install build/cpp --prefix build/cpp-install
./build/cpp-install/bin/gMKVExtractGUIQt
```

构建和运行依赖：

- CMake 3.21 或更新版本
- 支持 C++17 的编译器
- Qt 6.2 或更新版本，包含 Core、Widgets、Test 模块
- `mkvmerge`、`mkvinfo`、`mkvextract`

发布包：

- 适用于 Debian/Ubuntu 兼容系统的 `amd64` 和 `arm64` `.deb`
- 适用于 RPM 兼容系统的 `x86_64` 和 `aarch64` `.rpm`
- 适用于 `x86_64` 和 `aarch64` 的 AppImage

Linux 包不会内置 MKVToolNix。请安装发行版的 `mkvtoolnix` 包，或者用其他方式提供
`mkvmerge`、`mkvinfo` 和 `mkvextract`。

### Windows 使用方式

Windows 下请正常安装 MKVToolNix。程序会检测 MKVToolNix 安装目录，并验证其中
是否包含 `mkvmerge.exe`、`mkvinfo.exe` 和 `mkvextract.exe`。

在已配置 Visual Studio 和 Qt 的开发命令行中构建：

```powershell
cmake -S . -B build\cpp -G "Visual Studio 17 2022" -A x64
cmake --build build\cpp --config Debug
ctest --test-dir build\cpp -C Debug --output-on-failure
cmake --install build\cpp --config Debug --prefix build\cpp-install
windeployqt build\cpp-install\bin\gMKVExtractGUIQt.exe
```

发布包：

- Windows x64 安装器 `.exe`
- Windows x64 便携版 `.zip`

Windows 包不会内置 MKVToolNix。请单独安装 MKVToolNix，程序会检测其安装目录。

### 语言文件

语言 JSON 文件位于
`src/gMKVExtractGUI.Cpp/resources/locales/gmkvextract-*.json`，构建和安装时会
复制到可执行文件旁边。本次迁移以英文和简体中文作为主要维护语言，同时保留原有
语言文件。

### 文档

- 用户手册：[docs/README.md](docs/README.md)
- 构建和打包说明：
  [docs/CPP_BUILD_AND_PACKAGE.md](docs/CPP_BUILD_AND_PACKAGE.md)
- 手动对等检查清单：
  [docs/CPP_MANUAL_PARITY_CHECKLIST.md](docs/CPP_MANUAL_PARITY_CHECKLIST.md)
- 翻译指南：[docs/TRANSLATOR_GUIDE.md](docs/TRANSLATOR_GUIDE.md)

## License

This software is dedicated to the Public Domain under the terms of
[The Unlicense](https://unlicense.org/).
