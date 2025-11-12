import subprocess
import re
import pandas as pd
import numpy as np
from itertools import combinations
from pathlib import Path
from PyLTSpice import RawRead

# Importar funciones desde la lógica del esquemático
from logica_de_modificar_esquematico import (
    load_lines, save_lines, find_flags, find_resistors,
    apply_mapping, build_full_mapping
)

# ------------------------------------------------------------
# CONFIGURACIÓN GENERAL
# ------------------------------------------------------------
SIMULADOR   = "ngspice"   # "ltspice" o "ngspice"
DEBUG       = False

BASE_ASC    = Path("Esquematicos/matriz5x5.asc")
LTSPICE_EXE = r"C:\Users\---\AppData\Local\Programs\ADI\LTspice\LTspice.exe"
NGSPICE_EXE = "ngspice"

RESULT_DIR  = Path("resultados/5x5 filas con cambio de R")
RESULT_DIR.mkdir(exist_ok=True)

# ------------------------------------------------------------
# UTILIDADES
# ------------------------------------------------------------
def detectar_dimensiones(base_file: Path):
    lines = base_file.read_text(encoding="utf-8", errors="replace").splitlines()
    resistors = find_resistors(lines)
    filas, cols = set(), set()
    for name in resistors.keys():
        m = re.match(r"R(\d+)(\d+)_1", name)
        if m:
            filas.add(int(m.group(1)))
            cols.add(int(m.group(2)))
    if not filas or not cols:
        raise RuntimeError("No se detectaron resistencias RXY_1 en el archivo base.")
    return max(filas), max(cols)

def mostrar_combinacion(mapping_1, n_rows, n_cols):
    simbolos = {"VcON": "ON", "VcOFF": "OFF", "VcMAS": "+", "VcMENOS": "−"}
    matriz = [["·"] * n_cols for _ in range(n_rows)]
    for k, v in mapping_1.items():
        m = re.match(r"R(\d+)(\d+)_1", k)
        if m:
            i, j = int(m.group(1))-1, int(m.group(2))-1
            matriz[i][j] = simbolos.get(v, "?")
    print("📘 Configuración actual:")
    for fila in matriz:
        print(" ".join(fila))
    print()

# ------------------------------------------------------------
# SIMULACIÓN
# ------------------------------------------------------------
def correr_simulador(netlist_path: Path):
    print(f"▶ Ejecutando {SIMULADOR.upper()} sobre {netlist_path.name}")
    if SIMULADOR.lower() == "ltspice":
        result = subprocess.run([LTSPICE_EXE, "-b", "-Run", str(netlist_path)], capture_output=True, text=True)
        if result.returncode != 0 and DEBUG:
            print("⚠️ LTspice retornó código:", result.returncode)
            print(result.stdout)
            print(result.stderr)
        raw_path = netlist_path.with_suffix(".raw")
        if not raw_path.exists():
            alt_raw = netlist_path.with_name(netlist_path.stem + ".raw")
            if alt_raw.exists():
                raw_path = alt_raw
        if not raw_path.exists():
            raise FileNotFoundError(f"No se generó el archivo .raw para {netlist_path.name}")
        return raw_path

    elif SIMULADOR.lower() == "ngspice":
        log = netlist_path.with_suffix(".log")
        subprocess.run([NGSPICE_EXE, "-b", "-o", str(log), str(netlist_path)], check=False)
        return netlist_path.with_suffix(".raw")

    else:
        raise ValueError("Simulador desconocido (usa 'ltspice' o 'ngspice').")

