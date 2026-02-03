import numpy as np
import re

STEP_PREFIX = "Step Information:"

def _read_header(path: str) -> tuple[str, tuple[str, ...]]:
    # First non-empty line is treated as header (column names)
    with open(path, "r", encoding="utf-8", errors="replace") as f:
        header = ""
        for line in f:
            if line.strip():
                header = line.strip()
                break
    colnames = tuple(header.split("\t")) if "\t" in header else tuple(header.split())
    return header, colnames


def _detect_skip_header_auto(path: str, max_lines: int = 200) -> int:
    """Find the first *numeric* row (ignoring Step Information lines) and return how many lines to skip."""
    def is_numeric_row(s: str) -> bool:
        s = s.strip()
        if not s:
            return False
        if s.startswith(STEP_PREFIX):
            return False
        parts = s.split("\t") if "\t" in s else s.split()
        if len(parts) < 2:
            return False
        try:
            # consider it numeric if first two fields parse
            float(parts[0].replace(",", "."))
            float(parts[1].replace(",", "."))
            return True
        except Exception:
            return False

    with open(path, "r", encoding="utf-8", errors="replace") as f:
        for i, line in enumerate(f):
            if i >= max_lines:
                break
            if is_numeric_row(line):
                return i
    return 1


def read_ltspice_steps(path: str, skip_header: int | str = "auto"):
    """Read an LTspice export that contains .step blocks.

    Returns:
        header: str
        colnames: tuple[str, ...]
        steps: list[dict] each {"label": str, "data": np.ndarray}
              data is NxM (M = number of columns in header).
    """
    header, colnames = _read_header(path)
    ncols = len(colnames) if colnames else None

    if skip_header == "auto":
        skip_header = _detect_skip_header_auto(path)

    steps: list[dict] = []
    cur_rows: list[list[float]] = []
    cur_label: str | None = None
    saw_any_step = False

    def flush():
        nonlocal cur_rows, cur_label
        if cur_rows:
            steps.append({
                "label": cur_label or "Step",
                "data": np.asarray(cur_rows, dtype=float)
            })
        cur_rows = []
        cur_label = None

    with open(path, "r", encoding="utf-8", errors="replace") as f:
        # skip header lines
        for _ in range(int(skip_header)):
            next(f, None)

        for line in f:
            s = line.strip()
            if not s:
                continue

            if s.startswith(STEP_PREFIX):
                # start of a new step block
                flush()
                # Example: 'Step Information: Rval=500K  (Step: 3/3)'
                cur_label = s[len(STEP_PREFIX):].strip()
                saw_any_step = True
                continue

            parts = s.split("\t") if "\t" in s else s.split()
            if ncols is None:
                ncols = len(parts)

            if len(parts) != ncols:
                # ignore malformed / non-data lines
                continue

            try:
                cur_rows.append([float(p.replace(",", ".")) for p in parts])
            except ValueError:
                continue

    flush()

    # If there were no Step Information lines, treat as "no steps"
    if not saw_any_step:
        return header, colnames, []

    return header, colnames, steps


def read_ltspice_table(path: str, skip_header: int | str = 1):
    """Read an LTspice-exported table (tab/whitespace separated).

    - If the file has 'Step Information' lines, they are ignored (data is concatenated).
    - skip_header can be int or 'auto'.
    """
    header, colnames = _read_header(path)

    if skip_header == "auto":
        skip_header = _detect_skip_header_auto(path)

    # Filter out Step Information lines so numpy doesn't crash on sweeps
    filtered = []
    with open(path, "r", encoding="utf-8", errors="replace") as f:
        for i, line in enumerate(f):
            if i < int(skip_header):
                continue
            if line.lstrip().startswith(STEP_PREFIX):
                continue
            if not line.strip():
                continue
            filtered.append(line)

    data = np.genfromtxt(filtered, delimiter="\t")
    if data.ndim == 1 or (data.ndim == 2 and data.shape[1] < 2):
        data = np.genfromtxt(filtered, delimiter=None)

    if data.ndim != 2 or data.shape[1] < 2:
        raise ValueError("No pude leer 2 o más columnas numéricas. Revisá separador y header.")

    if colnames and len(colnames) != data.shape[1]:
        colnames = tuple(f"col_{i}" for i in range(data.shape[1]))

    return data, header, colnames


def read_ltspice_2col(path: str, skip_header: int | str = 1):
    data, header, colnames = read_ltspice_table(path, skip_header)
    return data[:, 0], data[:, 1], header, colnames
