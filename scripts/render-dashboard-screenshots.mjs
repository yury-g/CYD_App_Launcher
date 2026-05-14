#!/usr/bin/env node
import { writeFileSync } from "node:fs";

const SCREEN_WIDTH = 320;
const SCREEN_HEIGHT = 240;
const GRAPH_X = 8;
const GRAPH_Y = 48;
const GRAPH_W = 304;
const GRAPH_H = 112;
const PANEL_Y = 170;
const PANEL_H = 62;
const HEART_CENTER_X = 160;
const HEART_CENTER_Y = 22;
const HEART_MIN_SIZE = 8;
const HEART_MAX_SIZE = 15;
const VOL_MINUS_X = 226;
const VOL_VALUE_X = 258;
const VOL_PLUS_X = 292;
const VOL_Y = 9;
const VOL_BUTTON_SIZE = 22;
const VOLUME_START = 2;
const SIGNAL_QUALITY_STEPS = 12;
const LOCK_QUALITY_STEPS = 10;

const color565 = (value) => {
  const r = Math.round(((value >> 11) & 0x1f) * 255 / 31);
  const g = Math.round(((value >> 5) & 0x3f) * 255 / 63);
  const b = Math.round((value & 0x1f) * 255 / 31);
  return `#${[r, g, b].map((n) => n.toString(16).padStart(2, "0")).join("")}`;
};

const COLOR_BG = color565(0x0000);
const COLOR_PANEL = color565(0x0841);
const COLOR_PANEL_DARK = color565(0x0400);
const COLOR_GRID = color565(0x18E3);
const COLOR_GRID_SOFT = color565(0x10A2);
const COLOR_TEXT = color565(0xFFFF);
const COLOR_MUTED = color565(0x8C71);
const COLOR_CYAN = color565(0x07FF);
const COLOR_TEAL = color565(0x05F3);
const COLOR_RED = color565(0xF800);
const COLOR_RED_DARK = color565(0x6000);
const COLOR_AMBER = color565(0xFBE0);

const FONT = "Arial, sans-serif";

function text(x, y, content, size, fill, extra = "") {
  return `  <text x="${x}" y="${y}" fill="${fill}" font-family="${FONT}" font-size="${size}" ${extra}>${content}</text>`;
}

function roundRect(x, y, w, h, r, fill, stroke = null) {
  const strokeAttrs = stroke ? ` stroke="${stroke}" stroke-width="1"` : "";
  return `  <rect x="${x}" y="${y}" width="${w}" height="${h}" rx="${r}" fill="${fill}"${strokeAttrs}/>`;
}

function heart(centerX, centerY, size, color) {
  return [
    `  <circle cx="${centerX - Math.trunc(size / 2)}" cy="${centerY - Math.trunc(size / 3)}" r="${Math.trunc(size / 2)}" fill="${color}"/>`,
    `  <circle cx="${centerX + Math.trunc(size / 2)}" cy="${centerY - Math.trunc(size / 3)}" r="${Math.trunc(size / 2)}" fill="${color}"/>`,
    `  <path d="M${centerX - size} ${centerY - Math.trunc(size / 4)} L${centerX + size} ${centerY - Math.trunc(size / 4)} L${centerX} ${centerY + size} Z" fill="${color}"/>`,
  ];
}

function centeredText(content, x, y, w, size, fill, extra = "") {
  const charW = 6 * size;
  const cursorX = x + Math.max(0, Math.trunc((w - content.length * charW) / 2));
  return text(cursorX, y, content, size * 8, fill, extra);
}

