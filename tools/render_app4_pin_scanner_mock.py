from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

OUT_DIR = Path("docs/screenshots/app4-pin-scanner-render")
OUT_DIR.mkdir(parents=True, exist_ok=True)


def rgb565(value):
    r = ((value >> 11) & 0x1F) * 255 // 31
    g = ((value >> 5) & 0x3F) * 255 // 63
    b = (value & 0x1F) * 255 // 31
    return (r, g, b)


MAIN = {
    "bg": rgb565(0x0000),
    "grid": rgb565(0x39E7),
    "grid_soft": rgb565(0x2945),
    "text": rgb565(0xFFFF),
    "cyan": rgb565(0x07FF),
    "yellow": rgb565(0xFFF2),
    "red": rgb565(0xF800),
    "light_blue": rgb565(0x02F6),
    "light_amber": rgb565(0xBC20),
}

PALETTES = {
    "mono_dark": {
        "title": "Monochrome Dark",
        "name": "M DARK",
        "bg": (0, 0, 0),
        "text": (255, 255, 255),
        "grid": (255, 255, 255),
        "grid_soft": (255, 255, 255),
        "value": (255, 255, 255),
        "bar": (255, 255, 255),
        "hot": (255, 255, 255),
        "rail": (255, 255, 255),
        "nav_fill": (0, 0, 0),
        "nav_text": (255, 255, 255),
    },
    "mono_light": {
        "title": "Monochrome Light",
        "name": "M LIGHT",
        "bg": (255, 255, 255),
        "text": (0, 0, 0),
        "grid": (0, 0, 0),
        "grid_soft": (0, 0, 0),
        "value": (0, 0, 0),
        "bar": (0, 0, 0),
        "hot": (0, 0, 0),
        "rail": (0, 0, 0),
        "nav_fill": (255, 255, 255),
        "nav_text": (0, 0, 0),
    },
    "color_dark": {
        "title": "Color Dark",
        "name": "C DARK",
        "bg": MAIN["bg"],
        "text": MAIN["text"],
        "grid": MAIN["grid"],
        "grid_soft": MAIN["grid_soft"],
        "value": MAIN["yellow"],
        "bar": MAIN["cyan"],
        "hot": MAIN["yellow"],
        "rail": MAIN["red"],
        "nav_fill": MAIN["bg"],
        "nav_text": MAIN["text"],
    },
    "color_light": {
        "title": "Color Light",
        "name": "C LIGHT",
        "bg": (255, 255, 255),
        "text": (0, 0, 0),
        "grid": (0, 0, 0),
        "grid_soft": (0, 0, 0),
        "value": MAIN["light_blue"],
        "bar": MAIN["light_blue"],
        "hot": MAIN["light_amber"],
        "rail": (120, 0, 0),
        "nav_fill": (0, 0, 255),
        "nav_text": (255, 255, 255),
    },
}

PINS = [
    ("P3  IO35", 2810, 316, False),
    ("CN1 IO27", 1948, 114, False),
    ("IO22", 1512, 58, False),
    ("LDR IO34", 4077, 7, True),
    ("IO32", 32, 4, True),
    ("IO33", 2066, 2, False),
]

try:
    FONT_1 = ImageFont.truetype("/System/Library/Fonts/Menlo.ttc", 10)
    FONT_2 = ImageFont.truetype("/System/Library/Fonts/Menlo.ttc", 20)
except OSError:
    FONT_1 = ImageFont.load_default()
    FONT_2 = ImageFont.load_default()


def draw_button(draw, x, y, label, p):
    draw.rounded_rectangle((x, y, x + 43, y + 27), radius=4, fill=p["nav_fill"], outline=p["grid"], width=2)
    bbox = draw.textbbox((0, 0), label, font=FONT_2)
    draw.text((x + (44 - (bbox[2] - bbox[0])) // 2, y + 3), label, font=FONT_2, fill=p["nav_text"])


def draw_nav(draw, p):
    draw_button(draw, 182, 7, "<", p)
    draw_button(draw, 228, 7, ">", p)
    draw_button(draw, 274, 7, "*", p)


def dotted_line(draw, y, p):
    for x in range(0, 320, 5):
        draw.rectangle((x, y, x, y), fill=p["grid_soft"])


def draw_screen(mode_key):
    p = PALETTES[mode_key]
    image = Image.new("RGB", (320, 240), p["bg"])
    draw = ImageDraw.Draw(image)
    draw.line((0, 41, 320, 41), fill=p["grid"])
    draw.text((10, 8), "Pin Scanner", font=FONT_1, fill=p["text"])
    draw.text((10, 24), f"{p['name']} raw ADC 0..4095", font=FONT_1, fill=p["text"])
    draw_nav(draw, p)

    hot_index = 0
    for idx, (label, value, movement, railed) in enumerate(PINS):
        y = 50 + idx * 28
        hot = idx == hot_index
        dotted_line(draw, y + 27, p)
        if hot:
            draw.rectangle((0, y + 2, 3, y + 24), fill=p["hot"])
        draw.text((8, y + 5), label, font=FONT_1, fill=p["hot"] if hot else p["text"])
        draw.rectangle((82, y + 8, 225, y + 19), outline=p["grid"])
        fill_w = max(0, int((value / 4095) * 144) - 2)
        draw.rectangle((83, y + 9, 83 + fill_w, y + 18), fill=p["hot"] if hot else p["bar"])
        draw.text((244, y + 2), f"{value:4d}", font=FONT_1, fill=p["value"])
        draw.text((244, y + 13), f"d{movement:4d}", font=FONT_1, fill=p["hot"] if hot else p["text"])
        if hot:
            draw.text((292, y + 2), "hot", font=FONT_1, fill=p["hot"])
        if railed:
            draw.text((292, y + 13), "rail", font=FONT_1, fill=p["rail"])

    draw.text((8, 226), "Known: IO35 IO22 IO27  HOT=movement", font=FONT_1, fill=p["text"])
    path = OUT_DIR / f"app4-pin-scanner-{mode_key}.png"
    image.save(path)
    print(path)
    return path


def draw_contact_sheet(paths):
    margin = 16
    label_h = 24
    sheet = Image.new("RGB", (320 * 2 + margin * 3, (240 + label_h) * 2 + margin * 3), (24, 24, 24))
    draw = ImageDraw.Draw(sheet)
    for idx, path in enumerate(paths):
        mode_key = path.stem.replace("app4-pin-scanner-", "")
        p = PALETTES[mode_key]
        x = margin + (idx % 2) * (320 + margin)
        y = margin + (idx // 2) * (240 + label_h + margin)
        draw.text((x, y), p["title"], font=FONT_2, fill=(255, 255, 255))
        sheet.paste(Image.open(path).convert("RGB"), (x, y + label_h))
    path = OUT_DIR / "app4-pin-scanner-contact-sheet.png"
    sheet.save(path)
    print(path)


paths = [draw_screen(key) for key in ["mono_dark", "mono_light", "color_dark", "color_light"]]
draw_contact_sheet(paths)
