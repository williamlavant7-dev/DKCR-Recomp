"""Function discovery and instruction census for an NSO module."""
import struct
import sys
from collections import Counter

import capstone
from nso import Nso, R_AARCH64_RELATIVE


def read_encoded(img, off, enc, pc_base):
    fmt = enc & 0x0F
    app = enc & 0x70
    if fmt == 0x0B:      # sdata4
        v, size = struct.unpack_from('<i', img, off)[0], 4
    elif fmt == 0x03:    # udata4
        v, size = struct.unpack_from('<I', img, off)[0], 4
    elif fmt in (0x0C, 0x04):
        v, size = struct.unpack_from('<q' if fmt == 0x0C else '<Q', img, off)[0], 8
    else:
        raise ValueError(f'unsupported encoding {enc:#x}')
    if app == 0x10:
        v += off
    elif app == 0x30:
        v += pc_base
    return v, size


def eh_frame_functions(n):
    """Function start addresses from .eh_frame_hdr's binary search table."""
    img, hdr = n.image, n.eh_start
    if not hdr or hdr >= len(img):
        return []
    ver, ptr_enc, cnt_enc, tbl_enc = img[hdr:hdr + 4]
    off = hdr + 4
    _, sz = read_encoded(img, off, ptr_enc, hdr)
    off += sz
    count, sz = read_encoded(img, off, cnt_enc, hdr)
    off += sz
    starts = []
    for _ in range(count):
        loc, sz = read_encoded(img, off, tbl_enc, hdr)
        off += sz
        _, sz = read_encoded(img, off, tbl_enc, hdr)
        off += sz
        starts.append(loc)
    return starts


def _uleb(img, off):
    v = shift = 0
    while True:
        b = img[off]
        off += 1
        v |= (b & 0x7F) << shift
        shift += 7
        if not b & 0x80:
            return v, off


def _sleb(img, off):
    v = shift = 0
    while True:
        b = img[off]
        off += 1
        v |= (b & 0x7F) << shift
        shift += 7
        if not b & 0x80:
            if b & 0x40:
                v -= 1 << shift
            return v, off


def _read_ptr(img, off, enc):
    """A DW_EH_PE-encoded value at off (pc-relative or absolute module offsets; 0xFF: omitted): (value, next off)."""
    if enc == 0xFF:
        return None, off
    fmt, app = enc & 0x0F, enc & 0x70
    if fmt == 0x01:
        v, nxt = _uleb(img, off)
    elif fmt == 0x09:
        v, nxt = _sleb(img, off)
    else:
        size = {0x00: 8, 0x02: 2, 0x03: 4, 0x04: 8, 0x0A: 2, 0x0B: 4, 0x0C: 8}[fmt]
        code = {0x00: '<Q', 0x02: '<H', 0x03: '<I', 0x04: '<Q', 0x0A: '<h', 0x0B: '<i', 0x0C: '<q'}[fmt]
        v, nxt = struct.unpack_from(code, img, off)[0], off + size
    if app == 0x10:  # pcrel
        v += off
    return v, nxt


