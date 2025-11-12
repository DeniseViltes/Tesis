import re
from pathlib import Path
import subprocess


# Ruta al netlist original
netlist_path = Path("Netlist5x5_sinJFET.cir")
#acá lo va a guardar
modified_path = Path("matriz_modificada.cir")


#fila x columna
celdas_encendidas = [(1, 1), (1, 2), (3, 3), (4, 4), (5, 5)]

V_ON = 5.0
V_OFF = 0.0

text = netlist_path.read_text()


for fila in range(1, 6):
    banco = []
    for col in range(1, 6):
        nombre = f"Vcontrol{fila}{col}"

        valor = V_ON if (fila, col) in celdas_encendidas else V_OFF
        #sintaxis en el netlist Vcontrol{row}{col} Vc{row}{col} 0 5
        text = re.sub(
            rf"^{nombre}(\s+[\w\d_]+\s+[\w\d_]+\s+)[-\d.eE+]+",
            lambda m: f"{nombre}{m.group(1)}{int(valor)}",
            text,
            flags=re.MULTILINE
        )
        banco.append(valor)
    if all(v == V_OFF for v in banco):
        nombre_banco = f"VcBanco{fila}"
        text = re.sub(
            rf"^{nombre_banco}(\s+[\w\d_]+\s+[\w\d_]+\s+)[-\d.eE+]+",
            lambda m: f"{nombre_banco}{m.group(1)}{int(V_ON)}",
            text,
            flags=re.MULTILINE
        )


modified_path.write_text(text)
print(f"Netlist actualizado guardado en: {modified_path.resolve()}")

# Ejecutar NGSpice
print("Ejecutando NGSpice...")

ngspice_path = r"C:\Users\---\Desktop\Spice64\bin\ngspice.exe"

#subprocess.run([ngspice_path, "-b", str(modified_path), "-o", "sim_output.log"])
subprocess.run([ngspice_path, str(modified_path)])


print("Simulación completada. Revisá sim_output.log para ver resultados.")
