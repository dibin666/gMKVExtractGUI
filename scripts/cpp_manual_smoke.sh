#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="${repo_root}/build/cpp"
work_dir="${repo_root}/build/cpp-manual-smoke"
base_file="${work_dir}/base.mkv"
sample_file="${work_dir}/sample.mkv"
extract_dir="${work_dir}/extract"
subtitle_file="${work_dir}/subtitle.srt"
chapters_file="${work_dir}/chapters.xml"
tags_file="${work_dir}/tags.xml"
attachment_file="${work_dir}/attachment.txt"

require_tool() {
    if ! command -v "$1" >/dev/null 2>&1; then
        echo "Missing required tool: $1" >&2
        exit 2
    fi
}

require_nonempty_file() {
    if [[ ! -s "$1" ]]; then
        echo "Expected non-empty file was not created: $1" >&2
        exit 5
    fi
}

require_nonempty_specs() {
    local spec
    local output_file
    for spec in "$@"; do
        output_file="${spec#*:}"
        require_nonempty_file "${output_file}"
    done
}

require_tool cmake
require_tool ffmpeg
require_tool mkvmerge
require_tool mkvinfo
require_tool mkvextract

mkvtoolnix_dir="$(dirname "$(command -v mkvmerge)")"

cmake -S "${repo_root}" -B "${build_dir}" -DCMAKE_BUILD_TYPE=Debug
cmake --build "${build_dir}"
/usr/bin/ctest --test-dir "${build_dir}" --output-on-failure

rm -rf "${work_dir}"
mkdir -p "${extract_dir}"

ffmpeg -hide_banner -loglevel error -y \
    -f lavfi -i testsrc=duration=2:size=128x72:rate=24 \
    -f lavfi -i sine=frequency=1000:duration=2 \
    -c:v ffv1 -c:a flac \
    "${base_file}"

cat > "${subtitle_file}" <<'SRT'
1
00:00:00,000 --> 00:00:01,000
gMKVExtractGUI smoke subtitle

2
00:00:01,000 --> 00:00:02,000
Subtitle extraction coverage
SRT

cat > "${chapters_file}" <<'XML'
<?xml version="1.0" encoding="UTF-8"?>
<Chapters>
  <EditionEntry>
    <ChapterAtom>
      <ChapterTimeStart>00:00:00.000</ChapterTimeStart>
      <ChapterTimeEnd>00:00:01.000</ChapterTimeEnd>
      <ChapterDisplay>
        <ChapterString>Smoke Start</ChapterString>
        <ChapterLanguage>eng</ChapterLanguage>
      </ChapterDisplay>
    </ChapterAtom>
    <ChapterAtom>
      <ChapterTimeStart>00:00:01.000</ChapterTimeStart>
      <ChapterTimeEnd>00:00:02.000</ChapterTimeEnd>
      <ChapterDisplay>
        <ChapterString>Smoke End</ChapterString>
        <ChapterLanguage>eng</ChapterLanguage>
      </ChapterDisplay>
    </ChapterAtom>
  </EditionEntry>
</Chapters>
XML

cat > "${tags_file}" <<'XML'
<?xml version="1.0" encoding="UTF-8"?>
<Tags>
  <Tag>
    <Targets />
    <Simple>
      <Name>TITLE</Name>
      <String>gMKVExtractGUI Smoke Sample</String>
    </Simple>
  </Tag>
</Tags>
XML

printf 'gMKVExtractGUI smoke attachment\n' > "${attachment_file}"

mkvmerge -o "${sample_file}" \
    --title "gMKVExtractGUI Smoke Sample" \
    --chapters "${chapters_file}" \
    --global-tags "${tags_file}" \
    --attachment-mime-type text/plain \
    --attachment-name smoke-attachment.txt \
    --attach-file "${attachment_file}" \
    "${base_file}" \
    --language 0:eng \
    --track-name 0:"Smoke Subtitles" \
    "${subtitle_file}" >/dev/null

mkvmerge -J "${sample_file}" > "${work_dir}/identify.json"
mkvinfo "${sample_file}" > "${work_dir}/mkvinfo.txt"

