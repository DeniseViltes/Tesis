import pandas as pd
import plotly.graph_objects as go
from plotly.subplots import make_subplots
from pathlib import Path
from rich.console import Console
from rich.prompt import Prompt

console = Console()
RESULT_DIR = Path("resultados/5x5 filas con cambio de R")

# ------------------------------------------------------------
# FUNCIONES DE LECTURA
# ------------------------------------------------------------
def cargar_csvs():
    csv_files = sorted(RESULT_DIR.glob("*.csv"))
    if not csv_files:
        console.print("[red]No se encontraron archivos CSV en la carpeta 'resultados'.[/red]")
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
# GRAFICAR CORRIENTE Y TENSIÓN
# ------------------------------------------------------------
def graficar_corriente_tension(df):
    fig = make_subplots(rows=1, cols=2, subplot_titles=("Corriente de carga [mA]", "Tensión diferencial [V]"))

    if "I_RL_mA" in df:
        fig.add_trace(go.Scatter(x=df["t_s"], y=df["I_RL_mA"], mode="lines", name="I_RL [mA]"), row=1, col=1)
    if "V_out_diff_V" in df:
        fig.add_trace(go.Scatter(x=df["t_s"], y=df["V_out_diff_V"], mode="lines", name="V_out_diff [V]"), row=1, col=2)

    fig.update_layout(title=f"Configuración: {df['config'].iloc[0]}", template="plotly_white")
    fig.update_xaxes(title="Tiempo [s]", row=1, col=1)
    fig.update_xaxes(title="Tiempo [s]", row=1, col=2)
    fig.show()

# ------------------------------------------------------------
# GRAFICAR ENERGÍA
# ------------------------------------------------------------
def graficar_energia(df):
    if "E_out_mJ" not in df:
        console.print("[yellow]⚠️ Este archivo no contiene datos de energía.[/yellow]")
        return
    fig = go.Figure()
    fig.add_trace(go.Scatter(x=df["t_s"], y=df["E_out_mJ"], mode="lines", name="Energía"))
    fig.update_layout(title=f"Energía acumulada — {df['config'].iloc[0]}",
                      xaxis_title="Tiempo [s]", yaxis_title="Energía [mJ]", template="plotly_white")
    fig.show()

# ------------------------------------------------------------
# GRAFICAR CORRIENTES DE CELDAS
# ------------------------------------------------------------
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

    fig = go.Figure()
    for c in elegidas:
        fig.add_trace(go.Scatter(x=df["t_s"], y=df[c], mode="lines", name=c))

    fig.update_layout(
        title=f"Corrientes de celdas — {df['config'].iloc[0]}",
        xaxis_title="Tiempo [s]",
        yaxis_title="Corriente [mA]",
        legend_title="Celdas",
        template="plotly_white",
        width=900,
        height=500
    )

    fig.show()

# ------------------------------------------------------------
# COMPARAR CONFIGURACIONES
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

    fig = go.Figure()
    for df in dfs:
        if campo in df:
            fig.add_trace(go.Scatter(x=df["t_s"], y=df[campo], mode="lines", name=df["config"].iloc[0]))

    fig.update_layout(title=f"Comparación — {etiqueta}", xaxis_title="Tiempo [s]", yaxis_title=etiqueta, template="plotly_white")
    fig.show()

# ------------------------------------------------------------
# MENÚ PRINCIPAL
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
