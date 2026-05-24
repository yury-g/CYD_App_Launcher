from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

OUT_DIR = Path("docs/screenshots/pulse-render")
OUT_DIR.mkdir(parents=True, exist_ok=True)

COLOR_BG = (0, 0, 0)
COLOR_PANEL = (8, 8, 8)
COLOR_PANEL_DARK = (0, 8, 0)
COLOR_GRID = (58, 60, 58)
COLOR_GRID_SOFT = (41, 40, 41)
COLOR_TEXT = (255, 255, 255)
COLOR_CYAN = (0, 255, 255)
COLOR_CYAN_DARK = (0, 136, 144)
COLOR_TEAL = (0, 188, 156)
COLOR_SIGNAL_YELLOW = (255, 255, 144)
COLOR_LOCK_GREEN = (0, 255, 0)
COLOR_RED = (255, 0, 0)
COLOR_RED_DARK = (96, 0, 0)

TOOLBAR_BUTTON_WIDTH = 44
TOOLBAR_BUTTON_HEIGHT = 28
APP_BUTTON_GAP = 2

try:
    FONT_1 = ImageFont.truetype("/System/Library/Fonts/Menlo.ttc", 10)
    FONT_2 = ImageFont.truetype("/System/Library/Fonts/Menlo.ttc", 20)
    FONT_3 = ImageFont.truetype("/System/Library/Fonts/Menlo.ttc", 30)
    FONT_4 = ImageFont.truetype("/System/Library/Fonts/Menlo.ttc", 40)
except OSError:
    FONT_1 = ImageFont.load_default()
    FONT_2 = ImageFont.load_default()
    FONT_3 = ImageFont.load_default()
    FONT_4 = ImageFont.load_default()


