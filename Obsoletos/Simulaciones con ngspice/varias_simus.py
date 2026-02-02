import re
from pathlib import Path

netlist_original = Path("Netlist5x5_sinJFET.cir")
netlist_modificado = Path("matriz_modificada.cir")

# Configuración de etapas
tiempo_total = 40e-3
n_etapas = 4
duracion_etapa = tiempo_total / n_etapas

# Estados de cada etapa (mismo formato que antes)
estados = [
    [(1, 1)],
    [(1, 1), (1, 2)],
    [(1, 2), (3, 3)],
    [(3, 3), (4, 4), (5, 5)]
]

V_ON = 5
V_OFF = 0

# Leemos netlist base
text = netlist_original.read_text()

# Eliminamos las líneas Vcontrol existentes
text = re.sub(r"^Vcontrol\d+\d+.*$", "", text, flags=re.MULTILINE)

# Creamos nuevas líneas Vcontrol con pulsos
lineas_vcontrol = []
for fila in range(1, 6):
    for col in range(1, 6):
        nombre = f"Vcontrol{fila}{col}"
        nodo = f"ctrl{fila}{col}"

        # Construimos secuencia de pulsos según las etapas
        # Cada etapa de 10 ms: 0→5V o 5→0V
        # Usamos una tabla temporal de tipo PWL
        puntos = []
        for i, activas in enumerate(estados):
            t = i * duracion_etapa
            valor = V_ON if (fila, col) in activas else V_OFF
            puntos.append(f"{t:.6f} {valor}")
            puntos.append(f"{t + duracion_etapa - 1e-6:.6f} {valor}")  # mantener hasta casi fin etapa

        # Unimos en formato PWL(...)
        pwl_str = " PWL(" + " ".join(puntos) + ")"
        linea = f"{nombre} {nodo} 0{pwl_str}"
        lineas_vcontrol.append(linea)

# Insertamos los nuevos controles al final del netlist
text += "\n\n* === Fuentes de control dinámicas ===\n"
text += "\n".join(lineas_vcontrol)
text += "\n\n.tran 10u 40m\n"

# Guardamos y ejecutamos
netlist_modificado.write_text(text)
print("✅ Netlist con pulsos creado:", netlist_modificado)

