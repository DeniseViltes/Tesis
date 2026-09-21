from PIL import Image, ImageDraw, ImageFont
from pathlib import Path
import math

OUT = Path(__file__).parent
S = 2
BG = '#f3f4f5'
INK = '#424c56'
BLUE = '#7a76c2ff'
def font(n):
    return ImageFont.truetype('C:/Windows/Fonts/consola.ttf', n*S)
def start(title, subtitle, h=850):
    global im,d
    im=Image.new('RGB',(1500*S,h*S),BG); d=ImageDraw.Draw(im)
    d.rectangle((18*S,18*S,1482*S,(h-18)*S),outline='#b9ccdc',width=2)
    d.rectangle((18*S,18*S,1482*S,65*S),fill=BLUE)
    text(42,31,title,23)
    text(42,88,subtitle,19)
def text(x,y,s,size=18,color=INK):
    d.multiline_text((x*S,y*S),s,font=font(size),fill=color,spacing=6*S)
def box(x,y,w,h,title,body=''):
    d.rounded_rectangle((x*S,y*S,(x+w)*S,(y+h)*S),radius=8*S,fill=BLUE)
    text(x+18,y+20,title,20)
    if body: text(x+18,y+57,body,17)
def arrow(points,label='',at=None,color=INK):
    pp=[(int(x*S),int(y*S)) for x,y in points]
    d.line(pp,fill=color,width=2*S)
    a,b=pp[-2:]; ang=math.atan2(b[1]-a[1],b[0]-a[0]); r=12*S
    d.polygon([b,(b[0]-r*math.cos(ang-.43),b[1]-r*math.sin(ang-.43)),(b[0]-r*math.cos(ang+.43),b[1]-r*math.sin(ang+.43))],fill=color)
    if label: text(*at,label,17,color)
def initial(x,y):
    d.ellipse(((x-8)*S,(y-8)*S,(x+8)*S,(y+8)*S),fill=INK)
def note(x,y,w,h,s):
    d.rectangle((x*S,y*S,(x+w)*S,(y+h)*S),fill='#fff9b1')
    text(x+15,y+13,s,17)
def save(name): im.save(OUT/name)

start('Funcionamiento del prototipo','Flujo funcional de 3 bancos de 2 celdas: estados y eventos.',1030)
initial(65,220)
arrow([(75,220),(170,220)],'Controller_Init()', (65,150))
box(170,190,430,165,'REPOSO / ESPERA','Todas las celdas OFF\nTodos los switches de bypass ON\nNinguna celda en modo senal\nEspera un evento')
box(960,190,440,165,'CONTROL DE CELDAS ACTIVO','Alguna celda ON o en modo senal\nConserva la configuracion\nEspera comandos y ticks')
arrow([(600,245),(960,245)],'cell ... on o sw ...\n/ configurar celdas',(665,190),'#247cae')
arrow([(1050,355),(1050,450),(380,450),(380,355)],'Boton manual / apagar celdas, cancelar senales y encender bypass',(405,405),'#b76735')
arrow([(1270,355),(1270,570),(250,570),(250,355)],'cell ... off [todas las celdas quedan OFF y sin senal] / volver a reposo',(350,530),'#247cae')
arrow([(1130,190),(1130,135),(1450,135),(1450,310),(1400,310)])
text(1090,92,'Comandos / actualizar celdas',17)
arrow([(1400,330),(1450,330),(1450,705),(1160,705),(1160,355)],'Tick cada 1 ms\n/ actualizar grupos\ny celdas en modo senal',(1200,620))
arrow([(170,310),(95,310),(95,690),(200,690),(200,355)],'Boton manual\n/ conservar reposo',(220,620),'#b76735')
note(65,765,1370,100,'En ambos estados: las consultas UART responden sin cambiar el estado funcional.\nLos comandos invalidos se rechazan. cell ... off conserva reposo si ya estaba en reposo.\nEn ACTIVO, un cambio de celda conserva el estado mientras alguna siga ON o en modo senal.')
note(65,885,1370,100,'El inicio representa el estado que deja Controller_Init(); el arranque de perifericos queda fuera.\nEl boton vuelve al reposo funcional sin ejecutar Controller_Init() ni reinicializar los grupos.\nEstados descriptivos del comportamiento utilizado, sin ADC ni control automatico.')
save('01_funcionamiento_general.png')

