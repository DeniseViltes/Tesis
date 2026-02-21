import numpy as np
import re

STEP_PREFIX = "Step Information:"


def _guess_delimiter(path: str, max_lines: int = 200) -> str | None:
    """Infer delimiter from early lines, favoring consistent multi-column parsing.

    Returns ',', ';', '\t', or None (whitespace split).
    """
    lines: list[str] = []
    with open(path, "r", encoding="utf-8", errors="replace") as f:
        for i, line in enumerate(f):
            if i >= max_lines:
                break
            s = line.strip()
            if not s or s.startswith(STEP_PREFIX):
                continue
            lines.append(s)

    if not lines:
        return None

    candidates = ["\t", ";", ","]
    best_delim: str | None = None
    best_score = -1.0

    for delim in candidates:
        counts = []
        for line in lines:
            if delim not in line:
                continue
            n = len([p for p in line.split(delim) if p.strip()])
            if n >= 2:
                counts.append(n)

        if not counts:
            continue

        mode_cols = max(set(counts), key=counts.count)
        stable = sum(1 for c in counts if c == mode_cols)
        score = stable / len(lines)

        # Prefer delimiters that parse >= 2 columns in many lines,
        # and with a stable column count. This avoids picking ',' when
        # decimal commas appear inside ';' separated files.
        if mode_cols >= 2 and score > best_score:
            best_score = score
            best_delim = delim

    return best_delim


def _split_fields(s: str, delimiter: str | None) -> list[str]:
    s = s.strip()
    if not s:
        return []
    if delimiter is None:
        return s.split()
    return [p.strip() for p in s.split(delimiter) if p.strip()]


def _parse_num(token: str, delimiter: str | None) -> float:
    t = token.strip()
    # decimal comma support for ';' or whitespace separated files
    if delimiter != ",":
        t = t.replace(",", ".")
    return float(t)

def _read_header(path: str, line_idx: int | None = None, delimiter: str | None = None) -> tuple[str, tuple[str, ...]]:
    # If line_idx is given, that line is treated as header; otherwise first non-empty line.
    with open(path, "r", encoding="utf-8", errors="replace") as f:
        header = ""
        if line_idx is None:
            for line in f:
                if line.strip():
                    header = line.strip()
                    break
        else:
            for i, line in enumerate(f):
                if i == line_idx:
                    header = line.strip()
                    break

    fields = _split_fields(header, delimiter)
    colnames = tuple(fields)
    return header, colnames


def _detect_skip_header_auto(path: str, max_lines: int = 200) -> int:
    """Find the first *numeric* row (ignoring Step Information lines) and return how many lines to skip."""
    delim = _guess_delimiter(path, max_lines=max_lines)

    def is_numeric_row(s: str) -> bool:
        s = s.strip()
        if not s:
            return False
        if s.startswith(STEP_PREFIX):
            return False
        parts = _split_fields(s, delim)
        if len(parts) < 2:
            return False
        try:
            # consider it numeric if first two fields parse
            _parse_num(parts[0], delim)
            _parse_num(parts[1], delim)
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
    delim = _guess_delimiter(path)
    if skip_header == "auto":
        skip_header = _detect_skip_header_auto(path)

    header_idx = max(int(skip_header) - 1, 0)
    header, colnames = _read_header(path, line_idx=header_idx, delimiter=delim)
    ncols = len(colnames) if colnames else None

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

            parts = _split_fields(s, delim)
            if ncols is None:
                ncols = len(parts)

            if len(parts) != ncols:
                # ignore malformed / non-data lines
                continue

            try:
                cur_rows.append([_parse_num(p, delim) for p in parts])
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
    delim = _guess_delimiter(path)

    if skip_header == "auto":
        skip_header = _detect_skip_header_auto(path)

    header_idx = max(int(skip_header) - 1, 0)
    header, colnames = _read_header(path, line_idx=header_idx, delimiter=delim)

    rows: list[list[float]] = []
    ncols: int | None = None
    with open(path, "r", encoding="utf-8", errors="replace") as f:
        for i, line in enumerate(f):
            if i < int(skip_header):
                continue
            if line.lstrip().startswith(STEP_PREFIX):
                continue
            if not line.strip():
                continue
            parts = _split_fields(line, delim)
            if len(parts) < 2:
                continue
            try:
                parsed = [_parse_num(p, delim) for p in parts]
            except ValueError:
                continue

            if ncols is None:
                ncols = len(parsed)
            if len(parsed) != ncols:
                continue
            rows.append(parsed)

    data = np.asarray(rows, dtype=float)

    if data.ndim != 2 or data.shape[1] < 2:
        raise ValueError("No pude leer 2 o más columnas numéricas. Revisá separador y header.")

    if colnames and len(colnames) != data.shape[1]:
        colnames = tuple(f"col_{i}" for i in range(data.shape[1]))

    return data, header, colnames


def read_ltspice_2col(path: str, skip_header: int | str = 1):
    data, header, colnames = read_ltspice_table(path, skip_header)
    return data[:, 0], data[:, 1], header, colnames
