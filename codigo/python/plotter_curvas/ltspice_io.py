import numpy as np


def _read_header(path: str) -> tuple[str, tuple[str, ...]]:
    with open(path, "r", encoding="utf-8", errors="replace") as f:
        header = ""
        for line in f:
            if line.strip():
                header = line.strip()
                break

    colnames = tuple(header.split("\t")) if "\t" in header else tuple(header.split())
    return header, colnames


def read_ltspice_table(path: str, skip_header: int = 1):
    header, colnames = _read_header(path)

    data = np.genfromtxt(path, delimiter="\t", skip_header=skip_header)
    if data.ndim == 1 or (data.ndim == 2 and data.shape[1] < 2):
        data = np.genfromtxt(path, delimiter=None, skip_header=skip_header)

    if data.ndim != 2 or data.shape[1] < 2:
        raise ValueError("No pude leer 2 o más columnas numéricas. Revisá separador y header.")

    if colnames and len(colnames) != data.shape[1]:
        colnames = tuple(f"col_{i}" for i in range(data.shape[1]))

    return data, header, colnames


def read_ltspice_2col(path: str, skip_header: int = 1):
    data, header, colnames = read_ltspice_table(path, skip_header)
    return data[:, 0], data[:, 1], header, colnames
