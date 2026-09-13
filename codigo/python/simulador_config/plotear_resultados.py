import argparse
from pathlib import Path

import matplotlib.pyplot as plt
from matplotlib.widgets import Button, RadioButtons
import pandas as pd
from unidades_resultados import unidades_originales


SCRIPT_DIR = Path(__file__).resolve().parent
RESULT_DIR = SCRIPT_DIR / "resultados"


def cargar_csvs(paths=None):
    """Carga CSV indicados o busca recursivamente dentro de resultados/."""
    csv_files = (
        [Path(path).expanduser().resolve() for path in paths]
        if paths
        else sorted(RESULT_DIR.rglob("*.csv")) if RESULT_DIR.exists() else []
    )
    if not csv_files:
        print(f"No se encontraron archivos CSV en: {RESULT_DIR}")
        return []

    dfs = []
    for file_path in csv_files:
        if not file_path.is_file():
            print(f"AVISO: no existe {file_path}")
            continue
        try:
            df = unidades_originales(pd.read_csv(file_path))
            if "t_s" not in df.columns:
                print(f"AVISO: {file_path.name} no contiene la columna t_s; se omite.")
                continue
            df["config"] = file_path.stem
            df.attrs["source_path"] = str(file_path)
            dfs.append(df)
            print(f"CSV cargado: {file_path}")
        except Exception as exc:
            print(f"AVISO: no se pudo leer {file_path.name}: {exc}")
    return dfs


def _mostrar(fig):
    # Mostrar las coordenadas originales, sin offset ni factor científico.
    for ax in fig.axes:
        ax.ticklabel_format(axis="both", style="plain", useOffset=False)
    fig.tight_layout(rect=(0, 0.11, 1, 1))
    plt.show()


def _agregar_selector_eje_x(fig, axes, line_sources):
    """Agrega un botón para cambiar X; t_s continúa siendo el valor automático."""
    if not line_sources:
        return
    if hasattr(axes, "flat"):
        axes = list(axes.flat)
    elif isinstance(axes, (list, tuple)):
        axes = list(axes)
    else:
        axes = [axes]
    dataframes = [df for _, df in line_sources]
    numeric_sets = [
        set(df.select_dtypes(include="number").columns) - {"config_id"}
        for df in dataframes
    ]
    common = set.intersection(*numeric_sets) if numeric_sets else set()
    ordered = [column for column in dataframes[0].columns if column in common]
    if "t_s" in ordered:
        ordered.remove("t_s")
        ordered.insert(0, "t_s")
    if not ordered:
        return

    button_ax = fig.add_axes((0.79, 0.025, 0.18, 0.052))
    button = Button(button_ax, "Cambiar eje X")

    def choose_x(_event):
        chooser = plt.figure(figsize=(5.2, max(3.0, 0.32 * len(ordered) + 1.2)))
        chooser.canvas.manager.set_window_title("Elegir eje X")
        radio_ax = chooser.add_axes((0.08, 0.08, 0.84, 0.84))
        labels = ["Automático (t_s)"] + [column for column in ordered if column != "t_s"]
        radio = RadioButtons(radio_ax, labels, active=0)

        def apply_x(label):
            column = "t_s" if label == "Automático (t_s)" else label
            for line, df in line_sources:
                line.set_xdata(df[column].to_numpy())
            for ax in axes:
                ax.set_xlabel(column)
                ax.relim()
                ax.autoscale_view()
            fig.canvas.draw_idle()
            plt.close(chooser)

        radio.on_clicked(apply_x)
        chooser._x_axis_radio = radio
        try:
            chooser.canvas.manager.show()
        except Exception:
            # Permite pruebas o exportaciones con backends no interactivos.
            pass

    button.on_clicked(choose_x)
    fig._x_axis_button = button
    fig._x_axis_button_callback = choose_x


def graficar_corriente_tension(df):
    fig, axes = plt.subplots(1, 2, figsize=(12, 5))
    plotted = False
    if "I_RL_A" in df.columns:
        line_i = axes[0].plot(df["t_s"], df["I_RL_A"], label="I_RL [A]")[0]
        axes[0].set_title("Corriente de carga")
        axes[0].set_ylabel("Corriente [A]")
        axes[0].legend(loc="best")
        plotted = True
    else:
        axes[0].text(0.5, 0.5, "Sin columna I_RL_A", ha="center", va="center")
    if "V_out_diff_V" in df.columns:
        line_v = axes[1].plot(df["t_s"], df["V_out_diff_V"], label="V_out_diff [V]")[0]
        axes[1].set_title("Tensión diferencial")
        axes[1].set_ylabel("Tensión [V]")
        axes[1].legend(loc="best")
        plotted = True
    else:
        axes[1].text(0.5, 0.5, "Sin columna V_out_diff_V", ha="center", va="center")
    for ax in axes:
        ax.set_xlabel("Tiempo [s]")
        ax.grid(True, alpha=0.3)
    fig.suptitle(f"Configuración: {df['config'].iloc[0]}")
    if plotted:
        sources = []
        if "I_RL_A" in df.columns:
            sources.append((line_i, df))
        if "V_out_diff_V" in df.columns:
            sources.append((line_v, df))
        _agregar_selector_eje_x(fig, axes, sources)
        _mostrar(fig)
    else:
        plt.close(fig)
        print("El CSV no contiene corriente de carga ni tensión diferencial.")


