#!/usr/bin/env python3
"""
Download map tiles from OpenStreetMap and convert them to RGB666 binary format
for ESP32 LCD displays (ILI9488).

Usage:
    python download_tiles.py

Output:
    ./output/{zoom}/{tile_x}/{tile_y}.bin (RGB666 raw, 192 KB each)
    ./output/{zoom}/{tile_x}/{tile_y}.png (original downloaded PNG)

Requirements:
    pip install requests Pillow
"""

from __future__ import annotations

import math
import time
from io import BytesIO
from pathlib import Path
from typing import Optional

import requests
from PIL import Image

# ============================================================
# Configuration
# ============================================================

# Center coordinate (Dinh Độc Lập, TP.HCM)
CENTER_LAT = 10.7770
CENTER_LON = 106.6953

# Zoom level: 15 is a good balance for street-level maps.
ZOOM = 16

# Number of tiles to download in each direction from the center.
TILES_RADIUS = 7

# Output directory for the converted RGB666 files.
OUTPUT_DIR = Path(__file__).resolve().parent / "output"

# OSM tile server.
TILE_SERVER = "https://tile.openstreetmap.org/{z}/{x}/{y}.png"

# Request delay to stay respectful to the OSM tile server.
REQUEST_DELAY = 0.5


# ============================================================
# Coordinate helpers
# ============================================================

def lon_to_tile_x(lon: float, zoom: int) -> float:
    """Convert longitude to tile X coordinate."""
    return (lon + 180.0) / 360.0 * (2**zoom)


def lat_to_tile_y(lat: float, zoom: int) -> float:
    """Convert latitude to tile Y coordinate."""
    lat_rad = math.radians(lat)
    return (1.0 - math.asinh(math.tan(lat_rad)) / math.pi) / 2.0 * (2**zoom)


def tile_x_to_lon(x: float, zoom: int) -> float:
    """Convert tile X to longitude."""
    return x / (2**zoom) * 360.0 - 180.0


def tile_y_to_lat(y: float, zoom: int) -> float:
    """Convert tile Y to latitude."""
    n = math.pi - 2.0 * math.pi * y / (2**zoom)
    return math.degrees(math.atan(math.sinh(n)))


# ============================================================
# Download helpers
# ============================================================

def download_tile(zoom: int, tile_x: int, tile_y: int) -> Optional[Image.Image]:
    """Download a single tile PNG from the OSM server."""
    url = TILE_SERVER.format(z=zoom, x=tile_x, y=tile_y)

    try:
        response = requests.get(
            url,
            timeout=30,
            headers={
                    "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/138.0 Safari/537.36",
                    "Accept": "image/png,image/*,*/*;q=0.8",
            },
        )
        if (response.status_code == 200
            and response.headers.get("Content-Type", "").startswith("image/png")
        ):
            return Image.open(BytesIO(response.content))


        print(response.status_code)
        print(response.headers.get("Content-Type"))
        print(response.text[:300])
        print(f"HTTP {response.status_code} for tile ({tile_x}, {tile_y})")
        return None
    except Exception as exc:
        print(f"Error downloading tile ({tile_x}, {tile_y}): {exc}")
        return None


# ============================================================
# Conversion helpers: PNG -> RGB666
# ============================================================

def image_to_rgb666(img: Image.Image) -> bytes:
    """Convert a PIL image to RGB666 raw binary data."""
    if img.size != (256, 256):
        img = img.resize((256, 256), Image.LANCZOS)

    img = img.convert("RGB")
    binary_data = bytearray()

    for r, g, b in img.getdata():
        r6 = (r >> 2) & 0x3F
        g6 = (g >> 2) & 0x3F
        b6 = (b >> 2) & 0x3F

        binary_data.append(r6)
        binary_data.append(g6)
        binary_data.append(b6)

    return bytes(binary_data)


