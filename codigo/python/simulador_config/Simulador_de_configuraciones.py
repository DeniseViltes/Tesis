import subprocess
import re
import argparse
import pandas as pd
import numpy as np
from itertools import combinations
from pathlib import Path

# Importar funciones desde la lógica del esquemático
from logica_de_modificar_esquematico import (
    load_lines, save_lines, find_flags, find_resistors,
    apply_mapping, build_full_mapping
)

# ------------------------------------------------------------
# CONFIGURACIÓN GENERAL
# ------------------------------------------------------------
DEBUG       = False

SCRIPT_DIR  = Path(__file__).resolve().parent
BASE_ASC    = SCRIPT_DIR / "Esquematicos/version3.3.asc"
LTSPICE_EXE = r"C:\Users\---\AppData\Local\Programs\ADI\LTspice\LTspice.exe"

RESULT_DIR  = SCRIPT_DIR / "resultados/version3.3 seleccionadas"
RESULT_DIR.mkdir(parents=True, exist_ok=True)

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
    print(f"▶ Ejecutando LTSPICE sobre {netlist_path.name}")
    if not Path(LTSPICE_EXE).is_file():
        raise FileNotFoundError(f"No se encontró LTspice en: {LTSPICE_EXE}")
    result = subprocess.run(
        [LTSPICE_EXE, "-b", "-Run", str(netlist_path)],
        capture_output=True,
        text=True,
    )
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

# ------------------------------------------------------------
# LECTURA DE RESULTADOS
# ------------------------------------------------------------
def leer_resultados(raw_file: Path, n_rows: int, n_cols: int, config_id: int) -> pd.DataFrame:
    try:
        from PyLTSpice import RawRead
    except ImportError as exc:
        raise RuntimeError(
            "Falta PyLTSpice para leer los resultados .raw. "
            "Instalalo en este intérprete con: python -m pip install PyLTSpice"
        ) from exc
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
                datos[f"Icell{i}{j}_A"] = raw.get_trace(name).get_wave(0)

    for tr in traces:
        if "rl" in tr.lower():
            datos["I_RL_A"] = raw.get_trace(tr).get_wave(0)
            break

    vpos = next((raw.get_trace(t).get_wave(0) for t in traces if "out_pos" in t.lower()), None)
    vneg = next((raw.get_trace(t).get_wave(0) for t in traces if "out_neg" in t.lower()), None)
    if vpos is not None and vneg is not None:
        datos["V_out_diff_V"] = vpos - vneg

    df = pd.DataFrame(datos)
    if df.empty:
        return pd.DataFrame({"config_id": [config_id]})

    if "V_out_diff_V" in df and "I_RL_A" in df:
        v = df["V_out_diff_V"]
        i = df["I_RL_A"]
        t = df["t_s"]
        dt = np.gradient(t)
        energia = np.cumsum(v * i * dt)
        df["E_out_J"] = energia

    return df

# ------------------------------------------------------------
# COMBINACIONES
# ------------------------------------------------------------
def generar_combinaciones_auto(
    base_file: Path,
    estado_activo: str = "VcMAS",
    estado_inactivo: str = "VcOFF",
    prefijo: str = "MAS_OFF",
):
    n_rows, n_cols = detectar_dimensiones(base_file)
    print(f"🔍 Matriz detectada: {n_rows}×{n_cols}")

    combos = []
    for k_on in range(1, n_rows):
        for rows_on in combinations(range(1, n_rows + 1), k_on):
            mapping_1 = {}
            for i in range(1, n_rows + 1):
                flag = estado_activo if i in rows_on else estado_inactivo
                for j in range(1, n_cols + 1):
                    mapping_1[f"R{i}{j}_1"] = flag
            nombre = prefijo + "_filas_" + "-".join(str(r) for r in rows_on)
            combos.append((nombre, mapping_1))
    return combos, n_rows, n_cols


def _pedir_estado(celda: str) -> str:
    opciones = {"1": "VcOFF", "2": "VcON", "3": "VcMAS", "4": "VcMENOS"}
    while True:
        value = input(f"  {celda} [1=OFF, 2=ON, 3=MAS, 4=MENOS] (1): ").strip() or "1"
        if value in opciones:
            return opciones[value]
        value_normalized = value.upper()
        if not value_normalized.startswith("VC"):
            value_normalized = "Vc" + value_normalized
        for state in opciones.values():
            if value_normalized.lower() == state.lower():
                return state
        print("  Opción inválida.")


