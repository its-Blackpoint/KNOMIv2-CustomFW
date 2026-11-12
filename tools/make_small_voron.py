"""Generate a small version of the KNOMI Voron logo animation.

Usage: python3 make_small_voron.py <gif_voron.gif> <out.gif>

Crops the full 150x150 gif_voron animation to the area used by the flying bars,
scales it so the final logo is 68 px wide (75x71 px frame) and writes a GIF with
the same frame timing. Pixels are mapped to 64 shades of red (logo is pure red on
black), so the theme recoloring in gif_theme.cpp can shift them to any color.
Run strip_lct.py on the result afterwards.
"""
import sys
from PIL import Image
src, out = sys.argv[1], sys.argv[2]
CROP=(8,0,150,135); SCALE=68/129
im=Image.open(src)
W=round((CROP[2]-CROP[0])*SCALE); H=round((CROP[3]-CROP[1])*SCALE)
pal=[]
for k in range(64): pal+= [round(255*k/63),0,0]
pal+= [0,0,0]*(256-64)
frames=[]; durs=[]
for i in range(im.n_frames):
    im.seek(i); durs.append(im.info['duration'])
    r=im.convert('RGB').crop(CROP).resize((W,H),Image.LANCZOS).split()[0]
    p=Image.frombytes('P',(W,H),bytes(round(v*63/255) for v in r.tobytes())); p.putpalette(pal)
    frames.append(p)
frames[0].save(out, save_all=True, append_images=frames[1:], duration=durs, loop=0, optimize=False, disposal=1)
print(W,H,len(frames))
