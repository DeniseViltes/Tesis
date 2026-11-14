"""Ventana de seguimiento para una captura del HMC8012."""

from collections import deque
from concurrent.futures import ThreadPoolExecutor
from threading import Lock

import matplotlib.pyplot as plt


def mostrar(args, detener, capturar):
    recientes = deque()
    lock = Lock()

    def al_medir(tiempo, valor):
        with lock:
            recientes.append((tiempo, valor))
            while recientes[0][0] < tiempo - args.ventana:
                recientes.popleft()

    fig, ax = plt.subplots(figsize=(6, 3.4), layout="constrained")
    if fig.canvas.required_interactive_framework is None:
        plt.close(fig)
        raise ImportError("Seleccioná un backend gráfico de Matplotlib, como TkAgg.")
    fig.canvas.manager.set_window_title("HMC8012 en vivo")
    linea, = ax.plot([], [], marker=".", linewidth=1.2)
    ax.set(xlabel="Tiempo transcurrido [s]",
           ylabel=f"Medición [{args.unidad}]" if args.unidad else "Medición",
           title="Conectando al HMC8012…", xlim=(0, args.ventana))
    ax.grid(True, alpha=0.3)
    timer = fig.canvas.new_timer(interval=100)
    ultimo_tiempo = None

    def actualizar():
        nonlocal ultimo_tiempo
        if captura.done() and captura.exception() is not None:
            plt.close(fig)
            return
        with lock:
            puntos = list(recientes) if recientes and recientes[-1][0] != ultimo_tiempo else []
        if puntos:
            tiempos, valores = zip(*puntos)
            ultimo_tiempo = tiempos[-1]
            derecha = max(args.ventana, ultimo_tiempo)
            linea.set_data(tiempos, valores)
            ax.set_xlim(derecha - args.ventana, derecha)
            ax.relim()
            ax.autoscale_view(scalex=False)
            ax.set_title("HMC8012 en vivo")
        if captura.done():
            ax.set_title("Captura finalizada")
            timer.stop()
        if puntos or captura.done():
            fig.canvas.draw_idle()

    with ThreadPoolExecutor(max_workers=1) as ejecutor:
        captura = ejecutor.submit(capturar, args, detener, al_medir)
        timer.add_callback(actualizar)
        try:
            timer.start()
            plt.show(block=True)
        finally:
            detener.set()
            timer.stop()
            plt.close(fig)
        captura.result()