mapfile -t track_args < <(python3 - "${work_dir}/identify.json" "${extract_dir}" <<'PY'
import json
import pathlib
import sys

identify_path = pathlib.Path(sys.argv[1])
extract_dir = pathlib.Path(sys.argv[2])
data = json.loads(identify_path.read_text(encoding="utf-8"))
for track in data.get("tracks", []):
    track_id = track.get("id")
    track_type = track.get("type", "track")
    if track_id is None:
        continue
    print(f"{track_id}:{extract_dir / f'{track_type}_{track_id}.bin'}")
PY
)

mapfile -t timestamp_args < <(python3 - "${work_dir}/identify.json" "${extract_dir}" <<'PY'
import json
import pathlib
import sys

identify_path = pathlib.Path(sys.argv[1])
extract_dir = pathlib.Path(sys.argv[2])
data = json.loads(identify_path.read_text(encoding="utf-8"))
for track in data.get("tracks", []):
    track_id = track.get("id")
    track_type = track.get("type", "track")
    if track_id is None:
        continue
    print(f"{track_id}:{extract_dir / f'{track_type}_{track_id}_timestamps.txt'}")
PY
)

mapfile -t cue_args < <(python3 - "${work_dir}/identify.json" "${extract_dir}" <<'PY'
import json
import pathlib
import sys

identify_path = pathlib.Path(sys.argv[1])
extract_dir = pathlib.Path(sys.argv[2])
data = json.loads(identify_path.read_text(encoding="utf-8"))
for track in data.get("tracks", []):
    track_id = track.get("id")
    track_type = track.get("type", "track")
    if track_id is None or track_type != "video":
        continue
    print(f"{track_id}:{extract_dir / f'{track_type}_{track_id}_cues.txt'}")
PY
)

mapfile -t attachment_args < <(python3 - "${work_dir}/identify.json" "${extract_dir}" <<'PY'
import json
import pathlib
import re
import sys

identify_path = pathlib.Path(sys.argv[1])
extract_dir = pathlib.Path(sys.argv[2])
data = json.loads(identify_path.read_text(encoding="utf-8"))
for attachment in data.get("attachments", []):
    attachment_id = attachment.get("id")
    name = attachment.get("file_name") or f"attachment_{attachment_id}"
    safe_name = re.sub(r"[^A-Za-z0-9._-]+", "_", name)
    if attachment_id is None:
        continue
    print(f"{attachment_id}:{extract_dir / safe_name}")
PY
)

if ((${#track_args[@]} == 0)); then
    echo "No extractable tracks were found in ${sample_file}" >&2
    exit 3
fi

if ((${#attachment_args[@]} == 0)); then
    echo "No extractable attachments were found in ${sample_file}" >&2
    exit 4
fi

mkvextract "${sample_file}" tracks "${track_args[@]}"
require_nonempty_specs "${track_args[@]}"
mkvextract "${sample_file}" timestamps_v2 "${timestamp_args[@]}"
require_nonempty_specs "${timestamp_args[@]}"
mkvextract "${sample_file}" cues "${cue_args[@]}"
require_nonempty_specs "${cue_args[@]}"
mkvextract "${sample_file}" chapters "${extract_dir}/chapters.xml"
require_nonempty_file "${extract_dir}/chapters.xml"
mkvextract "${sample_file}" tags "${extract_dir}/tags.xml"
require_nonempty_file "${extract_dir}/tags.xml"
mkvextract "${sample_file}" attachments "${attachment_args[@]}"
require_nonempty_specs "${attachment_args[@]}"
if ! mkvextract "${sample_file}" cuesheet "${extract_dir}/sample.cue" 2> "${work_dir}/cuesheet.err"; then
    echo "Generated smoke fixture does not expose a CUE sheet; see ${work_dir}/cuesheet.err"
else
    require_nonempty_file "${extract_dir}/sample.cue"
fi
"${build_dir}/src/gMKVExtractGUI.Cpp/gMKVExtractGUIQt" --analyze-smoke-test "${mkvtoolnix_dir}" "${sample_file}"
"${build_dir}/src/gMKVExtractGUI.Cpp/gMKVExtractGUIQt" --smoke-test

echo "Manual smoke artifacts written to ${work_dir}"
