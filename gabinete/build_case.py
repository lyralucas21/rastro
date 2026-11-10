"""Gera o gabinete do RASTRO para impressao 3D (STL) a partir de voxels.

Coordenadas do modelo (mm): x = largura, y = profundidade (0 = face frontal), z = altura.
Pecas: caixa (frente + paredes, impressa com a frente na mesa) e tampa traseira.
"""
import numpy as np
from PIL import Image, ImageDraw, ImageFont
from pathlib import Path

R = 0.4                      # resolucao (mm por voxel)
W, H, D = 130.0, 150.0, 68.0  # medidas externas da caixa
T = 2.4                      # espessura das paredes
OUT = Path(__file__).parent

# --------------------------------------------------------------------------- utilidades
def grid(w, d, h):
    nx, ny, nz = int(round(w / R)), int(round(d / R)), int(round(h / R))
    x = (np.arange(nx) + 0.5) * R
    y = (np.arange(ny) + 0.5) * R
    z = (np.arange(nz) + 0.5) * R
    return np.zeros((nx, ny, nz), bool), x, y, z

def xz_mask(x, z, fn):
    X, Z = np.meshgrid(x, z, indexing='ij')
    return fn(X, Z)

def circle(cx, cz, r):
    return lambda X, Z: (X - cx) ** 2 + (Z - cz) ** 2 <= r * r

def rect(cx, cz, w, h):
    return lambda X, Z: (abs(X - cx) <= w / 2) & (abs(Z - cz) <= h / 2)

def ring(cx, cz, r_out, width):
    return lambda X, Z: ((X - cx) ** 2 + (Z - cz) ** 2 <= r_out ** 2) & ((X - cx) ** 2 + (Z - cz) ** 2 >= (r_out - width) ** 2)

def almond(cx, cz, w, h):
    # olho: intersecao de dois circulos (forma de amendoa)
    rr = (w * w / 4 + h * h / 4) / h  # raio que passa pelas pontas e pelo topo
    off = rr - h / 2
    def f(X, Z):
        a = (X - cx) ** 2 + (Z - (cz - off)) ** 2 <= rr * rr
        b = (X - cx) ** 2 + (Z - (cz + off)) ** 2 <= rr * rr
        return a & b
    return f

FONTES = ['/usr/share/fonts/opentype/inter/Inter-Bold.otf', '/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf',
          'C:/Windows/Fonts/arialbd.ttf', '/Library/Fonts/Arial Bold.ttf']

def fonte_padrao():
    import os
    for f in FONTES:
        if os.path.exists(f):
            return f
    raise SystemExit('Nenhuma fonte encontrada: ajuste a lista FONTES em build_case.py')

def text_mask(x, z, txt, cx, cz, height_mm, font=None, spacing=0.12):
    font = font or fonte_padrao()
    px = 40  # pixels por mm na rasterizacao
    f = ImageFont.truetype(font, int(height_mm * px * 1.38))
    # desenha com espacamento entre letras
    widths = [f.getbbox(c)[2] - f.getbbox(c)[0] for c in txt]
    gap = spacing * height_mm * px
    total = sum(widths) + gap * (len(txt) - 1)
    img = Image.new('L', (int(total + 4 * px), int(height_mm * px * 2.2)), 0)
    d = ImageDraw.Draw(img)
    cur = 2 * px
    for c, wd in zip(txt, widths):
        bb = f.getbbox(c)
        d.text((cur - bb[0], img.height / 2), c, font=f, fill=255, anchor='ls')
        cur += wd + gap
    a = np.array(img) > 127
    ys, xs = np.nonzero(a)
    a = a[ys.min():ys.max() + 1, xs.min():xs.max() + 1]
    th, tw = a.shape[0] / px, a.shape[1] / px
    X, Z = np.meshgrid(x, z, indexing='ij')
    u = ((X - (cx - tw / 2)) * px).astype(int)
    v = (((cz + th / 2) - Z) * px).astype(int)
    ok = (u >= 0) & (u < a.shape[1]) & (v >= 0) & (v < a.shape[0])
    m = np.zeros_like(X, bool)
    m[ok] = a[v[ok], u[ok]]
    return m

def cyl_y(vox, x, y, z, cx, cz, r, y0, y1, value=True):
    m = xz_mask(x, z, circle(cx, cz, r))
    sel = (y >= y0) & (y <= y1)
    vox[:, sel, :] = np.where(m[:, None, :], value, vox[:, sel, :]) if value else vox[:, sel, :] & ~m[:, None, :]

def apply_xz(vox, y, mask, y0, y1, value):
    sel = (y >= y0) & (y < y1)
    sub = vox[:, sel, :]
    if value:
        sub |= mask[:, None, :]
    else:
        sub &= ~mask[:, None, :]
    vox[:, sel, :] = sub

