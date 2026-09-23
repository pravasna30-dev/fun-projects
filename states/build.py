#!/usr/bin/env python3
"""Build the "Pick Your State" page: decode the us-atlas Albers-USA TopoJSON into
SVG paths, attach the verified state dataset, and inject both into index.html."""
import json, os, subprocess

RAW = 'states_albers_raw.json'
if not os.path.exists(RAW):  # pre-projected Albers USA (975x610), AK/HI inset
    subprocess.run(['curl', '-sL', '-o', RAW,
                    'https://cdn.jsdelivr.net/npm/us-atlas@3/states-albers-10m.json'], check=True)

# ---------------------------------------------------------------------------
# VERIFIED DATASET (checked Sept 2026)
#  inc   = top marginal individual income-tax rate, tax year 2026 (None = no tax on wages)
#          Tax Foundation Facts & Figures 2026, Table 11 (as of Mar 23 2026), plus
#          laws signed after that: GA 5.19->4.99 (HB 463, May 11 2026),
#          SC -> 1.99%/5.21% (H.4216, Mar 30 2026), UT 4.5->4.45 (retroactive Jan 1 2026)
#  kind  = none | flat | grad
#  st,sl = state / combined avg state+local sales tax %, Tax Foundation, July 1 2026
#  prop  = effective property-tax rate on owner-occupied homes %, CY2024 (TF F&F Table 33)
#  burd  = state-local tax burden as % of income, CY2022 (TF F&F Table 2)
#  gas   = total state gas tax $/gal, Jan 1 2026 (TF F&F Table 22)
#  ret   = WalletHub "Best States to Retire" 2026 rank (1 = best)
#  gdp   = 2025 current-dollar GDP, $ millions (BEA SAGDP1, released Apr 8 2026)
# ---------------------------------------------------------------------------
S = [
 # fips ab  name              inc    kind    st    sl     prop  burd  gas     ret gdp
 ("01","AL","Alabama",        5.00,"grad", 4.00, 9.46, 0.37, 9.8, 0.31,   37, 341154.2),
 ("02","AK","Alaska",         None,"none", 0.00, 1.82, 0.90, 4.6, 0.0895,  6,  75011.6),
 ("04","AZ","Arizona",        2.50,"flat", 5.60, 8.54, 0.43, 9.5, 0.19,   18, 598188.8),
 ("05","AR","Arkansas",       3.90,"grad", 6.50, 9.48, 0.54,10.2, 0.25,   44, 198422.0),
 ("06","CA","California",    13.30,"grad", 7.25, 9.03, 0.69,13.5, 0.7092, 25,4250840.8),
 ("08","CO","Colorado",       4.40,"flat", 2.90, 7.89, 0.52, 9.7, 0.2918,  4, 584323.9),
 ("09","CT","Connecticut",    6.99,"grad", 6.35, 6.35, 1.36,15.4, 0.25,   34, 376455.0),
 ("10","DE","Delaware",       6.60,"grad", 0.00, 0.00, 0.51,12.4, 0.23,    7, 117218.0),
 ("11","DC","Washington, D.C.",10.75,"grad",6.00, 6.00, 0.63,12.0, 0.357, None,192617.5),
 ("12","FL","Florida",        None,"none", 6.00, 6.98, 0.76, 9.1, 0.401,   2,1834641.4),
 ("13","GA","Georgia",        4.99,"flat", 4.00, 7.56, 0.77, 8.9, 0.3405, 29, 924828.9),
 ("15","HI","Hawaii",        11.00,"grad", 4.00, 4.50, 0.31,14.1, 0.185,  46, 124607.8),
 ("16","ID","Idaho",          5.30,"flat", 6.00, 6.03, 0.43,10.7, 0.33,   16, 135552.8),
 ("17","IL","Illinois",       4.95,"flat", 6.25, 8.98, 1.79,12.9, 0.664,  38,1201996.1),
 ("18","IN","Indiana",        2.95,"flat", 7.00, 7.00, 0.76, 9.3, 0.524,  22, 545234.0),
 ("19","IA","Iowa",           3.80,"flat", 6.00, 6.94, 1.25,11.2, 0.30,   10, 277110.1),
 ("20","KS","Kansas",         5.58,"grad", 6.50, 8.71, 1.20,11.2, 0.2503, 26, 241378.4),
 ("21","KY","Kentucky",       3.50,"flat", 6.00, 6.00, 0.72, 9.6, 0.264,  50, 306897.4),
 ("22","LA","Louisiana",      3.00,"flat", 5.00,10.13, 0.56, 9.1, 0.2093, 39, 340079.8),
 ("23","ME","Maine",          7.15,"grad", 5.50, 5.50, 0.90,12.4, 0.3145, 19, 102843.7),
 ("24","MD","Maryland",       6.50,"grad", 6.00, 6.00, 0.90,11.3, 0.4621, 36, 568139.7),
 ("25","MA","Massachusetts",  9.00,"grad", 6.25, 6.25, 0.95,11.5, 0.2756, 23, 820104.9),
 ("26","MI","Michigan",       4.25,"flat", 6.00, 6.00, 1.13, 8.6, 0.534,  21, 730067.6),
 ("27","MN","Minnesota",      9.85,"grad", 6.88, 8.14, 0.99,12.1, 0.327,   5, 531464.9),
 ("28","MS","Mississippi",    4.00,"flat", 7.00, 7.06, 0.54, 9.8, 0.214,  48, 165069.3),
 ("29","MO","Missouri",       4.70,"grad", 4.23, 8.44, 0.85, 9.3, 0.2999, 15, 468469.7),
 ("30","MT","Montana",        5.65,"grad", 0.00, 0.00, 0.59,10.5, 0.3375, 24,  82357.8),
 ("31","NE","Nebraska",       4.55,"grad", 5.50, 6.98, 1.38,11.5, 0.327,  31, 198073.3),
 ("32","NV","Nevada",         None,"none", 6.85, 8.24, 0.50, 9.6, 0.2381, 28, 281453.8),
 ("33","NH","New Hampshire",  None,"none", 0.00, 0.00, 1.35, 9.6, 0.2375,  9, 125523.2),
 ("34","NJ","New Jersey",    10.75,"grad", 6.63, 6.60, 1.68,13.2, 0.4915, 35, 887174.6),
 ("35","NM","New Mexico",     5.90,"grad", 4.88, 7.68, 0.61,10.2, 0.1888, 41, 152778.6),
 ("36","NY","New York",      10.90,"grad", 4.00, 8.54, 1.23,15.9, 0.2418, 45,2467674.0),
 ("37","NC","North Carolina", 3.99,"flat", 4.75, 7.10, 0.62, 9.9, 0.4125, 13, 893762.5),
 ("38","ND","North Dakota",   2.50,"grad", 5.00, 7.09, 0.94, 8.8, 0.2303, 14,  81883.3),
 ("39","OH","Ohio",           2.75,"flat", 5.75, 7.29, 1.28,10.0, 0.385,  20, 966779.9),
 ("40","OK","Oklahoma",       4.50,"grad", 4.50, 9.06, 0.78, 9.0, 0.20,   49, 274421.0),
 ("41","OR","Oregon",         9.90,"grad", 0.00, 0.00, 0.79,10.8, 0.40,   40, 342850.1),
 ("42","PA","Pennsylvania",   3.07,"flat", 6.00, 6.34, 1.14,10.6, 0.587,   8,1056446.3),
 ("44","RI","Rhode Island",   5.99,"grad", 7.00, 7.00, 1.00,11.4, 0.4112, 42,  83956.0),
 ("45","SC","South Carolina", 5.21,"grad", 6.00, 7.49, 0.44, 8.9, 0.2875, 17, 378830.6),
 ("46","SD","South Dakota",   None,"none", 4.20, 6.11, 1.00, 8.4, 0.30,    3,  80650.3),
 ("47","TN","Tennessee",      None,"none", 7.00, 9.61, 0.46, 7.6, 0.274,  32, 589817.5),
 ("48","TX","Texas",          None,"none", 6.25, 8.20, 1.24, 8.6, 0.20,   33,2904427.8),
 ("49","UT","Utah",           4.45,"flat", 6.10, 7.42, 0.45,12.1, 0.3855, 27, 315973.2),
 ("50","VT","Vermont",        8.75,"grad", 6.00, 6.43, 1.40,13.6, 0.3151, 30,  48350.3),
 ("51","VA","Virginia",       5.75,"grad", 5.30, 5.77, 0.75,12.5, 0.416,  12, 798447.9),
 ("53","WA","Washington",     None,"none", 6.50, 9.57, 0.74,10.7, 0.5904, 43, 894990.1),
 ("54","WV","West Virginia",  4.82,"grad", 6.00, 6.60, 0.48, 9.8, 0.357,  47, 109276.6),
 ("55","WI","Wisconsin",      7.65,"grad", 5.00, 5.72, 1.19,10.9, 0.329,  11, 473037.4),
 ("56","WY","Wyoming",        None,"none", 4.00, 5.39, 0.58, 7.5, 0.24,    1,  52622.3),
]
KEYS = ["fips","ab","name","inc","kind","st","sl","prop","burd","gas","ret","gdp"]

