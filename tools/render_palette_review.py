from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

OUT_DIR = Path("docs/screenshots/palette-review")
OUT_DIR.mkdir(parents=True, exist_ok=True)

W, H = 320, 240
BUTTON_W = 44
BUTTON_H = 28
GAP = 2

try:
    FONT_1 = ImageFont.truetype("/System/Library/Fonts/Menlo.ttc", 10)
    FONT_2 = ImageFont.truetype("/System/Library/Fonts/Menlo.ttc", 20)
    FONT_3 = ImageFont.truetype("/System/Library/Fonts/Menlo.ttc", 30)
    FONT_4 = ImageFont.truetype("/System/Library/Fonts/Menlo.ttc", 40)
    FONT_LABEL = ImageFont.truetype("/System/Library/Fonts/Menlo.ttc", 14)
except OSError:
    FONT_1 = ImageFont.load_default()
    FONT_2 = ImageFont.load_default()
    FONT_3 = ImageFont.load_default()
    FONT_4 = ImageFont.load_default()
    FONT_LABEL = ImageFont.load_default()


PALETTES = {
    "current": {
        "title": "A Current tuned",
        "bg": (0, 0, 0),
        "panel": (0, 0, 0),
        "panel_dark": (0, 8, 0),
        "text": (255, 255, 255),
        "muted": (92, 92, 92),
        "grid": (90, 92, 88),
        "acquire": (255, 255, 144),
        "lock": (0, 255, 0),
        "heart": (255, 0, 0),
        "beat": (0, 188, 255),
        "button_fill": (0, 0, 0),
        "button_active": (4, 160, 0),
        "button_outline": (255, 255, 255),
    },
    "brand_dark": {
        "title": "B Brand teal dark",
        "bg": (18, 18, 17),
        "panel": (12, 22, 20),
        "panel_dark": (5, 13, 12),
        "text": (255, 255, 255),
        "muted": (92, 94, 92),
        "grid": (70, 75, 72),
        "acquire": (251, 205, 10),
        "lock": (16, 132, 116),
        "heart": (232, 24, 36),
        "beat": (153, 105, 255),
        "button_fill": (18, 18, 17),
        "button_active": (16, 132, 116),
        "button_outline": (255, 255, 255),
    },
    "chromatic_dark": {
        "title": "C Chromatic dark",
        "bg": (5, 7, 9),
        "panel": (13, 17, 20),
        "panel_dark": (2, 4, 5),
        "text": (255, 255, 255),
        "muted": (76, 82, 86),
        "grid": (61, 68, 72),
        "acquire": (251, 205, 10),
        "lock": (0, 207, 135),
        "heart": (255, 42, 61),
        "beat": (178, 107, 255),
        "button_fill": (5, 7, 9),
        "button_active": (0, 105, 84),
        "button_outline": (255, 255, 255),
    },
    "brand_light": {
        "title": "D Brand light",
        "bg": (249, 250, 251),
        "panel": (255, 255, 255),
        "panel_dark": (237, 245, 245),
        "text": (18, 18, 17),
        "muted": (190, 194, 194),
        "grid": (205, 212, 212),
        "acquire": (172, 124, 0),
        "lock": (16, 132, 116),
        "heart": (210, 0, 25),
        "beat": (113, 54, 210),
        "button_fill": (255, 255, 255),
        "button_active": (221, 242, 239),
        "button_outline": (18, 18, 17),
    },
}


