# Marvels — Architecture & Scaling Plan

> How to grow `marvels/` from a single self-contained page (Tamil Nadu + Kerala,
> Hindu deities) into an atlas of **all states, all religions** — and keep it useful.
> This is a forward-looking design doc, not a description of the current code.

## Status

- **Today:** one file, `index.html` (~950 lines). Inline India GeoJSON (~70 KB) + an
  inline `PLACES` array (54 temples) + a `CATEGORIES` registry. Vanilla JS/SVG,
  no build step, deployed via GitHub Pages on `main`.
- **Goal:** every state, every religion; comprehensive coverage *and* curated depth;
  trustworthy (fact-checked, sourced); still a static site.

## Decisions locked in

| Decision | Choice | Why |
| --- | --- | --- |
| In-state rendering at scale | **Hybrid** — illustrated macro map for identity; MapLibre/Leaflet for the dense in-state view; featured "marvel" icons layered on top | Keeps the bespoke aesthetic where it matters, but real geography + clustering handle 100k+ points |
| Comprehensive data tier | **Wikidata-first + curate** — seed the catalog tier from Wikidata SPARQL (CC0), hand-write the featured tier | Cleanest licensing & provenance; structured coords + religion + region for free |

## The core problem

The current design **fuses data and presentation**, and the taxonomy is
Hindu-specific (`cat: "shiva"`). That is ideal for 54 places and breaks at scale for
three reasons:

1. **Volume** — comprehensive places-of-worship data is tens-to-hundreds of thousands
   of points. Can't ship inline; inline SVG markers can't render it.
2. **Taxonomy** — `cat` assumes a Hindu deity. Christianity wants denomination + saint;
   Islam wants sect + dargah; Sikhism wants gurudwara. Needs to be **faith-neutral**.
3. **Sourcing & trust** — can't hand-write 100k entries, but accuracy still matters.

**Three architectural moves solve all three:** separate data from code, make the
taxonomy faith-neutral and data-driven, and split data into two tiers.

> The existing `CATEGORIES` registry is already the right *pattern* (data-driven lookup
> the UI renders generically). The scaling move is to lift it one level up into a
> `taxonomy.json` describing each religion. The render engine never names a religion.

## 1. Faith-neutral data model

The Hindu-specific `cat` splits into orthogonal facets; "categories" become
**filterable facets whose values are derived from the data** for the current region +
religion. A place record:

```jsonc
{
  "id": "in-tn-srirangam-ranganatha",     // stable slug = primary key
  "name": "Ranganathaswamy Temple",
  "altNames": ["Thiruvarangam"],
  "type": "temple",                        // temple|mosque|church|gurudwara|dargah|monastery|synagogue|stupa…
  "religion": "hinduism",
  "traditions": ["vaishnavism"],           // sect / denomination
  "figures": ["vishnu"],                   // deity / saint / figure venerated
  "tags": ["divya-desam"],                 // curated groupings — the old "cat" lives here
  "geo": { "lat": 10.8624, "lng": 78.6896 },
  "admin": { "country": "IN", "state": "Tamil Nadu", "district": "Tiruchirappalli", "city": "Srirangam" },
  "tier": "featured",                      // featured (rich) | catalog (thin)
  "built": { "era": "Chola", "circa": "9th c. CE" },
  "summary": "…",
  "marvels": ["…"],
  "media":   [{ "type": "image", "url": "…", "credit": "…", "license": "CC BY-SA" }],
  "sources": [{ "label": "Wikipedia", "url": "…", "retrieved": "2026-06-14" }],
  "confidence": "verified",                // verified | unverified | disputed
  "updated": "2026-06-14"
}
```

A separate **`taxonomy.json`** describes each religion's facet dimensions, colours,
glyphs, icon style, and labels — so deity pills, marker icons (gopuram vs dome vs
spire), and colours are all just data. Facet pills for the current region are computed
from the records present, so a Christian region shows denomination/saint facets and an
Islamic region shows sect/dargah facets on the *same* engine.

## 2. Two tiers — how "all the data" stays honest

- **Catalog tier** — thin records (id, name, type, religion, geo, admin, one source),
  machine-seeded. Gives **breadth**; map looks complete. Rendered as plain dots.
- **Featured / "marvel" tier** — hand-curated, fact-checked, rich (summary, marvels,
  media, verified sources). Gives **depth**. Rendered as illustrated towers.

Breadth from machines, depth from a human.

## 3. Data sourcing pipeline

A Node build script normalizes sources into the schema:

- **Wikidata (SPARQL)** — backbone. Places of worship with coordinate location (P625),
  located-in admin region (P131), religion (P140), heritage status. CC0; links to
  Wikipedia.
