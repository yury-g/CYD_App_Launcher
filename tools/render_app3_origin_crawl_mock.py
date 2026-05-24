from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

OUT_DIR = Path("docs/screenshots/app3-origin-crawl-render")
OUT_DIR.mkdir(parents=True, exist_ok=True)

BLACK = (0, 0, 0)
WHITE = (255, 255, 255)
BLUE = (0, 0, 255)
CYAN = (0, 255, 255)
GOLD = (255, 214, 72)
GOLD_DIM = (176, 128, 28)
GRID = (58, 60, 58)

TOOLBAR_BUTTON_WIDTH = 44
TOOLBAR_BUTTON_HEIGHT = 28
APP_BUTTON_GAP = 2

try:
    FONT_1 = ImageFont.truetype("/System/Library/Fonts/Menlo.ttc", 10)
    FONT_2 = ImageFont.truetype("/System/Library/Fonts/Menlo.ttc", 20)
    FONT_3 = ImageFont.truetype("/System/Library/Fonts/Menlo.ttc", 30)
    FONT_CRAWL = ImageFont.truetype("/System/Library/Fonts/Menlo.ttc", 14)
    FONT_CRAWL_SMALL = ImageFont.truetype("/System/Library/Fonts/Menlo.ttc", 11)
except OSError:
    FONT_1 = ImageFont.load_default()
    FONT_2 = ImageFont.load_default()
    FONT_3 = ImageFont.load_default()
    FONT_CRAWL = ImageFont.load_default()
    FONT_CRAWL_SMALL = ImageFont.load_default()


CRAWL_LINES = [
    "EPISODE PPG",
    "A TINY SENSOR",
    "FINDS THE BEAT",
    "",
    "From Brooklyn shops",
    "and Parsons classrooms,",
    "Joel Murphy and Yury Gitman",
    "built an open hardware",
    "heart-rate sensor",
    "for makers.",
    "",
    "World Famous Electronics",
    "began as a Kickstarter",
    "project in 2012,",
    "then kept making",
    "PulseSensor and teaching",
    "it in public.",
    "",
    "The sensor shines green light",
    "into capillary tissue and watches",
    "the returning brightness. Each",
    "pulse wave nudges the signal.",
    "",
    "Its origin is delightfully practical:",
    "breadboards, op-amps, filters,",
    "a phone-style light sensor,",
    "and a reverse-mount green LED",
    "that made finger placement better.",
    "",
    "Now the signal lands here,",
    "on a Cheap Yellow Display:",
    "open, tiny, and alive with code.",
]


def text_size(draw, text, font):
    bbox = draw.textbbox((0, 0), text, font=font)
    return bbox[2] - bbox[0], bbox[3] - bbox[1]