def draw_centered(draw, text, x, y, w, font, fill):
    bbox = draw.textbbox((0, 0), text, font=font)
    text_w = bbox[2] - bbox[0]
    draw.text((x + max(0, (w - text_w) // 2), y), text, font=font, fill=fill)


def draw_button(draw, x, y, w, label, active=False):
    fill = COLOR_CYAN_DARK if active else COLOR_PANEL
    outline = COLOR_CYAN if active else COLOR_GRID
    draw.rounded_rectangle((x, y, x + w - 1, y + TOOLBAR_BUTTON_HEIGHT - 1), radius=4, fill=fill, outline=outline)
    draw_centered(draw, label, x, y + 4, w, FONT_2, COLOR_TEXT)


def draw_nav(draw, width, y):
    rotate_x = width - TOOLBAR_BUTTON_WIDTH - 4
    settings_x = rotate_x - TOOLBAR_BUTTON_WIDTH - APP_BUTTON_GAP
    next_x = settings_x - TOOLBAR_BUTTON_WIDTH - APP_BUTTON_GAP
    prev_x = next_x - TOOLBAR_BUTTON_WIDTH - APP_BUTTON_GAP
    draw_button(draw, prev_x, y, TOOLBAR_BUTTON_WIDTH, "<")
    draw_button(draw, next_x, y, TOOLBAR_BUTTON_WIDTH, ">")
    draw_button(draw, settings_x, y, TOOLBAR_BUTTON_WIDTH, "*")
    draw_button(draw, rotate_x, y, TOOLBAR_BUTTON_WIDTH, "R")


def draw_heart(draw, cx, cy, size=13):
    draw.ellipse((cx - size, cy - size, cx, cy), fill=COLOR_CYAN)
    draw.ellipse((cx, cy - size, cx + size, cy), fill=COLOR_CYAN)
    draw.polygon([(cx - size, cy - 2), (cx + size, cy - 2), (cx, cy + size + 6)], fill=COLOR_CYAN)
    inner = max(5, size - 3)
    draw.ellipse((cx - inner, cy - inner, cx, cy), fill=COLOR_RED_DARK)
    draw.ellipse((cx, cy - inner, cx + inner, cy), fill=COLOR_RED_DARK)
    draw.polygon([(cx - inner, cy - 1), (cx + inner, cy - 1), (cx, cy + inner + 4)], fill=COLOR_RED_DARK)


def draw_graph(draw, x, y, w, h, locked=False):
    draw.rounded_rectangle((x - 2, y - 2, x + w + 1, y + h + 1), radius=6, fill=COLOR_PANEL_DARK, outline=COLOR_GRID)
    draw.rectangle((x, y, x + w, y + h), fill=COLOR_BG)
    for gx in range(0, w + 1, 38):
        draw.line((x + gx, y, x + gx, y + h), fill=COLOR_GRID_SOFT)
    for gy in range(0, h + 1, 28):
        draw.line((x, y + gy, x + w, y + gy), fill=COLOR_GRID_SOFT)
    draw.text((x + 6, y + 5), "LIVE LINE", font=FONT_1, fill=COLOR_TEXT)
    draw.text((x + w - 48, y + 5), "THR 550", font=FONT_1, fill=COLOR_TEXT)
    threshold_y = y + 36
    for px in range(x, x + w, 6):
        draw.point((px, threshold_y), fill=COLOR_CYAN)
    points = []
    for i in range(0, w + 1, 16):
        offset = [0, -3, 2, -2, 1, -4, 3][(i // 16) % 7]
        points.append((x + i, y + h // 2 + offset))
    draw.line(points, fill=COLOR_TEXT if locked else COLOR_CYAN, width=2)


def draw_metric_panel(draw, x, y, w, h, label, value, bg, valid=False, unit=""):
    outline = COLOR_TEAL if valid else COLOR_GRID
    draw.rounded_rectangle((x, y, x + w, y + h), radius=6, fill=bg, outline=outline)
    text = COLOR_BG
    draw.text((x + 8, y + 6), label, font=FONT_1, fill=text)
    value_font = FONT_4 if w > 90 else FONT_3
    draw.text((x + 8, y + 19), value, font=value_font, fill=text)
    if unit:
        draw.text((x + w - 30, y + h - 18), unit, font=FONT_1, fill=text)


def draw_signal_panel(draw, x, y, w, h, locked=False):
    bg = COLOR_LOCK_GREEN if locked else COLOR_SIGNAL_YELLOW
    draw.rounded_rectangle((x, y, x + w, y + h), radius=6, fill=bg, outline=COLOR_BG)
    draw.text((x + 8, y + 6), "SIG GPIO35", font=FONT_1, fill=COLOR_BG)
    for i in range(12):
        color = COLOR_BG if i < 6 else COLOR_GRID
        draw.rectangle((x + 9 + i * 6, y + 28, x + 12 + i * 6, y + 42), fill=color)


def render(width, height, locked, filename):
    portrait = height > width
    image = Image.new("RGB", (width, height), COLOR_BG)
    draw = ImageDraw.Draw(image)

    if portrait:
        header_h = 74
        graph = (8, 82, width - 16, 146)
        panels = [(8, 240, 68, 72), (84, 240, 68, 72), (160, 240, 72, 72)]
        nav_y = 4
        heart = (24, 52)
        title_xy = (52, 38)
        coach_xy = (62, 58)
    else:
        header_h = 42
        graph = (8, 48, width - 16, 112)
        panels = [(8, 170, 102, 62), (118, 170, 102, 62), (228, 170, 84, 62)]
        nav_y = 7
        heart = (112, 22)
        title_xy = (10, 8)
        coach_xy = (10, 25)

    draw.rectangle((0, 0, width, header_h), fill=COLOR_BG)
    draw.line((0, header_h - 1, width, header_h - 1), fill=COLOR_GRID)
    draw.text(title_xy, "PulseSensor.com", font=FONT_1, fill=COLOR_TEXT)
    draw.text(coach_xy, "QUALIFIED BEAT" if locked else "GOOD WAVE", font=FONT_1, fill=COLOR_SIGNAL_YELLOW if not locked else COLOR_LOCK_GREEN)
    draw_heart(draw, *heart)
    draw_nav(draw, width, nav_y)

    draw_graph(draw, *graph, locked=locked)
    draw_metric_panel(draw, *panels[0], "BPM", "72" if locked else "--", COLOR_LOCK_GREEN if locked else COLOR_SIGNAL_YELLOW, locked)
    draw_metric_panel(draw, *panels[1], "IBI", "833" if locked else "--", COLOR_SIGNAL_YELLOW if locked else COLOR_LOCK_GREEN, locked, "ms" if locked else "")
    draw_signal_panel(draw, *panels[2], locked=locked)

    path = OUT_DIR / filename
    image.save(path)
    print(path)


render(320, 240, False, "pulse-landscape-searching.png")
render(320, 240, True, "pulse-landscape-locked.png")
render(240, 320, False, "pulse-portrait-searching.png")
render(240, 320, True, "pulse-portrait-locked.png")
