def reemplazar_valores_y_modelos(input_file, output_file, cambios_valores, cambios_modelos, encoding="latin1"):
    """
    Cambia valores de resistencias y modelos (SYMATTR Value ...) en un archivo .asc de LTspice.
    Funciona con el formato:
        SYMATTR InstName R14_5
        SYMATTR Value 300k
        ...
        SYMATTR InstName D51
        SYMATTR Value DI_1N4730A
    """
    with open(input_file, "r", encoding=encoding) as f:
        lineas = f.readlines()

    nuevas = []
    nombre_actual = None
    total_valores = 0
    total_modelos = 0

    for linea in lineas:
        texto = linea.strip()

        # Guardar el nombre del componente actual
        if texto.startswith("SYMATTR InstName"):
            nombre_actual = texto.split()[-1]

        # Reemplazo de resistencias
        if texto.startswith("SYMATTR Value") and nombre_actual and nombre_actual.startswith("R"):
            partes = texto.split()
            if len(partes) == 3:
                valor_viejo = partes[2]
                if valor_viejo in cambios_valores:
                    nuevo_valor = cambios_valores[valor_viejo]
                    linea = f"SYMATTR Value {nuevo_valor}\n"
                    total_valores += 1
                    print(f"🔧 {nombre_actual}: {valor_viejo} → {nuevo_valor}")

        # Reemplazo de modelos (solo si no es resistencia)
        elif texto.startswith("SYMATTR Value") and nombre_actual and not nombre_actual.startswith("R"):
            partes = texto.split()
            if len(partes) == 3:
                modelo_viejo = partes[2]
                if modelo_viejo in cambios_modelos:
                    nuevo_modelo = cambios_modelos[modelo_viejo]
                    linea = f"SYMATTR Value {nuevo_modelo}\n"
                    total_modelos += 1
                    print(f"🔄 {nombre_actual}: {modelo_viejo} → {nuevo_modelo}")

        nuevas.append(linea)

    with open(output_file, "w", encoding=encoding) as f:
        f.writelines(nuevas)

    print(f"\n✅ Archivo guardado en '{output_file}'")
    print(f"🔧 Resistencias modificadas: {total_valores}")
    print(f"🔄 Modelos modificados: {total_modelos}")
    print(f"📈 Total cambios: {total_valores + total_modelos}\n")


# === ejemplo de uso ===
if __name__ == "__main__":
    cambios_valores = {
        "300k": "30k",
        "49.9k": "4.9k",
        "49k": "4.9k"
    }

    cambios_modelos = {
        "DI_1N4730A": "1N4736A",
    }

    reemplazar_valores_y_modelos(
        "matriz5x5.asc",
        "matriz5x5.asc",
        cambios_valores,
        cambios_modelos
    )