def draw_centered(draw, text, x, y, w, font, fill):
    text_w, _ = text_size(draw, text, font)
    draw.text((x + max(0, (w - text_w) // 2), y), text, font=font, fill=fill)


def draw_button(draw, x, y, w, label, active=False):
    fill = (0, 92, 180) if active else BLUE
    outline = CYAN if active else (0, 140, 255)
    border_w = 3 if active else 2
    draw.rounded_rectangle(
        (x, y, x + w - 1, y + TOOLBAR_BUTTON_HEIGHT - 1),
        radius=4,
        fill=fill,
        outline=outline,
        width=border_w,
    )
    draw_centered(draw, label, x, y + 4, w, FONT_2, WHITE)


def draw_nav(draw, width, y):
    settings_x = width - TOOLBAR_BUTTON_WIDTH - 4
    next_x = settings_x - TOOLBAR_BUTTON_WIDTH - APP_BUTTON_GAP
    prev_x = next_x - TOOLBAR_BUTTON_WIDTH - APP_BUTTON_GAP
    draw_button(draw, prev_x, y, TOOLBAR_BUTTON_WIDTH, "<")
    draw_button(draw, next_x, y, TOOLBAR_BUTTON_WIDTH, ">")
    draw_button(draw, settings_x, y, TOOLBAR_BUTTON_WIDTH, "*")


def draw_header(draw, width, title):
    draw.rectangle((0, 0, width, 42), fill=BLACK)
    draw.line((0, 41, width, 41), fill=GRID)
    draw.text((10, 8), title, font=FONT_1, fill=WHITE)
    draw.text((10, 25), "APP 3  ORIGIN CRAWL", font=FONT_1, fill=GOLD)
    draw_nav(draw, width, 7)


def draw_starfield(draw, width, height, frame):
    stars = [
        (18, 55), (42, 94), (64, 148), (86, 72), (109, 210), (130, 118),
        (151, 60), (173, 169), (195, 96), (217, 222), (240, 142),
        (263, 75), (286, 190), (306, 114), (28, 222), (300, 55),
    ]
    for i, (x, y) in enumerate(stars):
        twinkle = 90 + ((i * 37 + frame * 42) % 130)
        fill = (twinkle, twinkle, twinkle)
        draw.point((x, y), fill=fill)
        if i % 5 == frame % 5:
            draw.point((min(319, x + 1), y), fill=fill)


def draw_title_card(draw, width, height):
    draw.rectangle((0, 42, width, height), fill=BLACK)
    draw_centered(draw, "PulseSensor", 0, 76, width, FONT_3, CYAN)
    draw_centered(draw, "ORIGIN STORY", 0, 112, width, FONT_2, GOLD)
    draw_centered(draw, "open hardware", 0, 152, width, FONT_1, WHITE)
    draw_centered(draw, "heart-rate sensing for makers", 0, 168, width, FONT_1, WHITE)
    draw_centered(draw, "mock App 3 crawl", 0, 206, width, FONT_1, GOLD_DIM)


def draw_crawl(draw, width, height, offset, frame):
    horizon_y = 54
    base_y = 230 - offset
    line_gap = 18
    center_x = width // 2
    draw.polygon([(44, height), (276, height), (173, horizon_y), (147, horizon_y)], fill=(3, 3, 0))
    for idx, line in enumerate(CRAWL_LINES):
        y = base_y + idx * line_gap
        if y < horizon_y - 10 or y > height + 24:
            continue
        depth = max(0.0, min(1.0, (y - horizon_y) / (height - horizon_y)))
        font = FONT_CRAWL if depth > 0.42 else FONT_CRAWL_SMALL
        color_mix = int(100 + depth * 155)
        fill = (color_mix, int(78 + depth * 136), int(8 + depth * 64))
        text_w, text_h = text_size(draw, line, font)
        perspective_w = max(80, int(width * (0.32 + depth * 0.58)))
        x = center_x - min(text_w // 2, perspective_w // 2)
        if line == "":
            continue
        if idx < 2:
            draw_centered(draw, line, center_x - perspective_w // 2, y, perspective_w, FONT_CRAWL, GOLD)
        else:
            draw.text((x, y), line, font=font, fill=fill)
    fade = Image.new("RGBA", (width, height), (0, 0, 0, 0))
    fade_draw = ImageDraw.Draw(fade)
    for y in range(60):
        fade_draw.line((0, y, width, y), fill=(0, 0, 0, max(0, 180 - y * 3)))
    return fade


def render(filename, frame, offset=None, title_card=False):
    width, height = 320, 240
    image = Image.new("RGB", (width, height), BLACK)
    draw = ImageDraw.Draw(image)
    draw_starfield(draw, width, height, frame)
    draw_header(draw, width, "PulseSensor.com")
    if title_card:
        draw_title_card(draw, width, height)
    else:
        fade = draw_crawl(draw, width, height, offset or 0, frame)
        image = Image.alpha_composite(image.convert("RGBA"), fade).convert("RGB")
        draw = ImageDraw.Draw(image)
        draw_header(draw, width, "PulseSensor.com")
    path = OUT_DIR / filename
    image.save(path)
    print(path)


frames = [
    ("app3-origin-title.png", 0, None, True),
    ("app3-origin-crawl-start.png", 1, 40, False),
    ("app3-origin-crawl-mid.png", 2, 128, False),
    ("app3-origin-crawl-late.png", 3, 216, False),
]

for filename, frame, offset, title_card in frames:
    render(filename, frame=frame, offset=offset, title_card=title_card)

contact = Image.new("RGB", (640, 480), BLACK)
for index, (filename, _, _, _) in enumerate(frames):
    frame = Image.open(OUT_DIR / filename)
    x = 0 if index % 2 == 0 else 320
    y = 0 if index < 2 else 240
    contact.paste(frame, (x, y))
contact_path = OUT_DIR / "app3-origin-contact-sheet.png"
contact.save(contact_path)
print(contact_path)