def landing_pads(n):
    """{function start: (its end, [landing pad, ...])} from the .eh_frame FDEs' LSDAs (.gcc_except_table call sites).

    The exception handlers and cleanups the unwinder resumes at (runtime: rt_eh_resume): each is reached only through
    the unwinder, never by a branch, so the generator gives them labels and a resume switch of their own."""
    img, hdr = n.image, n.eh_start
    if not hdr or hdr >= len(img):
        return {}
    _, ptr_enc, cnt_enc, tbl_enc = img[hdr:hdr + 4]
    off = hdr + 4
    _, sz = read_encoded(img, off, ptr_enc, hdr)
    off += sz
    count, sz = read_encoded(img, off, cnt_enc, hdr)
    off += sz
    cies = {}  # CIE offset -> (augmentation string, its data: the FDE pointer and LSDA encodings)
    out = {}
    for _ in range(count):
        _, sz = read_encoded(img, off, tbl_enc, hdr)
        off += sz
        fde, sz = read_encoded(img, off, tbl_enc, hdr)
        off += sz
        try:
            length = struct.unpack_from('<I', img, fde)[0]
            if length in (0, 0xFFFFFFFF):
                continue
            cie = fde + 4 - struct.unpack_from('<I', img, fde + 4)[0]
            if cie not in cies:
                p = cie + 8  # (length, id)
                p += 1  # version
                aug_end = img.index(b'\0', p)
                aug = bytes(img[p:aug_end]).decode('latin1')
                p = aug_end + 1
                _, p = _uleb(img, p)  # code alignment
                _, p = _sleb(img, p)  # data alignment
                _, p = _uleb(img, p)  # return address register
                fde_enc, lsda_enc = 0x00, 0xFF
                if aug.startswith('z'):
                    _, p = _uleb(img, p)  # augmentation length
                    for ch in aug[1:]:
                        if ch == 'L':
                            lsda_enc = img[p]
                            p += 1
                        elif ch == 'R':
                            fde_enc = img[p]
                            p += 1
                        elif ch == 'P':
                            penc = img[p]
                            _, p = _read_ptr(img, p + 1, penc)
                        elif ch in 'SB':
                            pass
                        else:
                            break
                cies[cie] = (aug, fde_enc, lsda_enc)
            aug, fde_enc, lsda_enc = cies[cie]
            if 'L' not in aug or lsda_enc == 0xFF:
                continue
            p = fde + 8
            start, p = _read_ptr(img, p, fde_enc)
            length, p = _read_ptr(img, p, fde_enc & 0x0F)  # range (no application)
            _, p = _uleb(img, p)  # augmentation length
            lsda, p = _read_ptr(img, p, lsda_enc)
            if not lsda:
                continue
            # the LSDA: LPStart (default: the function's start), TType, the call-site table
            q = lsda
            lp_enc = img[q]
            q += 1
            lp_start = start
            if lp_enc != 0xFF:
                lp_start, q = _read_ptr(img, q, lp_enc)
            tt_enc = img[q]
            q += 1
            if tt_enc != 0xFF:
                _, q = _uleb(img, q)
            cs_enc = img[q]
            q += 1
            cs_len, q = _uleb(img, q)
            end = q + cs_len
            pads = set()
            while q < end:
                _, q = _read_ptr(img, q, cs_enc & 0x0F)  # call site start (relative to the function)
                _, q = _read_ptr(img, q, cs_enc & 0x0F)  # its length
                lp, q = _read_ptr(img, q, cs_enc & 0x0F)  # its landing pad (0: none)
                _, q = _uleb(img, q)  # action
                if lp:
                    pads.add(lp_start + lp)
            if pads:
                out[start] = (start + length, sorted(pads))
        except (IndexError, KeyError, ValueError, struct.error):
            continue
    return out


def discover(n):
    text_lo, text_hi = n.text[0], n.text[0] + n.text[1]
    in_text = lambda a: text_lo <= a < text_hi and a % 4 == 0
    funcs = {}
    for a in eh_frame_functions(n):
        if in_text(a):
            funcs[a] = 'eh'
    for name, info, shndx, value, size in n.syms:
        if shndx and in_text(value) and (info & 0xF) == 2:
            funcs.setdefault(value, 'sym')
    # Code pointers stored in data (vtables, callbacks)
    ptr_targets = set()
    for off, typ, sym, add in n.relocs:
        if typ == R_AARCH64_RELATIVE and in_text(add):
            ptr_targets.add(add)
    for a in ptr_targets:
        funcs.setdefault(a, 'ptr')
    # BL targets
    code = bytes(n.image[text_lo:text_hi])
    words = struct.unpack(f'<{len(code) // 4}I', code)
    for i, w in enumerate(words):
        if (w & 0xFC000000) == 0x94000000:
            imm = w & 0x03FFFFFF
            if imm & 0x02000000:
                imm -= 0x04000000
            t = text_lo + i * 4 + imm * 4
            if in_text(t):
                funcs.setdefault(t, 'bl')
    return funcs, words


def census(n, words):
    md = capstone.Cs(capstone.CS_ARCH_ARM64, capstone.CS_MODE_ARM)
    c = Counter()
    bad = 0
    code = struct.pack(f'<{len(words)}I', *words)
    off = 0
    while off < len(code):
        got = False
        for ins in md.disasm_lite(code[off:], n.text[0] + off):
            c[ins[2]] += 1
            off += 4
            got = True
        if off < len(code):   # undecodable word: skip it
            bad += 1
            off += 4
    return c, bad


if __name__ == '__main__':
    n = Nso(sys.argv[1])
    funcs, words = discover(n)
    print(f'functions: {len(funcs)}  by source: {dict(Counter(funcs.values()))}')
    if '--census' in sys.argv:
        c, bad = census(n, words)
        print(f'instructions: {sum(c.values())}  undecodable words: {bad}  distinct mnemonics: {len(c)}')
        print(' '.join(f'{m}:{k}' for m, k in c.most_common()))
