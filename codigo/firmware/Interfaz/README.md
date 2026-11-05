# Interfaz del controlador STM32

Aplicación independiente de escritorio con PyQt6 y pySerial. Implementada según
la conversación compartida y Core/Src/cli.c del proyecto Controlador/Final.

## Iniciar en Windows

Desde esta carpeta, con Python 3.10 o posterior:

```powershell
python -m pip install -r requirements.txt
python main.py
```

También se puede usar `iniciar.bat`. Para probar el diseño sin placa ni pySerial:

```powershell
python main.py --demo
```

La demo genera mediciones ficticias; no simula la electrónica ni los efectos de
los comandos sobre celdas. No abre puertos serie.

## Uso con la placa

1. Cerrar PuTTY u otro programa que tenga abierto el mismo COM.
2. Seleccionar COM y conectar: 115200 baudios, 8 bits, sin paridad, 1 stop.
3. Elegir bancos y celdas y pulsar Inicializar. Se envía `init47`, por ejemplo,
   porque el parser actual usa `init%1u%1u%c` y no acepta los espacios que indica
   la ayuda. La aplicación no cambia el firmware.
4. Activar carga o descarga (con RL en ohm). Solicitar Manual o Automático.
5. En manual, usar los controles de banco, celda y switching. En automático,
   indicar objetivo en mV. Todos los comandos y respuestas quedan en consola.

La consulta periódica alterna `medir` y `status`, esperando el prompt `>` antes
de enviar el siguiente comando. El intervalo es configurable: el firmware no
publica por sí solo un flujo continuo de mediciones. No se envían comandos de
activación automáticamente al conectar. Un timeout de 4 s cierra la conexión,
vacía la cola y pausa las consultas; reconectar manualmente.

Las mediciones CELL_NEG son los valores de nodos que informa el barrido; no se
etiquetan como tensión individual de celda. El estado ON/OFF es una instantánea
de `status`, por lo que no representa cada fase de switching entre consultas.
El modo se confirma únicamente al recibir respuestas conocidas: si se conecta
a un equipo ya activo, permanece desconocido hasta una respuesta de modo.

El botón de bypass envía `apagar`: espera que termine el comando en curso y no
equivale a una llave física de emergencia. La CLI no expone el estado de esa
llave. Las fallas recibidas se conservan visibles; no se interpreta la ausencia
de mensajes como ausencia de fallas. Cerrar o desconectar la app no modifica el
estado del controlador. Las protecciones permanecen a cargo del firmware.

El registro exportado corresponde a las últimas 3000 líneas visibles.
