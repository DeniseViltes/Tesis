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
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")

def find_flags(lines: List[str]) -> List[Tuple[int, int, int, str]]:
    out = []
    for i, line in enumerate(lines):
        m = FLAG_RE.match(line)
        if not m:
            continue
        x, y, name = int(m.group(1)), int(m.group(2)), m.group(3)
        out.append((i, x, y, name))
    return out

def find_resistors(lines: List[str]) -> Dict[str, Tuple[int,int,int,str,int]]:
    """
    Encuentra resistencias RXY_1, RXY_6 y RX_9.
    Devuelve: inst_name -> (symbol_line_idx, sx, sy, orient, inst_line_idx)
    """
    res_positions: Dict[str, Tuple[int,int,int,str,int]] = {}
    for i, line in enumerate(lines):
        m = RES_NAME_RE.match(line)
        if not m:
            continue
        inst = m.group(1)
        sym = None
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

def nearest_allowed_flag(sx: int, sy: int, flags: List[Tuple[int,int,int,str]]) -> Tuple[int,int,int,str,int]:
    """Devuelve el flag permitido más cercano (distancia Manhattan)."""
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
    res_positions: Dict[str, Tuple[int,int,int,str,int]],
    flags: List[Tuple[int,int,int,str]],
    mapping: Dict[str, str],
    max_snap: Optional[int] = None
) -> List[str]:
    """Devuelve copia modificada del esquema aplicando los mapeos de flags."""
    out = list(lines)
    for inst, desired in mapping.items():
        if desired not in ALLOWED_FLAGS and not desired.endswith("_delayed"):
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
    return out

# ---------------- Lógica de mapeo (para usar en Simulador_Matriz) ----------------

def build_full_mapping(mapping_1: Dict[str, str]) -> Dict[str, str]:
    """
    A partir del mapeo de RXY_1:
      - genera los pares _6 con el opuesto
      - genera RX_9 con las reglas extendidas (delayed)
    """
    mapping_total = mapping_1.copy()

    # Crear pares _6 automáticos
    for k, v in list(mapping_1.items()):
        opp = OPPOSITE.get(v)
        if opp:
            mapping_total[k.replace("_1", "_6")] = opp

    # Agrupar por fila para los R?_9
    filas: Dict[int, List[str]] = {}
    for k, v in mapping_total.items():
        m = re.match(r"R(\d+)\d+_6", k)
        if m:
            fila = int(m.group(1))
            filas.setdefault(fila, []).append(v)

    for fila, vals in filas.items():
        k9 = f"R{fila}_9"
        if len(vals) == 5 and all(val == vals[0] for val in vals):
            base = vals[0]
            if base == "VcMAS":
                mapping_total[k9] = "VcMAS_delayed"
            elif base == "VcMENOS":
                mapping_total[k9] = "VcMENOS_delayed"
            else:
                mapping_total[k9] = base
        else:
            mapping_total[k9] = ROW9_DEFAULT

    return mapping_total
