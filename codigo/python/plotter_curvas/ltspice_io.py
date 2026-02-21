import numpy as np
import re

STEP_PREFIX = "Step Information:"
NUM_RE = re.compile(r"[-+]?(?:\d+\.?\d*|\.\d+)(?:[eE][-+]?\d+)?")


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
    """Parse first numeric value from token.

    Supports values with units like '10000.0us' and decimal comma for non-CSV-comma formats.
    """
    t = token.strip()
    if delimiter != ",":
        t = t.replace(",", ".")

    m = NUM_RE.search(t)
    if not m:
        raise ValueError(f"No numeric value found in token: {token!r}")
    return float(m.group(0))


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
    """Find the first numeric row and return how many lines to skip."""
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


def _mode_len(rows: list[list[float]]) -> int:
    lengths = [len(r) for r in rows if len(r) >= 2]
    if not lengths:
        return 0
    return max(set(lengths), key=lengths.count)


def read_ltspice_steps(path: str, skip_header: int | str = "auto"):
    """Read an LTspice export that contains .step blocks."""
    delim = _guess_delimiter(path)
    if skip_header == "auto":
        skip_header = _detect_skip_header_auto(path)

    header_idx = max(int(skip_header) - 1, 0)
    header, colnames = _read_header(path, line_idx=header_idx, delimiter=delim)

    steps: list[dict] = []
    cur_rows: list[list[float]] = []
    cur_label: str | None = None
    saw_any_step = False

    def flush():
        nonlocal cur_rows, cur_label
        if cur_rows:
            ncols = _mode_len(cur_rows)
            if ncols >= 2:
                filtered = [r for r in cur_rows if len(r) == ncols]
                if filtered:
                    steps.append({
                        "label": cur_label or "Step",
                        "data": np.asarray(filtered, dtype=float)
                    })
        cur_rows = []
        cur_label = None

    with open(path, "r", encoding="utf-8", errors="replace") as f:
        for _ in range(int(skip_header)):
            next(f, None)

        for line in f:
            s = line.strip()
            if not s:
                continue

            if s.startswith(STEP_PREFIX):
                flush()
                cur_label = s[len(STEP_PREFIX):].strip()
                saw_any_step = True
                continue

            parts = _split_fields(s, delim)
            if len(parts) < 2:
                continue

            try:
                cur_rows.append([_parse_num(p, delim) for p in parts])
            except ValueError:
                continue

    flush()

    if not saw_any_step:
        return header, colnames, []

    if steps and colnames and len(colnames) != steps[0]["data"].shape[1]:
        colnames = tuple(f"col_{i}" for i in range(steps[0]["data"].shape[1]))

    return header, colnames, steps


def read_ltspice_table(path: str, skip_header: int | str = 1):
    """Read an LTspice/CSV table and keep only numeric rows."""
    delim = _guess_delimiter(path)

    if skip_header == "auto":
        skip_header = _detect_skip_header_auto(path)

    header_idx = max(int(skip_header) - 1, 0)
    header, colnames = _read_header(path, line_idx=header_idx, delimiter=delim)

    rows: list[list[float]] = []
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
                rows.append([_parse_num(p, delim) for p in parts])
            except ValueError:
                continue

    ncols = _mode_len(rows)
    filtered = [r for r in rows if len(r) == ncols] if ncols >= 2 else []
    data = np.asarray(filtered, dtype=float)

    if data.ndim != 2 or data.shape[1] < 2:
        raise ValueError("No pude leer 2 o más columnas numéricas. Revisá separador y header.")

    if colnames and len(colnames) != data.shape[1]:
        colnames = tuple(f"col_{i}" for i in range(data.shape[1]))

    return data, header, colnames


def read_ltspice_2col(path: str, skip_header: int | str = 1):
    data, header, colnames = read_ltspice_table(path, skip_header)
    return data[:, 0], data[:, 1], header, colnames
