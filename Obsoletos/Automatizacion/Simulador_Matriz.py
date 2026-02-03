# Simulador_Matriz.py

import subprocess
from itertools import combinations
from pathlib import Path
import re
import pandas as pd
import numpy as np
from PyLTSpice import RawRead

from Modificar_Esquematico_logica import (
    load_lines, save_lines, find_flags, find_resistors,
    apply_mapping, build_full_mapping
)

# ------------------------------------------------------------
# CONFIGURACIÓN GENERAL
# ------------------------------------------------------------
SIMULADOR   = "ltspice"   # "ltspice" o "ngspice"
DEBUG       = False

# Ruta de tu base (esquemático .asc con .tran y las flags definidas)
BASE_ASC    = Path("matriz_base_con_delay.asc")
BASE_CIR    = Path("matriz_base.cir")  # por si en el futuro usás .cir

# Ruta del ejecutable de LTspice (AJUSTAR A TU USUARIO)
LTSPICE_EXE = r"C:\Users\---\AppData\Local\Programs\ADI\LTspice\LTspice.exe"
# Si usás ngspice:
NGSPICE_EXE = "ngspice"

RESULT_DIR  = Path("resultados")
RESULT_DIR.mkdir(exist_ok=True)

# ------------------------------------------------------------
# UTILIDADES
# ------------------------------------------------------------
def detectar_dimensiones(base_file: Path):
    """
    Detecta cantidad de filas/columnas buscando resistencias RXY_1.
    Requiere que find_resistors() devuelva claves tipo 'R14_1', etc.
    """
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
    """Imprime matriz con ON/OFF/+/- a modo de vista rápida."""
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
    """Ejecuta el simulador y devuelve la ruta al .raw generado."""
    print(f"▶ Ejecutando {SIMULADOR.upper()} sobre {netlist_path.name}")
    if SIMULADOR.lower() == "ltspice":
        # Ejecuta LTspice en modo batch
        result = subprocess.run([LTSPICE_EXE, "-b", "-Run", str(netlist_path)], capture_output=True, text=True)
        if result.returncode != 0 and DEBUG:
            print("⚠️ LTspice retornó código:", result.returncode)
            print(result.stdout)
            print(result.stderr)
        raw_path = netlist_path.with_suffix(".raw")
        if not raw_path.exists():
            # Intento alternativo (algunas instalaciones guardan al lado del .asc original)
            alt_raw = netlist_path.with_name(netlist_path.stem + ".raw")
            if alt_raw.exists():
                raw_path = alt_raw
        if not raw_path.exists():
            raise FileNotFoundError(f"No se generó el archivo .raw para {netlist_path.name}")
        return raw_path

    elif SIMULADOR.lower() == "ngspice":
        log = netlist_path.with_suffix(".log")
        result = subprocess.run([NGSPICE_EXE, "-b", "-o", str(log), str(netlist_path)], capture_output=True, text=True)
        if result.returncode != 0 and DEBUG:
            print("⚠️ ngspice retornó código:", result.returncode)
            print(result.stdout)
            print(result.stderr)
        raw_path = netlist_path.with_suffix(".raw")
        if not raw_path.exists() and log.exists() and DEBUG:
            print("📄 Log ngspice:")
            print(log.read_text(errors="replace"))
        return raw_path

    else:
        raise ValueError("Simulador desconocido (usa 'ltspice' o 'ngspice').")

# ------------------------------------------------------------
# LECTURA DE RESULTADOS
# ------------------------------------------------------------
def leer_resultados(raw_file: Path, n_rows: int, n_cols: int) -> pd.DataFrame:
    """Devuelve DataFrame con corrientes en mA y tensiones en V."""
    raw = RawRead(str(raw_file))
    traces = raw.get_trace_names()

    datos = {}
    # Corrientes de celdas
    for i in range(1, n_rows + 1):
        for j in range(1, n_cols + 1):
            name = f"I(Vcell{i}{j})"
            if name in traces:
                datos[name] = 1e3 * raw.get_trace(name).get_wave(0)  # mA

    # Corriente en la carga RL (detecta por nombre)
    for tr in traces:
        if "rl" in tr.lower():  # ej. I(RL), I(Rl), etc.
            datos["I_RL_mA"] = 1e3 * raw.get_trace(tr).get_wave(0)
            break

    # Tensión diferencial de salida
    vpos = next((raw.get_trace(t).get_wave(0) for t in traces if "out_pos" in t.lower()), None)
    vneg = next((raw.get_trace(t).get_wave(0) for t in traces if "out_neg" in t.lower()), None)
    if vpos is not None and vneg is not None:
        datos["V_out_diff"] = vpos - vneg

    # Tiempo si existiera (no siempre hace falta guardarlo si no lo querés)
    if "time" in traces:
        datos["t"] = raw.get_trace("time").get_wave(0)

    return pd.DataFrame(datos)

