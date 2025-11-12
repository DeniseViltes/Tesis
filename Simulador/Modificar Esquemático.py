#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
LTspice .asc flag editor (GUI) with:
- Bundle controls (All / Row / Column) for RXY_1
- Automatic opposite flags for RXY_6 (VcON<->VcOFF, VcMAS<->VcMENOS)
- Automatic RX_9 per row:
    If all RXY_6 (Y=1..5) in the row share the same flag -> RX_9 copies it
    else -> RX_9 = VcOFF (default)

Run directly from your IDE (press Run). No console args needed.
"""

from __future__ import annotations
import re
from pathlib import Path
from typing import Dict, List, Tuple, Optional

import tkinter as tk
from tkinter import ttk, filedialog, messagebox

# ---------------- Config / helpers ----------------

ALLOWED_FLAGS = {"VcON", "VcOFF", "VcMAS", "VcMENOS"}
FLAGS_ORDERED = ["VcON", "VcOFF", "VcMAS", "VcMENOS"]
OPPOSITE = {"VcON": "VcOFF", "VcOFF": "VcON", "VcMAS": "VcMENOS", "VcMENOS": "VcMAS"}
ROW9_DEFAULT = "VcOFF"

# Match RXY_1, RXY_6 and RX_9
RES_NAME_RE = re.compile(r"^SYMATTR\s+InstName\s+(R([1-5])(?:(?:([1-5])_(1|6))|(?:_9)))\s*$")
#   Groups: full name, row=r, (maybe col=y and kind=1|6) or _9 w/o col
SYMBOL_RES_RE = re.compile(r"^SYMBOL\s+res\s+(-?\d+)\s+(-?\d+)\s+(\w+)\s*$")
FLAG_RE = re.compile(r"^FLAG\s+(-?\d+)\s+(-?\d+)\s+(\S+)\s*$")

def load_lines(path: Path) -> List[str]:
    return path.read_text(encoding="utf-8", errors="replace").splitlines()

def save_lines(path: Path, lines: List[str]) -> None:
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")

def find_flags(lines: List[str]) -> List[Tuple[int, int, int, str]]:
    """Return list of (line_index, x, y, name)."""
    out = []
    for i, line in enumerate(lines):
        m = FLAG_RE.match(line)
        if not m:
            continue
        x, y, name = int(m.group(1)), int(m.group(2)), m.group(3)
        out.append((i, x, y, name))
    return out

def find_resistors(lines: List[str]) -> Dict[str, Tuple[int, int, int, str, int]]:
    """
    Find RXY_1, RXY_6 and RX_9 resistors.
    Returns: inst_name -> (symbol_line_idx, sx, sy, orient, inst_line_idx)
    """
    res_positions: Dict[str, Tuple[int,int,int,str,int]] = {}
    for i, line in enumerate(lines):
        m = RES_NAME_RE.match(line)
        if not m:
            continue
        inst = m.group(1)  # e.g., R23_1, R23_6, R2_9
        sym = None
        # try a small backward window near the InstName
        for j in range(i - 1, max(-1, i - 60), -1):
            sm = SYMBOL_RES_RE.match(lines[j])
            if sm:
                sx, sy, orient = int(sm.group(1)), int(sm.group(2)), sm.group(3)
                sym = (j, sx, sy, orient)
                break
        if sym is None:
            # fallback: wider search
            for j in range(0, i):
                sm = SYMBOL_RES_RE.match(lines[j])
                if sm:
                    sx, sy, orient = int(sm.group(1)), int(sm.group(2)), sm.group(3)
                    sym = (j, sx, sy, orient)
                    break
        if sym is None:
            raise RuntimeError(f"Could not find SYMBOL line for {inst} near line {i}.")
        res_positions[inst] = (sym[0], sym[1], sym[2], sym[3], i)
    return res_positions

def nearest_allowed_flag(sx: int, sy: int, flags: List[Tuple[int,int,int,str]]) -> Tuple[int,int,int,str,int]:
    """
    Among allowed flags only, return the nearest in Manhattan distance.
    Returns (line_idx, fx, fy, name, manhattan_distance)
    """
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
        raise RuntimeError("No allowed flags found in the schematic.")
    return best

def apply_mapping(
    lines: List[str],
    res_positions: Dict[str, Tuple[int,int,int,str,int]],
    flags: List[Tuple[int,int,int,str]],
    mapping: Dict[str, str],
    max_snap: Optional[int] = None
) -> List[str]:
    """
    Produce a modified copy of 'lines' where each instance's nearest allowed FLAG
    is renamed to the desired flag.
    """
    out = list(lines)
    for inst, desired in mapping.items():
        if desired not in ALLOWED_FLAGS:
            raise ValueError(f"Mapping for {inst} -> '{desired}' is not one of {sorted(ALLOWED_FLAGS)}")
        if inst not in res_positions:
            continue
        _, sx, sy, _, _ = res_positions[inst]
        li, fx, fy, _curr, dist = nearest_allowed_flag(sx, sy, flags)
        if max_snap is not None and dist > max_snap:
            raise RuntimeError(
                f"No nearby allowed flag for {inst}: nearest distance={dist} exceeds snap limit {max_snap}."
            )
        out[li] = f"FLAG {fx} {fy} {desired}"
    return out

# ---------------- GUI ----------------

class App(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("LTspice Flag Editor — RXY_1 bundles + RXY_6 opposites + RX_9 per row")
        self.geometry("1120x740")
        self.minsize(1060, 700)

        self.lines: Optional[List[str]] = None
        self.flags: Optional[List[Tuple[int,int,int,str]]] = None
        self.res_positions: Optional[Dict[str, Tuple[int,int,int,str,int]]] = None
        self.input_path: Optional[Path] = None

        # Top bar
        top = ttk.Frame(self, padding=8)
        top.pack(fill="x")

        ttk.Button(top, text="Open .asc…", command=self.on_open).pack(side="left")
        ttk.Label(top, text="Snap (max distance):").pack(side="left", padx=(12, 4))
        self.snap_var = tk.StringVar(value="")
        ttk.Entry(top, textvariable=self.snap_var, width=8).pack(side="left")

        self.save_btn = ttk.Button(top, text="Save edited .asc…", command=self.on_save, state="disabled")
        self.save_btn.pack(side="right")

        # Info
        self.info_var = tk.StringVar(value="Open a schematic to begin.")
        ttk.Label(self, textvariable=self.info_var, padding=8).pack(fill="x")

        # Main content: left = table; right = bundle + RX_9 preview
        main = ttk.Frame(self)
        main.pack(fill="both", expand=True, padx=8, pady=8)
        main.columnconfigure(0, weight=3)
        main.columnconfigure(1, weight=1)
        main.rowconfigure(0, weight=1)

        # Left: scrollable table (RXY_1 controls)
        left = ttk.Frame(main)
        left.grid(row=0, column=0, sticky="nsew", padx=(0,8))
        self.canvas = tk.Canvas(left)
        self.scroll_y = ttk.Scrollbar(left, orient="vertical", command=self.canvas.yview)
        self.scroll_x = ttk.Scrollbar(left, orient="horizontal", command=self.canvas.xview)
        self.inner = ttk.Frame(self.canvas)
        self.inner.bind("<Configure>", lambda e: self.canvas.configure(scrollregion=self.canvas.bbox("all")))
        self.canvas.create_window((0, 0), window=self.inner, anchor="nw")
        self.canvas.configure(yscrollcommand=self.scroll_y.set, xscrollcommand=self.scroll_x.set)
        self.canvas.grid(row=0, column=0, sticky="nsew")
        self.scroll_y.grid(row=0, column=1, sticky="ns")
        self.scroll_x.grid(row=1, column=0, sticky="ew")
        left.rowconfigure(0, weight=1)
        left.columnconfigure(0, weight=1)

        # Table headers
        header = ttk.Frame(self.inner, padding=(0, 0, 0, 6))
        header.grid(row=0, column=0, sticky="w")
        ttk.Label(header, text="Resistor (RXY_1)", width=16).grid(row=0, column=0, sticky="w")
        ttk.Label(header, text="Current (nearest flag)", width=28).grid(row=0, column=1, sticky="w")
        ttk.Label(header, text="Desired flag", width=18).grid(row=0, column=2, sticky="w")
        ttk.Label(header, text="Distance", width=10).grid(row=0, column=3, sticky="w")
        ttk.Label(header, text="→ Will set RXY_6 to", width=24).grid(row=0, column=4, sticky="w")

        # Right: bundle controls + RX_9 preview
        right = ttk.LabelFrame(main, text="Bundle apply & RX_9 preview", padding=10)
        right.grid(row=0, column=1, sticky="nsew")
        right.columnconfigure(0, weight=1)
        right.columnconfigure(1, weight=1)

        # All
        ttk.Label(right, text="All (RXX_1):").grid(row=0, column=0, sticky="w")
        self.all_var = tk.StringVar(value=FLAGS_ORDERED[0])
        ttk.Combobox(right, textvariable=self.all_var, values=FLAGS_ORDERED, state="readonly", width=12)\
            .grid(row=0, column=1, sticky="w")
        ttk.Button(right, text="Apply", command=self.apply_all).grid(row=0, column=2, padx=(6,0))

        # Rows
        ttk.Separator(right, orient="horizontal").grid(row=1, column=0, columnspan=3, sticky="ew", pady=6)
        ttk.Label(right, text="Rows (R1X_1 … R5X_1)").grid(row=2, column=0, columnspan=3, sticky="w")

        self.row_vars: Dict[int, tk.StringVar] = {}
        rstart = 3
        for r in range(1, 6):
            ttk.Label(right, text=f"Row {r} (R{r}X_1):").grid(row=rstart + (r-1), column=0, sticky="w", pady=2)
            var = tk.StringVar(value=FLAGS_ORDERED[0])
            self.row_vars[r] = var
            ttk.Combobox(right, textvariable=var, values=FLAGS_ORDERED, state="readonly", width=12)\
                .grid(row=rstart + (r-1), column=1, sticky="w")
            ttk.Button(right, text="Apply", command=lambda rr=r: self.apply_row(rr))\
                .grid(row=rstart + (r-1), column=2, padx=(6,0))

        # Columns
        crow = rstart + 5 + 1
        ttk.Separator(right, orient="horizontal").grid(row=crow, column=0, columnspan=3, sticky="ew", pady=6)
        ttk.Label(right, text="Columns (RX1_1 … RX5_1)").grid(row=crow+1, column=0, columnspan=3, sticky="w")

        self.col_vars: Dict[int, tk.StringVar] = {}
        for c in range(1, 6):
            base = crow + 2 + (c-1)
            ttk.Label(right, text=f"Col {c} (RX{c}_1):").grid(row=base, column=0, sticky="w", pady=2)
            var = tk.StringVar(value=FLAGS_ORDERED[0])
            self.col_vars[c] = var
            ttk.Combobox(right, textvariable=var, values=FLAGS_ORDERED, state="readonly", width=12)\
                .grid(row=base, column=1, sticky="w")
            ttk.Button(right, text="Apply", command=lambda cc=c: self.apply_col(cc))\
                .grid(row=base, column=2, padx=(6,0))

        # RX_9 live preview
        r9row = base + 2
        ttk.Separator(right, orient="horizontal").grid(row=r9row, column=0, columnspan=3, sticky="ew", pady=8)
        ttk.Label(right, text="RX_9 (auto per row)").grid(row=r9row+1, column=0, columnspan=3, sticky="w", pady=(0,4))

        self.row9_vars: Dict[int, tk.StringVar] = {}
        for r in range(1, 6):
            ttk.Label(right, text=f"R{r}_9 →").grid(row=r9row+1+r, column=0, sticky="w", pady=2)
            var = tk.StringVar(value=ROW9_DEFAULT)
            self.row9_vars[r] = var
            ttk.Label(right, textvariable=var).grid(row=r9row+1+r, column=1, sticky="w", pady=2)

        # Populated after loading:
        # name1 -> {"desired": StringVar, "opp_text": tk.StringVar}
        self.row_widgets: Dict[str, Dict[str, object]] = {}

    # ---------- Bundle helpers ----------
    @staticmethod
    def _name_to_row_col(name: str) -> Tuple[int,int]:
        # Expect "Rxy_1" where x,y are digits 1..5
        return int(name[1]), int(name[2])

    def _update_opp_label(self, name: str):
        vars_dict = self.row_widgets.get(name)
        if not vars_dict: return
        desired = vars_dict["desired"].get()
        opp = OPPOSITE.get(desired, "?")
        vars_dict["opp_text"].set(opp)
        # any change may affect RX_9 preview
        self.recompute_row9_preview()

    def apply_all(self):
        val = self.all_var.get()
        for name, vars_dict in self.row_widgets.items():
            vars_dict["desired"].set(val)
        self.recompute_row9_preview()

    def apply_row(self, row_idx: int):
        val = self.row_vars[row_idx].get()
        for name, vars_dict in self.row_widgets.items():
            try:
                r, _c = self._name_to_row_col(name)
            except Exception:
                continue
            if r == row_idx:
                vars_dict["desired"].set(val)
        self.recompute_row9_preview()

    def apply_col(self, col_idx: int):
        val = self.col_vars[col_idx].get()
        for name, vars_dict in self.row_widgets.items():
            try:
                _r, c = self._name_to_row_col(name)
            except Exception:
                continue
            if c == col_idx:
                vars_dict["desired"].set(val)
        self.recompute_row9_preview()

    # ---------- RX_9 computation ----------
    def compute_row9_from_current(self, row_idx: int) -> str:
        """
        From current UI selections (RXY_1), derive RXY_6 via opposite,
        then apply row rule:
          if all RXY_6 equal -> RX_9 = that flag
          else -> RX_9 = ROW9_DEFAULT
        If any of the 5 cells is missing in the schematic, we treat as not unanimous.
        """
        values6 = []
        for y in range(1, 6):
            name1 = f"R{row_idx}{y}_1"
            if name1 not in self.row_widgets:
                return ROW9_DEFAULT  # missing → not unanimous
            desired1 = self.row_widgets[name1]["desired"].get()
            opp = OPPOSITE.get(desired1)
            if opp not in ALLOWED_FLAGS:
                return ROW9_DEFAULT
            # also require the matching RXY_6 to exist physically to be eligible
            name6 = f"R{row_idx}{y}_6"
            if not (self.res_positions and name6 in self.res_positions):
                return ROW9_DEFAULT
            values6.append(opp)
        # now check unanimity
        if len(values6) == 5 and all(v == values6[0] for v in values6):
            return values6[0]
        return ROW9_DEFAULT

    def recompute_row9_preview(self):
        for r in range(1, 6):
            self.row9_vars[r].set(self.compute_row9_from_current(r))

    # ---------- File actions ----------
    def on_open(self):
        path = filedialog.askopenfilename(
            title="Open LTspice schematic",
            filetypes=[("LTspice schematic", "*.asc"), ("All files", "*.*")]
        )
        if not path:
            return
        try:
            self.input_path = Path(path)
            self.lines = load_lines(self.input_path)
            self.flags = find_flags(self.lines)
            self.res_positions = find_resistors(self.lines)  # includes _1, _6, and _9
        except Exception as e:
            messagebox.showerror("Error", f"Could not parse .asc:\n{e}")
            return

        # Rebuild table rows for _1 only
        for child in list(self.inner.grid_slaves()):
            if int(child.grid_info().get("row", 0)) > 0:
                child.destroy()
        self.row_widgets.clear()

        row_idx = 1
        count_found_1 = 0
        count_found_6 = 0
        count_found_9 = 0

        for x in range(1, 6):
            for y in range(1, 6):
                name1 = f"R{x}{y}_1"
                name6 = f"R{x}{y}_6"
                if self.res_positions and name6 in self.res_positions:
                    count_found_6 += 1
                if self.res_positions and name1 in self.res_positions:
                    count_found_1 += 1
                    _, sx, sy, _, _ = self.res_positions[name1]
                    li, fx, fy, curr, dist = nearest_allowed_flag(sx, sy, self.flags)

                    frame = ttk.Frame(self.inner)
                    frame.grid(row=row_idx, column=0, sticky="we", pady=2)
                    ttk.Label(frame, text=name1, width=16).grid(row=0, column=0, sticky="w")
                    ttk.Label(frame, text=f"{curr}", width=28).grid(row=0, column=1, sticky="w")

                    var = tk.StringVar(value=curr if curr in ALLOWED_FLAGS else FLAGS_ORDERED[0])
                    cb = ttk.Combobox(frame, textvariable=var, values=FLAGS_ORDERED, width=12, state="readonly")
                    cb.grid(row=0, column=2, sticky="w", padx=(6, 0))

                    ttk.Label(frame, text=str(dist), width=10).grid(row=0, column=3, sticky="w", padx=(6, 0))

                    opp_text = tk.StringVar(value=OPPOSITE.get(var.get(), "?"))
                    ttk.Label(frame, textvariable=opp_text, width=24).grid(row=0, column=4, sticky="w")

                    # when selection changes, update the shown opposite and row9 previews
                    var.trace_add("write", lambda *_args, n=name1: self._update_opp_label(n))

                    self.row_widgets[name1] = {"desired": var, "opp_text": opp_text}
                    row_idx += 1
                else:
                    frame = ttk.Frame(self.inner)
                    frame.grid(row=row_idx, column=0, sticky="we", pady=2)
                    ttk.Label(frame, text=name1, width=16).grid(row=0, column=0, sticky="w")
                    ttk.Label(frame, text="(not found in file)", width=28, foreground="#888").grid(row=0, column=1, sticky="w")
                    cb = ttk.Combobox(frame, values=FLAGS_ORDERED, width=12, state="disabled")
                    cb.grid(row=0, column=2, sticky="w", padx=(6, 0))
                    ttk.Label(frame, text="-", width=10).grid(row=0, column=3, sticky="w", padx=(6, 0))
                    ttk.Label(frame, text="-", width=24).grid(row=0, column=4, sticky="w")
                    row_idx += 1

        # Count existing RX_9s
        for r in range(1, 6):
            if self.res_positions and f"R{r}_9" in self.res_positions:
                count_found_9 += 1

        # Initial RX_9 preview
        self.recompute_row9_preview()

        self.info_var.set(
            f"Loaded: {self.input_path.name} — found RXY_1: {count_found_1}/25, RXY_6: {count_found_6}/25, RX_9: {count_found_9}/5."
        )
        self.save_btn.config(state="normal")

    def on_save(self):
        if not (self.lines and self.flags and self.res_positions and self.input_path):
            messagebox.showwarning("No file", "Please open a schematic first.")
            return

        # Gather mapping from current dropdowns (RXY_1)
        mapping_1: Dict[str, str] = {}
        for name1, vars_dict in self.row_widgets.items():
            desired = vars_dict["desired"].get()
            if desired and desired in ALLOWED_FLAGS:
                mapping_1[name1] = desired

        # Derived mapping for RXY_6 (opposites)
        mapping_6: Dict[str, str] = {}
        for name1, desired in mapping_1.items():
            name6 = name1.replace("_1", "_6")
            opposite = OPPOSITE.get(desired)
            if opposite and name6 in (self.res_positions or {}):
                mapping_6[name6] = opposite

        # Derived mapping for RX_9 per row
        mapping_9: Dict[str, str] = {}
        for r in range(1, 6):
            r9_name = f"R{r}_9"
            if not (self.res_positions and r9_name in self.res_positions):
                continue
            # Build the 5 values actually applied to RrY_6 (only if all 5 _6 exist)
            vals = []
            full_row_present = True
            for y in range(1, 6):
                name6 = f"R{r}{y}_6"
                if name6 not in (self.res_positions or {}):
                    full_row_present = False
                    break
                # derive from selected _1
                name1 = f"R{r}{y}_1"
                if name1 not in self.row_widgets:
                    full_row_present = False
                    break
                desired1 = self.row_widgets[name1]["desired"].get()
                opp = OPPOSITE.get(desired1)
                if opp not in ALLOWED_FLAGS:
                    full_row_present = False
                    break
                vals.append(opp)

            if full_row_present and len(vals) == 5 and all(v == vals[0] for v in vals):
                mapping_9[r9_name] = vals[0]
            else:
                mapping_9[r9_name] = ROW9_DEFAULT

        # Merge all
        mapping_all = {**mapping_1, **mapping_6, **mapping_9}

        # Snap (optional)
        snap_text = self.snap_var.get().strip()
        if snap_text == "":
            snap_val: Optional[int] = None
        else:
            try:
                snap_val = int(snap_text)
                if snap_val < 0:
                    raise ValueError
            except Exception:
                messagebox.showerror("Invalid snap", "Snap must be a non-negative integer or left blank.")
                return

        try:
            modified = apply_mapping(self.lines, self.res_positions, self.flags, mapping_all, max_snap=snap_val)
        except Exception as e:
            messagebox.showerror("Apply error", str(e))
            return

        default_name = f"{self.input_path.stem}__edited.asc"
        out_path = filedialog.asksaveasfilename(
            title="Save edited schematic",
            initialfile=default_name,
            defaultextension=".asc",
            filetypes=[("LTspice schematic", "*.asc"), ("All files", "*.*")]
        )
        if not out_path:
            return
        try:
            save_lines(Path(out_path), modified)
        except Exception as e:
            messagebox.showerror("Save error", f"Could not save file:\n{e}")
            return

        # Summary
        c1 = len(mapping_1)
        c6 = len(mapping_6)
        c9 = len(mapping_9)
        messagebox.showinfo("Done", f"Saved:\n{out_path}")

if __name__ == "__main__":
    try:
        import ctypes
        ctypes.windll.shcore.SetProcessDpiAwareness(1)  # crisp scaling on HiDPI
    except Exception:
        pass
    app = App()
    app.mainloop()