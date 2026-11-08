#!/bin/sh
# Generates the media used by docs/on-device-tests.md. Needs ffmpeg.
# Usage: tools/devicetest/make_test_media.sh /path/to/iPod/Music/RockpodTests
set -e
OUT="${1:?usage: $0 OUTPUT_DIR}"
mkdir -p "$OUT/formats" "$OUT/hires" "$OUT/video"
# Spec 0009: alternate sample rates and codecs to stress the format switch.
i=0
for spec in "flac 44100" "mp3 48000" "flac 48000" "mp3 44100" "m4a 44100" "flac 96000" "mp3 32000" "flac 44100"; do
    set -- $spec; i=$((i+1))
    case $1 in
        m4a) c="-c:a alac" ;; *) c="" ;;
    esac
    ffmpeg -loglevel error -y -f lavfi -i "sine=frequency=$((300+i*50)):sample_rate=$2:duration=20" \
        $c -metadata title="Switch $i ($1 $2)" -metadata album="Format switch" \
        "$OUT/formats/$(printf %02d $i).$1"
done
# 1.1: hi-res FLAC, 24-bit.
for r in 96000 176400 192000; do
    ffmpeg -loglevel error -y -f lavfi -i "sine=frequency=1000:sample_rate=$r:duration=60" \
        -sample_fmt s32 -c:a flac -metadata title="Hi-res $r" -metadata album="Hi-res" \
        "$OUT/hires/hires_$r.flac"
done
# Spec 0017: H.264 Constrained Baseline, level 3.0, 320x240, AAC.
ffmpeg -loglevel error -y -f lavfi -i "testsrc=size=320x240:rate=30:duration=30" \
    -f lavfi -i "sine=frequency=440:duration=30" -c:v libx264 -profile:v baseline \
    -level 3.0 -pix_fmt yuv420p -c:a aac -b:a 128k -shortest "$OUT/video/test_h264.mp4"
echo "Test media written to $OUT"