# ------------------------------------------------------------
# LECTURA DE RESULTADOS
# ------------------------------------------------------------
def leer_resultados(raw_file: Path, n_rows: int, n_cols: int, config_id: int) -> pd.DataFrame:
    raw = RawRead(str(raw_file))
    traces = raw.get_trace_names()

    if "time" not in traces:
        print(f"⚠️ No se encontró la traza de tiempo en {raw_file.name}")
        return pd.DataFrame({"config_id": [config_id]})

    t = raw.get_trace("time").get_wave(0)
    datos = {"config_id": np.full_like(t, config_id, dtype=float), "t_s": t}

    for i in range(1, n_rows + 1):
        for j in range(1, n_cols + 1):
            name = f"I(Vcell{i}{j})"
            if name in traces:
                datos[f"Icell{i}{j}_mA"] = 1e3 * raw.get_trace(name).get_wave(0)

    for tr in traces:
        if "rl" in tr.lower():
            datos["I_RL_mA"] = 1e3 * raw.get_trace(tr).get_wave(0)
            break

    vpos = next((raw.get_trace(t).get_wave(0) for t in traces if "out_pos" in t.lower()), None)
    vneg = next((raw.get_trace(t).get_wave(0) for t in traces if "out_neg" in t.lower()), None)
    if vpos is not None and vneg is not None:
        datos["V_out_diff_V"] = vpos - vneg

    df = pd.DataFrame(datos)
    if df.empty:
        return pd.DataFrame({"config_id": [config_id]})

    if "V_out_diff_V" in df and "I_RL_mA" in df:
        v = df["V_out_diff_V"]
        i = df["I_RL_mA"] / 1000.0
        t = df["t_s"]
        dt = np.gradient(t)
        energia = np.cumsum(v * i * dt) * 1000
        df["E_out_mJ"] = energia

    return df

# ------------------------------------------------------------
# COMBINACIONES
# ------------------------------------------------------------
def generar_combinaciones_auto(base_file: Path):
    n_rows, n_cols = detectar_dimensiones(base_file)
    print(f"🔍 Matriz detectada: {n_rows}×{n_cols}")

    combos = []
    for k_on in range(1, n_rows):
        for rows_on in combinations(range(1, n_rows + 1), k_on):
            mapping_1 = {}
            for i in range(1, n_rows + 1):
                flag = "VcMAS" if i in rows_on else "VcOFF"
                for j in range(1, n_cols + 1):
                    mapping_1[f"R{i}{j}_1"] = flag
            nombre = "MAS_OFF_filas_" + "-".join(str(r) for r in rows_on)
            combos.append((nombre, mapping_1))
    return combos, n_rows, n_cols

# ------------------------------------------------------------
# MAIN
# ------------------------------------------------------------
def main():
    base_file = BASE_ASC
    combos, n_rows, n_cols = generar_combinaciones_auto(base_file)
    resumen = []

    for idx, (nombre, mapping) in enumerate(combos, 1):
        print(f"\n=== Simulación {idx}/{len(combos)} — {nombre} ===")
        mostrar_combinacion(mapping, n_rows, n_cols)
        out_asc = RESULT_DIR / f"{nombre}.asc"
        out_csv = RESULT_DIR / f"{nombre}.csv"

        lines = load_lines(base_file)
        flags = find_flags(lines)
        res_positions = find_resistors(lines)
        mapping_total = build_full_mapping(mapping)

        modified = apply_mapping(lines, res_positions, flags, mapping_total)
        save_lines(out_asc, modified)

        raw = correr_simulador(out_asc)
        df = leer_resultados(raw, n_rows, n_cols, idx)
        df.to_csv(out_csv, index=False)
        print(f"✅ CSV guardado: {out_csv.name}")

        # Limpieza
        for ext in [".asc",".log",".net", ".plt", ".raw", ".op.raw", ".temp", ".out"]:
            temp_file = out_asc.with_suffix(ext)
            if temp_file.exists():
                try:
                    temp_file.unlink()
                except Exception as e:
                    if DEBUG:
                        print(f"⚠️ No se pudo eliminar {temp_file.name}: {e}")

    print(f"\n✅ Todas las simulaciones completadas.\n")

if __name__ == "__main__":
    main()
