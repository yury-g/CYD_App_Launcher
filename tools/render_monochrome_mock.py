from pathlib import Path
from PIL import Image, ImageDraw, ImageFont, ImageOps

OUT_DIR = Path("docs/screenshots/monochrome-render")
OUT_DIR.mkdir(parents=True, exist_ok=True)

BLACK = (0, 0, 0)
WHITE = (255, 255, 255)

TOOLBAR_BUTTON_WIDTH = 44
TOOLBAR_BUTTON_HEIGHT = 28
APP_BUTTON_GAP = 2
SETTINGS_ROW_H = 32
SETTINGS_ROW_COUNT = 11

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


def draw_dotted_line(draw, start, end, fill=WHITE, step=4, thickness=1):
    x1, y1 = start
    x2, y2 = end
    if y1 == y2:
        for x in range(min(x1, x2), max(x1, x2) + 1, step):
            draw.rectangle((x, y1, x + thickness - 1, y1 + thickness - 1), fill=fill)
    elif x1 == x2:
        for y in range(min(y1, y2), max(y1, y2) + 1, step):
            draw.rectangle((x1, y, x1 + thickness - 1, y + thickness - 1), fill=fill)


def draw_dotted_rect(draw, box, fill=WHITE, step=4):
    x1, y1, x2, y2 = box
    draw_dotted_line(draw, (x1, y1), (x2, y1), fill, step)
    draw_dotted_line(draw, (x1, y2), (x2, y2), fill, step)
    draw_dotted_line(draw, (x1, y1), (x1, y2), fill, step)
    draw_dotted_line(draw, (x2, y1), (x2, y2), fill, step)


ROTATE_ICON = "__ROTATE_ICON__"


def draw_rotate_icon(draw, x, y, w, h, fill=WHITE):
    cx = x + w // 2
    cy = y + h // 2
    draw.arc((cx - 9, cy - 8, cx + 9, cy + 8), 35, 325, fill=fill, width=2)
    draw.line((cx + 6, cy - 8, cx + 11, cy - 8), fill=fill, width=2)
    draw.line((cx + 11, cy - 8, cx + 11, cy - 3), fill=fill, width=2)
    draw.polygon([(cx + 8, cy - 11), (cx + 15, cy - 8), (cx + 11, cy - 2)], fill=fill)


def draw_button(draw, x, y, w, label, active=False, base=BLACK):
    fill = BLACK
    ink = WHITE
    outline = WHITE
    border_w = 3 if active else 2
    draw.rounded_rectangle(
        (x, y, x + w - 1, y + TOOLBAR_BUTTON_HEIGHT - 1),
        radius=4,
        fill=fill,
        outline=outline,
        width=border_w,
    )
    if label == ROTATE_ICON:
        draw_rotate_icon(draw, x, y, w, TOOLBAR_BUTTON_HEIGHT, ink)
    else:
        draw_centered(draw, label, x, y + 4, w, FONT_2, ink)


def draw_nav(draw, width, y, active_settings=False):
    settings_x = width - TOOLBAR_BUTTON_WIDTH - 4
    next_x = settings_x - TOOLBAR_BUTTON_WIDTH - APP_BUTTON_GAP
    prev_x = next_x - TOOLBAR_BUTTON_WIDTH - APP_BUTTON_GAP
    draw_button(draw, prev_x, y, TOOLBAR_BUTTON_WIDTH, "<")
    draw_button(draw, next_x, y, TOOLBAR_BUTTON_WIDTH, ">")
    draw_button(draw, settings_x, y, TOOLBAR_BUTTON_WIDTH, "*", active_settings)


def draw_header(draw, width, height, title, subtitle="", active_settings=False):
    portrait = height > width
    header_h = 74 if portrait else 42
    nav_y = 4 if portrait else 7
    draw.rectangle((0, 0, width, header_h), fill=BLACK)
    draw.line((0, header_h - 1, width, header_h - 1), fill=WHITE, width=1)
    draw.text((8 if not portrait else 52, 17 if not portrait else 38), title, font=FONT_1, fill=WHITE)
    if subtitle:
        draw.text((10 if not portrait else 62, 25 if not portrait else 58), subtitle, font=FONT_1, fill=WHITE)
    draw_nav(draw, width, nav_y, active_settings)
    return header_h


