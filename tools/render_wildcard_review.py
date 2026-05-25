from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

OUT_DIR = Path("docs/screenshots/wildcard-review")
OUT_DIR.mkdir(parents=True, exist_ok=True)

W, H = 320, 240

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


def centered(draw, text, x, y, w, font, fill):
    box = draw.textbbox((0, 0), text, font=font)
    draw.text((x + max(0, (w - (box[2] - box[0])) // 2), y), text, font=font, fill=fill)


def heart(draw, cx, cy, red, outline):
    for color, size in [(outline, 15), (red, 12)]:
        draw.ellipse((cx - size, cy - size, cx, cy), fill=color)
        draw.ellipse((cx, cy - size, cx + size, cy), fill=color)
        draw.polygon([(cx - size, cy - 3), (cx + size, cy - 3), (cx, cy + size + 7)], fill=color)


def apple_button(draw, x, y, text, active=False):
    fill = (239, 239, 244) if active else (255, 255, 255)
    outline = (174, 174, 178)
    draw.rounded_rectangle((x, y, x + 39, y + 25), radius=8, fill=fill, outline=outline)
    centered(draw, text, x, y + 3, 40, FONT_2, (28, 28, 30))


def apple_dashboard(locked, filename):
    bg = (242, 242, 247)
    panel = (255, 255, 255)
    text = (28, 28, 30)
    muted = (142, 142, 147)
    separator = (209, 209, 214)
    acquire = (255, 159, 10)
    lock = (52, 199, 89)
    beat = (0, 122, 255)
    red = (255, 59, 48)

    image = Image.new("RGB", (W, H), bg)
    draw = ImageDraw.Draw(image)
    draw.rectangle((0, 0, W, 42), fill=panel)
    draw.line((0, 41, W, 41), fill=separator)
    draw.text((10, 7), "PulseSensor", font=FONT_1, fill=text)
    draw.text((10, 22), "Live PPG", font=FONT_1, fill=muted)
    heart(draw, 112, 22, red, panel)
    x = 180
    for label in ["<", ">", "*"]:
        apple_button(draw, x, 8, label, active=label == "*")
        x += 45

    gx, gy, gw, gh = 10, 52, 300, 104
    draw.rounded_rectangle((gx, gy, gx + gw, gy + gh), radius=8, fill=panel, outline=separator)
    for i in range(1, 4):
        draw.line((gx + i * gw // 4, gy + 8, gx + i * gw // 4, gy + gh - 8), fill=(229, 229, 234))
    for i in range(1, 3):
        draw.line((gx + 8, gy + i * gh // 3, gx + gw - 8, gy + i * gh // 3), fill=(229, 229, 234))
    draw.text((gx + 10, gy + 8), "Signal", font=FONT_1, fill=muted)
    wave = []
    offsets = [0, -4, 2, -3, 1, -6, 4, -2]
    for i in range(0, gw - 20, 9):
        wave.append((gx + 10 + i, gy + gh // 2 + offsets[(i // 9) % len(offsets)]))
    draw.line(wave, fill=lock if locked else acquire, width=5)
    for bx in [74, 156, 236]:
        by = gy + gh // 2 + offsets[(bx // 9) % len(offsets)]
        draw.ellipse((gx + bx - 5, by - 5, gx + bx + 5, by + 5), fill=beat)

    cards = [(10, 166, 90, "BPM", "84" if locked else "--"),
             (110, 166, 100, "IBI", "720" if locked else "--"),
             (220, 166, 90, "LOCK", "YES" if locked else "NO")]
    for x, y, w, label, value in cards:
        draw.rounded_rectangle((x, y, x + w, y + 64), radius=8, fill=panel, outline=separator)
        draw.text((x + 10, y + 8), label, font=FONT_1, fill=muted)
        draw.text((x + 10, y + 24), value, font=FONT_4 if len(value) < 4 else FONT_3, fill=text)
        draw.line((x + 8, y + 60, x + w - 8, y + 60), fill=lock if locked else acquire, width=3)

    path = OUT_DIR / filename
    image.save(path)
    return path


def apple_settings(filename):
    bg = (242, 242, 247)
    panel = (255, 255, 255)
    text = (28, 28, 30)
    muted = (142, 142, 147)
    separator = (209, 209, 214)
    accent = (255, 159, 10)
    image = Image.new("RGB", (W, H), bg)
    draw = ImageDraw.Draw(image)
    draw.rectangle((0, 0, W, 42), fill=panel)
    draw.text((10, 10), "Settings", font=FONT_2, fill=text)
    draw.text((10, 29), "2026-05-24", font=FONT_1, fill=muted)
    for x, label in [(184, "<"), (229, ">"), (274, "*")]:
        apple_button(draw, x, 8, label, active=label == "*")
    y = 52
    rows = [("Volume", "1/10"), ("Rotation", "screen 1"), ("Display", "chromatic"), ("LED", "beat pulse")]
    for label, value in rows:
        draw.rounded_rectangle((10, y, 310, y + 34), radius=8, fill=panel, outline=separator)
        draw.text((20, y + 8), label, font=FONT_1, fill=text)
        box = draw.textbbox((0, 0), value, font=FONT_1)
        draw.text((300 - (box[2] - box[0]), y + 8), value, font=FONT_1, fill=accent)
        y += 42
    path = OUT_DIR / filename
    image.save(path)
    return path


def lab_button(draw, x, y, text, active=False, w=44):
    fill = (255, 190, 20) if active else (18, 23, 30)
    outline = (255, 190, 20)
    draw.rounded_rectangle((x, y, x + w, y + 27), radius=4, fill=fill, outline=outline, width=2)
    centered(draw, text, x, y + 4, w, FONT_2 if len(text) < 3 else FONT_1, (8, 12, 16) if active else (255, 255, 255))


def lab_dashboard(locked, filename):
    bg = (8, 12, 16)
    panel = (18, 23, 30)
    text = (255, 255, 255)
    grid = (68, 76, 84)
    yellow = (255, 190, 20)
    blue = (0, 172, 255)
    lock = (0, 220, 130)
    orange = (255, 100, 34)
    purple = (170, 90, 255)

    image = Image.new("RGB", (W, H), bg)
    draw = ImageDraw.Draw(image)
    draw.rectangle((0, 0, W, 42), fill=(12, 16, 22))
    draw.rectangle((0, 38, W, 42), fill=yellow if not locked else lock)
    draw.text((10, 6), "PULSE LAB", font=FONT_2, fill=text)
    draw.text((10, 28), "TEST RIG: CYD", font=FONT_1, fill=yellow)
    heart(draw, 122, 22, orange, text)
    x = 184
    for label in ["<", ">", "*"]:
        lab_button(draw, x, 7, label, active=label == "*")
        x += 45

    gx, gy, gw, gh = 8, 52, 304, 102
    draw.rounded_rectangle((gx, gy, gx + gw, gy + gh), radius=4, fill=(4, 7, 10), outline=yellow, width=2)
    for ix in range(0, gw + 1, 38):
        draw.line((gx + ix, gy, gx + ix, gy + gh), fill=grid)
    for iy in range(0, gh + 1, 26):
        draw.line((gx, gy + iy, gx + gw, gy + iy), fill=grid)
    draw.text((gx + 8, gy + 6), "LIVE WAVEFORM", font=FONT_1, fill=yellow)
    draw.text((gx + gw - 60, gy + 6), "THR 550", font=FONT_1, fill=text)
    wave = []
    offsets = [0, -5, 3, -4, 2, -8, 5, -2]
    for i in range(0, gw - 20, 9):
        wave.append((gx + 10 + i, gy + gh // 2 + offsets[(i // 9) % len(offsets)]))
    draw.line(wave, fill=lock if locked else yellow, width=5)
    for bx in [76, 158, 240]:
        by = gy + gh // 2 + offsets[(bx // 9) % len(offsets)]
        draw.ellipse((gx + bx - 6, by - 6, gx + bx + 6, by + 6), fill=purple, outline=text, width=1)

    y = 164
    cards = [("BPM", "84" if locked else "--", 8, 98),
             ("IBI", "720" if locked else "--", 112, 104),
             ("SIG", "LIVE" if locked else "ARM", 224, 88)]
    for label, value, x, w in cards:
        draw.rounded_rectangle((x, y, x + w, y + 66), radius=4, fill=panel, outline=blue if locked else yellow, width=2)
        draw.text((x + 8, y + 6), label, font=FONT_1, fill=yellow)
        draw.text((x + 8, y + 24), value, font=FONT_4 if len(value) < 4 else FONT_3, fill=text)
    path = OUT_DIR / filename
    image.save(path)
    return path


def lab_settings(filename):
    bg = (8, 12, 16)
    panel = (18, 23, 30)
    text = (255, 255, 255)
    yellow = (255, 190, 20)
    orange = (255, 100, 34)
    image = Image.new("RGB", (W, H), bg)
    draw = ImageDraw.Draw(image)
    draw.rectangle((0, 0, W, 42), fill=(12, 16, 22))
    draw.rectangle((0, 38, W, 42), fill=yellow)
    draw.text((10, 8), "CONTROL BAY", font=FONT_2, fill=text)
    draw.text((10, 29), "pulse rig setup", font=FONT_1, fill=yellow)
    for x, label in [(184, "<"), (229, ">"), (274, "*")]:
        lab_button(draw, x, 7, label, active=label == "*")
    y = 52
    for label, value in [("Volume", "1/10"), ("Rotation", "screen 1"), ("Display", "LAB DARK"), ("LED", "beat pulse")]:
        draw.rectangle((8, y, 312, y + 34), fill=panel, outline=yellow)
        draw.rectangle((8, y, 13, y + 34), fill=orange)
        draw.text((20, y + 5), label.upper(), font=FONT_1, fill=yellow)
        draw.text((20, y + 18), value, font=FONT_1, fill=text)
        y += 42
    path = OUT_DIR / filename
    image.save(path)
    return path


def contact(paths):
    labels = [
        ("Apple-inspired searching", paths[0]),
        ("Apple-inspired locked", paths[1]),
        ("Apple-inspired settings", paths[2]),
        ("Lab/Rober-inspired searching", paths[3]),
        ("Lab/Rober-inspired locked", paths[4]),
        ("Lab/Rober-inspired settings", paths[5]),
    ]
    sheet = Image.new("RGB", (W * 3, (H + 24) * 2), (28, 28, 30))
    draw = ImageDraw.Draw(sheet)
    for i, (label, path) in enumerate(labels):
        col = i % 3
        row = i // 3
        x = col * W
        y = row * (H + 24)
        draw.text((x + 8, y + 5), label, font=FONT_LABEL, fill=(255, 255, 255))
        sheet.paste(Image.open(path), (x, y + 24))
    out = OUT_DIR / "wildcard-contact-sheet.png"
    sheet.save(out)
    print(out)


paths = [
    apple_dashboard(False, "apple-searching.png"),
    apple_dashboard(True, "apple-locked.png"),
    apple_settings("apple-settings.png"),
    lab_dashboard(False, "lab-searching.png"),
    lab_dashboard(True, "lab-locked.png"),
    lab_settings("lab-settings.png"),
]
contact(paths)
