# Índice general del proyecto

## 1) Código
### Firmware
- `codigo/firmware/Controlador/Prototipo/`
  - Proyecto STM32 (Core/Drivers/Debug, etc.)

### Python
- `codigo/python/plotter_curvas/`
  - Scripts para graficar / procesar curvas (Streamlit + libs)
  - ⚠️ Recom: versionar scripts, **no** versionar `.venv/` ni `.idea/`

- `codigo/python/simulador_config/`
  - Generación/automatización de esquemáticos + corridas masivas
  - Carpetas:
    - `Esquematicos/`
    - `resultados/`
    - `Resultados de las simulaciones en masa/` (sets por casos)

---

## 2) Documentación
- `docs/burocracia/` (trámites / formalidades)
- `docs/compras/` (presupuestos / compras)
- `docs/datasheets/` (hojas de datos)
- `docs/informe/`
  - `Entregas Parciales/` (hitos y entregas)
- `docs/Papers/` (papers y bibliografía)

---

## 3) Hardware
- `hardware/Prototipo/`
  - `Archivos de fabricación/PDF/` (plots, gerbers PDF, etc.)
  - `PCB/`
    - `Huellas.pretty/` (footprints KiCad)
    - `Modelos 3D/`
    - `prototipo-backups/`
  - `Simulación/` esquematico base del pcb

---

## 4) LTspice
- `ltspice/descartables/` (pruebas rápidas)
- `ltspice/lib/` (bibliotecas/modelos comunes)
- `ltspice/proyectos/` (proyectos “fuente”)
  - `eleccion_pmos/`
  - `matriz_1x1/`
  - `matriz_2x2/`
  - `matriz_3x2/`
  - `matriz_5x5/`
  - `modo_carga/`

---

## 5) Media (salidas, figuras, evidencia)
- `media/Curvas/` (plots finales, png/svg)
- `media/Esquematicos/` (capturas/exports de esquemas)
- `media/Mediciones/` (fotos, csv, osciloscopio, etc.)
- `media/PCB/` (renders, capturas, fotos)

---

## 6) Obsoletos / archivo
- `obsoletos/` (material viejo / reemplazado)


