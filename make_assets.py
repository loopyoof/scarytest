#!/usr/bin/env python3
"""Convert a video into the frames + audio the app expects. Needs ffmpeg on PATH.
Usage: python make_assets.py input.mp4
Then copy the 'scarytest' output folder to the SD card at sd:/wiiu/scarytest/
"""
import subprocess, sys, pathlib

FPS = 15  # must match FPS in src/main.c
src = sys.argv[1]
out = pathlib.Path("scarytest")
(out / "frames").mkdir(parents=True, exist_ok=True)

vf = (f"fps={FPS},scale=854:480:force_original_aspect_ratio=decrease,"
      "pad=854:480:(ow-iw)/2:(oh-ih)/2")
subprocess.run(["ffmpeg", "-y", "-i", src, "-vf", vf, "-q:v", "5",
                str(out / "frames" / "f%05d.jpg")], check=True)
subprocess.run(["ffmpeg", "-y", "-i", src, "-vn", "-c:a", "libvorbis", "-q:a", "4",
                str(out / "audio.ogg")], check=True)
print("Done. Copy the 'scarytest' folder to sd:/wiiu/")
