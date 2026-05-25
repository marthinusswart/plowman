#!/usr/bin/env python3
import sys
import wave

def convert_wav_to_sfx(wav_path, sfx_path):
    print(f"Converting {wav_path} -> {sfx_path}...")
    with wave.open(wav_path, 'rb') as w:
        channels = w.getnchannels()
        sampwidth = w.getsampwidth()
        framerate = w.getframerate()
        nframes = w.getnframes()
        raw_bytes = w.readframes(nframes)
        
    print(f"WAV Info: channels={channels}, width={sampwidth}, rate={framerate}, frames={nframes}")
    
    # Extract mono channel if stereo, and convert to signed 8-bit
    samples = []
    if sampwidth == 1:
        # 8-bit unsigned -> signed 8-bit
        # Stereo audio might have interleaved samples; we take the first channel if channels > 1
        for i in range(0, len(raw_bytes), channels):
            samples.append(raw_bytes[i] - 128)
    elif sampwidth == 2:
        # 16-bit signed -> signed 8-bit
        # Raw bytes are in 16-bit little-endian
        for i in range(0, len(raw_bytes), 2 * channels):
            val = int.from_bytes(raw_bytes[i : i + 2], byteorder='little', signed=True)
            samples.append(val // 256)
    else:
        raise ValueError(f"Unsupported WAV sample width: {sampwidth}")

    # Even padding (from tSfx constructor)
    if len(samples) % 2 != 0:
        samples.append(0)

    # enforceEmptyFirstWord()
    # ptplayer requires the first word (first 2 samples/bytes) to be zero
    while len(samples) < 2 or samples[0] != 0 or samples[1] != 0:
        samples.insert(0, 0)

    # Write to SFX file
    with open(sfx_path, 'wb') as f:
        # 1. Version (1 byte)
        f.write(bytes([2]))
        # 2. Word Length (2 bytes, big-endian)
        f.write((len(samples) // 2).to_bytes(2, byteorder='big'))
        # 3. Sample Rate (2 bytes, big-endian)
        f.write(framerate.to_bytes(2, byteorder='big'))
        # 4. Compressed Size (4 bytes, big-endian) - 0 for uncompressed
        f.write((0).to_bytes(4, byteorder='big'))
        # 5. Raw signed 8-bit sample data
        out_bytes = bytearray()
        for s in samples:
            # clamp to [-128, 127]
            s = max(-128, min(127, s))
            if s < 0:
                s += 256
            out_bytes.append(s)
        f.write(out_bytes)
        
    print("Conversion complete!")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: wav2sfx.py <input.wav> <output.sfx>")
        sys.exit(1)
    convert_wav_to_sfx(sys.argv[1], sys.argv[2])
