from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

OUT_DIR = Path("docs/screenshots/settings-render")
OUT_DIR.mkdir(parents=True, exist_ok=True)

COLOR_BG = (0, 0, 0)
COLOR_PANEL = (8, 8, 8)
COLOR_GRID = (58, 60, 58)
COLOR_GRID_SOFT = (41, 40, 41)
COLOR_TEXT = (255, 255, 255)
COLOR_CYAN = (0, 255, 255)
COLOR_CYAN_DARK = (0, 136, 144)
COLOR_RED = (255, 0, 0)
COLOR_SIGNAL_YELLOW = (255, 255, 144)

TOOLBAR_BUTTON_WIDTH = 44
TOOLBAR_BUTTON_HEIGHT = 28
APP_BUTTON_GAP = 2
SETTINGS_ROW_H = 40
SETTINGS_ROW_COUNT = 9

try:
    FONT_1 = ImageFont.truetype("/System/Library/Fonts/Menlo.ttc", 10)
    FONT_2 = ImageFont.truetype("/System/Library/Fonts/Menlo.ttc", 20)
except OSError:
    FONT_1 = ImageFont.load_default()
    FONT_2 = ImageFont.load_default()


def draw_centered(draw, text, x, y, w, font, fill):
    bbox = draw.textbbox((0, 0), text, font=font)
    text_w = bbox[2] - bbox[0]
    draw.text((x + max(0, (w - text_w) // 2), y), text, font=font, fill=fill)


def draw_button(draw, x, y, w, label, active=False):
    fill = COLOR_CYAN_DARK if active else COLOR_PANEL
    outline = COLOR_CYAN if active else COLOR_GRID
    draw.rounded_rectangle((x, y, x + w - 1, y + TOOLBAR_BUTTON_HEIGHT - 1), radius=4, fill=fill, outline=outline)
    draw_centered(draw, label, x, y + 4, w, FONT_2, COLOR_TEXT)


def draw_nav(draw, width, y, active_settings=True):
    rotate_x = width - TOOLBAR_BUTTON_WIDTH - 4
    settings_x = rotate_x - TOOLBAR_BUTTON_WIDTH - APP_BUTTON_GAP
    next_x = settings_x - TOOLBAR_BUTTON_WIDTH - APP_BUTTON_GAP
    prev_x = next_x - TOOLBAR_BUTTON_WIDTH - APP_BUTTON_GAP
    draw_button(draw, prev_x, y, TOOLBAR_BUTTON_WIDTH, "<")
    draw_button(draw, next_x, y, TOOLBAR_BUTTON_WIDTH, ">")
    draw_button(draw, settings_x, y, TOOLBAR_BUTTON_WIDTH, "*", active_settings)
    draw_button(draw, rotate_x, y, TOOLBAR_BUTTON_WIDTH, "R")


def row_screen_y(header_h, scroll_y, row_index):
    return header_h + 4 + row_index * SETTINGS_ROW_H - scroll_y


def draw_row(draw, width, y, label, value):
    index = getattr(draw_row, "index", 0)
    bg = COLOR_SIGNAL_YELLOW if index % 2 == 0 else (0, 255, 0)
    draw.rectangle((0, y, width, y + SETTINGS_ROW_H), fill=bg)
    draw.line((0, y + SETTINGS_ROW_H - 1, width, y + SETTINGS_ROW_H - 1), fill=COLOR_BG)
    draw.text((10, y + 0), label, font=FONT_2, fill=COLOR_BG)
    draw.text((10, y + 21), value, font=FONT_2, fill=COLOR_BG)


def render(width, height, scroll_y, filename):
    portrait = height > width
    header_h = 74 if portrait else 42
    nav_y = 4 if portrait else 7
    scroll_button_y = height - TOOLBAR_BUTTON_HEIGHT - 2
    content_top = header_h + 4
    content_bottom = scroll_button_y - 3
    viewport_h = max(1, content_bottom - content_top)
    max_scroll = max(0, SETTINGS_ROW_COUNT * SETTINGS_ROW_H - viewport_h)
    scroll_y = min(max(scroll_y, 0), max_scroll)

    image = Image.new("RGB", (width, height), COLOR_BG)
    draw = ImageDraw.Draw(image)

    draw.rectangle((0, 0, width, header_h), fill=COLOR_BG)
    draw.line((0, header_h - 1, width, header_h - 1), fill=COLOR_GRID)
    draw.text((10, 38 if portrait else 8), "Settings 2026-05-24", font=FONT_1, fill=COLOR_TEXT)
    draw_nav(draw, width, nav_y)

    settings_vol_minus_x = width - (TOOLBAR_BUTTON_WIDTH * 2) - APP_BUTTON_GAP - 4
    settings_vol_plus_x = settings_vol_minus_x + TOOLBAR_BUTTON_WIDTH + APP_BUTTON_GAP
    settings_rotate_x = width - 90
    settings_led_x = settings_rotate_x
    swatch_red_x = width - 118
    swatch_yellow_x = swatch_red_x + 40
    swatch_cyan_x = swatch_yellow_x + 40

    rows = [
        ("Volume", "1/10"),
        ("Rotation", "rot 1"),
        ("WiFi", "setup later"),
        ("Bluetooth", "setup later"),
        ("LED Control", "beat pulse"),
        ("Color", "tap"),
        ("About", "PulseSensor CYD"),
        ("Version", "0.3.0-app-shell"),
        ("Firmware", "2026-05-24"),
    ]

    for index, (label, value) in enumerate(rows):
        y = row_screen_y(header_h, scroll_y, index)
        if y < content_top or y + SETTINGS_ROW_H > content_bottom:
            continue
        draw_row.index = index
        draw_row(draw, width, y, label, value)
        button_y = y + 8
        if index == 0:
            draw_button(draw, settings_vol_minus_x, button_y, TOOLBAR_BUTTON_WIDTH, "-")
            draw_button(draw, settings_vol_plus_x, button_y, TOOLBAR_BUTTON_WIDTH, "+")
        elif index == 1:
            draw_button(draw, settings_rotate_x, button_y, 86, "ROT")
        elif index == 4:
            draw_button(draw, settings_led_x, button_y, 86, "BEAT", True)
        elif index == 5:
            for x, color, active in [
                (swatch_red_x, COLOR_RED, True),
                (swatch_yellow_x, COLOR_SIGNAL_YELLOW, False),
                (swatch_cyan_x, COLOR_CYAN, False),
            ]:
                outline = COLOR_TEXT if active else COLOR_GRID
                draw.rounded_rectangle((x, button_y, x + 33, button_y + TOOLBAR_BUTTON_HEIGHT - 1), radius=4, fill=color, outline=outline)

    draw.rectangle((0, scroll_button_y - 3, width, height), fill=COLOR_BG)
    draw.line((0, scroll_button_y - 4, width, scroll_button_y - 4), fill=COLOR_GRID)
    scroll_button_w = (width - APP_BUTTON_GAP) // 2
    up_x = 0
    down_x = scroll_button_w + APP_BUTTON_GAP
    draw_button(draw, up_x, scroll_button_y, scroll_button_w, "^", scroll_y > 0)
    draw_button(draw, down_x, scroll_button_y, scroll_button_w, "v", scroll_y < max_scroll)

    path = OUT_DIR / filename
    image.save(path)
    print(path)


render(320, 240, 0, "settings-landscape-top.png")
render(320, 240, 120, "settings-landscape-middle.png")
render(320, 240, 199, "settings-landscape-bottom.png")
render(240, 320, 0, "settings-portrait-top.png")
render(240, 320, 80, "settings-portrait-middle.png")
render(240, 320, 151, "settings-portrait-bottom.png")
