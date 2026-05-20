# gMKVExtractGUI C++/Qt

<p align="center">
  <a href="#中文"><kbd>中文</kbd></a>
  <a href="#english"><kbd>English</kbd></a>
</p>

## 中文

### 项目信息

gMKVExtractGUI C++/Qt 是 gMKVExtractGUI 的原生 C++/Qt 移植版本，用于查看
Matroska/WebM 文件并通过 MKVToolNix 命令行工具提取轨道、字幕、章节、标签、
附件、时间码、Cue 和 CUE Sheet。

- 发布下载：[Releases](https://github.com/dibin666/gMKVExtractGUI/releases)
- 当前开发仓库：`https://github.com/dibin666/gMKVExtractGUI`
- 原项目：`https://github.com/Gpower2/gMKVExtractGUI`

### Linux 使用

下载适合系统架构的 `.deb` 或 `.rpm` 包并安装。Linux 版本不会检测 MKVToolNix
目录，而是直接调用 `mkvmerge`、`mkvinfo` 和 `mkvextract`；请确保这三个命令在
`PATH` 中，或在程序中手动选择 `mkvmerge` 可执行文件。

```bash
sudo apt install ./gMKVExtractGUI-0.1.0-linux-amd64.deb
```

```bash
sudo dnf install ./gMKVExtractGUI-0.1.0-linux-x86_64.rpm
```

### Windows 使用

下载 Windows x64 安装器 `.exe` 或便携版 `.zip`。Windows 包不会内置
MKVToolNix；请先单独安装 MKVToolNix，程序会检测其安装目录并使用
`mkvmerge.exe`、`mkvinfo.exe` 和 `mkvextract.exe`。

### 致谢

- 原项目：Gpower2/gMKVExtractGUI
- MKVToolNix：Moritz Bunkus 及贡献者
- Qt：The Qt Company 及 Qt 项目贡献者
- C++/Qt 移植与维护：dibin666

## English

### Project

gMKVExtractGUI C++/Qt is a native C++/Qt port of gMKVExtractGUI. It inspects
Matroska/WebM files and extracts tracks, subtitles, chapters, tags,
attachments, timecodes, cues, and CUE sheets through MKVToolNix command-line
tools.

- Downloads: [Releases](https://github.com/dibin666/gMKVExtractGUI/releases)
- Active fork: `https://github.com/dibin666/gMKVExtractGUI`
- Original project: `https://github.com/Gpower2/gMKVExtractGUI`

### Linux Usage

Download and install the `.deb` or `.rpm` package for your architecture. The
Linux build does not detect an MKVToolNix directory; it calls `mkvmerge`,
`mkvinfo`, and `mkvextract` directly. Keep those commands on `PATH`, or choose
the `mkvmerge` executable manually in the app.

```bash
sudo apt install ./gMKVExtractGUI-0.1.0-linux-amd64.deb
```

```bash
sudo dnf install ./gMKVExtractGUI-0.1.0-linux-x86_64.rpm
```

### Windows Usage

Download the Windows x64 installer `.exe` or portable `.zip`. Windows packages
do not bundle MKVToolNix; install MKVToolNix separately, then the app will
detect its installation directory and use `mkvmerge.exe`, `mkvinfo.exe`, and
`mkvextract.exe`.

### Credits

- Original project: Gpower2/gMKVExtractGUI
- MKVToolNix: Moritz Bunkus and contributors
- Qt: The Qt Company and Qt Project contributors
- C++/Qt port and maintenance: dibin666
