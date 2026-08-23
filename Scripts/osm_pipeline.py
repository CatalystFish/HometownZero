#!/usr/bin/env python3
"""Hometown Zero Phase 0 pipeline: OSM bounding box -> district.json (+ OBJ preview).

Usage:
    python osm_pipeline.py --bbox "minlat,minlon,maxlat,maxlon" --out district.json [--obj preview.obj]
"""
import argparse
import json
import math
import random
import sys
import urllib.parse
import urllib.request

OVERPASS_ENDPOINTS = [
    "https://overpass-api.de/api/interpreter",
    "https://overpass.kumi.systems/api/interpreter",
]

M_PER_DEG_LAT = 110540.0

ROAD_WIDTHS_M = {
    "motorway": 12.0, "trunk": 12.0, "primary": 10.0, "secondary": 8.0,
    "tertiary": 7.0, "residential": 6.0, "unclassified": 5.0,
    "service": 4.0, "living_street": 5.0, "pedestrian": 4.0,
    "footway": 1.5, "cycleway": 1.5, "path": 1.5, "steps": 1.5, "track": 3.0,
}

HEIGHT_RANGES_M = {
    "medical": (9, 30), "emergency": (8, 15), "education": (6, 18),
    "food": (4, 10), "hardware": (5, 10), "weapons_outdoors": (4, 9),
    "fuel": (4, 8), "retail": (5, 14), "office": (9, 40),
    "industrial": (6, 12), "residential": (4, 12), "civic": (8, 20),
    "unknown": (4, 12),
}

LEVEL_HEIGHT_M = 3.2

MEDICAL_AMENITIES = {"hospital", "clinic", "pharmacy", "doctors"}
EMERGENCY_AMENITIES = {"police", "fire_station"}
EDUCATION_AMENITIES = {"school", "kindergarten", "college", "university", "library"}
FOOD_SHOPS = {"supermarket", "convenience", "greengrocer", "bakery", "butcher",
              "deli", "seafood", "alcohol", "beverages", "cheese", "chocolate"}
HARDWARE_SHOPS = {"hardware", "doityourself", "trade", "paint", "electrical"}
OUTDOOR_SHOPS = {"weapons", "hunting", "outdoor", "sports", "fishing", "camping", "guns"}
RESIDENTIAL_BUILDINGS = {"house", "detached", "semidetached_house", "terrace",
                         "apartments", "residential", "dormitory", "hut", "bungalow"}
HEALTHCARE_JUNK = {"spa", "sauna", "massage", "tanning", "beauty"}
CRAFT_FOOD = {"bakery", "brewery", "winery", "distillery"}

JUNK_AMENITIES = {
    "bench", "waste_basket", "waste_disposal", "recycling", "toilets",
    "drinking_water", "telephone", "post_box", "bicycle_parking", "parking",
    "parking_space", "parking_entrance", "fountain", "grit_bin", "clock",
    "shelter", "loading_dock",
}


def fetch_overpass(bbox, timeout=90):
    query = (
        f"[out:json][timeout:{timeout}];"
        "("
        f'way["building"]({bbox});'
        f'way["highway"]({bbox});'
        f'node["shop"]({bbox});'
        f'node["amenity"]({bbox});'
        f'node["office"]({bbox});'
        f'node["healthcare"]({bbox});'
        f'node["craft"]({bbox});'
        ");out geom;"
    )
    payload = urllib.parse.urlencode({"data": query}).encode()
    last_err = None
    for endpoint in OVERPASS_ENDPOINTS:
        try:
            print(f"[fetch] {endpoint} ...")
            req = urllib.request.Request(endpoint, data=payload, headers={
                "Content-Type": "application/x-www-form-urlencoded",
                "User-Agent": "HometownZero-Phase0-Spike/0.1",
            })
            with urllib.request.urlopen(req, timeout=timeout + 30) as resp:
                data = json.load(resp)
            print(f"[fetch] ok, {len(data.get('elements', []))} elements")
            return data
        except Exception as err:
            print(f"[fetch] failed: {err}")
            last_err = err
    raise SystemExit(f"All Overpass endpoints failed: {last_err}")