def graficar_energia(df):
    if "E_out_J" not in df.columns:
        print("Este archivo no contiene datos de energía (E_out_J).")
        return
    fig, ax = plt.subplots(figsize=(9, 5))
    line = ax.plot(df["t_s"], df["E_out_J"], label="Energía")[0]
    ax.set_title(f"Energía acumulada — {df['config'].iloc[0]}")
    ax.set_xlabel("Tiempo [s]")
    ax.set_ylabel("Energía [J]")
    ax.grid(True, alpha=0.3)
    ax.legend(loc="best")
    _agregar_selector_eje_x(fig, [ax], [(line, df)])
    _mostrar(fig)


def graficar_corrientes_celdas(df):
    columns = [column for column in df.columns if column.startswith("Icell")]
    if not columns:
        print("No se encontraron corrientes de celdas en este archivo.")
        return
    print("\nCorrientes disponibles:")
    for index, column in enumerate(columns, 1):
        print(f" {index}. {column}")
    print(" A. Todas\n Q. Cancelar")
    selection = input("Seleccioná una opción (ej. 1,2 o A) [A]: ").strip().upper() or "A"
    if selection == "Q":
        return
    if selection == "A":
        chosen = columns
    else:
        try:
            indexes = [int(value.strip()) - 1 for value in selection.split(",")]
            chosen = [columns[index] for index in indexes if 0 <= index < len(columns)]
        except ValueError:
            print("Selección inválida.")
            return
        if not chosen:
            print("No se seleccionaron columnas válidas.")
            return
    fig, ax = plt.subplots(figsize=(10, 5.5))
    sources = []
    for column in chosen:
        line = ax.plot(df["t_s"], df[column], label=column)[0]
        sources.append((line, df))
    ax.set_title(f"Corrientes de celdas — {df['config'].iloc[0]}")
    ax.set_xlabel("Tiempo [s]")
    ax.set_ylabel("Corriente [A]")
    ax.grid(True, alpha=0.3)
    ax.legend(title="Celdas", loc="best")
    _agregar_selector_eje_x(fig, [ax], sources)
    _mostrar(fig)


def comparar_configuraciones(dfs):
    options = {
        "1": ("I_RL_A", "Corriente de carga [A]"),
        "2": ("V_out_diff_V", "Tensión diferencial [V]"),
        "3": ("E_out_J", "Energía [J]"),
    }
    print("\nMagnitud a comparar:")
    for key, (_, label) in options.items():
        print(f" {key}. {label}")
    selection = input("Opción [1]: ").strip() or "1"
    if selection not in options:
        print("Selección inválida.")
        return
    field, label = options[selection]
    fig, ax = plt.subplots(figsize=(10, 5.5))
    count = 0
    sources = []
    for df in dfs:
        if field in df.columns:
            line = ax.plot(df["t_s"], df[field], label=df["config"].iloc[0])[0]
            sources.append((line, df))
            count += 1
    if not count:
        plt.close(fig)
        print(f"Ningún CSV contiene la columna {field}.")
        return
    ax.set_title(f"Comparación — {label}")
    ax.set_xlabel("Tiempo [s]")
    ax.set_ylabel(label)
    ax.grid(True, alpha=0.3)
    ax.legend(loc="best")
    _agregar_selector_eje_x(fig, [ax], sources)
    _mostrar(fig)


def menu_principal(paths=None):
    dfs = cargar_csvs(paths)
    if not dfs:
        return
    options = {str(index + 1): df for index, df in enumerate(dfs)}
    while True:
        print("\nMenú de visualización:")
        for key, df in options.items():
            print(f" {key}. {df['config'].iloc[0]}")
        print(" A. Comparar configuraciones\n Q. Salir")
        selection = input("Seleccioná una opción: ").strip().upper()
        if selection == "Q":
            break
        if selection == "A":
            comparar_configuraciones(dfs)
            continue
        if selection not in options:
            print("Selección inválida.")
            continue
        df = options[selection]
        print("\n1. Corriente y tensión\n2. Energía\n3. Corrientes de celdas")
        suboption = input("Elegí una opción: ").strip()
        if suboption == "1":
            graficar_corriente_tension(df)
        elif suboption == "2":
            graficar_energia(df)
        elif suboption == "3":
            graficar_corrientes_celdas(df)
        else:
            print("Selección inválida.")


def main():
    parser = argparse.ArgumentParser(description="Visualiza CSV producidos por el simulador")
    parser.add_argument("csv", nargs="*", type=Path, help="CSV a abrir; sin argumentos busca en resultados/")
    args = parser.parse_args()
    menu_principal(args.csv)


if __name__ == "__main__":
    main()
