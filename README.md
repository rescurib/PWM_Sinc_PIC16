# Conversión Digital-Analógica por PWM filtrado en PIC16F15313
Este repositorio es apunte público en el que trataré varios conceptos que son necesario conocer para entender el funcionamiento del programa. Para mantenerlo corto haré referencias aquellos que pueden estudiarse en otras fuentes. Comenzaré con la razón por la que necesité hacer este programa. Requería construir un pulso para caracterizar la respuesta de un sistema (<a href="https://www.youtube.com/watch?v=YiJRgKJ0My8" target="_blank">identificación de sistemas</a>). Un impulso o <a href="https://es.khanacademy.org/math/differential-equations/laplace-transform/properties-of-laplace-transform/v/dirac-delta-function" target="_blank">Delta de Dirac</a> es una distribución que tiene la peculiaridad de tener un espectro que abarca todas las frecuencias. En teoría, si queremos saber entonces cual será la respuesta de un sistema a todas las frecuencias entonces debemos aplicar como entrada una señal impulsiva. En la práctica hay algunos problemas. Es imposible construir un impulso ideal y aunque fuera posible, los sistemas físicos (como los sensores) están siempre limitados a una banda de frecuencias. Peor aún, al digitalizar la señal de un sensor analógico se restringe a un más el ancho de banda útil. Es por eso que en algunos casos en mejor utilizar una <a href="https://ite.unison.mx/cual-es-la-funcion-sinc-y-por-que-es-importante-en-ingenieria/">función Sinc</a> (o seno cardinal) como entrada de caracterización ya que permite definir bastante bien el ancho de banda de trabajo. En mi caso particular, necesito una señal Sinc con un periodo de lóbulo principal de 1/(3.8kHz) o 263.16 us y con los bordes suavizados por una gaussiana. El periodo del lóbulo principal se mide entre sus cruces por 0. Su gráfica en el tiempo y su espectro se muestra a continuación:

<p align="center">
<img src="https://1.bp.blogspot.com/-iD9qVImIAws/YD_hiSBiYRI/AAAAAAAACeQ/AL8OOfyHchsPUblIvn4p64rMcemttlBcwCLcBGAsYHQ/s580/Sinc_Espectro.png">
</p>

Se puede notar que haciendo esto he definido mi ancho de banda de trabajo de 0 a 3.8kHz. Ahora lo que procede es escribir un programa en un microcontrolador que me de un PWM que codifique mi señal Sinc. Por restricciones de espacio, mi aplicación requiere de un microcontrolador muy pequeño por lo que elegí el [PIC16F15313](https://www.microchip.com/wwwproducts/en/PIC16F15313) que tiene solo 8 pines (justo los que requería). El circuito de la etapa de generación del PWM es la siguiente:

<p align="center">
<img src="https://1.bp.blogspot.com/-ONVe0puQ8Qg/YD_kTk_N7lI/AAAAAAAACeY/CD_dcmt3J7YYslfX0zznnaeuZp221lptACLcBGAsYHQ/s1600/PIC16F15313_Circuit_PWM.png">
</p>

Necesitamos un arreglo de datos de 8 bits que almacene nuestra señal Sinc. Para generar la declaración de ese arregló escribí el siguiente código de Python:
```Python
import numpy as np
from scipy.signal import gaussian
from scipy.interpolate import interp1d

#--- Sinc ---
F = 3800/np.pi # Frecuencia normalizada en pi
T = 1/F    # Periodo
Fs = 48000 # Frecuencia de muestreo
Ts = 1/Fs  # Periodo de muestreo
K = 2      # Número de periodos de cada lado del origen
t = np.linspace(-K*T,K*T,int(2*K*T*Fs))
N = len(t) # Número de elementos
y1 = np.sinc(2*np.pi*F*t) # Sinc
y2 = gaussian(len(y1),30) # Envolvente gaussiana
y = y1*y2 
#-----------

#--- Remapeo 0-255 ---
mapf = interp1d([y.min(),y.max()],[0,255])
y8b = [mapf(x) for x in y ]
y8b = np.round(y8b).astype(np.int)
#--------------------------

#--- Declaración de arreglo en C ---
print("#define N %d"%N)
valss = ','.join('{}'.format(x) for x in y8b)
print("const char Y[N] = {%s};"%valss)
#-----------------------------------
```

Lo que resta ahora en software es hacer dos cosas:
* Configurar el módulo CCP1 como PWM a una frecuencia suficientemente alta (>100 kHz)
* Configurar una rutina de interupción por timer a 48 kHz\* que mande una muestra de la sinc al PWM

\*Una regla de dedo es mantaner la Fs aprox. 10 veces arriba de la freq de trabajo más alta y además 48kHz ya es un estandar.

He comentado el código lo mejor que pude haciendo referencias a las páginas de la datasheet del PIC16F15313 dónde se puede consultar detalles adicionales. Es bastante corto y espero será facil de entender si se revisa directamente. Finalmente se construyo un filtro pasa-bajas de orden 4 diseñado en la herramienta Filter-Pro de Texas Instruments (la verción actual sólo puede usarse como [aplicación web](https://webench.ti.com/filter-design-tool/filter-type)). La frecuencia de corte que elegí es de 12 kHz para tener un buen margen para modificaciones del ancho de banda de la sinc pero una de 6-8 kHz podría funcionar tambien. El circuito es el siguiente:

<p align="center">
<img src="https://1.bp.blogspot.com/-H4vRJ9JR4TA/YD_nFkD6h3I/AAAAAAAACeg/i9k8OpmsNMoKbQ2CabMb5FAs-SXrDS7xgCLcBGAsYHQ/s1710/LPF_12kHz_4th_MCP602.png">
</p>

Utilicé dos MCP602 que tienen dos opamps por integrado pero podrian usar un solo MCP604. Todos los componentes de este ejemplo pueden ser conseguidos en México en [AG Electronica](https://www.agelectronica.com/). Un LM324 que es más facil de conseguir en cualquier tienda de componentes podría funcionar pero no estoy seguro si pueda dar la misma calidad en la señal. Los resultados de la implementación en físico de todo el sistema se muestran en esta captura de pantalla de un osciloscopio:

<p align="center">
<img src="https://1.bp.blogspot.com/-NyPX2CSNrLA/YD_simy9zPI/AAAAAAAACeo/stY1q0qne0omPRGseFiegsF_pMMXKy9gQCLcBGAsYHQ/s800/PWM_Sinc_3k8_UTD2072CEX.png">
</p>
