# C++/Qt Manual Parity Checklist

Use this checklist before declaring the C++ rewrite feature-complete. The C++
app remains side-by-side with the C# application until this checklist has real
Linux evidence. Windows smoke evidence is retained for release packaging, but it
was explicitly skipped for the 2026-05-20 implementation pass by user request.

## Prerequisites

- Qt 6.2 or newer.
- CMake 3.21 or newer.
- A C++17 compiler.
- MKVToolNix installed and available as `mkvmerge`, `mkvinfo`, and
  `mkvextract`.
- At least one representative Matroska/WebM file, preferably with video, audio,
  subtitle, chapters, attachments, tags, and non-English language metadata.

## Build And Install

| Check | Linux Evidence | Windows Evidence |
| --- | --- | --- |
| Configure clean tree | 2026-05-20: `cmake -S . -B build/cpp -DCMAKE_BUILD_TYPE=Debug` passed | Skipped by user request |
| Build | 2026-05-20: `cmake --build build/cpp` passed | Skipped by user request |
| Tests | 2026-05-20: `/usr/bin/ctest --test-dir build/cpp --output-on-failure` passed, 2/2 tests | Skipped by user request |
| Install/package staging | 2026-05-20: `cmake --install build/cpp --prefix build/cpp-install` passed | Skipped by user request; command remains `cmake --install build\cpp --config Debug --prefix build\cpp-install`, then run `windeployqt` |
| Locale files staged beside executable | 2026-05-20: 18 `gmkvextract-*.json` files staged in `build/cpp-install/bin` | Skipped by user request |

## Linux Smoke Evidence

2026-05-20: `scripts/cpp_manual_smoke.sh` passed on Linux Mint 22.3. The script
generated `build/cpp-manual-smoke/sample.mkv` with video, audio, SRT subtitles,
chapters, global tags, and an attachment. It identified the file with
`mkvmerge -J`, captured `mkvinfo` output, extracted tracks, timestamps, cues,
chapters, tags, attachments, and a CUE sheet with `mkvextract`, and ran
`gMKVExtractGUIQt --analyze-smoke-test <mkvtoolnix-dir> sample.mkv` to verify
the C++ analyzer sees video, audio, subtitles, chapters, and attachments. It
also ran `gMKVExtractGUIQt --smoke-test`. The Qt smoke path constructs and shows
the Main, Options, Log, Job Manager, and Translation Editor windows in offscreen
mode, then asserts representative controls exist in each window.

## Main Window

| Check | Expected Result | Evidence |
| --- | --- | --- |
| Launch without .NET/Mono | Qt app opens directly | |
| MKVToolNix path Browse and Auto Detect | Valid folder is accepted; invalid folder is rejected | |
| Drag/drop file | File appears in input tree and selected-file info updates | |
| Drag/drop directory | Supported files are discovered recursively | |
| Append on drag/drop | Enabled appends, disabled replaces | |
| Segment tree | Tracks, chapters, attachments, tags/timecode-capable items are visible and checkable | |
| Context menu basic selection | Check/uncheck all and by type works | |
| Context menu advanced filters | Language, IETF language, codec, resolution/channels, name, and forced filters work | |
| Open/remove actions | Open selected file/folder, remove selected, and remove all work | |
| Output directory Browse | Folder selection updates output path | |
| Output default context menu | Set As Default and Use Currently Set Default Directory work | |
| Use Source | Output path follows selected source file directory | |
| Dark Mode | Main, context menus, Log, Job Manager, Options, and Translation Editor remain readable | |
| Runtime language switching | Changing culture in Options refreshes open Main, Log, and Job Manager windows | |

## Extraction Modes

Run each mode against representative input and compare output names with the C#
reference app using the same settings file and filename patterns.

| Mode | Expected Result | Evidence |
| --- | --- | --- |
| Tracks | Selected tracks extracted with expected extensions | |
| Cue_Sheet | CUE sheet output created where supported | |
| Tags | Tags output created | |
| Timecodes | Timecode files created for selected tracks | |
| Tracks_And_Timecodes | Both tracks and timecodes created | |
| Cues | Cue output created for selected tracks | |
| Tracks_And_Cues | Both tracks and cues created | |
| Tracks_And_Cues_And_Timecodes | Tracks, cues, and timecodes created | |
| Chapter XML/OGM/CUE/PBF | Selected chapter format is honored | |
| Raw/fullraw/BOM options | Command output matches selected advanced options | |
| Overwrite disabled | Existing output names are de-duplicated | |
| Overwrite enabled | Existing output files may be replaced | |
| Abort current | Current extraction stops without freezing UI | |
| Abort all | Remaining queued work stops and UI returns to ready state | |

## Secondary Windows

| Window | Check | Expected Result | Evidence |
| --- | --- | --- | --- |
| Options | Placeholder Add menus | Insert placeholders at cursor | |
| Options | Default and Defaults buttons | Reset single/all filename patterns | |
| Options | Advanced extraction options | Values persist in `gMKVExtractGUI.ini` | |
| Log | Refresh, clear, copy, save | Log text updates and can be saved | |
| Job Manager | Add jobs, save, load | Jobs survive app restart through XML | |
| Job Manager | Select/deselect/change-ready context menu | Selected rows update correctly | |
| Job Manager | Run Jobs, abort, abort all, popup | Queue state and progress update correctly | |
| Translation Editor | Load/create/sync/filter/save locale | Existing `gmkvextract-*.json` contract is preserved | |
| Translation Editor | Notes column | Read-only context; not saved as user edits | |

## Localization Fallback

| Check | Expected Result | Evidence |
| --- | --- | --- |
| Existing locale load | All packaged cultures appear in Options | |
| Missing locale directory | Built-in English fallback keeps UI usable | |
| Partial locale | Missing keys fall back to English | |
| Bad format string | Lookup reports `!BadFormat:Key!` and logs the error | |
| Culture aliases | `cn`, `zh-cn`, and `zh-tw` resolve as documented | |

## Completion Gate

The rewrite is not complete until every row above has dated evidence for Linux.
Windows build/launch/package evidence should be gathered on a Windows host before
release packaging unless the project owner explicitly waives that pass.
