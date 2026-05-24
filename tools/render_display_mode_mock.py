from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

OUT_DIR = Path("docs/screenshots/display-mode-render")
REVIEW_DIR = OUT_DIR / "review-20260524-display-modes-v2"
OUT_DIR.mkdir(parents=True, exist_ok=True)


def rgb565(value):
    r = ((value >> 11) & 0x1F) * 255 // 31
    g = ((value >> 5) & 0x3F) * 255 // 63
    b = (value & 0x1F) * 255 // 31
    return (r, g, b)


def invert(color):
    return tuple(255 - channel for channel in color)


MAIN = {
    "bg": rgb565(0x0000),
    "panel": rgb565(0x0841),
    "panel_dark": rgb565(0x0400),
    "grid": rgb565(0x39E7),
    "grid_soft": rgb565(0x2945),
    "text": rgb565(0xFFFF),
    "cyan": rgb565(0x07FF),
    "cyan_dark": rgb565(0x0452),
    "teal": rgb565(0x05F3),
    "acquire_blue": rgb565(0x3DFF),
    "lock_green": rgb565(0x07E0),
    "red": rgb565(0xF800),
    "red_dark": rgb565(0x6000),
    "signal_yellow": rgb565(0xFFF2),
}

PALETTES = {
    "color_dark": {
        **MAIN,
        "name": "C DARK",
        "title": "Color Dark",
        "mode_value": "color dark",
        "button_fill": MAIN["cyan_dark"],
        "button_active_fill": MAIN["cyan_dark"],
        "button_inactive_fill": MAIN["bg"],
        "button_outline": MAIN["cyan"],
        "button_active_text": MAIN["text"],
        "button_inactive_text": MAIN["text"],
        "row_line": MAIN["grid"],
        "inactive": MAIN["grid"],
        "label_text": MAIN["text"],
        "value_text": MAIN["signal_yellow"],
    },
    "color_light": {
        "name": "C LIGHT",
        "title": "Color Light",
        "mode_value": "color light",
        "bg": (255, 255, 255),
        "panel": (255, 255, 255),
        "panel_dark": (255, 255, 255),
        "grid": (0, 0, 0),
        "grid_soft": (0, 0, 0),
        "text": (0, 0, 0),
        "cyan": (0, 140, 255),
        "cyan_dark": (0, 92, 180),
        "teal": (0, 150, 135),
        "acquire_blue": (0, 115, 255),
        "lock_green": (0, 150, 0),
        "red": (220, 0, 0),
        "red_dark": (120, 0, 0),
        "signal_yellow": (190, 135, 0),
        "button_fill": (225, 250, 255),
        "button_active_fill": (225, 250, 255),
        "button_inactive_fill": (0, 0, 255),
        "button_outline": (0, 92, 180),
        "button_active_text": (0, 0, 0),
        "button_inactive_text": (255, 255, 255),
        "row_line": (0, 0, 0),
        "inactive": (150, 150, 150),
        "label_text": (0, 0, 0),
        "value_text": (0, 92, 180),
    },
    "mono_dark": {
        "name": "M DARK",
        "title": "Monochrome Dark",
        "mode_value": "mono dark",
        "bg": (0, 0, 0),
        "panel": (0, 0, 0),
        "panel_dark": (0, 0, 0),
        "grid": (255, 255, 255),
        "grid_soft": (255, 255, 255),
        "text": (255, 255, 255),
        "cyan": (255, 255, 255),
        "cyan_dark": (0, 0, 0),
        "teal": (255, 255, 255),
        "acquire_blue": (255, 255, 255),
        "lock_green": (255, 255, 255),
        "red": (255, 255, 255),
        "red_dark": (0, 0, 0),
        "signal_yellow": (255, 255, 255),
        "button_fill": (0, 0, 0),
        "button_active_fill": (0, 0, 0),
        "button_inactive_fill": (0, 0, 0),
        "button_outline": (255, 255, 255),
        "button_active_text": (255, 255, 255),
        "button_inactive_text": (255, 255, 255),
        "row_line": (255, 255, 255),
        "inactive": (255, 255, 255),
        "label_text": (255, 255, 255),
        "value_text": (255, 255, 255),
    },
    "mono_light": {
        "name": "M LIGHT",
        "title": "Monochrome Light",
        "mode_value": "mono light",
        "bg": (255, 255, 255),
        "panel": (255, 255, 255),
        "panel_dark": (255, 255, 255),
        "grid": (0, 0, 0),
        "grid_soft": (0, 0, 0),
        "text": (0, 0, 0),
        "cyan": (0, 0, 0),
        "cyan_dark": (255, 255, 255),
        "teal": (0, 0, 0),
        "acquire_blue": (0, 0, 0),
        "lock_green": (0, 0, 0),
        "red": (0, 0, 0),
        "red_dark": (255, 255, 255),
        "signal_yellow": (0, 0, 0),
        "button_fill": (255, 255, 255),
        "button_active_fill": (255, 255, 255),
        "button_inactive_fill": (255, 255, 255),
        "button_outline": (0, 0, 0),
        "button_active_text": (0, 0, 0),
        "button_inactive_text": (0, 0, 0),
        "row_line": (0, 0, 0),
        "inactive": (0, 0, 0),
        "label_text": (0, 0, 0),
        "value_text": (0, 0, 0),
    },
}

