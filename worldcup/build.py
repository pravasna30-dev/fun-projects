#!/usr/bin/env python3
"""Build the 2026 World Cup map page: simplify world GeoJSON, compute flag-pin
locations, and inject the verified team dataset into index.html via a marker."""
import json, math

# ---------------------------------------------------------------------------
# VERIFIED DATASET --- 2026 FIFA World Cup, 48 teams.
# "app" = the ordinal number of this World Cup appearance, INCLUDING 2026.
# Cross-checked against Wikipedia's all-time FIFA World Cup table
# (updated 20 June 2026; anchor: Brazil = 23, every tournament 1930-2026).
# Successor states folded in: Germany incl. West Germany (21); Czechia incl.
# Czechoslovakia (10); DR Congo incl. Zaire 1974 (2).
# iso3 = Natural Earth ADM0_A3 (for the map); iso2 = flagcdn code (for the flag).
# ---------------------------------------------------------------------------
TEAMS = [
    # CONCACAF hosts
    {"name":"Canada","iso3":"CAN","iso2":"ca","app":3,"conf":"CONCACAF","host":True},
    {"name":"Mexico","iso3":"MEX","iso2":"mx","app":18,"conf":"CONCACAF","host":True},
    {"name":"United States","iso3":"USA","iso2":"us","app":12,"conf":"CONCACAF","host":True,"pin":[-98.5,39.5]},
    # CONMEBOL
    {"name":"Argentina","iso3":"ARG","iso2":"ar","app":19,"conf":"CONMEBOL"},
    {"name":"Brazil","iso3":"BRA","iso2":"br","app":23,"conf":"CONMEBOL"},
    {"name":"Colombia","iso3":"COL","iso2":"co","app":7,"conf":"CONMEBOL"},
    {"name":"Ecuador","iso3":"ECU","iso2":"ec","app":5,"conf":"CONMEBOL"},
    {"name":"Paraguay","iso3":"PRY","iso2":"py","app":9,"conf":"CONMEBOL"},
    {"name":"Uruguay","iso3":"URY","iso2":"uy","app":15,"conf":"CONMEBOL"},
    # UEFA
    {"name":"Austria","iso3":"AUT","iso2":"at","app":8,"conf":"UEFA"},
    {"name":"Belgium","iso3":"BEL","iso2":"be","app":15,"conf":"UEFA"},
    {"name":"Bosnia & Herzegovina","iso3":"BIH","iso2":"ba","app":2,"conf":"UEFA"},
    {"name":"Croatia","iso3":"HRV","iso2":"hr","app":7,"conf":"UEFA"},
    {"name":"Czechia","iso3":"CZE","iso2":"cz","app":10,"conf":"UEFA"},
    {"name":"England","iso3":"GBR","iso2":"gb-eng","app":17,"conf":"UEFA","pin":[-1.2,52.6]},
    {"name":"France","iso3":"FRA","iso2":"fr","app":17,"conf":"UEFA","pin":[2.4,46.6]},
    {"name":"Germany","iso3":"DEU","iso2":"de","app":21,"conf":"UEFA"},
    {"name":"Netherlands","iso3":"NLD","iso2":"nl","app":12,"conf":"UEFA","pin":[5.6,52.2]},
    {"name":"Norway","iso3":"NOR","iso2":"no","app":4,"conf":"UEFA","pin":[9.0,61.0]},
    {"name":"Portugal","iso3":"PRT","iso2":"pt","app":9,"conf":"UEFA","pin":[-8.2,39.6]},
    {"name":"Scotland","iso3":"GBR","iso2":"gb-sct","app":9,"conf":"UEFA","pin":[-4.2,56.9]},
    {"name":"Spain","iso3":"ESP","iso2":"es","app":17,"conf":"UEFA","pin":[-3.7,40.2]},
    {"name":"Sweden","iso3":"SWE","iso2":"se","app":13,"conf":"UEFA","pin":[15.0,60.5]},
    {"name":"Switzerland","iso3":"CHE","iso2":"ch","app":13,"conf":"UEFA"},
    {"name":"Türkiye","iso3":"TUR","iso2":"tr","app":3,"conf":"UEFA"},
    # AFC
    {"name":"Australia","iso3":"AUS","iso2":"au","app":7,"conf":"AFC","pin":[134,-25]},
    {"name":"Iran","iso3":"IRN","iso2":"ir","app":7,"conf":"AFC"},
    {"name":"Iraq","iso3":"IRQ","iso2":"iq","app":2,"conf":"AFC"},
    {"name":"Japan","iso3":"JPN","iso2":"jp","app":8,"conf":"AFC"},
    {"name":"Jordan","iso3":"JOR","iso2":"jo","app":1,"conf":"AFC","debut":True},
    {"name":"South Korea","iso3":"KOR","iso2":"kr","app":12,"conf":"AFC"},
    {"name":"Qatar","iso3":"QAT","iso2":"qa","app":2,"conf":"AFC"},
    {"name":"Saudi Arabia","iso3":"SAU","iso2":"sa","app":7,"conf":"AFC"},
    {"name":"Uzbekistan","iso3":"UZB","iso2":"uz","app":1,"conf":"AFC","debut":True},
    # CAF
    {"name":"Algeria","iso3":"DZA","iso2":"dz","app":5,"conf":"CAF"},
    {"name":"Cabo Verde","iso3":"CPV","iso2":"cv","app":1,"conf":"CAF","debut":True,"pin":[-23.6,15.9]},
    {"name":"DR Congo","iso3":"COD","iso2":"cd","app":2,"conf":"CAF"},
    {"name":"Côte d'Ivoire","iso3":"CIV","iso2":"ci","app":4,"conf":"CAF"},
    {"name":"Egypt","iso3":"EGY","iso2":"eg","app":4,"conf":"CAF"},
    {"name":"Ghana","iso3":"GHA","iso2":"gh","app":5,"conf":"CAF"},
    {"name":"Morocco","iso3":"MAR","iso2":"ma","app":7,"conf":"CAF"},
    {"name":"Senegal","iso3":"SEN","iso2":"sn","app":4,"conf":"CAF"},
    {"name":"South Africa","iso3":"ZAF","iso2":"za","app":4,"conf":"CAF"},
    {"name":"Tunisia","iso3":"TUN","iso2":"tn","app":7,"conf":"CAF"},
    # CONCACAF (qualified)
    {"name":"Curaçao","iso3":"CUW","iso2":"cw","app":1,"conf":"CONCACAF","debut":True,"pin":[-69.0,12.2]},
    {"name":"Haiti","iso3":"HTI","iso2":"ht","app":2,"conf":"CONCACAF"},
    {"name":"Panama","iso3":"PAN","iso2":"pa","app":2,"conf":"CONCACAF"},
    # OFC
    {"name":"New Zealand","iso3":"NZL","iso2":"nz","app":3,"conf":"OFC","pin":[172,-42]},
]