# Kid-friendly one-liners — each checked against the dataset above.
NOTES = {
 "WA": "No tax on paychecks — but it does tax big stock-market profits (capital gains) and has one of the highest sales taxes.",
 "NH": "No wage tax, no sales tax! The catch: some of the highest property taxes in the U.S.",
 "TX": "No income tax, but property tax (1.24%) and sales tax (8.2%) are both above average.",
 "FL": "No income tax and a 6% state sales tax — #2 on WalletHub's retirement list.",
 "WY": "WalletHub's #1 state to retire in 2026 — and the lowest overall tax burden in the lower 48.",
 "AK": "No state income tax or state sales tax — lowest overall tax burden in the country (4.6%).",
 "CA": "Biggest economy of any state ($4.25 trillion) and the highest top income-tax rate (13.3%).",
 "OR": "No sales tax at all, so shopping is cheap — but the income tax tops out at 9.9%.",
 "MT": "No general sales tax (a few resort towns add a small local one).",
 "DE": "No sales tax at all — shoppers from nearby states drive in to buy big stuff.",
 "IL": "Highest effective property tax rate in the country (1.79%).",
 "HI": "Lowest property tax rate (0.31%) — but the second-highest top income tax (11%).",
 "LA": "Highest combined sales tax in the U.S. (10.13%) — a $100 sneaker costs $110.13.",
 "TN": "No income tax, but sales tax averages 9.61% — the 2nd highest.",
 "GA": "Cut its flat income tax to 4.99% for 2026 (law signed May 2026).",
 "SC": "New for 2026: just two brackets — 1.99% and a 5.21% top rate.",
 "NY": "Top rate 10.9% applies only above $25 million a year!",
 "MA": "5% on most income, plus a 4% 'millionaire surtax' on income above ~$1.08M = 9%.",
 "DC": "Not a state, but it has its own taxes — 10.75% top rate.",
 "NJ": "2nd-highest property tax rate (1.68%) and a 10.75% top income tax.",
 "NV": "No income tax — casinos and tourism help pay the bills.",
 "SD": "No income tax and #3 on WalletHub's retirement list.",
 "AZ": "Flat 2.5% income tax — the lowest flat rate in the country.",
 "IN": "One of the lowest flat income taxes: 2.95%.",
 "ND": "Top income-tax rate is just 2.5%, and it only starts above ~$244,000.",
}