- **OpenStreetMap (Overpass)** — `amenity=place_of_worship` for coverage Wikidata
  misses. Mind the **ODbL** share-alike/attribution. *(Deferred per the locked-in
  Wikidata-first choice; add when breadth demands it.)*
- **ASI / UNESCO / govt heritage lists** — accuracy for the featured tier.
- **Manual curation** — the marvels, fact-checked.

The script dedupes by proximity + name, stamps provenance + retrieval date, and flags
conflicts for review. Keep raw pulls under `data/sources/` and the normalized output
versioned in git so a re-pull is a reviewable diff. Later: a scheduled GitHub Action
re-pulls and opens a PR.

## 4. File layout (stays static / GitHub-Pages friendly)

```
/data
  taxonomy.json                 # religions → facets, colours, glyphs, icon style
  index.json                    # tiny: region list + bbox + counts + religions present
  /regions/in-tamil-nadu.json   # catalog for one state (lazy-loaded on click)
  /featured/in-tamil-nadu.json  # rich records for that state
  /sources/…                    # raw Wikidata/OSM pulls (for diffable re-pulls)
/geo
  india-states.json             # boundaries — swap to a current post-Telangana dataset
/build
  validate.mjs                  # schema + dedupe + provenance checks
  bundle.mjs                    # emit per-region files + a search index
index.html                      # app shell: loads taxonomy + index, lazy-loads regions
```

**Loading strategy:** boot with `taxonomy.json` + `index.json` (kilobytes) → draw the
macro map. Click a state → `fetch` that one region file. Never download more than the
region in view.

## 5. Rendering at scale (Hybrid)

Inline SVG markers die in the thousands. The committed approach:

- **Macro map (India → eventually world):** keep the hand-illustrated map — it's the
  brand. Show **aggregate count badges** per state, not individual points.
- **In-state view:** **MapLibre GL** (or Leaflet) with a real basemap + clustered
  GeoJSON for the catalog tier (handles 100k points, "near me", distance). **Featured
  marvels keep their custom illustrated icons** (gopuram / dome / spire) layered on top.

## 6. Usefulness features

- **Deep-linkable URLs** (`?state=…&religion=…&figure=…&place=…`) — shareable,
  embeddable, SEO-able; restores the view.
- **Per-place static pages** generated at build — a referenceable encyclopedia, not
  just a viewer.
- **Faceted search** over a prebuilt index (FlexSearch/MiniSearch, client-side).
- **Pilgrimage circuits as first-class objects** — Navagraha, Char Dham, Arupadai
  Veedu are *routes*; already have three. Distinctive feature.
- **Provenance on every card** (source + date + confidence) — trust differentiator,
  matches the fact-check ethos.
- **"Near me" + distance**, and a **PR/issue contribution path** to crowdsource the
  long tail.

## 7. Build & deploy

Stay static. Small Node build step: (1) validate schema, (2) bundle per-region +
search index, (3) optionally pre-render per-place HTML for SEO. No server. Optional
GitHub Action re-pulls sources on a schedule and opens a PR with diffs.

## 8. Phased path

| Phase | Scope | Risk |
| --- | --- | --- |
| **0 — Refactor** | Extract current 54 temples + `CATEGORIES` into `taxonomy.json` + region JSON; generalize `cat → religion/traditions/figures/tags`; lazy-load. **Zero behaviour change.** | Lowest; unblocks everything |
| **1 — Faith-neutral** | Add religion/type/facet dims; add a few churches/mosques in Kerala to prove multi-faith UI on the same engine | Low |
| **2 — Breadth** | Wikidata pull → catalog tier for 1–2 states; introduce MapLibre + clustering for the in-state view | Medium |
| **3 — Scale-out** | All states; search; deep links; circuits; per-place pages | Medium |
| **4 — World** | Other countries; macro map becomes a world map | Higher |

**Phase 0 is the highest-leverage step** and ships with no visible change.

## Notes carried from the current build

- `animateVB` must use a single clock (`performance.now()`) and clamp/snap progress —
  mixing it with the rAF timestamp stalled the Kerala zoom mid-animation.
- Headless verification: rAF advances under Chrome `--screenshot` /
  `--virtual-time-budget`, **not** under `--dump-dom`.
- Replace the old India GeoJSON (pre-Telangana split; "Orissa"/"Uttaranchal") with a
  current dataset as part of Phase 2/3.
- Kerala temples currently use the Dravidian gopuram icon — add a Kerala sloped-roof
  marker when icon style becomes data (`taxonomy.json`).