HILITE = {t["iso3"] for t in TEAMS}

# --- geometry helpers ------------------------------------------------------
def perp_dist(p, a, b):
    (x,y),(x1,y1),(x2,y2)=p,a,b
    dx,dy=x2-x1,y2-y1
    if dx==0 and dy==0: return math.hypot(x-x1,y-y1)
    t=((x-x1)*dx+(y-y1)*dy)/(dx*dx+dy*dy)
    t=max(0,min(1,t))
    return math.hypot(x-(x1+t*dx), y-(y1+t*dy))

def rdp(pts, eps):
    if len(pts)<3: return pts
    dmax,idx=0,0
    for i in range(1,len(pts)-1):
        d=perp_dist(pts[i],pts[0],pts[-1])
        if d>dmax: dmax,idx=d,i
    if dmax>eps:
        return rdp(pts[:idx+1],eps)[:-1]+rdp(pts[idx:],eps)
    return [pts[0],pts[-1]]

def simplify_ring(ring, eps):
    if len(ring)<5: return [[round(x,3),round(y,3)] for x,y in ring]
    s=rdp(ring,eps)
    if len(s)<4: s=ring[:: max(1,len(ring)//4)]
    return [[round(x,3),round(y,3)] for x,y in s]

def ring_area_centroid(ring):
    a=cx=cy=0.0
    for i in range(len(ring)-1):
        x0,y0=ring[i]; x1,y1=ring[i+1]
        cr=x0*y1-x1*y0; a+=cr; cx+=(x0+x1)*cr; cy+=(y0+y1)*cr
    if a==0: return 0,(ring[0][0],ring[0][1])
    a*=0.5
    return abs(a),(cx/(6*a),cy/(6*a))

# --- process features ------------------------------------------------------
src=json.load(open('ne_50m_raw.geojson'))
EPS=0.28
out_feats=[]
centroids={}  # iso3 -> (lng,lat) of largest polygon
for f in src['features']:
    p=f['properties']
    iso=p.get('ADM0_A3')
    geom=f['geometry']; gtype=geom['type']
    polys = geom['coordinates'] if gtype=='MultiPolygon' else [geom['coordinates']]
    new_polys=[]; best_area=-1; best_c=None
    for poly in polys:
        new_rings=[]
        for ri,ring in enumerate(poly):
            sr=simplify_ring(ring,EPS)
            if len(sr)>=4: new_rings.append(sr)
            if ri==0:
                ar,c=ring_area_centroid(ring)
                if ar>best_area: best_area,best_c=ar,c
        if new_rings: new_polys.append(new_rings)
    if not new_polys: continue
    if iso in HILITE and best_c: centroids[iso]=best_c
    out_feats.append({
        "i": iso,
        "h": 1 if iso in HILITE else 0,
        "c": new_polys,
    })

# attach pin location to each team (override or computed centroid)
for t in TEAMS:
    if "pin" not in t:
        c=centroids.get(t["iso3"])
        t["pin"]=[round(c[0],2),round(c[1],2)] if c else [0,0]
    else:
        t["pin"]=[round(t["pin"][0],2),round(t["pin"][1],2)]

blob={"feats":out_feats,"teams":TEAMS}
js=json.dumps(blob,separators=(',',':'),ensure_ascii=False)
open('data_blob.json','w').write(js)
print("features:",len(out_feats),"| highlighted:",len([f for f in out_feats if f['h']]))
print("teams:",len(TEAMS),"| blob KB:",round(len(js)/1024))

# inject into template
tpl=open('index.template.html',encoding='utf-8').read()
html=tpl.replace('/*__DATA__*/', js)
open('index.html','w',encoding='utf-8').write(html)
print("wrote index.html KB:",round(len(html)/1024))