# --------------------------------------------------------------------------- caixa
def build_box():
    vox, x, y, z = grid(W, D, H)
    X3 = x[:, None, None]; Y3 = y[None, :, None]; Z3 = z[None, None, :]
    vox[:] = True
    # cavidade interna (aberta atras)
    cav = (X3 > T) & (X3 < W - T) & (Z3 > T) & (Z3 < H - T) & (Y3 > T)
    vox &= ~cav

    EYE_Z, CAM_Z, LCD_Z = 100.0, 134.0, 54.0
    cx = W / 2

    # pilares dos parafusos da tampa (cantos internos)
    for px_ in (T + 4.6, W - T - 4.6):
        for pz in (T + 4.6, H - T - 4.6):
            m = xz_mask(x, z, circle(px_, pz, 4.6))
            apply_xz(vox, y, m, T, D, True)
            hole = xz_mask(x, z, circle(px_, pz, 1.3))   # furo piloto p/ parafuso M3 auto-atarraxante
            apply_xz(vox, y, hole, D - 16, D, False)

    # --- suportes internos (antes de furar a frente)
    # LCD 16x2: 4 colunas (furos 75 x 31 mm), altura 7,2 mm = moldura do display encosta na frente
    for dx in (-37.5, 37.5):
        for dz in (-15.5, 15.5):
            apply_xz(vox, y, xz_mask(x, z, circle(cx + dx, LCD_Z + dz, 3.0)), T, T + 7.2, True)
            apply_xz(vox, y, xz_mask(x, z, circle(cx + dx, LCD_Z + dz, 1.1)), T, T + 7.2, False)
    # ESP32-CAM: encaixe quadrado para o modulo OV2640 atras do furo da lente
    frame = xz_mask(x, z, rect(cx, CAM_Z, 13.4, 13.4)) & ~xz_mask(x, z, rect(cx, CAM_Z, 9.0, 9.0))
    apply_xz(vox, y, frame, T, T + 4.0, True)
    # PIR: anel guia ao redor da cupula
    apply_xz(vox, y, xz_mask(x, z, ring(cx, EYE_Z, 15.5, 2.0)), T, T + 3.0, True)

    # --- furos passantes na frente
    through = (xz_mask(x, z, circle(cx, EYE_Z, 11.9))          # cupula do PIR (pupila do olho)
               | xz_mask(x, z, circle(cx, CAM_Z, 4.5))          # lente da camera
               | xz_mask(x, z, rect(cx, LCD_Z, 66.0, 18.0)))    # janela do LCD
    # grade do buzzer (hexagonal)
    bz_x, bz_z = W - 22.0, 22.0
    for i, (ox, oz) in enumerate([(0, 0)] + [(5.2 * np.cos(a), 5.2 * np.sin(a)) for a in np.linspace(0, 2 * np.pi, 7)[:-1]]):
        through |= xz_mask(x, z, circle(bz_x + ox, bz_z + oz, 1.25))
    apply_xz(vox, y, through, 0, T + 8, False)
    # recolocar o que nao e da frente (furos nao podem cortar suportes): furos so na placa frontal
    # (os suportes estao fora das areas furadas, exceto o encaixe da camera, que e oco de proposito)

    # --- gravacoes na face frontal (0,8 mm de profundidade)
    eng = np.zeros((len(x), len(z)), bool)
    outer = xz_mask(x, z, almond(cx, EYE_Z, 104, 50))
    inner = xz_mask(x, z, almond(cx, EYE_Z, 99, 45))
    eng |= outer & ~inner                                       # contorno do olho
    eng |= xz_mask(x, z, ring(cx, EYE_Z, 19.0, 1.4))            # iris
    eng |= xz_mask(x, z, ring(cx, CAM_Z, 8.0, 1.2))             # anel da camera
    eng |= xz_mask(x, z, circle(cx + 13.0, CAM_Z, 2.0))         # ponto "REC"
    eng |= text_mask(x, z, 'RASTRO', cx - 6, 22.0, 8.0)
    eng |= xz_mask(x, z, rect(cx, LCD_Z, 76.0, 28.0)) & ~xz_mask(x, z, rect(cx, LCD_Z, 73.6, 25.6))  # moldura do LCD
    apply_xz(vox, y, eng, 0, 0.8, False)

    # --- passagem do cabo USB (parede de baixo, perto do fundo)
    X2, Y2 = np.meshgrid(x, y, indexing='ij')
    cab = (X2 - (W - 30)) ** 2 + (Y2 - (D - 12)) ** 2 <= 5.5 ** 2
    vox[:, :, z < T + 0.01] &= ~cab[:, :, None]
    # --- ventilacao no topo (5 fendas)
    for i in range(5):
        sl = (abs(X2 - (W / 2 - 24 + 12 * i)) <= 1.2) & (Y2 > D - 40) & (Y2 < D - 12)
        vox[:, :, z > H - T - 0.01] &= ~sl[:, :, None]
    return vox

