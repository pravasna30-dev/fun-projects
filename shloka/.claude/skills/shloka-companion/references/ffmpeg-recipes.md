# ffmpeg/ffprobe Recipes

## 1. Basic stream info (duration, is there a video track, resolution)

```bash
ffprobe -v error -show_entries stream=index,codec_type,codec_name,width,height,r_frame_rate,duration \
  -of default=noprint_wrappers=1 "input.mp4"
```

## 2. Extract a representative frame to inspect the video visually

Read the resulting PNG with the Read tool afterward — this is how you spot
label logos, "Sung by ..." credits, or other commercial-release markers
before trusting a file's provenance.

```bash
ffmpeg -y -ss 60 -i "input.mp4" -frames:v 1 -update 1 /path/to/frame_preview.png
```

(`-ss 60` grabs a frame at 60 seconds in — pick a timestamp inside the
content, not the very start, in case there's a blank intro.)

## 3. Silence/breath-pause detection (for the estimation fallback only)

Start strict, loosen if nothing shows up (a continuous fast chant with
background drone often needs a higher noise floor and shorter minimum
duration to catch brief breath pauses):

```bash
ffmpeg -i "input.mp4" -af silencedetect=noise=-30dB:d=0.4 -f null - 2>&1 | grep -E "silence_(start|end)"

# if that finds ~nothing, loosen:
ffmpeg -i "input.mp4" -af silencedetect=noise=-20dB:d=0.15 -f null - 2>&1 | grep -E "silence_(start|end)"
```

Remember: gap spacing tells you *where breaths happen*, not *which verse is
being said*. Use it only to estimate pace (average seconds per printed line),
and label any resulting timestamps as unverified estimates.

## 4. Recording-based concatenation with a fixed silence gap (preferred path)

Have the user record one short clip per chapter, named so they sort in
order (`01.wav`, `02.wav`, ... `34.wav` — zero-padded, matching the
companion doc's chapter numbers). Then:

```bash
GAP=2   # seconds of silence between chapters
WORKDIR=$(mktemp -d)

# silence clip matching the recordings' sample rate/channel layout
ffmpeg -y -f lavfi -i "anullsrc=r=48000:cl=stereo" -t $GAP "$WORKDIR/silence.wav" -loglevel error

CONCAT_LIST="$WORKDIR/list.txt"; > "$CONCAT_LIST"
cumulative=0
i=0
total=$(ls "$RECORDINGS_DIR" | wc -l)

for f in $(ls "$RECORDINGS_DIR" | sort -V); do
  i=$((i+1))
  norm="$WORKDIR/norm_$i.wav"
  ffmpeg -y -i "$RECORDINGS_DIR/$f" -ar 48000 -ac 2 "$norm" -loglevel error
  dur=$(ffprobe -v error -show_entries format=duration -of default=noprint_wrappers=1:nokey=1 "$norm")

  mins=$(( ${cumulative%.*} / 60 )); secs=$(( ${cumulative%.*} % 60 ))
  printf "%d:%02d  %s\n" "$mins" "$secs" "$f"   # this is your exact chapter timestamp

  echo "file '$norm'" >> "$CONCAT_LIST"
  cumulative=$(echo "$cumulative + $dur" | bc)
  if [ "$i" -lt "$total" ]; then
    echo "file '$WORKDIR/silence.wav'" >> "$CONCAT_LIST"
    cumulative=$(echo "$cumulative + $GAP" | bc)
  fi
done

ffmpeg -y -f concat -safe 0 -i "$CONCAT_LIST" -c copy "$WORKDIR/final_audio.wav" -loglevel error
```

This gives exact cumulative timestamps as a side effect of construction —
no estimation, no drift.

## 5. Mux audio with a generated background into a video file

Check for `drawtext` support first (some minimal ffmpeg builds lack
freetype and will error with "No such filter: 'drawtext'"):

```bash
ffmpeg -hide_banner -filters 2>&1 | grep -i drawtext
```

If available, a solid/gradient background with title text:

```bash
ffmpeg -y -f lavfi -i "color=c=0x0b1a33:s=1080x1080:d=<duration>" -i "audio.wav" \
  -vf "drawtext=fontfile='/System/Library/Fonts/Supplemental/Georgia Bold.ttf':text='<Title>':fontsize=90:fontcolor=white:x=(w-text_w)/2:y=(h-text_h)/2" \
  -map 0:v -map 1:a -c:v libx264 -pix_fmt yuv420p -c:a aac -b:a 192k -shortest "output.mp4"
```

If `drawtext` isn't available, skip text and use a plain generated
background (still avoids reusing any branded/commercial cover art):

```bash
ffmpeg -y -f lavfi -i "gradients=s=1080x1080:d=<duration>:c0=0x0b1a33:c1=0x3a1f0b:x0=0:y0=0:x1=1080:y1=1080" -i "audio_or_video_source" \
  -map 0:v -map 1:a -c:v libx264 -pix_fmt yuv420p -c:a aac -b:a 192k -shortest "output.mp4"
```

Set `-d` generously past the actual audio length — `-shortest` trims the
output to match the audio track regardless.

## 6. Python tooling (Pillow, Whisper) — use an isolated venv

The system Python here is externally-managed (PEP 668) — a plain `pip
install` fails with an error pointing at `--break-system-packages`. Don't
use that flag; create a throwaway venv in scratchpad instead, which avoids
touching the system install entirely and needs no cleanup decision later:

```bash
python3 -m venv /path/to/scratchpad/venv
source /path/to/scratchpad/venv/bin/activate
pip install --quiet Pillow openai-whisper
```

## 7. Burning text onto stanza images with Pillow (drawtext workaround)

When `ffmpeg drawtext` isn't available (see SKILL.md Step 7), composite text
directly onto the source image before it ever reaches ffmpeg:

```python
from PIL import Image, ImageDraw, ImageFont

W, H = 1280, 720
img = Image.open(source_path).convert("RGB")
ratio = min(W / img.width, H / img.height)
resized = img.resize((int(img.width * ratio), int(img.height * ratio)), Image.LANCZOS)
canvas = Image.new("RGB", (W, H), (10, 15, 25))
canvas.paste(resized, ((W - resized.width) // 2, (H - resized.height) // 2))

overlay = Image.new("RGBA", (W, H), (0, 0, 0, 0))
draw = ImageDraw.Draw(overlay)
font = ImageFont.truetype("/System/Library/Fonts/Supplemental/Georgia Bold.ttf", 42)
draw.rectangle([(0, 0), (W, 70)], fill=(0, 0, 0, 170))       # semi-transparent bar
draw.text((24, 14), "Verse 1 — Tato Yuddha", font=font, fill=(255, 255, 255, 255))

Image.alpha_composite(canvas.convert("RGBA"), overlay).convert("RGB").save(out_path, quality=90)
```

Then feed the composited images into the normal `-loop 1 -i image -t
<duration>` segment-building recipe (#4/#5 above) — no `drawtext` filter
needed anywhere in the ffmpeg pipeline.

## 8. Sourcing images from Wikimedia Commons without getting rate-limited

Search with `site:commons.wikimedia.org` in the query. Before using a file,
fetch its `/wiki/File:...` page and confirm the license (public domain, CC0,
or CC BY-SA — note the credit name required for the latter). Then download
the **direct** `upload.wikimedia.org` URL from that page.

Downloading many files back-to-back in a tight loop triggers a 429 "Too many
requests" error. If that happens, don't just retry the same URL — either add
a short delay between requests, or switch to a `thumb/` URL, which is
usually enough to route around the block:

```
https://upload.wikimedia.org/wikipedia/commons/thumb/<hash-path>/<file>/<width>px-<file>
```

Wikimedia only serves a fixed whitelist of thumbnail widths and 400s on
anything else ("Use thumbnail sizes listed on ..."): **20, 40, 60, 120, 250,
330, 500, 960, 1280, 1920, 3840**. Use one of these, not an arbitrary number.

## 9. Multi-step ffmpeg loops: write a `.sh` file, don't inline bash arrays

Constructs like `${!array[@]}` or `${#array[@]}` are real bash syntax but
fail with "bad substitution" if the command actually executes under zsh
(this environment's default shell). Rather than debugging shell-specific
array semantics inline, write the loop to a standalone `.sh` file and
execute it with an explicit `bash script.sh` — guarantees bash semantics
regardless of what shell the Bash tool would otherwise use.

## 10. Getting real timestamps out of a continuous recording (Whisper)

See SKILL.md Step 5, fallback tier 2, for when to reach for this. Practical
steps once Pillow/Whisper are installed per recipe #6:

```python
import whisper, json

model = whisper.load_model("base")
result = model.transcribe(audio_path, word_timestamps=True, verbose=False)

for seg in result["segments"]:
    print(f'{seg["start"]:.1f} - {seg["end"]:.1f}: {seg["text"].strip()}')
```

Read the printed (garbled) segments against the known Sanskrit text in
order — the phonetic resemblance is normally close enough to match each
segment to the correct verse-line despite Whisper mis-transcribing the
actual words. Segment *start times* are what you want; a verse's start time
is the start of its first line's segment. Watch for:
- **Occasional hallucinated repeated syllables** (e.g. a run of "ya ya ya
  ya...") on a held or emphasized sound — the segment's start time is still
  usable even when its text is garbage.
- **A recording covering less (or different) content than assumed** — if
  the last real segment ends well before the audio file's actual end, or a
  verse you expected to hear never shows up in order, the recording may
  simply not contain what you assumed it did. Don't paper over this by
  forcing a timestamp; go back and confirm the actual chapter list against
  what was really recorded.
