# Contact sheet straight from sim.c output: one tile per requested time (s).
import re, sys
from PIL import Image, ImageDraw
src = open(sys.argv[1]).read(); body = src[src.index('{', src.index('font[]')) + 1:]
FONT = [int(v, 16) for v in re.findall(r'0x[0-9A-Fa-f]{2}', body)]
data = open(sys.argv[2], 'rb').read(); STEP = 600; Z = 3
def tile(i):
    fb = bytearray(data[i*STEP:i*STEP+512]); text = data[i*STEP+512:(i+1)*STEP]
    for r in range(4):
        for c in range(21):
            ch = text[r*22+c]
            if ch != 32:
                for k in range(6): fb[r*128+c*6+k] = FONT[ch*6+k]
    img = Image.new('RGB', (128*Z, 32*Z), (10, 12, 20)); p = img.load()
    for y in range(32):
        for x in range(128):
            if fb[(y//8)*128+x] >> (y%8) & 1:
                for dy in range(Z-1):
                    for dx in range(Z-1): p[x*Z+dx, y*Z+dy] = (150, 220, 255)
    return img
times = [float(v) for v in sys.argv[4].split(',')]; cols = 3; w, h = 128*Z, 32*Z + 14
sheet = Image.new('RGB', (w*cols, h*((len(times)+cols-1)//cols)), (40, 40, 40)); d = ImageDraw.Draw(sheet)
for n, t in enumerate(times):
    x, y = (n % cols)*w, (n//cols)*h
    sheet.paste(tile(int(t*20)), (x, y+14)); d.text((x+4, y+1), f't={t}s', fill=(255, 255, 0))
sheet.save(sys.argv[3])
