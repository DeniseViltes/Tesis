from __future__ import annotations
import re
from pathlib import Path
from typing import Dict, List, Tuple, Optional

# ---------------- Configuración general ----------------

ALLOWED_FLAGS = {"VcON", "VcOFF", "VcMAS", "VcMENOS"}
FLAGS_ORDERED = ["VcON", "VcOFF", "VcMAS", "VcMENOS"]
OPPOSITE = {"VcON": "VcOFF", "VcOFF": "VcON", "VcMAS": "VcMENOS", "VcMENOS": "VcMAS"}
ROW9_DEFAULT = "VcOFF"

# ---------------- Expresiones regulares ----------------

RES_NAME_RE = re.compile(r"^SYMATTR\s+InstName\s+(R(\d+)(\d+)?_(1|6|9))\s*$")
SYMBOL_RES_RE = re.compile(r"^SYMBOL\s+res\s+(-?\d+)\s+(-?\d+)\s+(\w+)\s*$")
FLAG_RE = re.compile(r"^FLAG\s+(-?\d+)\s+(-?\d+)\s+(\S+)\s*$")

# ---------------- Funciones básicas ----------------

def load_lines(path: Path) -> List[str]:
    return path.read_text(encoding="utf-8", errors="replace").splitlines()

def save_lines(path: Path, lines: List[str]) -> None:
    # Reemplaza caracteres problemáticos y usa una codificación estándar
    clean = []
    for l in lines:
        l = l.replace("µ", "u")      # LTspice usa 'u' para micro
        l = l.replace("�", "u")      # reemplaza símbolo corrupto
        l = l.replace("\ufffd", "u") # por si aparece el carácter U+FFFD
        clean.append(l)

    # Guardar en ANSI compatible con LTspice
    with open(path, "w", encoding="cp1252", errors="ignore") as f:
        f.write("\n".join(clean) + "\n")



def find_flags(lines: List[str]) -> List[Tuple[int, int, int, str]]:
    """Busca las líneas FLAG y devuelve (idx, x, y, nombre)."""
    out = []
    for i, line in enumerate(lines):
        m = FLAG_RE.match(line)
        if m:
            x, y, name = int(m.group(1)), int(m.group(2)), m.group(3)
            out.append((i, x, y, name))
    return out

def find_resistors(lines: List[str]) -> Dict[str, Tuple[int, int, int, str, int]]:
    """
    Encuentra resistencias RXY_1, RXY_6 y RX_9.
    Devuelve: inst_name -> (symbol_line_idx, sx, sy, orient, inst_line_idx)
    """
    res_positions = {}
    for i, line in enumerate(lines):
        m = RES_NAME_RE.match(line)
        if not m:
            continue
        inst = m.group(1)
        sym = None
        # Buscar símbolo "SYMBOL res ..." cercano
        for j in range(i - 1, max(-1, i - 60), -1):
            sm = SYMBOL_RES_RE.match(lines[j])
            if sm:
                sx, sy, orient = int(sm.group(1)), int(sm.group(2)), sm.group(3)
                sym = (j, sx, sy, orient)
                break
        if sym is None:
            for j in range(0, i):
                sm = SYMBOL_RES_RE.match(lines[j])
                if sm:
                    sx, sy, orient = int(sm.group(1)), int(sm.group(2)), sm.group(3)
                    sym = (j, sx, sy, orient)
                    break
        if sym is None:
            raise RuntimeError(f"No se encontró SYMBOL para {inst} cerca de línea {i}.")
        res_positions[inst] = (sym[0], sym[1], sym[2], sym[3], i)
    return res_positions

def nearest_allowed_flag(sx: int, sy: int, flags: List[Tuple[int, int, int, str]]) -> Tuple[int, int, int, str, int]:
    """Devuelve el FLAG válido más cercano (distancia Manhattan)."""
    best = None
    bestd = None
    for li, fx, fy, name in flags:
        if name not in ALLOWED_FLAGS:
            continue
        d = abs(fx - sx) + abs(fy - sy)
        if bestd is None or d < bestd:
            bestd = d
            best = (li, fx, fy, name, d)
    if best is None:
        raise RuntimeError("No se encontraron flags válidos en el esquema.")
    return best

def apply_mapping(
    lines: List[str],
    res_positions: Dict[str, Tuple[int, int, int, str, int]],
    flags: List[Tuple[int, int, int, str]],
    mapping: Dict[str, str],
    max_snap: Optional[int] = None
) -> List[str]:
    """Devuelve copia modificada del esquema aplicando los mapeos de flags."""
    out = list(lines)
    reemplazos = 0
    for inst, desired in mapping.items():
        if desired not in ALLOWED_FLAGS:
            continue
        if inst not in res_positions:
            continue
        _, sx, sy, _, _ = res_positions[inst]
        li, fx, fy, _curr, dist = nearest_allowed_flag(sx, sy, flags)
        if max_snap is not None and dist > max_snap:
            raise RuntimeError(
                f"No hay FLAG cercano para {inst}: distancia={dist} > {max_snap}"
            )
        out[li] = f"FLAG {fx} {fy} {desired}"
        reemplazos += 1
    return out

# ---------------- Lógica de mapeo completa ----------------

def build_full_mapping(mapping_1: Dict[str, str]) -> Dict[str, str]:
    """
    A partir del mapeo de RXY_1:
      - RXY_6 = opuesta de RXY_1
      - R{r}_9 = opuesta de R{r}{y}_1 (por fila)
    """
    mapping_total = mapping_1.copy()

    # Asignar opuestas en RXY_6
    for k, v in list(mapping_1.items()):
        opp = OPPOSITE.get(v)
        if opp:
            mapping_total[k.replace("_1", "_6")] = opp

    # Asignar opuestas en R{r}_9
    filas: Dict[int, List[str]] = {}
    for k in mapping_1.keys():
        m = re.match(r"R(\d+)\d+_1", k)
        if m:
            fila = int(m.group(1))
            filas.setdefault(fila, []).append(k)

    for fila, resistencias in filas.items():
        if resistencias:
            first = resistencias[0]
            v = mapping_1[first]
            opp = OPPOSITE.get(v, ROW9_DEFAULT)
            mapping_total[f"R{fila}_9"] = opp

    return mapping_total
