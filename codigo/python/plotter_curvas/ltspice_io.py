import numpy as np

def read_ltspice_2col(path: str, skip_header: int = 1):
    with open(path, "r", encoding="utf-8", errors="replace") as f:
        header = ""
        for line in f:
            if line.strip():
                header = line.strip()
                break

    colnames = tuple(header.split("\t")) if "\t" in header else tuple(header.split())

    data = np.genfromtxt(path, delimiter="\t", skip_header=skip_header)
    if data.ndim == 1 or (data.ndim == 2 and data.shape[1] < 2):
        data = np.genfromtxt(path, delimiter=None, skip_header=skip_header)

    if data.ndim != 2 or data.shape[1] < 2:
        raise ValueError("No pude leer 2 columnas numéricas. Revisá separador y header.")

    return data[:, 0], data[:, 1], header, colnames