def draw_heart(draw, cx, cy, size=13, filled=True, compact=False):
    if compact:
        w = int(size * 1.35)
        h = max(7, int(size * 0.78))
        if filled:
            draw.ellipse((cx - w, cy - h, cx - 1, cy + h - 1), fill=WHITE)
            draw.ellipse((cx + 1, cy - h, cx + w, cy + h - 1), fill=WHITE)
            draw.polygon([(cx - w, cy + 1), (cx + w, cy + 1), (cx, cy + h + 7)], fill=WHITE)
        else:
            draw.ellipse((cx - w, cy - h, cx - 1, cy + h - 1), outline=WHITE, width=2)
            draw.ellipse((cx + 1, cy - h, cx + w, cy + h - 1), outline=WHITE, width=2)
            draw.line([(cx - w, cy + 1), (cx, cy + h + 7), (cx + w, cy + 1)], fill=WHITE, width=2)
        return

    if filled:
        draw.ellipse((cx - size, cy - size, cx, cy), fill=WHITE)
        draw.ellipse((cx, cy - size, cx + size, cy), fill=WHITE)
        draw.polygon([(cx - size, cy - 2), (cx + size, cy - 2), (cx, cy + size + 6)], fill=WHITE)
    else:
        draw.ellipse((cx - size, cy - size, cx, cy), outline=WHITE, width=2)
        draw.ellipse((cx, cy - size, cx + size, cy), outline=WHITE, width=2)
        draw.line([(cx - size, cy - 2), (cx, cy + size + 6), (cx + size, cy - 2)], fill=WHITE, width=2)


