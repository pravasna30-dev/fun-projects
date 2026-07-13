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
