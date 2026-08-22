#!/usr/bin/env python3
"""Convert a video to the lightweight RFSuite RFV1 RGB565 format."""

import argparse
import struct
import subprocess


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", help="source video supported by FFmpeg")
    parser.add_argument("output", help="destination .rfv file")
    parser.add_argument("--width", type=int, default=152)
    parser.add_argument("--height", type=int, default=86)
    parser.add_argument("--fps", type=int, default=10)
    args = parser.parse_args()

    if not 1 <= args.width <= 152 or not 1 <= args.height <= 86:
        parser.error("width/height must fit the 152x86 firmware canvas")
    if not 1 <= args.fps <= 30:
        parser.error("fps must be between 1 and 30")

    frame_size = args.width * args.height * 2
    video_filter = (
        f"fps={args.fps},"
        f"scale={args.width}:{args.height}:force_original_aspect_ratio=decrease,"
        f"pad={args.width}:{args.height}:(ow-iw)/2:(oh-ih)/2:black"
    )
    command = [
        "ffmpeg", "-v", "error", "-i", args.input,
        "-vf", video_filter, "-f", "rawvideo", "-pix_fmt", "rgb565le", "-",
    ]

    process = subprocess.Popen(command, stdout=subprocess.PIPE)
    frame_count = 0
    with open(args.output, "wb") as destination:
        destination.write(struct.pack("<4sHHBBI", b"RFV1", args.width,
                                      args.height, args.fps, 0, 0))
        assert process.stdout is not None
        while True:
            frame = process.stdout.read(frame_size)
            if not frame:
                break
            if len(frame) != frame_size:
                process.kill()
                raise RuntimeError("FFmpeg returned a partial video frame")
            destination.write(frame)
            frame_count += 1
        result = process.wait()
        if result != 0:
            raise RuntimeError(f"FFmpeg failed with exit code {result}")
        destination.seek(10)
        destination.write(struct.pack("<I", frame_count))

    if frame_count == 0:
        raise RuntimeError("source produced no frames")
    print(f"wrote {frame_count} frames to {args.output}")


if __name__ == "__main__":
    main()
