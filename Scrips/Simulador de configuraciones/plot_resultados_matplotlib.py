import pandas as pd
import matplotlib.pyplot as plt
from pathlib import Path
from rich.console import Console
from rich.prompt import Prompt

console = Console()
RESULT_DIR = Path("resultados")

# ------------------------------------------------------------
# Cargar CSVs
# ------------------------------------------------------------
def cargar_csvs():
    csv_files = sorted(RESULT_DIR.glob("*.csv"))
    if not csv_files:
        console.print("[red]No se encontraron archivos CSV en 'resultados'.[/red]")
        return []
    dfs = []
    for f in csv_files:
        try:
            df = pd.read_csv(f)
            df["config"] = f.stem
            dfs.append(df)
        except Exception as e:
            console.print(f"[yellow]⚠️ No se pudo leer {f.name}: {e}[/yellow]")
    return dfs

# ------------------------------------------------------------
# Gráficos individuales
# ------------------------------------------------------------
def graficar_corriente_tension(df):
    plt.figure(figsize=(10, 4))
    plt.subplot(1, 2, 1)
    if "I_RL_mA" in df:
        plt.plot(df["t_s"], df["I_RL_mA"], label="I_RL [mA]")
        plt.xlabel("Tiempo [s]")
        plt.ylabel("Corriente [mA]")
        plt.title("Corriente de carga")
        plt.grid(True)
        plt.legend()

    plt.subplot(1, 2, 2)
    if "V_out_diff_V" in df:
        plt.plot(df["t_s"], df["V_out_diff_V"], label="V_out_diff [V]", color="orange")
        plt.xlabel("Tiempo [s]")
        plt.ylabel("Tensión [V]")
        plt.title("Tensión diferencial")
        plt.grid(True)
        plt.legend()

    plt.suptitle(f"Configuración: {df['config'].iloc[0]}")
    plt.tight_layout()
    plt.show()

def graficar_energia(df):
    if "E_out_mJ" not in df:
        console.print("[yellow]⚠️ Este archivo no contiene datos de energía.[/yellow]")
        return
    plt.figure(figsize=(6, 4))
    plt.plot(df["t_s"], df["E_out_mJ"], label="Energía [mJ]", color="green")
    plt.xlabel("Tiempo [s]")
    plt.ylabel("Energía [mJ]")
    plt.title(f"Energía acumulada — {df['config'].iloc[0]}")
    plt.grid(True)
    plt.legend()
    plt.show()

def graficar_corrientes_celdas(df):
    cols = [c for c in df.columns if c.startswith("Icell")]
    if not cols:
        console.print("[yellow]⚠️ No se encontraron corrientes de celdas en este archivo.[/yellow]")
        return

    console.print("\n[cyan]Corrientes disponibles:[/cyan]")
    for i, c in enumerate(cols, 1):
        console.print(f" {i}. {c}")
    console.print(" A. [green]Todas[/green]")
    console.print(" Q. [red]Cancelar[/red]")

    seleccion = Prompt.ask("\nSeleccioná una opción (ej. 1,2 o A)", default="A").strip().upper()
    if seleccion == "Q":
        return
    elif seleccion == "A":
        elegidas = cols
    else:
        try:
            indices = [int(x) - 1 for x in seleccion.split(",")]
            elegidas = [cols[i] for i in indices if 0 <= i < len(cols)]
        except Exception:
            console.print("[red]Selección inválida.[/red]")
            return

    plt.figure(figsize=(8, 5))
    for c in elegidas:
        plt.plot(df["t_s"], df[c], label=c)
    plt.xlabel("Tiempo [s]")
    plt.ylabel("Corriente [mA]")
    plt.title(f"Corrientes de celdas — {df['config'].iloc[0]}")
    plt.grid(True)
    plt.legend(ncol=3, fontsize=8)
    plt.tight_layout()
    plt.show()


# ------------------------------------------------------------
# Comparar configuraciones
# ------------------------------------------------------------
def comparar_configuraciones(dfs):
    console.print("\n[cyan]Elegí qué magnitud comparar:[/cyan]")
    opciones = {
        "1": ("I_RL_mA", "Corriente de carga [mA]"),
        "2": ("V_out_diff_V", "Tensión diferencial [V]"),
        "3": ("E_out_mJ", "Energía [mJ]")
    }
    for k, (_, label) in opciones.items():
        console.print(f" {k}. {label}")

    eleccion = Prompt.ask("\nSeleccioná una opción", choices=["1", "2", "3"])
    campo, etiqueta = opciones[eleccion]

    plt.figure(figsize=(8, 5))
    for df in dfs:
        if campo in df:
            plt.plot(df["t_s"], df[campo], label=df["config"].iloc[0])
    plt.xlabel("Tiempo [s]")
    plt.ylabel(etiqueta)
    plt.title(f"Comparación — {etiqueta}")
    plt.grid(True)
    plt.legend(fontsize=8)
    plt.tight_layout()
    plt.show()

# ------------------------------------------------------------
# Menú principal
# ------------------------------------------------------------
def menu_principal():
    dfs = cargar_csvs()
    if not dfs:
        return
    nombres = [df["config"].iloc[0] for df in dfs]
    opciones = {str(i+1): n for i, n in enumerate(nombres)}

    while True:
        console.print("\n[bold cyan]📊 Menú de visualización:[/bold cyan]")
        for k, n in opciones.items():
            console.print(f" {k}. {n}")
        console.print(f" A. [green]Comparar configuraciones[/green]")
        console.print(f" Q. [red]Salir[/red]")

        eleccion = Prompt.ask("\nSeleccioná una opción").strip().upper()
        if eleccion == "Q":
            break
        elif eleccion == "A":
            comparar_configuraciones(dfs)
        elif eleccion in opciones:
            df = next(d for d in dfs if d["config"].iloc[0] == opciones[eleccion])
            console.print("\n[cyan]1.[/cyan] Corriente y tensión")
            console.print("[cyan]2.[/cyan] Energía")
            console.print("[cyan]3.[/cyan] Corrientes de celdas")
            sub = Prompt.ask("\nElegí una opción", choices=["1", "2", "3"])
            if sub == "1":
                graficar_corriente_tension(df)
            elif sub == "2":
                graficar_energia(df)
            elif sub == "3":
                graficar_corrientes_celdas(df)

if __name__ == "__main__":
    menu_principal()