def draw_graph(draw, x, y, w, h, locked=False, status_text=""):
    draw.rounded_rectangle((x - 2, y - 2, x + w + 1, y + h + 1), radius=6, fill=BLACK, outline=WHITE, width=2)
    draw.rectangle((x, y, x + w, y + h), fill=BLACK)
    for gx in range(0, w + 1, 38):
        draw_dotted_line(draw, (x + gx, y), (x + gx, y + h), WHITE, 7)
    for gy in range(0, h + 1, 28):
        draw_dotted_line(draw, (x, y + gy), (x + w, y + gy), WHITE, 7)
    draw.text((x + 6, y + 5), "LIVE LINE", font=FONT_1, fill=WHITE)
    draw.text((x + w - 48, y + 5), "THR 550", font=FONT_1, fill=WHITE)
    threshold_y = y + 36
    draw_dotted_line(draw, (x, threshold_y), (x + w, threshold_y), WHITE, 5)
    points = []
    for i in range(0, w + 1, 16):
        offset = [0, -3, 2, -2, 1, -4, 3][(i // 16) % 7]
        points.append((x + i, y + h // 2 + offset))
    if locked:
        draw.line(points, fill=WHITE, width=3)
    else:
        for a, b in zip(points, points[1:]):
            draw_dotted_line(draw, a, b, WHITE, 5)
    if status_text:
        text_w, text_h = text_size(draw, status_text, FONT_1)
        text_x = x + w - text_w - 6
        text_y = y + h - text_h - 6
        draw.rectangle((text_x - 3, text_y - 2, text_x + text_w + 3, text_y + text_h + 2), fill=BLACK)
        draw.text((text_x, text_y), status_text, font=FONT_1, fill=WHITE)


def draw_metric_panel(draw, x, y, w, h, label, value, valid=False, unit=""):
    draw.rounded_rectangle((x, y, x + w, y + h), radius=6, fill=BLACK, outline=WHITE, width=2)
    if not valid:
        draw_dotted_rect(draw, (x + 4, y + 4, x + w - 4, y + h - 4), WHITE, 5)
    draw.text((x + 8, y + 6), label, font=FONT_1, fill=WHITE)
    value_font = FONT_4 if w > 90 else FONT_3
    draw.text((x + 8, y + 19), value, font=value_font, fill=WHITE)
    if unit:
        draw.text((x + w - 30, y + h - 18), unit, font=FONT_1, fill=WHITE)


def draw_signal_panel(draw, x, y, w, h, locked=False):
    draw.rounded_rectangle((x, y, x + w, y + h), radius=6, fill=BLACK, outline=WHITE, width=2)
    ink = WHITE
    if not locked:
        draw_dotted_rect(draw, (x + 4, y + 4, x + w - 4, y + h - 4), WHITE, 5)
    draw.text((x + 8, y + 6), "SIG GPIO35", font=FONT_1, fill=ink)
    segment_w = 3 if w < 80 else 4
    segment_gap = 2
    segment_h = 10 if w < 80 else 14
    for i in range(12):
        bx = x + 9 + i * (segment_w + segment_gap)
        if i < 7:
            draw.rectangle((bx, y + 30, bx + segment_w - 1, y + 30 + segment_h), fill=ink)


def render_pulse(width, height, locked, filename):
    portrait = height > width
    image = Image.new("RGB", (width, height), BLACK)
    draw = ImageDraw.Draw(image)

    if portrait:
        graph = (8, 82, width - 16, 146)
        panels = [(8, 240, 68, 72), (84, 240, 68, 72), (160, 240, 72, 72)]
        heart = (120, 48)
        heart_size = 13
    else:
        graph = (8, 48, width - 16, 112)
        panels = [(8, 170, 102, 62), (118, 170, 102, 62), (228, 170, 84, 62)]
        logo_x = 8
        logo_w, _ = text_size(draw, "PulseSensor.com", FONT_1)
        nav_left = width - (TOOLBAR_BUTTON_WIDTH * 3) - (APP_BUTTON_GAP * 2) - 4
        heart = ((logo_x + logo_w + nav_left) // 2, 20)
        heart_size = 12

    draw_header(draw, width, height, "PulseSensor.com")
    draw_heart(draw, *heart, size=heart_size, filled=locked, compact=True)
    draw_graph(draw, *graph, locked=locked, status_text="QUALIFIED BEAT" if locked else "GOOD WAVE")
    draw_metric_panel(draw, *panels[0], "BPM", "72" if locked else "--", locked)
    draw_metric_panel(draw, *panels[1], "IBI", "833" if locked else "--", locked, "ms" if locked else "")
    draw_signal_panel(draw, *panels[2], locked=locked)
    save_bw(image, filename)


def row_screen_y(header_h, scroll_y, row_index):
    return header_h + 4 + row_index * SETTINGS_ROW_H - scroll_y


def draw_row(draw, width, y, index, label, value):
    bg = BLACK
    ink = WHITE
    draw.rectangle((0, y, width, y + SETTINGS_ROW_H), fill=bg)
    draw_dotted_line(draw, (0, y + SETTINGS_ROW_H - 2), (width, y + SETTINGS_ROW_H - 2), WHITE, 5, 2)
    draw.text((10, y + 4), label, font=FONT_SETTINGS, fill=ink)
    draw.text((10, y + 18), value, font=FONT_SETTINGS, fill=ink)
    return bg


def draw_swatch(draw, x, y, label, active=False, base=WHITE):
    fill = BLACK if base == WHITE else WHITE
    ink = WHITE if base == WHITE else BLACK
    draw.rounded_rectangle((x, y, x + 33, y + TOOLBAR_BUTTON_HEIGHT - 1), radius=4, fill=fill, outline=ink, width=2)
    if active:
        draw.line((x + 6, y + 8, x + 15, y + 20), fill=ink, width=2)
        draw.line((x + 15, y + 20, x + 28, y + 6), fill=ink, width=2)
    else:
        draw_centered(draw, label, x, y + 8, 34, FONT_1, ink)


def draw_mode_button_group(draw, width, y, mode_variant):
    if mode_variant == "cycle":
        draw_button(draw, width - 94, y, 90, "MONO", active=True)
        return

    labels = ["C", "M", "D", "L"]
    button_w = 36
    x = width - (button_w * 4) - (APP_BUTTON_GAP * 3) - 4
    for label in labels:
        draw_button(draw, x, y, button_w, label, active=label == "M")
        x += button_w + APP_BUTTON_GAP


def render_settings(width, height, scroll_y, filename, mode_variant):
    portrait = height > width
    header_h = 74 if portrait else 42
    scroll_button_y = height - TOOLBAR_BUTTON_HEIGHT - 2
    content_top = header_h + 4
    content_bottom = scroll_button_y - 3
    viewport_h = max(1, content_bottom - content_top)
    max_scroll = max(0, SETTINGS_ROW_COUNT * SETTINGS_ROW_H - viewport_h)
    scroll_y = min(max(scroll_y, 0), max_scroll)

    image = Image.new("RGB", (width, height), BLACK)
    draw = ImageDraw.Draw(image)
    draw_header(draw, width, height, "Settings 2026-05-24", active_settings=True)

    settings_vol_minus_x = width - (TOOLBAR_BUTTON_WIDTH * 2) - APP_BUTTON_GAP - 4
    settings_vol_plus_x = settings_vol_minus_x + TOOLBAR_BUTTON_WIDTH + APP_BUTTON_GAP
    settings_rotate_x = width - 90
    settings_led_x = settings_rotate_x
    swatch_red_x = width - 118
    swatch_yellow_x = swatch_red_x + 40
    swatch_cyan_x = swatch_yellow_x + 40

    rows = [
        ("Volume", "1/10"),
        ("Rotation", "screen 1"),
        ("Display", "monochrome" if mode_variant == "cycle" else "C M D L"),
        ("WiFi", "setup later"),
        ("Bluetooth", "setup later"),
        ("LED Control", "beat pulse"),
        ("Color", "tap"),
        ("About", "PulseSensor CYD"),
        ("Version", "0.4.10-perf-safe-pin-scanner"),
        ("Firmware", "2026-05-24"),
        ("Memory", "used 90K free 238K 72%"),
    ]

    for index, (label, value) in enumerate(rows):
        y = row_screen_y(header_h, scroll_y, index)
        if y < content_top or y + SETTINGS_ROW_H > content_bottom:
            continue
        bg = draw_row(draw, width, y, index, label, value)
        button_y = y + 2
        if index == 0:
            draw_button(draw, settings_vol_minus_x, button_y, TOOLBAR_BUTTON_WIDTH, "-", base=bg)
            draw_button(draw, settings_vol_plus_x, button_y, TOOLBAR_BUTTON_WIDTH, "+", base=bg)
        elif index == 1:
            draw_button(draw, settings_rotate_x, button_y, 86, ROTATE_ICON, base=bg)
        elif index == 2:
            draw_mode_button_group(draw, width, button_y, mode_variant)
        elif index == 5:
            draw_button(draw, settings_led_x, button_y, 86, "BEAT", True, bg)
        elif index == 6:
            draw_swatch(draw, swatch_red_x, button_y, "1", True, bg)
            draw_swatch(draw, swatch_yellow_x, button_y, "2", False, bg)
            draw_swatch(draw, swatch_cyan_x, button_y, "3", False, bg)

    draw.rectangle((0, scroll_button_y - 3, width, height), fill=BLACK)
    draw.line((0, scroll_button_y - 4, width, scroll_button_y - 4), fill=WHITE)
    scroll_button_w = (width - APP_BUTTON_GAP) // 2
    draw_button(draw, 0, scroll_button_y, scroll_button_w, "^", scroll_y > 0)
    draw_button(draw, scroll_button_w + APP_BUTTON_GAP, scroll_button_y, scroll_button_w, "v", scroll_y < max_scroll)
    save_bw(image, filename)


def render_placeholder(width, height, app_index, filename):
    image = Image.new("RGB", (width, height), BLACK)
    draw = ImageDraw.Draw(image)
    title = f"App {app_index}"
    message = "your app here" if app_index == 2 else "your app here too"
    draw_header(draw, width, height, title)
    text_w, text_h = text_size(draw, message, FONT_2)
    x = max(4, (width - text_w) // 2)
    y = max((74 if height > width else 42) + 8, (height - text_h) // 2)
    draw_dotted_rect(draw, (x - 8, y - 8, x + text_w + 8, y + text_h + 8), WHITE, 5)
    draw.text((x, y), message, font=FONT_2, fill=WHITE)
    save_bw(image, filename)


def save_bw(image, filename):
    image = image.convert("1", dither=Image.Dither.NONE).convert("RGB")
    colors = image.getcolors(maxcolors=256)
    bad = [color for _, color in colors if color not in (BLACK, WHITE)]
    if bad:
        raise SystemExit(f"{filename} contains non-black/white pixels: {bad[:5]}")
    path = OUT_DIR / filename
    path.parent.mkdir(parents=True, exist_ok=True)
    image.save(path)
    print(path)

    inverse_path = OUT_DIR / "inverse" / filename
    inverse_path.parent.mkdir(parents=True, exist_ok=True)
    ImageOps.invert(image).save(inverse_path)
    print(inverse_path)


render_pulse(320, 240, False, "pulse-landscape-searching.png")
render_pulse(320, 240, True, "pulse-landscape-locked.png")
render_pulse(240, 320, False, "pulse-portrait-searching.png")
render_pulse(240, 320, True, "pulse-portrait-locked.png")
for mode_variant in ["cycle", "separate"]:
    render_settings(320, 240, 0, f"{mode_variant}/settings-landscape-top.png", mode_variant)
    render_settings(320, 240, 96, f"{mode_variant}/settings-landscape-middle.png", mode_variant)
    render_settings(320, 240, 191, f"{mode_variant}/settings-landscape-bottom.png", mode_variant)
    render_settings(240, 320, 0, f"{mode_variant}/settings-portrait-top.png", mode_variant)
    render_settings(240, 320, 64, f"{mode_variant}/settings-portrait-middle.png", mode_variant)
    render_settings(240, 320, 143, f"{mode_variant}/settings-portrait-bottom.png", mode_variant)
render_placeholder(320, 240, 2, "app2-landscape.png")
render_placeholder(320, 240, 3, "app3-landscape.png")
render_placeholder(240, 320, 2, "app2-portrait.png")
render_placeholder(240, 320, 3, "app3-portrait.png")
