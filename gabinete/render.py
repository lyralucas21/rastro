"""Renderiza previas do gabinete a partir dos voxels (z-buffer + contornos, sem dependencias)."""
import numpy as np
from PIL import Image, ImageFilter

R = 0.4

def faces(vox):
    out = []
    for a in range(3):
        for d in (1, -1):
            sl_dst = [slice(None)] * 3; sl_src = [slice(None)] * 3
            if d == 1:
                sl_dst[a] = slice(0, -1); sl_src[a] = slice(1, None)
            else:
                sl_dst[a] = slice(1, None); sl_src[a] = slice(0, -1)
            nb = np.zeros_like(vox); nb[tuple(sl_dst)] = vox[tuple(sl_src)]
            idx = np.argwhere(vox & ~nb).astype(np.float32)
            out.append((a, d, idx))
    return out

def render(vox, yaw, pitch, px_mm=5.0, sub=3, color=(236, 232, 224), pad=40):
    cy, sy = np.cos(np.radians(yaw)), np.sin(np.radians(yaw))
    cp, sp = np.cos(np.radians(pitch)), np.sin(np.radians(pitch))
    M = np.array([[1, 0, 0], [0, cp, -sp], [0, sp, cp]]) @ np.array([[cy, -sy, 0], [sy, cy, 0], [0, 0, 1]])
    center = np.array(vox.shape) * R / 2
    allp, alln = [], []
    offs = (np.arange(sub) + 0.5) / sub - 0.5
    for a, d, idx in faces(vox):
        n = np.zeros(3); n[a] = d
        nv = M @ n
        if nv[1] >= 0:      # de costas para a camera
            continue
        base = (idx + 0.5) * R; base[:, a] += d * R / 2
        u, v = (a + 1) % 3, (a + 2) % 3
        for ou in offs:
            for ov in offs:
                p = base.copy(); p[:, u] += ou * R; p[:, v] += ov * R
                allp.append(p); alln.append(np.repeat(nv[None], len(p), 0))
    P = (np.concatenate(allp) - center) @ M.T
    Nn = np.concatenate(alln)
    xs, zs = P[:, 0] * px_mm, -P[:, 2] * px_mm
    w = int(np.ptp(xs) + 2 * pad); h = int(np.ptp(zs) + 2 * pad)
    u = (xs - xs.min() + pad).astype(np.int32); v = (zs - zs.min() + pad).astype(np.int32)
    order = np.argsort(-P[:, 1], kind='stable')
    pid = (v * w + u)[order]
    depth = np.full(w * h, np.inf, np.float32); nbuf = np.zeros((w * h, 3), np.float32)
    depth[pid] = P[order, 1]; nbuf[pid] = Nn[order]
    depth = depth.reshape(h, w); nbuf = nbuf.reshape(h, w, 3)
    mask = np.isfinite(depth)
    L = np.array([-0.45, -0.75, 0.5]); L /= np.linalg.norm(L)
    lam = np.clip(-(nbuf @ np.array([L[0], L[1], L[2]]) * np.array(1)) * -1, 0, 1)
    lam = np.clip(nbuf @ -L * -1, 0, 1)
    lam = np.clip((nbuf * np.array([-0.45, -0.75, 0.5])).sum(-1) * -1, 0, 1)
    shade = 0.55 + 0.45 * np.clip(-(nbuf[..., 1]) * 0.6 + nbuf[..., 2] * 0.35 - nbuf[..., 0] * 0.25, 0, 1)
    img = np.ones((h, w, 3), np.float32) * 255
    img[mask] = np.array(color)[None] * shade[mask][:, None]
    # contornos: descontinuidade de profundidade ou de normal
    dm = np.where(mask, depth, depth[mask].max() + 50)
    gx = np.abs(np.diff(dm, axis=1, append=dm[:, -1:])); gy = np.abs(np.diff(dm, axis=0, append=dm[-1:, :]))
    nx = np.abs(np.diff(nbuf, axis=1, append=nbuf[:, -1:])).sum(-1); ny = np.abs(np.diff(nbuf, axis=0, append=nbuf[-1:, :])).sum(-1)
    edge = (gx > 0.3) | (gy > 0.3) | (nx > 0.5) | (ny > 0.5)
    img[edge] = img[edge] * 0.35
    im = Image.fromarray(img.clip(0, 255).astype(np.uint8))
    return im.filter(ImageFilter.SMOOTH)

if __name__ == '__main__':
    box = np.load('box_vox.npy'); lid = np.load('lid_vox.npy')
    v = [render(box, 0, 0), render(box, 35, 20), render(box, 205, 15), render(lid, 180, 0, color=(210, 210, 214))]
    for i, im in enumerate(v):
        im.save(f'previa_{i}.png')
    hmax = max(i.height for i in v)
    sheet = Image.new('RGB', (sum(i.width for i in v), hmax), 'white'); x = 0
    for im in v:
        sheet.paste(im, (x, (hmax - im.height) // 2)); x += im.width
    sheet.save('previa.png'); print(sheet.size)
