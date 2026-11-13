#!/usr/bin/env python3
"""Boot stage durations from Tera Term logs with timestamps.

Usage: tools/boot_time.py docs/captures/boot_timed_*.log
"""
import re
import sys

# Each stage starts at the first line matching its marker and ends where the next stage starts.
MARKERS = [
    ("TF-A BL2", r"NOTICE:  CPU: STM32MP"),
    ("BL31+OP-TEE", r"NOTICE:  BL31: v"),
    ("U-Boot", r"^U-Boot 20"),
    ("Kernel", r"^Starting kernel"),
    ("initrd+rootfs", r"Run /init as init"),
    ("systemd", r"^Welcome to "),
    ("login", r"login:"),
]

TIMESTAMP = re.compile(r"^\[\d{4}-\d\d-\d\d (\d\d):(\d\d):(\d\d\.\d+)\] ?(.*)$")


def stage_times(path):
    times = {}
    with open(path, encoding="utf-8", errors="replace") as f:
        for line in f:
            m = TIMESTAMP.match(line.rstrip("\r\n"))
            if not m:
                continue
            t = int(m[1]) * 3600 + int(m[2]) * 60 + float(m[3])
            for name, pattern in MARKERS:
                if name not in times and re.search(pattern, m[4]):
                    times[name] = t
    missing = [name for name, _ in MARKERS if name not in times]
    if missing:
        sys.exit(f"{path}: marker not found: {', '.join(missing)}")
    names = [name for name, _ in MARKERS]
    durations = [times[names[i + 1]] - times[names[i]] for i in range(len(names) - 1)]
    return durations, times["login"] - times["TF-A BL2"]


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__.strip())
    header = [name for name, _ in MARKERS[:-1]] + ["Total"]
    print("| Log | " + " | ".join(header) + " |")
    print("|---" * (len(header) + 1) + "|")
    for path in sys.argv[1:]:
        durations, total = stage_times(path)
        cells = [f"{d:.3f}" for d in durations] + [f"{total:.3f}"]
        print(f"| {path.split('/')[-1]} | " + " | ".join(cells) + " |")


if __name__ == "__main__":
    main()