# ------------------------------------------------------------
# COMBINACIONES
# ------------------------------------------------------------
def generar_combinaciones_auto(base_file: Path):
    """
    Devuelve:
      combos: List[Tuple[str, dict]]  -> (nombre, mapping_1)
      n_rows, n_cols
    Donde 'nombre' indica las filas encendidas (p.ej. 'ON_rows_1-3').
    """
    n_rows, n_cols = detectar_dimensiones(base_file)
    print(f"🔍 Matriz detectada: {n_rows}×{n_cols}")

    combos = []

    # 1 .. (n_rows-1) filas encendidas
    for k_on in range(1, n_rows):
        for rows_on in combinations(range(1, n_rows + 1), k_on):
            mapping_1 = {}
            for i in range(1, n_rows + 1):
                flag = "VcMAS" if i in rows_on else "VcMENOS"
                for j in range(1, n_cols + 1):
                    mapping_1[f"R{i}{j}_1"] = flag

            nombre = "ON_rows_" + "-".join(str(r) for r in rows_on)
            combos.append((nombre, mapping_1))

    return combos, n_rows, n_cols

# ------------------------------------------------------------
# RESUMEN
# ------------------------------------------------------------
def resumen_resultados(df: pd.DataFrame, nombre: str):
    out = {"config": nombre}
    if "I_RL_mA" in df:
        out["I_RL_max_mA"]  = float(np.max(np.abs(df["I_RL_mA"])))
        out["I_RL_mean_mA"] = float(np.mean(df["I_RL_mA"]))
    if "V_out_diff" in df:
        out["V_out_max_V"]  = float(np.max(np.abs(df["V_out_diff"])))
        out["V_out_mean_V"] = float(np.mean(df["V_out_diff"]))
    return out

# ------------------------------------------------------------
# MAIN
# ------------------------------------------------------------
def main():
    # Validación de ruta LTspice si corresponde
    if SIMULADOR.lower() == "ltspice":
        if not Path(LTSPICE_EXE).exists():
            raise FileNotFoundError(
                f"No se encontró LTspice en:\n{LTSPICE_EXE}\n"
                "Ajustá la variable LTSPICE_EXE con tu ruta real."
            )

    base_file = BASE_ASC if SIMULADOR.lower() == "ltspice" else BASE_CIR


    combos, n_rows, n_cols = generar_combinaciones_auto(base_file)

    resumen = []

    for idx, (nombre, mapping) in enumerate(combos, 1):
        print(f"\n=== Simulación {idx}/{len(combos)} — {nombre} ===")
        mostrar_combinacion(mapping, n_rows, n_cols)
        out_asc = RESULT_DIR / f"{nombre}.asc"
        out_csv = RESULT_DIR / f"{nombre}.csv"

        # Cargar base y posiciones/flags
        lines = load_lines(base_file)
        flags = find_flags(lines)
        res_positions = find_resistors(lines)

        # Construir mapeo completo (pares _6 y RX_9 con delayed)
        mapping_total = build_full_mapping(mapping)

        # Aplicar y guardar .asc temporal
        modified = apply_mapping(lines, res_positions, flags, mapping_total)
        save_lines(out_asc, modified)

        # Ejecutar simulación
        raw = correr_simulador(out_asc)

        # Leer y guardar CSV individual
        df = leer_resultados(raw, n_rows, n_cols)
        df.to_csv(out_csv, index=False)
        print(f"✅ Resultados guardados en: {out_csv}")

        # Agregar al resumen
        resumen.append(resumen_resultados(df, nombre))

        # Limpieza del .asc temporal
        try:
            out_asc.unlink()
            if DEBUG:
                print(f"🧹 Eliminado temporal: {out_asc.name}")
        except Exception as e:
            print(f"⚠️ No se pudo eliminar {out_asc.name}: {e}")

    # Guardar resumen general
    resumen_path = RESULT_DIR / "resumen.csv"
    pd.DataFrame(resumen).to_csv(resumen_path, index=False)
    print("\n Todas las simulaciones completadas.")
    print(f"📘 Resumen general guardado en: {resumen_path}")

if __name__ == "__main__":
    main()