def classify(tags):
    amenity = tags.get("amenity", "")
    shop = tags.get("shop", "")
    building = tags.get("building", "yes")
    if amenity in MEDICAL_AMENITIES or building == "hospital":
        return "medical"
    hc = tags.get("healthcare")
    if hc and hc not in HEALTHCARE_JUNK:
        return "medical"
    if amenity in EMERGENCY_AMENITIES or building in ("fire_station",):
        return "emergency"
    if amenity in EDUCATION_AMENITIES or building in ("school", "university", "college"):
        return "education"
    if amenity == "place_of_worship" or building in ("church", "cathedral", "chapel",
                                                     "mosque", "synagogue", "temple"):
        return "civic"
    if amenity == "fuel":
        return "fuel"
    craft = tags.get("craft", "")
    if craft:
        if craft in CRAFT_FOOD:
            return "food"
        return "hardware"
    if shop in FOOD_SHOPS or building in ("supermarket", "commercial_kiosk"):
        return "food"
    if shop in HARDWARE_SHOPS:
        return "hardware"
    if shop in OUTDOOR_SHOPS:
        return "weapons_outdoors"
    if shop:
        return "retail"
    if tags.get("office") or building == "office":
        return "office"
    if building in ("industrial", "warehouse", "factory", "farm_auxiliary"):
        return "industrial"
    if building in ("retail", "supermarket"):
        return "retail"
    if building in RESIDENTIAL_BUILDINGS:
        return "residential"
    return "unknown"


def parse_height_m(raw):
    raw = raw.strip().lower().replace("'", "ft").replace('"', "")
    try:
        if raw.endswith("ft"):
            return float(raw[:-2].strip()) * 0.3048
        return float(raw.rstrip("m").strip())
    except ValueError:
        return None


def estimate_height(tags, category, way_id):
    h = parse_height_m(tags["height"]) if "height" in tags else None
    if h and 2 < h < 400:
        return round(h, 1)
    if "building:levels" in tags:
        try:
            levels = float(tags["building:levels"])
            if 0 < levels < 150:
                return round(levels * LEVEL_HEIGHT_M, 1)
        except ValueError:
            pass
    lo, hi = HEIGHT_RANGES_M[category]
    rng = random.Random(way_id)
    return round(rng.uniform(lo, hi), 1)


def project_origin(bbox):
    min_lat, min_lon, max_lat, max_lon = bbox
    lat0 = (min_lat + max_lat) / 2.0
    lon0 = (min_lon + max_lon) / 2.0
    m_per_deg_lon = 111320.0 * math.cos(math.radians(lat0))
    return lat0, lon0, m_per_deg_lon


def to_local(lon, lat, lat0, lon0, m_per_deg_lon):
    return (round((lon - lon0) * m_per_deg_lon, 2),
            round((lat - lat0) * M_PER_DEG_LAT, 2))


