import pandas as pd
import matplotlib.pyplot as plt
from pathlib import Path

RESULT_DIR = Path("resultados")

def main():
    csv_files = sorted(RESULT_DIR.glob("*.csv"), key=lambda f: f.name.lower())
    if not csv_files:
        print("⚠️ No se encontraron archivos CSV en la carpeta 'resultados'.")
        return

    print("\n📂 Archivos encontrados (orden alfabético):")
    for i, f in enumerate(csv_files, 1):
        print(f"{i:2d}. {f.name}")

    try:
        choice = int(input("\nSeleccione el número del archivo para visualizar: "))
        if not (1 <= choice <= len(csv_files)):
            raise ValueError
    except ValueError:
        print("❌ Opción inválida.")
        return

    file = csv_files[choice - 1]
    print(f"\n📈 Mostrando: {file.name}\n")

    df = pd.read_csv(file)
    if "I_RL" not in df or "V_out_diff" not in df:
        print("⚠️ El archivo no contiene las columnas necesarias para graficar.")
        return

    # Convertir energía a mJ
    energia = (df["I_RL"] * df["V_out_diff"]).cumtrapz(initial=0) * 1000  # mJ

    plt.figure(figsize=(10, 6))
    plt.plot(df["t"], energia)
    plt.title(f"Energía acumulada — {file.name}")
    plt.xlabel("Tiempo [s]")
    plt.ylabel("Energía [mJ]")
    plt.grid(True)
    plt.show()

if __name__ == "__main__":
    main()
