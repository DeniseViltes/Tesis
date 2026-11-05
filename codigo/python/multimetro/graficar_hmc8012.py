"""Grafica un CSV generado por medir_hmc8012.py."""

import argparse
import csv
import math
from pathlib import Path
import sys


def cargar_csv(ruta, campo=1):
    """Devuelve tiempos y valores; las sobrecargas dejan huecos en la curva."""
    tiempos, valores = [], []
    with ruta.open(newline="", encoding="utf-8-sig") as archivo:
        lector = csv.DictReader(archivo, delimiter=";")
        requeridas = {"tiempo_s", "respuesta", "estado"}
        if not requeridas.issubset(lector.fieldnames or []):
            raise ValueError("El CSV debe contener tiempo_s, respuesta y estado separados por ';'.")
        for numero, fila in enumerate(lector, start=2):
            try:
                tiempo = float(fila["tiempo_s"])
                partes = fila["respuesta"].split(",")
                valor = float(partes[campo - 1])
                if not math.isfinite(tiempo) or tiempo < 0:
                    raise ValueError("tiempo inválido")
                if tiempos and tiempo < tiempos[-1]:
                    raise ValueError("tiempos fuera de orden")
                if not math.isfinite(valor) or abs(valor) >= 9.9e37:
                    valor = math.nan
                elif fila["estado"].strip() != "OK":
                    valor = math.nan
            except (ValueError, TypeError, IndexError, AttributeError) as exc:
                raise ValueError(f"Fila {numero}: datos inválidos o campo {campo} ausente.") from exc
            tiempos.append(tiempo)
            valores.append(valor)
    if not tiempos or not any(math.isfinite(v) for v in valores):
        raise ValueError("El archivo no contiene mediciones válidas para graficar.")
    return tiempos, valores


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("csv", type=Path, help="CSV de la captura")
    parser.add_argument("--unidad", default="", help="Por ejemplo V, A u ohm")
    parser.add_argument("--titulo", default="Medición HMC8012")
    parser.add_argument("--campo", type=int, default=1,
                        help="Campo SCPI a graficar, contado desde 1 (default: 1)")
    parser.add_argument("--salida", type=Path, help="Guardar gráfico: .png, .pdf o .svg")
    parser.add_argument("--sin-mostrar", action="store_true",
                        help="Solo guardar, sin abrir una ventana")
    args = parser.parse_args()
    if args.campo < 1:
        parser.error("--campo debe ser mayor o igual a 1")
    if args.sin_mostrar and not args.salida:
        parser.error("--sin-mostrar requiere --salida")
    if args.salida and args.salida.suffix.lower() not in {".png", ".pdf", ".svg"}:
        parser.error("--salida debe terminar en .png, .pdf o .svg")
    try:
        tiempos, valores = cargar_csv(args.csv, args.campo)
        import matplotlib
        if args.sin_mostrar:
            matplotlib.use("Agg")
        import matplotlib.pyplot as plt

        fig, ax = plt.subplots(figsize=(10, 5), layout="constrained")
        ax.plot(tiempos, valores, marker=".", markersize=4, linewidth=1)
        ax.set(xlabel="Tiempo transcurrido [s]",
               ylabel=f"Medición [{args.unidad}]" if args.unidad else "Medición",
               title=args.titulo)
        ax.grid(True, alpha=0.3)
        invalidas = sum(not math.isfinite(v) for v in valores)
        if invalidas:
            print(f"Aviso: {invalidas} lecturas no válidas se muestran como huecos.")
        if args.salida:
            # Evitar sobrescribir tanto capturas como gráficos existentes.
            with args.salida.open("xb") as archivo:
                fig.savefig(archivo, format=args.salida.suffix[1:].lower(), dpi=180)
            print(f"Gráfico guardado: {args.salida.resolve()}")
        if not args.sin_mostrar:
            plt.show()
        plt.close(fig)
    except ImportError:
        print("Instalá matplotlib: python -m pip install -r requirements.txt", file=sys.stderr)
        return 1
    except (OSError, ValueError, csv.Error) as exc:
        print(f"Error: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
