from PySpice.Spice.Netlist import Circuit
from PySpice.Spice.Library import SpiceLibrary
from PySpice.Probe.Plot import plot
import matplotlib.pyplot as plt
from PySpice.Spice.NgSpice.Shared import NgSpiceShared
from PySpice.Unit import *

ruta = r"C:\Program Files\ngspice\Spice64\bin\ngspice.dll"
NgSpiceShared.library_path =ruta


# Crear circuito y agregar netlist
circuit = Circuit('Matriz 5x5')
circuit.include('matriz_modificada.cir')  # tu netlist ya modificado

# Simulación transitoria
simulator = circuit.simulator(temperature=25, nominal_temperature=25)
analysis = simulator.transient(step_time=10@u_us, end_time=40@u_ms)

# Plotear resultados
plt.figure()
v_diff = analysis['out_pos']
plt.plot(analysis.time, v_diff, label='Vdiff')
plt.xlabel('Tiempo [s]')
plt.ylabel('Voltaje [V]')
plt.legend()
plt.show()