def generar_configuracion_manual(base_file: Path, numero: int = 1):
    n_rows, n_cols = detectar_dimensiones(base_file)
    print("\nElegí el estado de cada celda. El banco de cada fila se calcula automáticamente.")
    mapping = {}
    for i in range(1, n_rows + 1):
        print(f"\nFila {i}:")
        for j in range(1, n_cols + 1):
            key = f"R{i}{j}_1"
            mapping[key] = _pedir_estado(key)
    default_name = f"manual_{numero}"
    name = input(f"Nombre de la simulación ({default_name}): ").strip() or default_name
    name = re.sub(r"[^A-Za-z0-9_-]+", "_", name).strip("_") or default_name
    return (name, mapping), n_rows, n_cols


def _parsear_indices(texto: str, maximo: int):
    seleccion = set()
    for part in texto.replace(" ", "").split(","):
        if not part:
            continue
        if "-" in part:
            start, end = (int(value) for value in part.split("-", 1))
            seleccion.update(range(min(start, end), max(start, end) + 1))
        else:
            seleccion.add(int(part))
    invalidos = [value for value in seleccion if value < 1 or value > maximo]
    if invalidos:
        raise ValueError(f"Índices fuera de rango: {invalidos}")
    return sorted(seleccion)


def seleccionar_combinaciones(base_file: Path):
    print("\nTipo de simulación:")
    print("  1) Configuración manual celda por celda")
    print("  2) Combinaciones por fila MAS / OFF")
    print("  3) Combinaciones por fila ON / OFF")
    print("  4) Combinaciones por fila MAS / MENOS")
    print("  5) Todas las combinaciones MAS / OFF (modo anterior)")
    option = input("Opción (1): ").strip() or "1"

    if option == "1":
        combos = []
        n_rows = n_cols = 0
        while True:
            combo, n_rows, n_cols = generar_configuracion_manual(base_file, len(combos) + 1)
            combos.append(combo)
            if input("¿Agregar otra configuración? [s/N]: ").strip().lower() not in {"s", "si", "sí", "y"}:
                break
        return combos, n_rows, n_cols

    modes = {
        "2": ("VcMAS", "VcOFF", "MAS_OFF"),
        "3": ("VcON", "VcOFF", "ON_OFF"),
        "4": ("VcMAS", "VcMENOS", "MAS_MENOS"),
        "5": ("VcMAS", "VcOFF", "MAS_OFF"),
    }
    if option not in modes:
        raise ValueError("Opción de simulación inválida.")
    combos, n_rows, n_cols = generar_combinaciones_auto(base_file, *modes[option])
    if option == "5":
        return combos, n_rows, n_cols

    print("\nConfiguraciones disponibles:")
    for index, (name, mapping) in enumerate(combos, 1):
        active_rows = sorted({
            int(re.match(r"R(\d+)(\d+)_1$", key).group(1))
            for key, value in mapping.items()
            if value == modes[option][0]
        })
        print(f"  {index:>2}) {name}  (filas activas: {active_rows})")
    while True:
        try:
            text = input("Elegí números separados por coma o rangos (ej. 1,3-5): ")
            indices = _parsear_indices(text, len(combos))
            if indices:
                return [combos[index - 1] for index in indices], n_rows, n_cols
            print("Seleccioná al menos una configuración.")
        except ValueError as exc:
            print(f"Selección inválida: {exc}")

# ------------------------------------------------------------
# MAIN
# ------------------------------------------------------------
def main():
    parser = argparse.ArgumentParser(description="Simulador de configuraciones de la matriz LTspice")
    parser.add_argument("--base", type=Path, default=BASE_ASC, help="Esquemático .asc de partida")
    parser.add_argument("--all", action="store_true", help="Ejecutar todas las combinaciones MAS/OFF")
    args = parser.parse_args()

    base_file = args.base.resolve()
    if not base_file.exists():
        raise FileNotFoundError(f"No existe el esquemático base: {base_file}")
    if args.all:
        combos, n_rows, n_cols = generar_combinaciones_auto(base_file)
    else:
        combos, n_rows, n_cols = seleccionar_combinaciones(base_file)
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
