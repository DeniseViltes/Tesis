# Registro del HMC8012 cada segundo

Implementación basada en `HMC8012` y `UsbInstrument` de
https://github.com/xvan/UTP3303C/blob/mediciones/UTP330C.py
y su ejemplo `test_HMC8012.py`, específicamente en la rama `mediciones`.
Se adaptaron la búsqueda de puertos para Windows y la identificación para
aceptar cualquier número de serie/firmware del modelo HMC8012.

## Preparación y uso

1. Conectar la PC al puerto USB de dispositivo del multímetro.
2. Seleccionar **USB VCP (puerto COM virtual)** en el instrumento e instalar el
   controlador VCP correspondiente si Windows no muestra el COM. USB TMC no
   funciona con este script. El manual adjunto explica las alternativas en pp. 3-4.
3. Configurar en el panel la magnitud, rango y **trigger automático**. Mantener
   esa configuración durante la captura para poder interpretar las unidades.
4. Desde esta carpeta ejecutar:

```powershell
python -m pip install -r requirements.txt
python medir_hmc8012.py --listar
python medir_hmc8012.py
```

Si hay exactamente un dispositivo con VID/PID `0aad:0135` (los del repositorio),
se selecciona automáticamente y se comprueba con `*IDN?`. Si la detección no
encuentra uno o hay varios, indicar el puerto que mostró `--listar`:

```powershell
python medir_hmc8012.py --puerto COM5
python medir_hmc8012.py --puerto COM5 --muestras 60 --salida ensayo.csv
```

Reemplazar COM5 por el puerto real. `--muestras 60` registra 60 consultas;
la primera es inmediata y la última se programa aproximadamente en t=59 s.
Sin ese argumento continúa hasta **Ctrl+C**. No requiere clonar el repositorio.

## Qué registra

Envía `FETC?` cada 1 s y guarda fecha/hora con zona horaria, tiempo transcurrido,
respuesta SCPI completa y estado en un CSV separado por punto y coma. Guarda
cada fila inmediatamente y nunca sobrescribe un archivo existente.

Según el manual (p. 24 del PDF, sección de comandos de sistema), `FETC?` lee
el valor de pantalla sin iniciar un trigger. `READ?` puede iniciar o detener
el trigger según el modo; por eso no se usa. No se envía `*RST` ni se fuerza
una medición de corriente como en el ejemplo original.

Los valores usan las unidades de la función configurada en el instrumento;
se preserva la respuesta sin asumir voltios o amperios. El valor de sobrecarga
`9.90000000E+37` se guarda y se marca `FUERA_DE_RANGO`.

El período es de consultas del PC, no una garantía de conversión ADC cada 1 s.
En trigger manual/single puede repetirse el último valor. La latencia USB y la
velocidad de conversión también pueden provocar valores repetidos. El reloj
monotónico evita acumular el tiempo de comunicación en cada intervalo. Si una
consulta tarda más que el período, se informa el atraso y se omiten intervalos
sin emitir consultas en ráfaga. Ante timeout o desconexión se detiene y conserva
las filas ya guardadas; no reintenta sobre respuestas potencialmente pendientes.

Opciones adicionales: `--intervalo 1`, `--timeout 2`, `--baudrate 9600`.
9600/8N1 sin control de flujo coincide con los valores implícitos de pyserial
en el driver original; el manual indica que USB no requiere parametrización.

No se validó con un instrumento físico conectado.

## Graficar una captura

Instalar las dependencias actualizadas y abrir el gráfico del CSV:

```powershell
python -m pip install -r requirements.txt
python graficar_hmc8012.py ensayo.csv --unidad V
```

Reemplazar `ensayo.csv` por el archivo generado y `V` por la unidad de la
medición (el CSV no guarda la unidad). El eje horizontal usa el tiempo real
transcurrido registrado por el script, en segundos. Las lecturas fuera de rango
se muestran como huecos para no distorsionar la escala.

Para guardar una imagen sin abrir una ventana:

```powershell
python graficar_hmc8012.py ensayo.csv --unidad V --salida ensayo.png --sin-mostrar
```

También permite `.pdf` y `.svg`, `--titulo "Tensión de batería"` y `--campo 2`
si la respuesta SCPI contiene varios valores separados por comas. Por defecto
grafica el primero. No sobrescribe archivos existentes. Se usa después de la
captura; no actualiza el gráfico en vivo.
