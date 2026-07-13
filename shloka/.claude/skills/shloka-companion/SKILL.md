---
name: shloka-companion
description: Guides turning a Sanskrit shloka/stotra (hymn) into a structured learning resource - reconciling the verse count against the source text, cross-checking verified English meanings across multiple published sources, producing a companion markdown doc (transliteration + meaning + YouTube chapter template), navigating audio-sourcing and copyright/licensing decisions (including spotting commercial/label recordings from video frames), computing real chapter timestamps, and filing everything under this repo's shloka/<hymn-name>/ convention. Use this whenever the user wants to learn, memorize, chant, or "master" a Sanskrit hymn/stotra/shloka, wants a YouTube audio track or video for a hymn with chapters, names a specific stotra (Aditya Hridayam, Vishnu Sahasranama, Lalitha Sahasranama, Suprabhatam, Hanuman Chalisa, etc.), or says things like "process this stotra," "add a hymn," "new shloka," or hands over a PDF/text of Sanskrit verses - even if they never say the word "skill."
---

# Shloka Companion

## What this is for

A repeatable pipeline for turning a source text of a Sanskrit hymn into
something a learner can actually use: a verified companion document, and
(eventually) a properly chaptered YouTube upload. It exists because this
workflow has real traps that are easy to get wrong quietly — miscounting
verses, trusting your own memory on scripture meaning, assuming a YouTube
video's audio is free to reuse, and generating chapter timestamps that look
precise but are actually guesses. Each section below explains the trap and
how to avoid it, not just the mechanical step.

Treat this as a *pipeline with checkpoints*, not a script to run start to
finish silently. The checkpoints (marked ⏸) are places where a past run of
this workflow got it wrong on the first guess — pause and ask there.

## Step 1 — Enumerate the source, don't assume the count

