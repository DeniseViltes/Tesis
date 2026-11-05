"""Panel de escritorio para la CLI del controlador STM32. Python 3.10+."""
import sys
import re
import time
import math
import unicodedata
from collections import deque
from pathlib import Path
from PyQt6 import QtCore, QtGui, QtWidgets as W

try:
    import serial
    from serial.tools import list_ports
except ImportError:
    serial = None


def normalized(text):
    return ''.join(c for c in unicodedata.normalize('NFD', text.lower())
                   if unicodedata.category(c) != 'Mn')


class Panel(W.QMainWindow):
    def __init__(self, demo=False):
        super().__init__()
        self.demo = demo
        self.port = None
        self.pending = None
        self.queue = deque()
        self.buffer = b''
        self.last_rx = 0
        self.last_measurement = 0
        self.last_poll = 0
        self.confirmed_mode = 'Desconocido'
        self.setWindowTitle('Controlador · Laboratorio de baterías')
        self.resize(1240, 880)
        root = W.QWidget()
        scroll = W.QScrollArea()
        scroll.setWidgetResizable(True)
        scroll.setWidget(root)
        self.setCentralWidget(scroll)
        layout = W.QVBoxLayout(root)
        layout.setContentsMargins(24, 20, 24, 20)
        title = W.QLabel('CONTROLADOR  /  LABORATORIO DE BATERÍAS')
        title.setObjectName('title')
        layout.addWidget(title)
        layout.addWidget(W.QLabel('Supervisión y control de bancos · STM32'))

        row = W.QHBoxLayout()
        self.ports = W.QComboBox()
        self.ports.setMinimumWidth(160)
        row.addWidget(self.ports)
        self.button(row, 'Actualizar puertos', self.refresh_ports)
        self.connect_button = self.button(row, 'Conectar', self.toggle_connection)
        row.addWidget(W.QLabel('115200 · 8N1'))
        row.addStretch()
        self.link = W.QLabel('DESCONECTADO')
        row.addWidget(self.link)
        layout.addLayout(row)
        self.alert = W.QLabel('Sin información de fallas. La llave física no tiene telemetría en esta CLI.')
        self.alert.setObjectName('alert')
        self.alert.setWordWrap(True)
        layout.addWidget(self.alert)

        cards = W.QHBoxLayout()
        self.values = {}
        for key, title, unit in [('sum', 'SUMA DE BANCOS', 'V'), ('out', 'SALIDA · LEY DE OHM', 'V'),
                                 ('charge', 'CORRIENTE DE CARGA', 'A'), ('discharge', 'CORRIENTE DE DESCARGA', 'A')]:
            group = W.QGroupBox(title)
            col = W.QVBoxLayout(group)
            value = W.QLabel('— ' + unit)
            value.setObjectName('metric')
            col.addWidget(value)
            self.values[key] = value
            cards.addWidget(group)
        layout.addLayout(cards)
        self.age = W.QLabel('Mediciones: sin datos')
        layout.addWidget(self.age)

        middle = W.QHBoxLayout()
        setup = W.QGroupBox('Configuración y operación')
        config = W.QVBoxLayout(setup)
        r = W.QHBoxLayout()
        self.banks = self.spin(r, 'Bancos', 1, 4, 4)
        self.cells = self.spin(r, 'Celdas', 1, 7, 7)
        self.button(r, 'Inicializar', lambda: self.send(f'init{self.banks.value()}{self.cells.value()}'))
        config.addLayout(r)
        r = W.QHBoxLayout()
        self.resistance = self.spin(r, 'RL (Ω)', 1, 1000, 10)
        self.button(r, 'Activar descarga', lambda: self.send(f'activar descarga {self.resistance.value()}'))
        self.button(r, 'Activar carga', lambda: self.send('activar carga'))
        config.addLayout(r)
        r = W.QHBoxLayout()
        self.button(r, 'Manual', lambda: self.send('manual'))
        self.target = self.spin(r, 'Objetivo (mV)', 0, 65535, 6500)
        self.button(r, 'Automático', lambda: self.send(f'auto {self.target.value()}'))
        self.button(r, 'Actualizar objetivo', lambda: self.send(f'objetivo {self.target.value()}'))
        config.addLayout(r)
        self.mode = W.QLabel('Modo confirmado: desconocido')
        config.addWidget(self.mode)
        r = W.QHBoxLayout()
        self.button(r, 'Reiniciar / reposo', lambda: self.send('reiniciar'))
        self.button(r, 'Solicitar bypass', self.bypass, 'danger')
        config.addLayout(r)
        note = W.QLabel('Inicializar deja el equipo en reposo. Activar carga/descarga antes de controlar.\nEl bypass es una solicitud por serie; el corte de protección depende del STM32.')
        note.setWordWrap(True)
        config.addWidget(note)
        middle.addWidget(setup, 3)

        manual = W.QGroupBox('Control manual')
        man = W.QVBoxLayout(manual)
        r = W.QHBoxLayout()
        self.bank = self.spin(r, 'Banco', 1, 4, 1)
        self.cell = self.spin(r, 'Celda', 1, 7, 1)
        man.addLayout(r)
        self.manual_buttons = []
        for prefix, label in [('c', 'Celda'), ('b', 'Banco')]:
            r = W.QHBoxLayout()
            for state, caption in [('on', 'Encender'), ('off', 'Bypass')]:
                btn = self.button(r, f'{label}: {caption}', lambda _, p=prefix, s=state: self.send(
                    f'c {self.bank.value()} {self.cell.value()} {s}' if p == 'c' else f'b {self.bank.value()} {s}'))
                self.manual_buttons.append(btn)
            man.addLayout(r)
        r = W.QHBoxLayout()
        self.period = self.spin(r, 'Fase (ms)', 1, 65535, 500)
        self.phase = W.QComboBox()
        self.phase.addItems(['Síncrono (s)', 'Complementario (c)'])
        r.addWidget(self.phase)
        man.addLayout(r)
        r = W.QHBoxLayout()
        self.manual_buttons.append(self.button(r, 'Switching celda', lambda: self.send(
            f'sw {self.bank.value()} {self.cell.value()} {self.period.value()} {"s" if self.phase.currentIndex() == 0 else "c"}')))
        self.manual_buttons.append(self.button(r, 'Detener switching', lambda: self.send(f'sw {self.bank.value()} {self.cell.value()} off')))
        man.addLayout(r)
        middle.addWidget(manual, 2)
        layout.addLayout(middle)

        matrix = W.QGroupBox('Control manual · matriz de botones')
        matrix_layout = W.QVBoxLayout(matrix)
        matrix_layout.addWidget(W.QLabel('Tocá una celda para alternar ON/OFF. El botón Banco controla todas sus celdas.'))
        buttons = W.QGridLayout()
        self.cell_buttons = {}
        self.bank_buttons = {}
        self.cell_states = {}
        self.demo_states = {(b, c): False for b in range(1, 5) for c in range(1, 8)}
        for c in range(1, 8):
            buttons.addWidget(W.QLabel(f'Celda {c}'), 0, c)
        for b in range(1, 5):
            bank_button = W.QPushButton(f'Banco {b} · —')
            bank_button.clicked.connect(lambda _, bank=b: self.toggle_matrix(bank))
            buttons.addWidget(bank_button, b, 0)
            self.bank_buttons[b] = bank_button
            for c in range(1, 8):
                button = W.QPushButton('—')
                button.setMinimumWidth(70)
                button.clicked.connect(lambda _, bank=b, cell=c: self.toggle_matrix(bank, cell))
                self.cell_buttons[b, c] = button
                buttons.addWidget(button, b, c)
        matrix_layout.addLayout(buttons)
        self.matrix_hint = W.QLabel('Conectá, solicitá modo Manual y consultá Estado para habilitar los botones.')
        matrix_layout.addWidget(self.matrix_hint)
        layout.addWidget(matrix)

        group = W.QGroupBox('Matriz de bancos y celdas · valores CELL_NEG del último barrido (mV)')
        grid = W.QVBoxLayout(group)
        self.table = W.QTableWidget(4, 9)
        self.table.setHorizontalHeaderLabels(['Banco', 'Tensión', *[f'C{i}' for i in range(1, 8)]])
        self.table.verticalHeader().hide()
        self.table.horizontalHeader().setSectionResizeMode(W.QHeaderView.ResizeMode.Stretch)
        self.table.setEditTriggers(W.QAbstractItemView.EditTrigger.NoEditTriggers)
        self.table.setMinimumHeight(165)
        self.table.setMaximumHeight(190)
        grid.addWidget(self.table)
        self.scan = W.QLabel('Barrido: sin datos · CELL_NEG no equivale necesariamente a tensión individual de celda.')
        grid.addWidget(self.scan)
        layout.addWidget(group)

        console_group = W.QGroupBox('Consola serie')
        console_layout = W.QVBoxLayout(console_group)
        self.console = W.QPlainTextEdit()
        self.console.setReadOnly(True)
        self.console.setMaximumBlockCount(3000)
        self.console.setMinimumHeight(100)
        console_layout.addWidget(self.console)
        r = W.QHBoxLayout()
        self.command = W.QLineEdit()
        self.command.setPlaceholderText('Comando CLI, por ejemplo: medir, status, mux 3…')
        self.command.returnPressed.connect(self.submit)
        r.addWidget(self.command, 1)
        self.button(r, 'Enviar', self.submit)
        self.button(r, 'Medir', lambda: self.send('medir'))
        self.button(r, 'Estado', lambda: self.send('status'))
        self.button(r, 'Ayuda', lambda: self.send('help'))
        self.button(r, 'Guardar registro', self.save_log)
        console_layout.addLayout(r)
        layout.addWidget(console_group, 1)
        r = W.QHBoxLayout()
        self.live = W.QCheckBox('Actualizar mediciones y estados')
        self.live.setChecked(True)
        r.addWidget(self.live)
        self.interval = self.spin(r, 'Consulta (ms)', 500, 60000, 1000)
        r.addStretch()
        r.addWidget(W.QLabel('DEMO · datos ficticios' if demo else 'Los estados se actualizan con las respuestas de la placa'))
        layout.addLayout(r)
        self.setStyleSheet('''
            QMainWindow, QWidget { background: #101923; color: #dce7ef; font: 10pt "Segoe UI"; }
            QLabel#title { font-size: 18pt; font-weight: 700; color: #f0f7ff; }
            QGroupBox { border: 1px solid #304253; border-radius: 9px; margin-top: 14px; padding: 12px 8px 8px; }
            QGroupBox::title { subcontrol-origin: margin; left: 12px; color: #90a9bc; }
            QLabel#metric { font-size: 25pt; font-weight: 600; color: #50ddc4; }
            QLabel#alert { background: #29313a; border-radius: 6px; padding: 9px; color: #ffd58a; }
            QPushButton { background: #23384a; border: 1px solid #39536a; border-radius: 5px; padding: 7px 11px; }
            QPushButton:hover { background: #315069; } QPushButton:disabled { color: #637383; background: #17232e; }
            QPushButton#danger { background: #8e3545; border-color: #c05465; }
            QLineEdit, QSpinBox, QComboBox { background: #192a38; padding: 5px; border: 1px solid #395064; border-radius: 4px; }
            QPlainTextEdit, QTableWidget { background: #0b131d; border: 1px solid #2b4052; gridline-color: #233749; font-family: Consolas; }
            QHeaderView::section { background: #223647; padding: 5px; border: none; }
        ''')
        self.refresh_ports()
        self.reset_values()
        self.timer = QtCore.QTimer(self)
        self.timer.timeout.connect(self.tick)
        self.timer.start(30)
        if demo:
            self.link.setText('DEMO · DATOS FICTICIOS')
            self.connect_button.setEnabled(False)

    def button(self, layout, text, callback, name=''):
        btn = W.QPushButton(text)
        btn.setObjectName(name)
        btn.clicked.connect(callback)
        layout.addWidget(btn)
        return btn

    def spin(self, layout, label, low, high, value):
        layout.addWidget(W.QLabel(label))
        spin = W.QSpinBox()
        spin.setRange(low, high)
        spin.setValue(value)
        layout.addWidget(spin)
        return spin

    def log(self, text):
        self.console.appendPlainText(time.strftime('%H:%M:%S') + '  ' + text)

    def reset_values(self):
        self.cell_states.clear()
        self.last_measurement = 0
        self.confirmed_mode = 'Desconocido'
        self.mode.setText('Modo confirmado: desconocido')
        self.scan.setText('Barrido: sin datos · valores CELL_NEG (no tensión individual de celda)')
        for key, value in self.values.items():
            value.setText('— ' + ('A' if key in ('charge', 'discharge') else 'V'))
        for row in range(4):
            for col in range(9):
                self.table.setItem(row, col, W.QTableWidgetItem(f'B{row + 1}' if col == 0 else '—'))
        self.update_matrix()

    def refresh_ports(self):
        selected = self.ports.currentText()
        self.ports.clear()
        if serial:
            self.ports.addItems([p.device for p in list_ports.comports()])
        if selected:
            self.ports.setCurrentText(selected)

    def toggle_connection(self):
        if self.port:
            self.disconnect()
            return
        if serial is None:
            W.QMessageBox.information(self, 'Dependencia pendiente', 'Instalá pySerial con: python -m pip install -r requirements.txt')
            return
        if not self.ports.currentText():
            self.log('Seleccioná un puerto COM y cerrá PuTTY si está usándolo.')
            return
        try:
            self.port = serial.Serial(self.ports.currentText(), 115200, timeout=0, write_timeout=0.2)
            self.buffer = b''
            self.last_rx = time.monotonic()
            self.reset_values()
            self.link.setText('CONECTADO · esperando datos')
            self.connect_button.setText('Desconectar')
            self.ports.setEnabled(False)
            self.log('Conectado. No se inicializa ni activa el circuito automáticamente.')
        except Exception as exc:
            self.port = None
            self.log(f'No se pudo conectar: {exc}')

    def disconnect(self):
        if self.port:
            self.port.close()
        self.port = None
        self.queue.clear()
        self.pending = None
        self.buffer = b''
        self.connect_button.setText('Conectar')
        self.ports.setEnabled(True)
        self.link.setText('DESCONECTADO')
        self.reset_values()

    def submit(self):
        command = self.command.text().strip()
        if command:
            self.send(command)
            self.command.clear()

    def send(self, command):
        try:
            encoded = command.encode('ascii')
        except UnicodeEncodeError:
            self.log('La CLI acepta comandos ASCII.')
            return
        if len(encoded) > 63 or '\r' in command or '\n' in command:
            self.log('Usá un único comando de hasta 63 caracteres.')
            return
        if not self.port and not self.demo:
            self.log('Conectá un puerto antes de enviar comandos.')
            return
        if len(self.queue) >= 20:
            self.log('Cola llena. Esperá las respuestas de la placa.')
            return
        self.queue.append(command)

    def bypass(self):
        self.queue.clear()
        self.send('apagar')

    def update_matrix(self):
        enabled = (bool(self.port) or self.demo) and self.confirmed_mode == 'Manual'
        busy = bool(self.queue) or self.pending is not None
        for (bank, cell), button in self.cell_buttons.items():
            state = self.cell_states.get((bank, cell))
            button.setText('—' if state is None else ('ON' if state else 'OFF'))
            button.setEnabled(enabled and not busy and state is not None)
            button.setStyleSheet('background: #176b59; border-color: #50ddc4; color: white;' if state else '')
            button.setToolTip(f'Banco {bank}, celda {cell}: ' + ('sin estado recibido' if state is None else ('apagar' if state else 'encender')))
        for bank, button in self.bank_buttons.items():
            states = [state for (b, _), state in self.cell_states.items() if b == bank]
            all_on = bool(states) and all(states)
            label = '—' if not states else ('ON' if all_on else ('PARCIAL' if any(states) else 'OFF'))
            button.setText(f'Banco {bank} · {label}')
            button.setEnabled(enabled and not busy and bool(states))
            button.setStyleSheet('background: #176b59; border-color: #50ddc4; color: white;' if all_on else '')
            button.setToolTip('Apagar todas las celdas' if all_on else 'Encender todas las celdas')
        self.matrix_hint.setText('Esperando respuesta y actualización de estados…' if busy else
                                ('Verde: ON confirmado · OFF: apagado · —: sin estado / no configurada' if enabled else
                                 'Conectá y solicitá modo Manual para habilitar la matriz.'))

    def toggle_matrix(self, bank, cell=None):
        if not (self.port or self.demo) or self.confirmed_mode != 'Manual' or self.pending or self.queue:
            return
        if cell is not None:
            state = self.cell_states.get((bank, cell))
            if state is None:
                return
            command = f'c {bank} {cell} {"off" if state else "on"}'
        else:
            states = [state for (b, _), state in self.cell_states.items() if b == bank]
            if not states:
                return
            command = f'b {bank} {"off" if all(states) else "on"}'
        self.send(command)
        self.send('status')
        self.update_matrix()

    def parse_line(self, line):
        text = normalized(line.strip())
        if text.startswith('falla:'):
            self.alert.setText(line)
            self.alert.setStyleSheet('background: #762c3b; color: white; padding: 9px;')
        if 'ok: modo manual' in text:
            self.confirmed_mode = 'Manual'
        elif 'ok: automatico solicitado' in text:
            self.confirmed_mode = 'Automático'
        elif 'ok: matriz configurada' in text or 'ok: controlador en reposo' in text:
            self.reset_values()
            self.confirmed_mode = 'Reposo'
        elif 'ok: circuito activo' in text:
            self.confirmed_mode = 'Activo · solicitar Manual o Automático'
        self.mode.setText('Modo confirmado: ' + self.confirmed_mode)
        patterns = [('sum', r'suma de tension de los bancos: (\d+) mv'),
                    ('out', r'tension en la salida por ley de ohm: (\d+) mv'),
                    ('charge', r'corriente de carga: (\d+) ma'),
                    ('discharge', r'corriente de descarga: (\d+) ma')]
        for key, pattern in patterns:
            match = re.fullmatch(pattern, text)
            if match:
                self.values[key].setText(f'{int(match[1]) / 1000:.3f} ' + ('A' if key in ('charge', 'discharge') else 'V'))
                self.last_measurement = time.monotonic()
        match = re.fullmatch(r'banco (\d+): (\d+) mv', text)
        if match and 1 <= int(match[1]) <= 4:
            self.table.item(int(match[1]) - 1, 1).setText(f'{int(match[2]) / 1000:.3f} V')
        match = re.fullmatch(r'banco (\d+), celda (\d+): cell_neg = (\d+) mv', text)
        if match and 1 <= int(match[1]) <= 4 and 1 <= int(match[2]) <= 7:
            item = self.table.item(int(match[1]) - 1, int(match[2]) + 1)
            item.setData(QtCore.Qt.ItemDataRole.UserRole, match[3])
            state = item.data(QtCore.Qt.ItemDataRole.UserRole + 1) or '?'
            item.setText(f'{match[3]} · {state}')
        match = re.match(r'banco (\d+): (c1=.*)', text)
        if match and 1 <= int(match[1]) <= 4:
            row = int(match[1]) - 1
            self.cell_states = {key: value for key, value in self.cell_states.items() if key[0] != row + 1}
            self.table.item(row, 0).setText(f'B{row + 1} · ' + ('ON' if 'banco_sw=on' in text else 'OFF'))
            for cell, state in re.findall(r'c([1-7])=(on|off)', match[2]):
                self.cell_states[row + 1, int(cell)] = state == 'on'
                item = self.table.item(row, int(cell) + 1)
                item.setData(QtCore.Qt.ItemDataRole.UserRole + 1, state.upper())
                item.setText(f'{item.data(QtCore.Qt.ItemDataRole.UserRole) or "—"} · {state.upper()}')
                item.setForeground(QtGui.QColor('#50ddc4' if state == 'on' else '#90a9bc'))
        if text.startswith('barrido cell_neg:'):
            self.scan.setText(line + ' · antigüedad informada por STM32 al consultar')
        self.update_matrix()

    def consume(self, data):
        self.buffer += data
        if len(self.buffer) > 65536:
            raise RuntimeError('Respuesta demasiado grande o sin terminadores de línea')
        while b'\n' in self.buffer:
            raw, self.buffer = self.buffer.split(b'\n', 1)
            line = raw.decode('utf-8', errors='replace').strip('\r')
            self.log('RX  ' + line)
            self.parse_line(line)
        # La CLI no termina el prompt con salto de línea. Esperar al prompt
        # serializa comandos y evita sobrescribir su único buffer de recepción.
        if self.buffer.strip() == b'>':
            self.buffer = b''
            self.pending = None

    def tick(self):
        self.update_matrix()
        now = time.monotonic()
        connected = bool(self.port) or self.demo
        for button in self.manual_buttons:
            button.setEnabled(connected and self.confirmed_mode == 'Manual')
        age = now - self.last_measurement if self.last_measurement else None
        self.age.setText('Mediciones: sin datos' if age is None else f'Última medición recibida hace {age:.1f} s' + (' · DATOS ANTIGUOS' if age > 3 else ''))
        try:
            if self.port and self.port.in_waiting:
                self.last_rx = now
                self.consume(self.port.read(min(self.port.in_waiting, 8192)))
                self.link.setText('CONECTADO · recibiendo')
            if self.pending and now - self.pending[1] > 4:
                self.log('Tiempo de espera agotado. Se pausa la consulta; reconectá para resincronizar la CLI.')
                self.live.setChecked(False)
                self.disconnect()
                return
            if self.port and now - self.last_rx > 3:
                self.link.setText('CONECTADO · sin datos recientes')
            if connected and not self.pending and not self.queue and self.live.isChecked() and now - self.last_poll >= self.interval.value() / 1000:
                self.last_poll = now
                self.queue.extend(['medir', 'status'])
            if connected and not self.pending and self.queue:
                command = self.queue.popleft()
                self.log('TX  ' + command)
                self.pending = (command, now)
                if self.demo:
                    self.simulate(command, now)
                    self.pending = None
                else:
                    self.port.write((command + '\r').encode('ascii'))
        except Exception as exc:
            self.log(f'Error de comunicación: {exc}')
            self.disconnect()

    def simulate(self, command, now):
        if command == 'medir':
            lines = [f'Suma de tensión de los bancos: {int(14500 + 100 * math.sin(now))} mV',
                     'Tensión en la salida por ley de ohm: 14210 mV',
                     'Corriente de carga: 0 mA', 'Corriente de descarga: 1421 mA',
                     'Barrido CELL_NEG: ultimo barrido realizado hace 180 ms']
            for bank in range(1, 5):
                lines.append(f'Banco {bank}: {3600 + bank * 10} mV')
                for cell in range(1, 8):
                    lines.append(f'Banco {bank}, celda {cell}: CELL_NEG = {400 + cell * 430} mV')
        elif command == 'status':
            lines = [f'BANCO {bank}: ' + ' | '.join(f'C{c}={"ON" if self.demo_states[bank, c] else "OFF"}' for c in range(1, 8)) + ' | BANCO_SW=OFF' for bank in range(1, 5)]
        elif re.fullmatch(r'c [1-4] [1-7] (on|off)', command):
            _, bank, cell, state = command.split()
            self.demo_states[int(bank), int(cell)] = state == 'on'
            lines = ['OK']
        elif re.fullmatch(r'b [1-4] (on|off)', command):
            _, bank, state = command.split()
            for cell in range(1, 8):
                self.demo_states[int(bank), cell] = state == 'on'
            lines = ['OK']
        elif command.startswith('init'):
            lines = ['OK: matriz configurada']
        elif command == 'manual':
            lines = ['OK: modo manual, bypass solicitado']
        elif command.startswith('auto '):
            lines = ['OK: automatico solicitado, objetivo en mV registrado']
        else:
            lines = ['DEMO: comando registrado; simulación de mediciones, sin modelo eléctrico ni actuación.']
        for line in lines:
            self.log('DEMO RX  ' + line)
            self.parse_line(line)

    def save_log(self):
        path, _ = W.QFileDialog.getSaveFileName(self, 'Guardar registro', 'registro-controlador.txt', 'Texto (*.txt)')
        if path:
            try:
                Path(path).write_text(self.console.toPlainText(), encoding='utf-8')
            except OSError as exc:
                W.QMessageBox.warning(self, 'No se pudo guardar', str(exc))

    def closeEvent(self, event):
        self.disconnect()
        event.accept()


if __name__ == '__main__':
    app = W.QApplication(sys.argv)
    window = Panel(demo='--demo' in sys.argv)
    window.show()
    sys.exit(app.exec())
