#!/usr/bin/env python3
"""Convert an image to the lightweight RFSuite RFI1 RGB565 format."""

import argparse
import struct
import subprocess


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", help="source image supported by FFmpeg")
    parser.add_argument("output", help="destination .rfi file")
    parser.add_argument("--width", type=int, default=152)
    parser.add_argument("--height", type=int, default=86)
    args = parser.parse_args()
    if not 1 <= args.width <= 152 or not 1 <= args.height <= 86:
        parser.error("width/height must fit the 152x86 firmware canvas")

    image_filter = (
        f"scale={args.width}:{args.height}:force_original_aspect_ratio=decrease,"
        f"pad={args.width}:{args.height}:(ow-iw)/2:(oh-ih)/2:black"
    )
    command = [
        "ffmpeg", "-v", "error", "-i", args.input, "-frames:v", "1",
        "-vf", image_filter, "-f", "rawvideo", "-pix_fmt", "rgb565le", "-",
    ]
    result = subprocess.run(command, check=True, capture_output=True)
    expected = args.width * args.height * 2
    if len(result.stdout) != expected:
        raise RuntimeError(f"expected {expected} image bytes, got {len(result.stdout)}")
    with open(args.output, "wb") as destination:
        destination.write(struct.pack("<4sHH", b"RFI1", args.width, args.height))
        destination.write(result.stdout)
    print(f"wrote {args.output} ({args.width}x{args.height})")


if __name__ == "__main__":
    main()