def process(elements, bbox):
    lat0, lon0, m_per_deg_lon = project_origin(bbox)
    buildings, roads, pois, skipped = [], [], [], 0

    for el in elements:
        etype = el.get("type")
        tags = el.get("tags") or {}
        way_id = el.get("id", 0)

        if etype == "node":
            if (tags
                    and any(k in tags for k in ("shop", "amenity", "office",
                                                "healthcare", "craft"))
                    and not JUNK_AMENITIES.intersection(tags.values())
                    and el.get("lon") is not None and el.get("lat") is not None):
                x, y = to_local(el["lon"], el["lat"], lat0, lon0, m_per_deg_lon)
                pois.append((x, y, tags))
            continue

        if etype != "way" or "geometry" not in el:
            continue
        coords = [(p["lon"], p["lat"]) for p in el["geometry"]
                  if p.get("lon") is not None and p.get("lat") is not None]

        if "building" in tags or any(
                k in tags for k in ("amenity", "shop")):
            closed = len(coords) >= 4 and coords[0] == coords[-1]
            if not closed:
                skipped += 1
                continue
            footprint = [to_local(lon, lat, lat0, lon0, m_per_deg_lon)
                         for lon, lat in coords[:-1]]
            area = polygon_area(footprint)
            if abs(area) < 9.0:
                skipped += 1
                continue
            if area < 0:
                footprint.reverse()
            category = classify(tags)
            buildings.append({
                "id": way_id,
                "category": category,
                "height_m": estimate_height(tags, category, way_id),
                "name": tags.get("name"),
                "footprint": footprint,
            })
        elif "highway" in tags:
            hw_class = tags["highway"]
            if hw_class not in ROAD_WIDTHS_M or len(coords) < 2:
                skipped += 1
                continue
            roads.append({
                "id": way_id,
                "class": hw_class,
                "width_m": ROAD_WIDTHS_M[hw_class],
                "polyline": [to_local(lon, lat, lat0, lon0, m_per_deg_lon)
                             for lon, lat in coords],
            })

    return buildings, roads, pois, skipped


def polygon_area(pts):
    total = 0.0
    n = len(pts)
    for i in range(n):
        x1, y1 = pts[i]
        x2, y2 = pts[(i + 1) % n]
        total += x1 * y2 - x2 * y1
    return total / 2.0


def point_in_ring(x, y, ring):
    inside = False
    j = len(ring) - 1
    for i in range(len(ring)):
        xi, yi = ring[i]
        xj, yj = ring[j]
        if (yi > y) != (yj > y):
            if x < (xj - xi) * (y - yi) / (yj - yi) + xi:
                inside = not inside
        j = i
    return inside


def join_pois(buildings, pois):
    boxes = []
    for b in buildings:
        xs = [p[0] for p in b["footprint"]]
        ys = [p[1] for p in b["footprint"]]
        boxes.append((min(xs), min(ys), max(xs), max(ys)))

    hits = [[] for _ in buildings]
    for x, y, tags in pois:
        for idx, b in enumerate(buildings):
            bx0, by0, bx1, by1 = boxes[idx]
            if bx0 <= x <= bx1 and by0 <= y <= by1 and \
                    point_in_ring(x, y, b["footprint"]):
                hits[idx].append(tags)
                break

    resolved = 0
    for b, poi_hits in zip(buildings, hits):
        if b["category"] != "unknown":
            continue
        for poi_tags in poi_hits:
            cat = classify(poi_tags)
            if cat != "unknown":
                b["category"] = cat
                if not b.get("name"):
                    b["name"] = poi_tags.get("name")
                resolved += 1
                break

    matched = sum(len(h) for h in hits)
    return resolved, matched


def write_json(path, bbox, buildings, roads, skipped):
    lat0, lon0, _ = project_origin(bbox)
    doc = {
        "format": "hometown-district",
        "version": 2,
        "crs": "local-meters-east-north-up",
        "origin": {"lat": lat0, "lon": lon0},
        "bbox": list(bbox),
        "buildings": buildings,
        "roads": roads,
    }
    with open(path, "w", encoding="utf-8") as fh:
        json.dump(doc, fh)
    cats = {}
    for b in buildings:
        cats[b["category"]] = cats.get(b["category"], 0) + 1
    print(f"[json ] {path}: {len(buildings)} buildings, {len(roads)} road segments, "
          f"{skipped} skipped")
    for cat, count in sorted(cats.items(), key=lambda kv: -kv[1]):
        print(f"         {cat:<16} {count}")


