"""Remove local color tables from a GIF that are identical to the global one.

Usage: python3 strip_lct.py <in.gif> <out.gif>

Pillow writes a local color table per frame. gif_theme.cpp only recolors the
global color table, so the local ones must be removed. Fails if a local table
differs from the global table.
"""
import sys
d=bytearray(open(sys.argv[1],'rb').read())
out=bytearray(d[:13]); p=13
gn=2<<(d[10]&7) if d[10]&0x80 else 0
gct=bytes(d[p:p+3*gn]); out+=gct; p+=3*gn; removed=0
while True:
    b=d[p]
    if b==0x3B: out.append(0x3B); break
    if b==0x21:
        q=p+2
        while d[q]: q+=d[q]+1
        out+=d[p:q+1]; p=q+1
    elif b==0x2C:
        desc=bytearray(d[p:p+10]); fl=desc[9]; p+=10
        if fl&0x80:
            n=2<<(fl&7); lct=bytes(d[p:p+3*n]); p+=3*n
            assert lct[:3*gn]==gct[:3*n] if n<=gn else False, 'LCT differs from GCT'
            desc[9]=fl&~0x87; removed+=1
        out+=desc
        q=p+1
        while d[q]: q+=d[q]+1
        out+=d[p:q+1]; p=q+1
    else: raise Exception('bad block')
open(sys.argv[2],'wb').write(out); print('removed LCT',removed,'size',len(d),'->',len(out))
