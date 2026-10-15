"""Couleur moyenne d'un carré de pixels d'une capture PNG (essais Android, scénario S3).

Usage : python scripts/lire-pixel.py <capture.png> <x> <y> [rayon]   → « R G B »
        python scripts/lire-pixel.py <capture.png> --taille          → « largeur hauteur »
Moyenne du carré de côté 2 × rayon + 1 (rayon 2 par défaut) ; coordonnées en pixels de l'appareil, négatives
comptées depuis le bord droit ou le bord bas.
"""
import sys

from PIL import Image


def main() -> int:
    if len(sys.argv) < 3:
        print(__doc__, file=sys.stderr)
        return 2
    image = Image.open(sys.argv[1]).convert("RGB")
    width, height = image.size
    if sys.argv[2] == "--taille":
        print(f"{width} {height}")
        return 0
    if len(sys.argv) < 4:
        print(__doc__, file=sys.stderr)
        return 2
    x, y = int(sys.argv[2]), int(sys.argv[3])
    radius = int(sys.argv[4]) if len(sys.argv) > 4 else 2
    x = x + width if x < 0 else x
    y = y + height if y < 0 else y
    total = [0, 0, 0]
    count = 0
    for yy in range(max(0, y - radius), min(height, y + radius + 1)):
        for xx in range(max(0, x - radius), min(width, x + radius + 1)):
            pixel = image.getpixel((xx, yy))
            for channel in range(3):
                total[channel] += pixel[channel]
            count += 1
    if count == 0:
        print("hors image", file=sys.stderr)
        return 1
    print(" ".join(str(round(value / count)) for value in total))
    return 0


if __name__ == "__main__":
    sys.exit(main())