def save_tile_binary(zoom: int, tile_x: int, tile_y: int, binary_data: bytes) -> Path:
    """Save RGB666 binary data to the output directory."""
    tile_dir = OUTPUT_DIR / str(zoom) / str(tile_x)
    tile_dir.mkdir(parents=True, exist_ok=True)

    tile_path = tile_dir / f"{tile_y}.bin"
    tile_path.write_bytes(binary_data)
    return tile_path


def save_tile_png(zoom: int, tile_x: int, tile_y: int, img: Image.Image) -> Path:
    """Save the original downloaded PNG tile to the output directory."""
    tile_dir = OUTPUT_DIR / str(zoom) / str(tile_x)
    tile_dir.mkdir(parents=True, exist_ok=True)

    tile_path = tile_dir / f"{tile_y}.png"
    img.save(tile_path, format="PNG")
    return tile_path


# ============================================================
# Main flow
# ============================================================

def main() -> None:
    print("=" * 60)
    print("OSM Tile Downloader → RGB666 Binary for ESP32 LCD")
    print("=" * 60)
    print()
    print(f"Center: lat={CENTER_LAT}, lon={CENTER_LON}")
    print(f"Zoom: {ZOOM}")
    print(f"Radius: {TILES_RADIUS} tiles each direction")
    print(f"Output: {OUTPUT_DIR}")
    print()

    center_x = lon_to_tile_x(CENTER_LON, ZOOM)
    center_y = lat_to_tile_y(CENTER_LAT, ZOOM)
    center_tile_x = int(center_x)
    center_tile_y = int(center_y)

    x_start = center_tile_x - TILES_RADIUS
    x_end = center_tile_x + TILES_RADIUS
    y_start = center_tile_y - TILES_RADIUS
    y_end = center_tile_y + TILES_RADIUS

    total_tiles = (x_end - x_start + 1) * (y_end - y_start + 1)

    print(f"Center tile: ({center_tile_x}, {center_tile_y})")
    print(f"Tile range: X[{x_start}..{x_end}], Y[{y_start}..{y_end}]")
    print(f"Total tiles to download: {total_tiles}")
    print(f"Estimated size: {total_tiles * 192} KB ({total_tiles * 192 / 1024:.1f} MB)")
    print()
    print("=" * 60)
    print()

    response = input(f"Download {total_tiles} tiles? (y/n): ").strip().lower()
    if response != "y":
        print("Cancelled.")
        return

    print()

    downloaded = 0
    failed = 0
    skipped = 0
    start_time = time.time()

    index = 0
    for ty in range(y_start, y_end + 1):
        for tx in range(x_start, x_end + 1):
            index += 1
            tile_dir = OUTPUT_DIR / str(ZOOM) / str(tx)
            bin_path = tile_dir / f"{ty}.bin"
            png_path = tile_dir / f"{ty}.png"

            if bin_path.exists() and png_path.exists():
                print(f"[{index}/{total_tiles}] SKIP ({tx}, {ty}) — already exists")
                skipped += 1
                continue

            print(f"[{index}/{total_tiles}] Downloading ({tx}, {ty})...", end=" ")
            img = download_tile(ZOOM, tx, ty)

            if img is not None:
                if not png_path.exists():
                    save_tile_png(ZOOM, tx, ty, img)
                if not bin_path.exists():
                    binary_data = image_to_rgb666(img)
                    save_tile_binary(ZOOM, tx, ty, binary_data)
                print(f"OK (png + bin)")
                downloaded += 1
            else:
                print("FAILED")
                failed += 1

            time.sleep(REQUEST_DELAY)

    elapsed = time.time() - start_time

    print()
    print("=" * 60)
    print("DONE!")
    print(f"Downloaded: {downloaded}")
    print(f"Skipped: {skipped}")
    print(f"Failed: {failed}")
    print(f"Time: {elapsed:.1f}s")
    print(f"Output: {OUTPUT_DIR}")
    print()
    print("Next: copy the 'output' folder to your SD card as '/maps/'")
    print(f"SD card structure: /maps/{ZOOM}/{{tile_x}}/{{tile_y}}.bin")
    print("=" * 60)


if __name__ == "__main__":
    main()