# --------------------------------------------------------------------------- tampa traseira
def build_lid():
    vox, x, y, z = grid(W, T, H)
    vox[:] = True
    holes = np.zeros((len(x), len(z)), bool)
    for px_ in (T + 4.6, W - T - 4.6):
        for pz in (T + 4.6, H - T - 4.6):
            holes |= xz_mask(x, z, circle(px_, pz, 1.75))
    # furos tipo "buraco de fechadura" para pendurar na parede
    for kx in (W / 2 - 35, W / 2 + 35):
        holes |= xz_mask(x, z, circle(kx, H - 30, 4.2))
        holes |= xz_mask(x, z, rect(kx, H - 24, 4.4, 12))
    # ventilacao
    for i in range(6):
        holes |= xz_mask(x, z, rect(W / 2, 30 + 6 * i, 50, 2.4))
    vox &= ~holes[:, None, :]
    # gravacao
    eng = text_mask(x, z, 'RASTRO  MOSTRA PIC', W / 2, 80, 5.0)[::-1, :]  # espelhado: lido por tras
    sel = y > T - 0.8
    vox[:, sel, :] &= ~eng[:, None, :]
    return vox

# --------------------------------------------------------------------------- malha (greedy meshing)
def mesh(vox):
    quads = []
    shp = vox.shape
    for a in range(3):
        u, v = (a + 1) % 3, (a + 2) % 3
        V = np.moveaxis(vox, (a, u, v), (0, 1, 2))
        pad = np.pad(V, ((1, 1), (0, 0), (0, 0)))
        for d in (1, -1):
            if d == 1:
                faces = pad[1:-1] & ~pad[2:]
            else:
                faces = pad[1:-1] & ~pad[:-2]
            for i in np.nonzero(faces.any(axis=(1, 2)))[0]:
                m = faces[i]
                plane = (i + 1) if d == 1 else i
                active = {}
                for r in range(m.shape[0] + 1):
                    runs = set()
                    if r < m.shape[0]:
                        row = np.concatenate(([0], m[r].astype(np.int8), [0]))
                        dd = np.diff(row)
                        st = np.nonzero(dd == 1)[0]; en = np.nonzero(dd == -1)[0]
                        runs = set(zip(st.tolist(), en.tolist()))
                    for key in list(active):
                        if key not in runs:
                            r0 = active.pop(key)
                            quads.append((a, d, plane, r0, r, key[0], key[1]))
                    for key in runs:
                        if key not in active:
                            active[key] = r
    tris = []
    for a, d, plane, u0, u1, v0, v1 in quads:
        u, v = (a + 1) % 3, (a + 2) % 3
        def P(uu, vv):
            p = [0.0, 0.0, 0.0]; p[a] = plane * R; p[u] = uu * R; p[v] = vv * R; return p
        c = [P(u0, v0), P(u1, v0), P(u1, v1), P(u0, v1)]
        if d == -1:
            c = c[::-1]
        tris.append((c[0], c[1], c[2])); tris.append((c[0], c[2], c[3]))
    return np.array(tris, np.float32), len(quads)

def write_stl(path, tris):
    n = np.cross(tris[:, 1] - tris[:, 0], tris[:, 2] - tris[:, 0])
    n /= np.linalg.norm(n, axis=1, keepdims=True) + 1e-12
    rec = np.zeros(len(tris), dtype=[('n', '<f4', 3), ('v', '<f4', (3, 3)), ('a', '<u2')])
    rec['n'] = n; rec['v'] = tris
    with open(path, 'wb') as f:
        f.write(b'RASTRO gabinete'.ljust(80, b' '))
        f.write(np.uint32(len(tris)).tobytes())
        f.write(rec.tobytes())

def to_print_orientation(tris, depth):
    # gira para imprimir com a face y=0 (frente) apoiada na mesa: z_print = y
    out = tris.copy()
    out[..., 0] = tris[..., 0]
    out[..., 1] = tris[..., 2]
    out[..., 2] = tris[..., 1]
    # a troca y<->z inverte a orientacao: reordena vertices
    return out[:, [0, 2, 1], :]

if __name__ == '__main__':
    import time, json
    t = time.time()
    box = build_box(); lid = build_lid()
    np.save(OUT / 'box_vox.npy', box); np.save(OUT / 'lid_vox.npy', lid)  # usados por render.py
    info = {}
    for name, v, depth in (('rastro_caixa', box, D), ('rastro_tampa', lid, T)):
        tris, nq = mesh(v)
        tris = to_print_orientation(tris, depth)
        write_stl(OUT / f'{name}.stl', tris)
        info[name] = dict(triangulos=len(tris), quads=nq, mm=[float(s * R) for s in v.shape])
    print(json.dumps(info), round(time.time() - t, 1), 's')