function header({ locked, ledBrightness }) {
  const liveTrace = locked ? COLOR_TEXT : COLOR_CYAN;
  const heartSize = Math.max(HEART_MIN_SIZE, Math.min(HEART_MAX_SIZE, Math.round(HEART_MIN_SIZE + (HEART_MAX_SIZE - HEART_MIN_SIZE) * ledBrightness / 255)));
  const heartFill = ledBrightness < 20 ? COLOR_RED_DARK : ledBrightness < 120 ? color565(0xA800) : COLOR_RED;

  return [
    `  <rect x="0" y="0" width="${SCREEN_WIDTH}" height="42" fill="${COLOR_BG}"/>`,
    `  <line x1="0" y1="41" x2="${SCREEN_WIDTH}" y2="41" stroke="${COLOR_GRID}" stroke-width="1"/>`,
    text(10, 16, "LIVE BEAT DETECTION", 8, COLOR_MUTED),
    ...heart(HEART_CENTER_X, HEART_CENTER_Y, heartSize + 2, liveTrace),
    ...heart(HEART_CENTER_X, HEART_CENTER_Y, heartSize, heartFill),
    text(10, 33, locked ? "QUALIFIED BEAT" : "SIGNAL SEARCH", 8, COLOR_TEXT),
    volumeControl(),
  ].flat();
}

function volumeControl() {
  return [
    `  <rect x="198" y="4" width="120" height="34" fill="${COLOR_BG}"/>`,
    text(201, 25, "VOL", 8, COLOR_CYAN),
    roundRect(VOL_MINUS_X, VOL_Y, VOL_BUTTON_SIZE, VOL_BUTTON_SIZE, 4, COLOR_PANEL, COLOR_GRID),
    roundRect(VOL_PLUS_X, VOL_Y, VOL_BUTTON_SIZE, VOL_BUTTON_SIZE, 4, COLOR_PANEL, COLOR_GRID),
    centeredText("-", VOL_MINUS_X, VOL_Y + 21, VOL_BUTTON_SIZE, 2, COLOR_TEXT),
    centeredText("+", VOL_PLUS_X, VOL_Y + 21, VOL_BUTTON_SIZE, 2, COLOR_TEXT),
    centeredText(String(VOLUME_START), VOL_VALUE_X, VOL_Y + 15, 26, 1, COLOR_TEXT),
  ];
}

function graph({ locked, beatFlash, points }) {
  const liveTrace = locked ? COLOR_TEXT : COLOR_CYAN;
  const lines = [
    roundRect(GRAPH_X - 2, GRAPH_Y - 2, GRAPH_W + 4, GRAPH_H + 4, 6, COLOR_PANEL_DARK, COLOR_GRID),
    `  <rect x="${GRAPH_X}" y="${GRAPH_Y}" width="${GRAPH_W}" height="${GRAPH_H}" fill="${COLOR_BG}"/>`,
    `  <g stroke="${COLOR_GRID_SOFT}" stroke-width="1">`,
  ];
  for (let x = 0; x <= GRAPH_W; x += 38) {
    lines.push(`    <line x1="${GRAPH_X + x}" y1="${GRAPH_Y}" x2="${GRAPH_X + x}" y2="${GRAPH_Y + GRAPH_H}"/>`);
  }
  for (let y = 0; y <= GRAPH_H; y += 28) {
    lines.push(`    <line x1="${GRAPH_X}" y1="${GRAPH_Y + y}" x2="${GRAPH_X + GRAPH_W}" y2="${GRAPH_Y + y}"/>`);
  }
  lines.push("  </g>");
  lines.push(text(GRAPH_X + 6, GRAPH_Y + 13, "LIVE LINE", 8, COLOR_MUTED));
  lines.push(`  <polyline points="${points.join(" ")}" fill="none" stroke="${liveTrace}" stroke-width="2"/>`);
  if (beatFlash) {
    lines.push(`  <circle cx="218" cy="62" r="3" fill="${COLOR_RED}"/>`);
  }
  return lines;
}