def centered(draw, text, x, y, w, font, color):
    box = draw.textbbox((0, 0), text, font=font)
    text_w = box[2] - box[0]
    draw.text((x + max(0, (w - text_w) // 2), y), text, font=font, fill=color)


def button(draw, x, y, label, p, active=False, w=BUTTON_W):
    draw.rounded_rectangle(
        (x, y, x + w - 1, y + BUTTON_H - 1),
        radius=4,
        fill=p["button_active"] if active else p["button_fill"],
        outline=p["button_outline"],
        width=2,
    )
    centered(draw, label, x, y + 4, w, FONT_2 if len(label) < 3 else FONT_1, p["text"])


def header(draw, p, settings=False):
    draw.rectangle((0, 0, W, 42), fill=p["bg"])
    draw.line((0, 41, W, 41), fill=p["muted"])
    draw.text((10, 4), "PulseSensor.com", font=FONT_1, fill=p["text"])
    draw.text((10, 15), "0.4.34-bold-wave", font=FONT_1, fill=p["text"])
    draw.text((10, 26), "2026-05-24", font=FONT_1, fill=p["acquire"] if settings else p["text"])
    x = W - BUTTON_W - 4
    button(draw, x - (BUTTON_W + GAP) * 2, 7, "<", p)
    button(draw, x - (BUTTON_W + GAP), 7, ">", p)
    button(draw, x, 7, "*", p, settings)


def heart(draw, cx, cy, p):
    outline = p["text"]
    red = p["heart"]
    for color, size in [(outline, 15), (red, 12)]:
        draw.ellipse((cx - size, cy - size, cx, cy), fill=color)
        draw.ellipse((cx, cy - size, cx + size, cy), fill=color)
        draw.polygon([(cx - size, cy - 3), (cx + size, cy - 3), (cx, cy + size + 6)], fill=color)


def graph(draw, p, locked):
    x, y, w, h = 8, 48, 304, 112
    draw.rounded_rectangle((x - 2, y - 2, x + w + 1, y + h + 1), radius=6, fill=p["panel_dark"], outline=p["grid"])
    draw.rectangle((x, y, x + w, y + h), fill=p["bg"])
    for gx in range(0, w + 1, 38):
        draw.line((x + gx, y, x + gx, y + h), fill=p["grid"])
    for gy in range(0, h + 1, 28):
        draw.line((x, y + gy, x + w, y + gy), fill=p["grid"])
    draw.text((x + 6, y + 5), "LIVE LINE", font=FONT_1, fill=p["text"])
    draw.text((x + w - 48, y + 5), "THR 550", font=FONT_1, fill=p["text"])
    for tx in range(x, x + w, 6):
        draw.line((tx, y + 65, tx, y + 67), fill=p["text"])

    points = []
    wave = [0, -5, 3, -3, 2, -7, 5, -2, 1, -4, 2]
    for i in range(0, w + 1, 10):
        points.append((x + i, y + h // 2 + wave[(i // 10) % len(wave)]))
    draw.line(points, fill=p["lock"] if locked else p["acquire"], width=5)
    for bx in [90, 168, 246]:
        by = y + h // 2 + wave[(bx // 10) % len(wave)]
        draw.ellipse((x + bx - 5, by - 5, x + bx + 5, by + 5), fill=p["beat"])


def panel(draw, x, y, w, h, label, value, p, locked, unit=""):
    outline = p["lock"] if locked else p["acquire"]
    draw.rounded_rectangle((x, y, x + w, y + h), radius=6, fill=p["panel"], outline=outline, width=2)
    draw.text((x + 8, y + 7), label, font=FONT_1, fill=p["text"])
    draw.text((x + 8, y + 19), value, font=FONT_4 if w > 90 else FONT_3, fill=p["text"])
    if unit:
        draw.text((x + w - 30, y + h - 17), unit, font=FONT_1, fill=p["text"])


def dashboard(p, locked, filename):
    image = Image.new("RGB", (W, H), p["bg"])
    draw = ImageDraw.Draw(image)
    header(draw, p)
    heart(draw, 112, 22, p)
    graph(draw, p, locked)
    panel(draw, 8, 170, 102, 62, "BPM", "84" if locked else "--", p, locked)
    panel(draw, 118, 170, 102, 62, "IBI", "720" if locked else "--", p, locked, "ms" if locked else "")
    panel(draw, 228, 170, 84, 62, "SIG", "12", p, locked)
    path = OUT_DIR / filename
    image.save(path)
    return path


def settings(p, filename):
    image = Image.new("RGB", (W, H), p["bg"])
    draw = ImageDraw.Draw(image)
    header(draw, p, settings=True)
    rows = [
        ("Volume", "1/10"),
        ("Rotation", "screen 1"),
        ("Display", "C DARK"),
        ("LED Control", "beat pulse"),
    ]
    y = 48
    for label, value in rows:
        draw.rectangle((0, y, W, y + 39), fill=p["bg"])
        draw.line((0, y + 38, W, y + 38), fill=p["acquire"])
        draw.text((10, y + 3), label, font=FONT_2, fill=p["text"])
        draw.text((10, y + 21), value, font=FONT_1, fill=p["acquire"])
        y += 40
    button(draw, 0, 210, "^", p, False, 159)
    button(draw, 161, 210, "v", p, True, 159)
    path = OUT_DIR / filename
    image.save(path)
    return path


def contact_sheet(paths):
    scale = 1
    label_h = 24
    cols = 3
    rows = len(PALETTES)
    sheet = Image.new("RGB", (cols * W, rows * (H + label_h)), (32, 32, 32))
    draw = ImageDraw.Draw(sheet)
    for row, (key, p) in enumerate(PALETTES.items()):
        draw.text((8, row * (H + label_h) + 5), p["title"], font=FONT_LABEL, fill=(255, 255, 255))
        for col, path in enumerate(paths[key]):
            image = Image.open(path)
            sheet.paste(image.resize((W * scale, H * scale)), (col * W, row * (H + label_h) + label_h))
    out = OUT_DIR / "palette-contact-sheet.png"
    sheet.save(out)
    print(out)


paths = {}
for key, palette in PALETTES.items():
    paths[key] = [
        dashboard(palette, False, f"{key}-searching.png"),
        dashboard(palette, True, f"{key}-locked.png"),
        settings(palette, f"{key}-settings.png"),
    ]

contact_sheet(paths)