Read the source PDF/text in full. Count every distinct chantable chunk:
opening invocations, numbered verses, and any closing narrative/dhyana/
salutation verses that trail after the last numbered verse (these are easy
to miss because they're often unnumbered).

⏸ **If the user states a verse count from memory** ("it's 20 stanzas") **and
the source document has a different actual count, say so and ask before
proceeding.** People misremember hymn lengths often (conflating a commonly-
cited count with what a specific edition actually contains, or forgetting
trailing verses). Silently "fixing" the mismatch to match what they said
would mean building the rest of the pipeline on a wrong foundation. State
the actual count, describe what the extra/missing pieces are, and let them
choose (all of it, a subset, or their own grouping).

Also check whether a scribal colophon line appears at the end (e.g. "Iti
Aarshe Srimad Ramaayane... samaaptah" for Ramayana-sourced stotras) — this
marks the end of a chapter in the source epic and is traditionally read as a
note, not chanted. Call this out rather than silently including or excluding
it.

## Step 2 — Verify meaning, don't recite it from memory

Even for extremely famous stotras, don't generate the English meaning purely
from your own training. Fetch several independently-published translations
(word-by-word meaning sites like greenmesg.org, established devotional
translation sites, etc. — search for "<hymn name> full English translation
verse by verse meaning" and pull from a handful of results) and cross-check
them against each other before writing the final wording.

Two things repeatedly go wrong if you skip this:
- **WebFetch summarizes instead of quoting.** Long stotra pages often get
  compressed into vague paragraph summaries ("verses 6-15 describe...")
  instead of literal per-verse meaning. If that happens, re-fetch in smaller
  batches (5-10 verses at a time) and explicitly ask for the literal meaning
  of each verse individually, not a summary.
- **Some epithets are genuinely disputed among traditional commentators**
  (a name that could be a place, a symbolic image, or a proper noun,
  depending on the commentary tradition). Don't present one reading as the
  only one — say so in the companion doc, briefly, with the most commonly
  published interpretation as the default.

## Step 3 — Build the companion document

Produce one markdown file per hymn with, for every chunk from Step 1: the
transliteration exactly as given in the source, and the verified English
meaning from Step 2. See `references/companion-template.md` for the exact
structure to follow (per-chapter blocks, plus the YouTube chapter/description
template block with the real chapter-numbering rules explained there).

## Step 4 — Audio: the part that goes wrong if rushed

You cannot download audio from YouTube, and you cannot synthesize or record
a chanted recitation yourself. This means audio always comes from the user —
either their own recording, or an existing file they hand you. Either way,
don't treat "the user wants to use this audio for their upload" as settled
until you've actually looked.

**If the user wants to reuse an existing YouTube video's audio:** don't
assume Creative Commons just because someone says so casually, and don't
trust a claim like "it's Creative Commons" at face value if you haven't
verified it — people often mean "I think it's fine to use" rather than
having actually checked. Try to check the license yourself; YouTube's
description "License:" line is the ground truth, but the video page is
JS-rendered and a plain fetch tool will often only return page chrome, not
the actual description text. Ask the user to check it directly on the video
page if you can't confirm it yourself.

**If you get a local audio/video file to work with, look at it before
trusting its provenance.** Run it through `ffprobe` for basic stream info,
and pull a representative frame with `ffmpeg` (see
`references/ffmpeg-recipes.md`) to actually *look* at the video — commercial
devotional recordings are usually easy to spot this way: a named professional
singer credited ("Sung by..."), a music label logo (e.g. a specific record
label), or a distribution network watermark. That's a strong, checkable
signal of a commercial release, independent of whatever the user believes
about its licensing.

**If it turns out to be commercial/label content:** don't help publish or
upload a video built on that audio, and don't help "launder" it either —
swapping out the visual/cover art while keeping the copyrighted audio track
doesn't change what's actually being redistributed, and removing the
identifying branding while keeping someone else's performance makes it look
more like evasion, not less. Say this plainly and explain why, then pivot to
recommending the user record their own recitation — which conveniently also
serves whatever their actual goal usually is ("I want to master this hymn"),
with zero rights questions attached.

⏸ This is a real decision point — walk the user through the reasoning above
and let them choose the path (verify license / record their own / use the
existing file privately only), rather than picking for them.

## Step 5 — Getting chapter timestamps that are actually accurate

YouTube requires: first chapter at `0:00`, every chapter at least 10 seconds
long, timestamps strictly ascending. Two ways to get real timestamps:

**Preferred: record each chapter as its own short clip.** If the user is
recording their own recitation anyway, have them record one clip per chapter
(matching Step 1's enumeration) instead of one continuous take. Then
concatenate them with a fixed silence gap (2 seconds works well — it also
gives a learner a natural beat to process each stanza) using `ffmpeg`. This
gives *exact* cumulative timestamps by construction — no estimation involved.
See `references/ffmpeg-recipes.md` for the concatenation approach.

**Fallback: a pre-existing continuous recording.** If the user insists on
using an already-recorded continuous file, you can't reliably know which
words are playing at which timestamp without an actual transcription/speech-
recognition tool — don't pretend otherwise. `ffmpeg silencedetect` can find
where breath-pauses are, but a fast continuous chant often has near-uniform
pause spacing that doesn't cleanly map one-to-one onto verse boundaries the
deeper into the file you go (drift compounds). The best honest fallback is
proportional distribution: measure the average pace from the detected gaps,
distribute time across chapters by their relative line-count, and **label
the result explicitly as an unverified estimate**, not a fact — and tell the
user it should be checked by ear (in a media player) before publishing, since
you can't listen to the audio content yourself.

⏸ **Always check whether individual stanza-length chapters clear the 10-
second minimum** at the recording's actual pace, and flag it if they don't
— a fast recitation can easily put 2-line stanzas under 10 seconds. Offer
the fix (pair verses two-per-chapter, or insert a deliberate pause per
stanza) rather than silently producing chapters YouTube will drop.

## Step 6 — Local/private rendering (never for upload)

If the user wants a local reference copy of someone else's copyrighted
recording — for their own private listening or practice, not for publishing
anywhere — that's a much lower-risk, reasonable ask (similar to format-
shifting for personal use). You can extract the audio and re-render it with
an original background in place of any branded cover art (a plain generated
gradient/color background via `ffmpeg`'s `lavfi` source is enough — check
first whether the local `ffmpeg` build actually has `drawtext`/freetype
support before planning to overlay text, since minimal Homebrew builds
sometimes lack it). Label the output file clearly as a private practice
copy, and don't commit it to git or suggest uploading it — see Step 7.

## Step 7 — Filing it under version control

This repo (`fun-projects`) keeps personal creative projects together, each
in its own lowercase folder (`marvels/`, `worldcup/`, etc.). For a hymn,
that means a matching folder under `shloka/`:

- `shloka/<hymn-name-in-kebab-case>/`
- The source PDF/text and the companion doc from Step 3
- A `REFERENCES.md` listing every URL actually fetched and used for Step 2's
  translations (kept distinct from sources that only showed up in a search
  results list but weren't directly used), plus a media log of any local
  audio/video files — call out explicitly which files must stay local-only
  and why

**Never commit or push copyrighted third-party audio/video**, including a
private practice copy from Step 6 — local-only means local-only, regardless
of whether the destination repo or its remote is public or private. If asked
to commit and push, stage and check what's actually being added before
running `git commit`, the same way you would for any other repo.

## Quick reference

| Step | Output | Key checkpoint |
|---|---|---|
| 1. Enumerate | Verified chunk list | Verse count matches the actual source, not memory |
| 2. Verify meaning | Cross-checked translations | Fetched real per-verse text, not a WebFetch summary |
| 3. Companion doc | `<hymn>-companion.md` | Follows `references/companion-template.md` |
| 4. Audio | Decision: record / license-cleared / private-only | Checked frames/branding before trusting provenance |
| 5. Timestamps | Real or clearly-labeled-estimated chapter list | 10s-minimum check done |
| 6. Private render | Local mp4, never committed | New background, not the copyrighted cover art |
| 7. Repo filing | `shloka/<hymn>/` + `REFERENCES.md` | No copyrighted media staged |

See `references/ffmpeg-recipes.md` for copy-paste `ffmpeg`/`ffprobe`
commands for frame extraction, silence detection, gap-concatenation, and
background generation.
