#!/usr/bin/env python3
import argparse
import csv
import datetime as dt
from pathlib import Path
import sys
import time

import serial


def default_output_path():
    stamp = dt.datetime.now().strftime("%Y%m%d-%H%M%S")
    return Path("logs") / f"signal-log-{stamp}.csv"


def main():
    parser = argparse.ArgumentParser(description="Capture rawDiag CSV lines from the CYD serial stream.")
    parser.add_argument("--port", required=True)
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--seconds", type=float, default=60)
    parser.add_argument("--out", type=Path, default=default_output_path())
    args = parser.parse_args()

    args.out.parent.mkdir(parents=True, exist_ok=True)
    start = time.time()
    rows = 0
    header = None

    with serial.Serial(args.port, args.baud, timeout=0.2) as ser, args.out.open("w", newline="") as f:
        ser.setDTR(False)
        ser.setRTS(False)
        writer = None
        while time.time() - start < args.seconds:
            line = ser.readline().decode("utf-8", errors="replace").strip()
            if not line.startswith("rawDiag,"):
                continue
            parts = line.split(",")
            if parts[1] == "ms":
                header = parts
                writer = csv.writer(f)
                writer.writerow(header)
                continue
            if writer is None:
                header = [
                    "rawDiag", "ms", "signal", "amp", "bpm", "ibi", "locked",
                    "quality", "p2p", "range", "clip", "inside", "beat",
                    "accept", "drop", "qStreak", "badStreak",
                ]
                writer = csv.writer(f)
                writer.writerow(header)
            writer.writerow(parts)
            rows += 1

    print(f"captured {rows} rawDiag rows to {args.out}")
    if rows == 0:
        print("no rawDiag rows captured; confirm RAW_SIGNAL_DIAGNOSTICS is enabled and the port is correct", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
