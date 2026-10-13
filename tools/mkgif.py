# Turns sim.c output into an animated GIF that looks like the 128x32 OLED.
import re, sys
from PIL import Image
font_src = open(sys.argv[1]).read()
body = font_src[font_src.index('{', font_src.index('font[]')) + 1:]
FONT = [int(v, 16) for v in re.findall(r'0x[0-9A-Fa-f]{2}', body)]
W, H, Z = 128, 32, 4
frames, data = [], open(sys.argv[2], 'rb').read()
step = 512 + 88
for off in range(0, len(data) - step + 1, step):
    fb = bytearray(data[off:off + 512]); text = data[off + 512:off + step]
    for row in range(4):
        for col in range(21):
            ch = text[row * 22 + col]
            if ch == 32: continue
            for i in range(6):
                fb[row * 128 + col * 6 + i] = FONT[ch * 6 + i] if i < 6 else 0
    img = Image.new('RGB', (W * Z + 16, H * Z + 16), (10, 12, 20))
    px = img.load()
    for y in range(H):
        for x in range(W):
            if fb[(y // 8) * 128 + x] >> (y % 8) & 1:
                for dy in range(Z - 1):
                    for dx in range(Z - 1):
                        px[8 + x * Z + dx, 8 + y * Z + dy] = (150, 220, 255)
    frames.append(img)
frames[0].save(sys.argv[3], save_all=True, append_images=frames[1:], duration=50, loop=0, optimize=True)
print(sys.argv[3], len(frames), 'frames')
