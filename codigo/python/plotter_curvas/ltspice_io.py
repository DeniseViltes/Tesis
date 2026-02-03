# ltspice_io.py
import numpy as np

STEP_PREFIX = "Step Information:"

def _read_header(path: str) -> tuple[str, tuple[str, ...]]:
    with open(path, "r", encoding="utf-8", errors="replace") as f:
        header = ""
        for line in f:
            if line.strip():
                header = line.strip()
                break
    colnames = tuple(header.split("\t")) if "\t" in header else tuple(header.split())
    return header, colnames

def read_ltspice_steps(path: str):
    """
    Lee un .txt exportado de LTspice con líneas 'Step Information: ...'
    y devuelve:
      - header, colnames
      - steps: lista de dicts: {"label": str, "data": np.ndarray}
    """
    header, colnames = _read_header(path)
    ncols = len(colnames) if colnames else None

    steps = []
    cur_rows = []
    cur_label = None

    def flush():
        nonlocal cur_rows, cur_label
        if cur_rows:
            arr = np.asarray(cur_rows, dtype=float)
            steps.append({"label": cur_label or "Step", "data": arr})
        cur_rows = []
        cur_label = None

    with open(path, "r", encoding="utf-8", errors="replace") as f:
        # saltar header (primera línea no vacía)
        saw_header = False
        for line in f:
            if not saw_header:
                if line.strip():
                    saw_header = True
                continue

            s = line.strip()
            if not s:
                continue

            if s.startswith(STEP_PREFIX):
                # nuevo bloque
                flush()
                cur_label = s[len(STEP_PREFIX):].strip()
                continue

            # parseo numérico robusto (tabs o espacios)
            parts = s.split("\t") if "\t" in s else s.split()
            # si no sabemos ncols, tomamos el primer renglón numérico como referencia
            if ncols is None:
                ncols = len(parts)

            # ignorar cualquier cosa que no tenga la cantidad esperada
            if len(parts) != ncols:
                continue

            try:
                cur_rows.append([float(p) for p in parts])
            except ValueError:
                continue

    flush()
    return header, colnames, steps

def read_ltspice_table(path: str, skip_header: int | str = 1):
    """
    - Sin STEP: devuelve (data, header, colnames) como antes.
    - Con STEP: concatena todos los steps (para compatibilidad) y además
      te deja obtener steps con read_ltspice_steps().
    """
    header, colnames = _read_header(path)

    # "auto" = saltar hasta el primer renglón numérico (ignorando Step Information)
    if skip_header == "auto":
        skip = 0
        with open(path, "r", encoding="utf-8", errors="replace") as f:
            for line in f:
                s = line.strip()
                skip += 1
                if not s:
                    continue
                if s.startswith(STEP_PREFIX):
                    continue
                # ¿parece fila numérica?
                parts = s.split("\t") if "\t" in s else s.split()
                try:
                    [float(p) for p in parts]
                    break
                except Exception:
                    continue
        skip_header = skip - 1  # genfromtxt cuenta desde 0

    # prefiltrar Step Information para que genfromtxt no reviente
    filtered = []
    with open(path, "r", encoding="utf-8", errors="replace") as f:
        for i, line in enumerate(f):
            if i < int(skip_header):
                continue
            if line.lstrip().startswith(STEP_PREFIX):
                continue
            filtered.append(line)

    # genfromtxt sobre el contenido filtrado
    data = np.genfromtxt(filtered, delimiter="\t")
    if data.ndim == 1 or (data.ndim == 2 and data.shape[1] < 2):
        data = np.genfromtxt(filtered, delimiter=None)

    if data.ndim != 2 or data.shape[1] < 2:
        raise ValueError("No pude leer 2 o más columnas numéricas. Revisá separador y header.")

    if colnames and len(colnames) != data.shape[1]:
        colnames = tuple(f"col_{i}" for i in range(data.shape[1]))

    return data, header, colnames