try:
    import numpy as np
    import mapbox_earcut as earcut

    def triangulate_ring(ring):
        verts = np.asarray(ring, dtype=np.float64)
        rings = np.array([len(ring)], dtype=np.uint32)
        arr = earcut.triangulate_float64(verts, rings)
        return [(int(arr[i]), int(arr[i + 1]), int(arr[i + 2]))
                for i in range(0, len(arr), 3)]
except ImportError:
    def triangulate_ring(ring):
        print("[warn ] mapbox_earcut not installed; using naive fan triangulation "
              "(concave footprints may look off). pip install mapbox-earcut")
        return [(0, i, i + 1) for i in range(1, len(ring) - 1)]


def write_obj(path, buildings, roads):
    lines = ["# Hometown Zero Phase 0 preview", ""]
    v_off = 1

    def emit(x, y, z):
        lines.append(f"v {x:.2f} {z:.2f} {-y:.2f}")

    for b in buildings:
        ring = b["footprint"]
        h = b["height_m"]
        lines.append(f"o b{b['id']}_{b['category']}")
        base = v_off
        for x, y in ring:
            emit(x, y, 0.0)
        v_off += len(ring)
        top = v_off
        for x, y in ring:
            emit(x, y, h)
        v_off += len(ring)
        n = len(ring)
        for i in range(n):
            j = (i + 1) % n
            lines.append(f"f {base + i} {base + j} {top + j}")
            lines.append(f"f {base + i} {top + j} {top + i}")
        for a, bb, c in triangulate_ring(ring):
            lines.append(f"f {top + a} {top + bb} {top + c}")

    for r in roads:
        pts = r["polyline"]
        w = r["width_m"] / 2.0
        left, right = [], []
        for k in range(len(pts)):
            x, y = pts[k]
            if k == 0:
                dx, dy = pts[1][0] - x, pts[1][1] - y
            elif k == len(pts) - 1:
                dx, dy = x - pts[k - 1][0], y - pts[k - 1][1]
            else:
                dx, dy = pts[k + 1][0] - pts[k - 1][0], pts[k + 1][1] - pts[k - 1][1]
            length = math.hypot(dx, dy) or 1.0
            nx, ny = -dy / length * w, dx / length * w
            left.append((x + nx, y + ny))
            right.append((x - nx, y - ny))
        lines.append(f"o r{r['id']}_{r['class']}")
        strip = []
        for (lx, ly), (rx, ry) in zip(left, right):
            emit(lx, ly, 0.05)
            strip.append(v_off)
            emit(rx, ry, 0.05)
            v_off += 2
        for k in range(len(strip) // 2 - 1):
            a = strip[2 * k]
            lines.append(f"f {a} {a + 1} {a + 3}")
            lines.append(f"f {a} {a + 3} {a + 2}")

    with open(path, "w", encoding="utf-8") as fh:
        fh.write("\n".join(lines))
    print(f"[obj  ] {path}: {v_off - 1} vertices")


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--bbox", required=True,
                    help="minlat,minlon,maxlat,maxlon")
    ap.add_argument("--out", default="district.json")
    ap.add_argument("--obj", help="optional Wavefront OBJ preview path")
    args = ap.parse_args()

    try:
        bbox = tuple(float(v) for v in args.bbox.split(","))
        if len(bbox) != 4:
            raise ValueError
    except ValueError:
        raise SystemExit("bbox must be four comma-separated numbers: "
                         "minlat,minlon,maxlat,maxlon")

    elements = fetch_overpass(args.bbox)["elements"]
    buildings, roads, pois, skipped = process(elements, bbox)
    if not buildings:
        raise SystemExit("No buildings found in bbox — check coordinates.")
    resolved, matched = join_pois(buildings, pois)
    print(f"[join ] {matched}/{len(pois)} POI nodes fell inside footprints; "
          f"{resolved} 'unknown' buildings re-classified")
    write_json(args.out, bbox, buildings, roads, skipped)
    if args.obj:
        write_obj(args.obj, buildings, roads)


if __name__ == "__main__":
    main()