TOOLBAR_BUTTON_WIDTH = 44
TOOLBAR_BUTTON_HEIGHT = 28
APP_BUTTON_GAP = 2
SETTINGS_ROW_H = 32

try:
    FONT_1 = ImageFont.truetype("/System/Library/Fonts/Menlo.ttc", 10)
    FONT_SETTINGS = ImageFont.truetype("/System/Library/Fonts/Menlo.ttc", 10)
    FONT_2 = ImageFont.truetype("/System/Library/Fonts/Menlo.ttc", 20)
    FONT_3 = ImageFont.truetype("/System/Library/Fonts/Menlo.ttc", 30)
    FONT_4 = ImageFont.truetype("/System/Library/Fonts/Menlo.ttc", 40)
except OSError:
    FONT_1 = ImageFont.load_default()
    FONT_SETTINGS = ImageFont.load_default()
    FONT_2 = ImageFont.load_default()
    FONT_3 = ImageFont.load_default()
    FONT_4 = ImageFont.load_default()


def text_size(draw, text, font):
    bbox = draw.textbbox((0, 0), text, font=font)
    return bbox[2] - bbox[0], bbox[3] - bbox[1]


def draw_centered(draw, text, x, y, w, font, fill):
    text_w, _ = text_size(draw, text, font)
    draw.text((x + max(0, (w - text_w) // 2), y), text, font=font, fill=fill)


def draw_dotted_line(draw, start, end, fill, step=5, thickness=1):
    x1, y1 = start
    x2, y2 = end
    if y1 == y2:
        for x in range(min(x1, x2), max(x1, x2) + 1, step):
            draw.rectangle((x, y1, x + thickness - 1, y1 + thickness - 1), fill=fill)
    elif x1 == x2:
        for y in range(min(y1, y2), max(y1, y2) + 1, step):
            draw.rectangle((x1, y, x1 + thickness - 1, y + thickness - 1), fill=fill)


def draw_button(draw, x, y, w, label, p, active=False):
    width = 3 if active else 2
    fill = p["button_active_fill"] if active else p["button_inactive_fill"]
    text = p["button_active_text"] if active else p["button_inactive_text"]
    draw.rounded_rectangle(
        (x, y, x + w - 1, y + TOOLBAR_BUTTON_HEIGHT - 1),
        radius=4,
        fill=fill,
        outline=p["button_outline"],
        width=width,
    )
    draw_centered(draw, label, x, y + 4, w, FONT_2 if len(label) <= 2 else FONT_1, text)


def draw_nav(draw, width, p):
    y = 7
    settings_x = width - TOOLBAR_BUTTON_WIDTH - 4
    next_x = settings_x - TOOLBAR_BUTTON_WIDTH - APP_BUTTON_GAP
    prev_x = next_x - TOOLBAR_BUTTON_WIDTH - APP_BUTTON_GAP
    draw_button(draw, prev_x, y, TOOLBAR_BUTTON_WIDTH, "<", p)
    draw_button(draw, next_x, y, TOOLBAR_BUTTON_WIDTH, ">", p)
    draw_button(draw, settings_x, y, TOOLBAR_BUTTON_WIDTH, "*", p, True)


def draw_header(draw, width, p, title):
    draw.rectangle((0, 0, width, 42), fill=p["bg"])
    draw.line((0, 41, width, 41), fill=p["grid"])
    draw.text((8, 17), title, font=FONT_1, fill=p["text"])
    draw_nav(draw, width, p)


def draw_display_row(draw, y, p, mode_variant):
    draw.text((10, y + 4), "Display", font=FONT_SETTINGS, fill=p["label_text"])
    draw.text((10, y + 18), p["mode_value"], font=FONT_SETTINGS, fill=p["value_text"])
    if mode_variant == "cycle":
        draw_button(draw, 226, y + 2, 90, p["name"], p, True)
    else:
        labels = [
            ("MD", "mono_dark"),
            ("ML", "mono_light"),
            ("CD", "color_dark"),
            ("CL", "color_light"),
        ]
        x = 160
        for label, mode in labels:
            draw_button(draw, x, y + 2, 38, label, p, active=mode == p["key"])
            x += 40
    draw_dotted_line(draw, (0, y + SETTINGS_ROW_H - 2), (320, y + SETTINGS_ROW_H - 2), p["row_line"], 5, 2)


def draw_settings_picker(mode_key, mode_variant):
    p = {**PALETTES[mode_key], "key": mode_key}
    image = Image.new("RGB", (320, 240), p["bg"])
    draw = ImageDraw.Draw(image)
    draw_header(draw, 320, p, "Settings 2026-05-24")

    rows = [
        ("Volume", "1/10"),
        ("Rotation", "screen 1"),
    ]
    y = 46
    for label, value in rows:
        draw.text((10, y + 4), label, font=FONT_SETTINGS, fill=p["label_text"])
        draw.text((10, y + 18), value, font=FONT_SETTINGS, fill=p["value_text"])
        draw_dotted_line(draw, (0, y + SETTINGS_ROW_H - 2), (320, y + SETTINGS_ROW_H - 2), p["row_line"], 5, 2)
        y += SETTINGS_ROW_H

    draw_display_row(draw, y, p, mode_variant)
    y += SETTINGS_ROW_H

    draw.text((10, y + 4), "WiFi", font=FONT_SETTINGS, fill=p["label_text"])
    draw.text((10, y + 18), "setup later", font=FONT_SETTINGS, fill=p["value_text"])
    draw_dotted_line(draw, (0, y + SETTINGS_ROW_H - 2), (320, y + SETTINGS_ROW_H - 2), p["row_line"], 5, 2)

    scroll_y = 210
    draw.rectangle((0, scroll_y - 3, 320, 240), fill=p["bg"])
    draw.line((0, scroll_y - 4, 320, scroll_y - 4), fill=p["grid"])
    draw_button(draw, 0, scroll_y, 159, "^", p)
    draw_button(draw, 161, scroll_y, 159, "v", p)

    path = REVIEW_DIR / mode_variant / f"settings-display-{mode_key}.png"
    path.parent.mkdir(parents=True, exist_ok=True)
    image.save(path)
    print(path)


def draw_pulse_preview(mode_key, locked):
    p = {**PALETTES[mode_key], "key": mode_key}
    image = Image.new("RGB", (320, 240), p["bg"])
    draw = ImageDraw.Draw(image)
    draw_header(draw, 320, p, "PulseSensor.com")

    graph = (8, 48, 304, 112)
    x, y, w, h = graph
    draw.rounded_rectangle((x - 2, y - 2, x + w + 1, y + h + 1), radius=6, fill=p["bg"], outline=p["grid"], width=2)
    for gx in range(0, w + 1, 38):
        draw_dotted_line(draw, (x + gx, y), (x + gx, y + h), p["grid_soft"], 7)
    for gy in range(0, h + 1, 28):
        draw_dotted_line(draw, (x, y + gy), (x + w, y + gy), p["grid_soft"], 7)
    draw.text((x + 6, y + 5), "LIVE LINE", font=FONT_1, fill=p["text"])
    draw.text((x + w - 48, y + 5), "THR 550", font=FONT_1, fill=p["text"])
    trace = p["text"] if locked else p["acquire_blue"]
    points = [(x + i, y + h // 2 + [0, -3, 2, -2, 1, -4, 3][(i // 16) % 7]) for i in range(0, w + 1, 16)]
    draw.line(points, fill=trace, width=3 if locked else 2)
    status = "QUALIFIED BEAT" if locked else "GOOD WAVE"
    tw, th = text_size(draw, status, FONT_1)
    draw.rectangle((x + w - tw - 9, y + h - th - 8, x + w - 3, y + h - 2), fill=p["bg"])
    draw.text((x + w - tw - 6, y + h - th - 6), status, font=FONT_1, fill=p["text"])

    panel_y = 170
    for px, pw, label, value in [(8, 102, "BPM", "72" if locked else "--"), (118, 102, "IBI", "833" if locked else "--"), (228, 84, "SIG GPIO35", "")]:
        outline = p["teal"] if locked else p["signal_yellow"]
        draw.rounded_rectangle((px, panel_y, px + pw, panel_y + 62), radius=6, fill=p["panel"], outline=outline, width=2)
        draw.text((px + 8, panel_y + 6), label, font=FONT_1, fill=p["text"])
        if value:
            draw.text((px + 8, panel_y + 19), value, font=FONT_4 if pw > 90 else FONT_3, fill=p["text"])
        else:
            bar_color = p["lock_green"] if locked else p["signal_yellow"]
            for i in range(12):
                bx = px + 9 + i * 6
                if i < 7:
                    draw.rectangle((bx, panel_y + 30, bx + 3, panel_y + 44), fill=bar_color)
                elif not mode_key.startswith("mono"):
                    draw.rectangle((bx, panel_y + 30, bx + 3, panel_y + 44), fill=p["inactive"])

    path = REVIEW_DIR / "screen-preview" / mode_key / f"pulse-landscape-{'locked' if locked else 'searching'}.png"
    path.parent.mkdir(parents=True, exist_ok=True)
    image.save(path)
    print(path)


MODE_KEYS = ["mono_dark", "mono_light", "color_dark", "color_light"]


for variant in ["cycle", "separate"]:
    for key in MODE_KEYS:
        draw_settings_picker(key, variant)

for key in MODE_KEYS:
    draw_pulse_preview(key, False)
    draw_pulse_preview(key, True)


def draw_contact_sheet():
    margin = 16
    label_h = 24
    cell_w = 320
    cell_h = 240
    cols = 3
    rows = len(MODE_KEYS)
    sheet_w = margin * 2 + cols * cell_w + (cols - 1) * margin
    sheet_h = margin * 2 + rows * (label_h + cell_h) + (rows - 1) * margin
    sheet = Image.new("RGB", (sheet_w, sheet_h), (24, 24, 24))
    draw = ImageDraw.Draw(sheet)
    headings = ["Settings", "Searching", "Locked"]
    for col, heading in enumerate(headings):
        x = margin + col * (cell_w + margin)
        draw.text((x, 2), heading, font=FONT_2, fill=(255, 255, 255))

    for row, key in enumerate(MODE_KEYS):
        p = PALETTES[key]
        y = margin + row * (label_h + cell_h + margin)
        draw.text((margin, y), p["title"], font=FONT_2, fill=(255, 255, 255))
        paths = [
            REVIEW_DIR / "cycle" / f"settings-display-{key}.png",
            REVIEW_DIR / "screen-preview" / key / "pulse-landscape-searching.png",
            REVIEW_DIR / "screen-preview" / key / "pulse-landscape-locked.png",
        ]
        for col, path in enumerate(paths):
            x = margin + col * (cell_w + margin)
            panel = Image.open(path).convert("RGB")
            sheet.paste(panel, (x, y + label_h))

    path = REVIEW_DIR / "full-panel-all-modes.png"
    sheet.save(path)
    print(path)


draw_contact_sheet()
