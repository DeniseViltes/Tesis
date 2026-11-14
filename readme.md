# Diseño de una matriz de conmutación para balanceo activo de celdas de litio

Trabajo Profesional de Ingeniería Electrónica · Facultad de Ingeniería, Universidad de Buenos Aires (FIUBA) · 2026

**Integrantes** · Sebastián Ezequiel García y Denise Oriana Viltes  
**Tutora** · Dra. Cecilia G. Galarza  
**Cotutor** · Ing. Juan Zuloaga

[Informe completo (PDF)](docs/informe/Informe_Final/main.pdf) · [Póster IEEE URUCON 2026](docs/Urocon/Poster_Urocon.pdf)

## Sobre el proyecto

Se diseñó, simuló y construyó una plataforma experimental para estudiar el balanceo activo por *bypass* en baterías de litio. La plataforma utiliza una matriz reconfigurable de celdas LiFePO₄. Las celdas de cada banco se conectan en paralelo y los bancos se conectan en serie. Mediante interruptores MOSFET, el sistema puede conectar o desconectar celdas de forma individual y puentear bancos completos para modificar la tensión de salida.

El diseño se divide en tres módulos: conmutación de celda, *bypass* de banco y circuito central. Una placa STM32 NUCLEO-F103RB controla la conmutación, adquiere las mediciones de tensión y corriente y permite operar el sistema en modo manual o automático. El repositorio reúne tanto la implementación final como las iteraciones de diseño y los datos de los ensayos.

## Contenido del repositorio

| Carpeta | Contenido |
| --- | --- |
| [Documentación y referencias](docs/) | [Fuentes LaTeX del informe final](docs/informe/Informe_Final/), [artículos de referencia](docs/Papers/), [hojas de datos](docs/datasheets/) y [material de IEEE URUCON 2026](docs/Urocon/). |
| [Diseño electrónico y PCB](hardware/) | [Proyectos KiCad de los módulos finales](hardware/Circuito%20Final/), esquemas, diseños de placas y [archivos de fabricación del prototipo](hardware/Archivos%20de%20fabricaci%C3%B3n/), incluidos Gerber y archivos de perforación. |
| [Simulaciones](Ltspice/) | [Proyectos LTspice](Ltspice/proyectos/) de distintas configuraciones de la matriz, etapas de conmutación y pruebas del modo de carga. |
| [Firmware](codigo/firmware/) | [Proyecto del controlador final](codigo/firmware/Controlador/Final/), firmware del prototipo y código de la interfaz. |
| [Herramientas de análisis](codigo/python/) | Scripts y notebooks para leer datos de instrumentos, graficar curvas y simular configuraciones de la matriz. |
| [Modelos 3D](Soporte%203D/) | Modelos editables en FreeCAD y archivos 3MF para imprimir las piezas de soporte del montaje. |
| [Mediciones experimentales](Mediciones/) | Registros del prototipo y del circuito final, con datos de instrumentos, bitácoras y gráficos. Los ensayos de caracterización se encuentran en [Mediciones finales](Mediciones/Circuito%20final/Mediciones%20finales/). |
| [Figuras e imágenes](media/) | Esquemas, curvas e imágenes de las placas y del montaje que acompañan la documentación. |
| [Versiones anteriores](Obsoletos/) | Simulaciones y herramientas de etapas previas del desarrollo. |

## Ensayos y montaje

El capítulo 5 del informe describe el banco de pruebas de 4 × 3 y los ensayos de eficiencia energética, tiempo de conmutación, conmutación de bancos y carga. El apéndice A detalla el montaje y la operación de la plataforma.