# --- TopoJSON decode --------------------------------------------------------
topo = json.load(open(RAW))
sx, sy = topo['transform']['scale']; tx, ty = topo['transform']['translate']
arcs = []
for arc in topo['arcs']:
    x = y = 0; pts = []
    for dx, dy in arc:
        x += dx; y += dy
        pts.append((x*sx+tx, y*sy+ty))
    arcs.append(pts)

def ring_pts(idx_list):
    pts = []
    for i in idx_list:
        a = arcs[i] if i >= 0 else arcs[~i][::-1]
        pts.extend(a if not pts else a[1:])
    return pts

def ring_path(pts):
    return 'M' + 'L'.join(f'{x:.1f},{y:.1f}' for x, y in pts) + 'Z'

def area_centroid(pts):
    a = cx = cy = 0.0
    for (x0,y0),(x1,y1) in zip(pts, pts[1:]+pts[:1]):
        c = x0*y1 - x1*y0; a += c; cx += (x0+x1)*c; cy += (y0+y1)*c
    if a == 0: return 0, pts[0]
    return abs(a/2), (cx/(3*a), cy/(3*a))

shapes = {}
for g in topo['objects']['states']['geometries']:
    polys = g['arcs'] if g['type'] == 'MultiPolygon' else [g['arcs']]
    d = ''; best = (-1, None)
    for poly in polys:
        for k, ring in enumerate(poly):
            pts = ring_pts(ring)
            d += ring_path(pts)
            if k == 0:
                ar, c = area_centroid(pts)
                if ar > best[0]: best = (ar, c)
    shapes[g['id']] = {"d": d, "c": [round(best[1][0],1), round(best[1][1],1)]}

# hand-nudged label spots where the area centroid looks off
LABEL_FIX = {"FL": (795, 470), "LA": (574, 440), "MI": (660, 205), "ID": (225, 175),
             "CA": (95, 285), "KY": (675, 305), "VA": (790, 280), "MD": None}

states = []
for row in S:
    rec = dict(zip(KEYS, row))
    shp = shapes[rec["fips"]]
    rec["d"] = shp["d"]
    lab = LABEL_FIX.get(rec["ab"], shp["c"])
    rec["c"] = list(lab) if lab else shp["c"]
    if rec["ab"] in NOTES: rec["note"] = NOTES[rec["ab"]]
    states.append(rec)

assert len(states) == 51, len(states)
js = json.dumps(states, separators=(',', ':'), ensure_ascii=False)
tpl = open('index.template.html', encoding='utf-8').read()
open('index.html', 'w', encoding='utf-8').write(tpl.replace('/*__DATA__*/', js))
print('states:', len(states), '| data KB:', round(len(js)/1024))