start('PROTOTIPO | ESTADOS DE UNA CELDA','El mismo comportamiento se aplica a cada una de las seis celdas.',1030)
initial(65,270); arrow([(75,270),(150,270)])
box(150,215,340,135,'CELDA ESTATICA OFF','Salida de celda = OFF\nModo = STATIC')
box(1010,215,340,135,'CELDA ESTATICA ON','Salida de celda = ON\nModo = STATIC')
arrow([(490,245),(1010,245)],'cell b c on / encender celda',(545,213),'#247cae')
arrow([(1010,320),(490,320)],'cell b c off / apagar celda',(545,343),'#247cae')
box(570,600,380,160,'CELDA EN MODO SENAL','Modo = SIGNAL\nGrupo y fase configurados\nSalida alterna ON/OFF')
arrow([(280,350),(280,650),(570,650)],'sw b c f g s|c\n/ configurar grupo\ny asignar celda',(65,435),'#247cae')
arrow([(1220,350),(1220,650),(950,650)],'sw b c f g s|c\n/ configurar grupo\ny asignar celda',(1235,435),'#247cae')
arrow([(660,600),(660,425),(415,425),(415,350)],'cell ... off o boton\n/ cancelar senal\ny apagar',(425,480))
arrow([(855,600),(855,425),(1085,425),(1085,350)],'cell ... on\n/ cancelar senal y encender',(865,520))
arrow([(1180,215),(1180,150),(320,150),(320,215)],'Boton manual / apagar celda',(560,122),'#b76735')
note(65,815,1370,150,'REGLA DEL BANCO: switch ON solo cuando sus dos celdas estan OFF.\nCada cambio de salida actualiza tambien el switch del banco.\nEl comando sw conserva la salida anterior hasta el siguiente cambio de fase del grupo.\nUn nuevo sw reconfigura el grupo; las otras celdas de ese grupo tambien quedan afectadas.')
save('02_estados_celda.png')

start('PROTOTIPO | GENERADOR DE SEÑAL POR GRUPO','Hay 3 grupos independientes (0, 1 y 2). Actualizacion por SysTick cada 1 ms.',870)
initial(70,290); arrow([(80,290),(150,290)])
box(150,235,340,120,'GRUPO DESHABILITADO','No incrementa contador\nNo genera transiciones')
box(950,200,370,115,'FASE 0','contador = 0 al configurar')
box(950,535,370,115,'FASE 1','contador = 0 al conmutar')
arrow([(490,275),(950,275)],'sw ... g ...\n/ habilitar grupo\n/ ticks = 500 / f\n/ fase = 0; contador = 0',(555,185),'#247cae')
arrow([(1040,315),(1040,535)],'contador >= ticks\n/ contador = 0\n/ fase = 1',(735,388))
arrow([(1230,535),(1400,535),(1400,250),(1320,250)],'contador >= ticks\n/ contador = 0\n/ fase = 0',(1160,395))
note(65,405,590,155,'Cada tick: incrementar contador.\nSi contador < ticks, conservar fase.\nAl cambiar fase: actualizar las celdas\nSIGNAL asignadas a este grupo.\nFrecuencia admitida por UART: 1..30 Hz.')
note(65,700,1370,105,'FASE DIRECTA (s): salida = fase del grupo. COMPLEMENTARIA (c): salida = fase invertida.\ncell on/off y el boton retiran las celdas del modo SIGNAL; el grupo puede seguir contando.\nLos semiperiodos usan milisegundos enteros: ticks = division entera de 500 por frecuencia.')
save('03_generador_grupo.png')
