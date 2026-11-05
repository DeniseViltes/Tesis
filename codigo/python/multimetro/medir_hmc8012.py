"""Registra el valor de pantalla del HMC8012 por USB VCP cada segundo.

Basado en HMC8012/UsbInstrument de xvan/UTP3303C, rama mediciones.
No reinicia el instrumento ni cambia su función, rango o trigger.
"""

import argparse
import csv
from datetime import datetime
import math
from pathlib import Path
import sys
import time

import serial
from serial.tools import list_ports


def consultar(conexion, comando):
    conexion.write((comando + "\n").encode("ascii"))
    respuesta = conexion.read_until(b"\n", size=4096)
    if not respuesta.endswith(b"\n"):
        raise TimeoutError(f"Respuesta incompleta o ausente a {comando}. "
                           "Revisá USB VCP, el puerto y el timeout.")
    return respuesta.decode("ascii").strip()


def elegir_puerto(puerto=None):
    if puerto:
        return puerto
    # VID/PID usados por el driver del repositorio. No sondear otros equipos.
    candidatos = [p.device for p in list_ports.comports()
                  if (p.vid, p.pid) == (0x0AAD, 0x0135)]
    if len(candidatos) != 1:
        raise ValueError(
            f"Se encontraron {len(candidatos)} candidatos HMC8012. "
            "Usá --listar y luego --puerto COMx. "
            "El instrumento debe usar USB VCP, no USB TMC."
        )
    return candidatos[0]


def registrar(conexion, archivo, intervalo, muestras):
    escritor = csv.writer(archivo, delimiter=";")
    escritor.writerow(["fecha_hora", "tiempo_s", "respuesta", "estado"])
    archivo.flush()
    inicio = siguiente = time.monotonic()
    contador = 0
    while muestras == 0 or contador < muestras:
        time.sleep(max(0.0, siguiente - time.monotonic()))
        # La fecha corresponde a la consulta del PC, no a un timestamp del ADC.
        fecha = datetime.now().astimezone().isoformat(timespec="milliseconds")
        transcurrido = time.monotonic() - inicio
        respuesta = consultar(conexion, "FETC?")
        # Se conserva la respuesta completa, incluso si el equipo devuelve
        # varios campos. La magnitud y las unidades son las del instrumento.
        valores = [float(v) for v in respuesta.split(",")]
        if not valores or any(not math.isfinite(v) for v in valores):
            raise ValueError(f"Lectura inválida: {respuesta!r}")
        estado = "FUERA_DE_RANGO" if any(abs(v) >= 9.9e37 for v in valores) else "OK"
        escritor.writerow([fecha, f"{transcurrido:.6f}", respuesta, estado])
        archivo.flush()
        contador += 1
        print(f"{fecha} | {transcurrido:.3f} s | {respuesta} | {estado}", flush=True)
        siguiente += intervalo
        ahora = time.monotonic()
        if siguiente < ahora:
            # Evitar ráfagas para recuperar consultas atrasadas.
            omitidos = math.floor((ahora - siguiente) / intervalo) + 1
            siguiente += omitidos * intervalo
            print(f"Aviso: se omitieron {omitidos} intervalos por demora de lectura.",
                  file=sys.stderr)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--listar", action="store_true", help="Listar puertos y salir")
    parser.add_argument("--puerto", help="Por ejemplo COM5; omitido: detectar por VID/PID")
    parser.add_argument("--intervalo", type=float, default=1.0, help="Segundos (default: 1)")
    parser.add_argument("--muestras", type=int, default=0, help="0: hasta Ctrl+C")
    parser.add_argument("--timeout", type=float, default=2.0, help="Timeout en segundos")
    parser.add_argument("--baudrate", type=int, default=9600,
                        help="Default del driver original: 9600")
    parser.add_argument("--salida", type=Path,
                        default=Path(datetime.now().strftime("hmc8012_%Y%m%d_%H%M%S.csv")))
    args = parser.parse_args()
    if args.listar:
        puertos = sorted(list_ports.comports(), key=lambda p: p.device)
        for p in puertos:
            print(f"{p.device}: {p.description} [{p.hwid}]")
        if not puertos:
            print("No hay puertos serie disponibles.")
        return 0
    if (not math.isfinite(args.intervalo) or args.intervalo <= 0
            or not math.isfinite(args.timeout) or args.timeout <= 0
            or args.muestras < 0 or args.baudrate <= 0):
        parser.error("Intervalo, timeout y baudrate deben ser positivos; muestras >= 0.")
    try:
        puerto = elegir_puerto(args.puerto)
        # Apertura exclusiva del CSV: nunca sobrescribir una captura anterior.
        with args.salida.open("x", newline="", encoding="utf-8-sig") as archivo:
            with serial.Serial(puerto, baudrate=args.baudrate, timeout=args.timeout,
                               write_timeout=args.timeout) as conexion:
                conexion.reset_input_buffer()
                identidad = consultar(conexion, "*IDN?")
                campos = identidad.split(",")
                if len(campos) < 2 or campos[1].strip().upper() != "HMC8012":
                    raise ValueError(f"El puerto {puerto} no identifica un HMC8012: {identidad}")
                print(f"Conectado a {puerto}: {identidad}")
                print(f"CSV: {args.salida.resolve()} | Ctrl+C para detener")
                registrar(conexion, archivo, args.intervalo, args.muestras)
    except KeyboardInterrupt:
        print("\nCaptura detenida; CSV cerrado.")
        return 0
    except (OSError, serial.SerialException, ValueError, TimeoutError) as exc:
        print(f"Error: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