function metricPanel(x, label, value, unit, valid) {
  const lines = [
    roundRect(x, PANEL_Y, 102, PANEL_H, 6, COLOR_PANEL, valid ? COLOR_TEAL : COLOR_GRID),
    text(x + 10, PANEL_Y + 18, label, 8, COLOR_MUTED),
  ];
  if (valid) {
    if (label === "BPM") {
      lines.push(text(x + 10, PANEL_Y + 52, String(value).padStart(3, " "), 32, COLOR_TEXT, 'font-weight="700"'));
    } else {
      lines.push(text(x + 10, PANEL_Y + 48, String(value).padStart(3, " "), 24, COLOR_TEXT, 'font-weight="700"'));
      lines.push(text(x + 72, PANEL_Y + 54, unit, 8, COLOR_MUTED));
    }
  } else {
    lines.push(text(x + 10, PANEL_Y + 52, "--", 32, COLOR_MUTED, 'font-weight="700"'));
  }
  return lines;
}

function signalPanel({ locked, quality, rearmCount }) {
  const x = 228;
  const lines = [
    roundRect(x, PANEL_Y, 84, PANEL_H, 6, COLOR_PANEL, locked ? COLOR_RED : COLOR_GRID),
    text(x + 9, PANEL_Y + 17, "QUALITY", 8, COLOR_MUTED),
    "  <g>",
  ];
  for (let i = 0; i < SIGNAL_QUALITY_STEPS; i++) {
    let fill = COLOR_GRID;
    if (i < quality) fill = i < LOCK_QUALITY_STEPS ? COLOR_AMBER : COLOR_TEAL;
    lines.push(`    <rect x="${x + 9 + i * 6}" y="${PANEL_Y + 24}" width="4" height="18" fill="${fill}"/>`);
  }
  lines.push("  </g>");
  lines.push(text(x + 9, PANEL_Y + 57, String(quality).padStart(2, "0") + "/12", 8, locked ? COLOR_TEAL : COLOR_AMBER));
  lines.push(text(x + 50, PANEL_Y + 57, `R${rearmCount}`, 8, COLOR_MUTED));
  return lines;
}

function panels(state) {
  return [
    ...metricPanel(8, "BPM", state.bpm, "", state.locked),
    ...metricPanel(118, "IBI", state.ibi, "ms", state.locked),
    ...signalPanel(state),
  ];
}

function render(state) {
  const lines = [
    `<svg xmlns="http://www.w3.org/2000/svg" width="640" height="480" viewBox="0 0 ${SCREEN_WIDTH} ${SCREEN_HEIGHT}" role="img" aria-label="${state.label}">`,
    `  <rect width="${SCREEN_WIDTH}" height="${SCREEN_HEIGHT}" fill="${COLOR_BG}"/>`,
    ...header(state),
    "",
    ...graph(state),
    "",
    ...panels(state),
    "</svg>",
    "",
  ];
  return lines.join("\n");
}

const searchingPoints = [
  "8,105", "24,104", "40,107", "56,103", "72,106", "88,104", "104,108",
  "120,105", "136,107", "152,102", "168,106", "184,104", "200,107",
  "216,105", "232,108", "248,104", "264,106", "280,103", "296,107", "312,105",
];

const lockedPoints = [
  "8,124", "18,121", "28,118", "38,110", "48,86", "58,58", "68,73",
  "78,112", "88,128", "98,125", "108,121", "118,111", "128,88",
  "138,60", "148,75", "158,113", "168,129", "178,126", "188,122",
  "198,112", "208,90", "218,62", "228,76", "238,114", "248,130",
  "258,126", "268,121", "278,110", "288,86", "298,60", "312,78",
];

writeFileSync("docs/screenshots/searching.svg", render({
  label: "CYD Pulse Dashboard searching screen",
  locked: false,
  ledBrightness: 0,
  bpm: 0,
  ibi: 0,
  quality: 4,
  rearmCount: 0,
  beatFlash: false,
  points: searchingPoints,
}));

writeFileSync("docs/screenshots/locked.svg", render({
  label: "CYD Pulse Dashboard locked screen",
  locked: true,
  ledBrightness: 255,
  bpm: 72,
  ibi: 833,
  quality: 12,
  rearmCount: 1,
  beatFlash: true,
  points: lockedPoints,
}));
