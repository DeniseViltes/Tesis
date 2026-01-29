import os, sys
from PyQt6.QtCore import Qt, QPoint
from PyQt6.QtGui import QCursor
from PyQt6.QtWidgets import (
    QApplication, QMainWindow, QWidget, QFileDialog, QVBoxLayout, QHBoxLayout,
    QPushButton, QLabel, QComboBox, QMessageBox, QLineEdit, QGroupBox, QFormLayout, QFrame
)
from matplotlib.backends.backend_qtagg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.backends.backend_qtagg import NavigationToolbar2QT as NavigationToolbar
from matplotlib.figure import Figure

from ltspice_io import read_ltspice_2col
from plot_tools import THEMES, SCALE_MAP, pick_auto_scale, scale_suffix, apply_theme, apply_layout
from probe_tools import nearest_line_snap


class FloatingCoords(QFrame):
    def __init__(self, parent=None):
        super().__init__(parent, Qt.WindowType.ToolTip)
        self.label = QLabel("", self)
        lay = QVBoxLayout(self)
        lay.setContentsMargins(8, 6, 8, 6)
        lay.addWidget(self.label)
        self.hide()

    def set_text(self, t: str):
        self.label.setText(t)
        self.adjustSize()


class MplCanvas(FigureCanvas):
    def __init__(self):
        fig = Figure()
        super().__init__(fig)
        self.fig = fig


class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("LTspice Plotter")

        self.x1 = self.y1 = None
        self.x2 = self.y2 = None
        self.lines = []

        # Probes A/B
        self.probeA = None
        self.probeB = None

        # Legend click map
        self.legend = None
        self.leg_map = {}

        # Axes references
        self.ax1 = None
        self.ax2 = None

        # Crosshairs A/B
        self.crossA = {"v": None, "h": None}
        self.crossB = {"v": None, "h": None}

        # Highlight
        self.highlighted = None

        root = QWidget()
        self.setCentralWidget(root)
        layout = QHBoxLayout(root)
        controls = QVBoxLayout()
        layout.addLayout(controls, 0)

        # Archivos
        grp_files = QGroupBox("Archivos")
        fl = QVBoxLayout(grp_files)
        b1 = QPushButton("Abrir archivo 1…")
        b1.clicked.connect(self.open1)
        b2 = QPushButton("Abrir archivo 2…")
        b2.clicked.connect(self.open2)
        bc = QPushButton("Quitar archivo 2")
        bc.clicked.connect(self.clear2)
        self.l1 = QLabel("Archivo 1: (no seleccionado)")
        self.l2 = QLabel("Archivo 2: (opcional)")
        fl.addWidget(b1)
        fl.addWidget(self.l1)
        row = QHBoxLayout()
        row.addWidget(b2)
        row.addWidget(bc)
        fl.addLayout(row)
        fl.addWidget(self.l2)
        controls.addWidget(grp_files)

        # Opciones
        grp_opts = QGroupBox("Opciones")
        form = QFormLayout(grp_opts)
        self.cmb_theme = QComboBox()
        self.cmb_theme.addItems(list(THEMES.keys()))
        self.cmb_theme.setCurrentText("Paper")

        self.cmb_mode = QComboBox()
        self.cmb_mode.addItems(["1 curva", "2 curvas (mismo eje Y)", "2 curvas (doble eje Y)"])

        self.cmb_x = QComboBox()
        self.cmb_x.addItems(["s", "ms"])
        self.cmb_x.setCurrentText("ms")

        self.cmb_y1 = QComboBox()
        self.cmb_y1.addItems(["Auto"] + list(SCALE_MAP.keys()))
        self.cmb_y2 = QComboBox()
        self.cmb_y2.addItems(["Auto"] + list(SCALE_MAP.keys()))

        self.cmb_legend = QComboBox()
        self.cmb_legend.addItems(
            ["Auto (best)", "Afuera derecha", "Afuera abajo", "Arriba derecha", "Arriba izquierda", "Abajo derecha", "Abajo izquierda"]
        )

        self.tit = QLineEdit("LTspice plot")
        self.xlab = QLineEdit("time")
        self.y1lab = QLineEdit("Y1")
        self.y2lab = QLineEdit("Y2")

        form.addRow("Tema:", self.cmb_theme)
        form.addRow("Modo:", self.cmb_mode)
        form.addRow("X:", self.cmb_x)
        form.addRow("Escala Y1:", self.cmb_y1)
        form.addRow("Escala Y2:", self.cmb_y2)
        form.addRow("Leyenda:", self.cmb_legend)
        form.addRow("Título:", self.tit)
        form.addRow("X label:", self.xlab)
        form.addRow("Y1 label:", self.y1lab)
        form.addRow("Y2 label:", self.y2lab)
        controls.addWidget(grp_opts)

        # Botones
        bp = QPushButton("Graficar")
        bp.clicked.connect(self.plot)

        bs_png = QPushButton("Guardar PNG…")
        bs_png.clicked.connect(self.save_png)

        bs_svg = QPushButton("Guardar SVG…")
        bs_svg.clicked.connect(self.save_svg)

        bs_pdf = QPushButton("Guardar PDF…")
        bs_pdf.clicked.connect(self.save_pdf)

        br = QPushButton("Reset cursores A/B")
        br.clicked.connect(self.reset_probes)

        controls.addWidget(bp)
        controls.addWidget(bs_png)
        controls.addWidget(bs_svg)
        controls.addWidget(bs_pdf)
        controls.addWidget(br)
        controls.addStretch(1)

        # Plot panel
        plot_panel = QVBoxLayout()
        self.canvas = MplCanvas()
        self.toolbar = NavigationToolbar(self.canvas, self)
        plot_panel.addWidget(self.toolbar)
        plot_panel.addWidget(self.canvas)
        layout.addLayout(plot_panel, 1)

        # Floating probe
        self.fbox = FloatingCoords(self)

        # Matplotlib events
        self.canvas.mpl_connect("motion_notify_event", self.on_move)
        self.canvas.mpl_connect("figure_leave_event", lambda e: self.fbox.hide())
        self.canvas.mpl_connect("button_press_event", self.on_click)

        # Legend click
        self.canvas.mpl_connect("pick_event", self.on_pick_legend)

    def _xscale(self):
        return 1e3 if self.cmb_x.currentText() == "ms" else 1.0

    def _scale(self, combo, y):
        t = combo.currentText()
        if t.startswith("Auto"):
            f = pick_auto_scale(y)
            combo.setItemText(0, "Auto" if f == 1.0 else f"Auto (x{f:g})")
            return f
        return SCALE_MAP.get(t, 1.0)

    def open1(self):
        p, _ = QFileDialog.getOpenFileName(self, "Abrir", "", "Text files (*.txt *.csv);;All (*.*)")
        if not p:
            return
        try:
            x, y, _, cols = read_ltspice_2col(p, 1)
            self.x1, self.y1 = x, y
            if len(cols) >= 2:
                self.xlab.setText(cols[0])
                self.y1lab.setText(cols[1])
            self.l1.setText(f"Archivo 1: {os.path.basename(p)}")
        except Exception as e:
            QMessageBox.critical(self, "Error", str(e))

    def open2(self):
        p, _ = QFileDialog.getOpenFileName(self, "Abrir", "", "Text files (*.txt *.csv);;All (*.*)")
        if not p:
            return
        try:
            x, y, _, cols = read_ltspice_2col(p, 1)
            self.x2, self.y2 = x, y
            if len(cols) >= 2:
                self.y2lab.setText(cols[1])
            self.l2.setText(f"Archivo 2: {os.path.basename(p)}")
        except Exception as e:
            QMessageBox.critical(self, "Error", str(e))

    def clear2(self):
        self.x2 = self.y2 = None
        self.l2.setText("Archivo 2: (opcional)")
        if self.cmb_mode.currentIndex() != 0:
            self.cmb_mode.setCurrentIndex(0)

    def _apply_legend(self, ax, handles=None, labels=None):
        mode = self.cmb_legend.currentText()
        if mode == "Auto (best)":
            leg = ax.legend(handles, labels, loc="best") if handles else ax.legend(loc="best")
        elif mode == "Afuera derecha":
            leg = (
                ax.legend(handles, labels, loc="upper left", bbox_to_anchor=(1.02, 1.0))
                if handles
                else ax.legend(loc="upper left", bbox_to_anchor=(1.02, 1.0))
            )
        elif mode == "Afuera abajo":
            leg = (
                ax.legend(handles, labels, loc="upper center", bbox_to_anchor=(0.5, -0.12), ncol=2)
                if handles
                else ax.legend(loc="upper center", bbox_to_anchor=(0.5, -0.12), ncol=2)
            )
        else:
            loc = {
                "Arriba derecha": "upper right",
                "Arriba izquierda": "upper left",
                "Abajo derecha": "lower right",
                "Abajo izquierda": "lower left",
            }[mode]
            leg = ax.legend(handles, labels, loc=loc) if handles else ax.legend(loc=loc)

        leg.set_draggable(True)
        return leg

    def _enable_legend_picking(self):
        """Hace la leyenda clickeable (texto y handle) para ocultar/mostrar curvas."""
        self.leg_map = {}
        if self.legend is None:
            return

        leg_lines = list(self.legend.get_lines())
        leg_texts = list(self.legend.get_texts())

        # Matplotlib suele mantener el orden de handles/labels con el orden de líneas
        for i, line in enumerate(self.lines):
            if i < len(leg_lines):
                a = leg_lines[i]
                a.set_picker(True)
                a.set_pickradius(5)
                self.leg_map[a] = line
                a.set_alpha(1.0 if line.get_visible() else 0.25)

            if i < len(leg_texts):
                t = leg_texts[i]
                t.set_picker(True)
                self.leg_map[t] = line
                t.set_alpha(1.0 if line.get_visible() else 0.25)

    def on_pick_legend(self, event):
        artist = event.artist
        line = self.leg_map.get(artist)
        if line is None:
            return

        line.set_visible(not line.get_visible())

        # feedback visual en leyenda: atenuar si está oculto
        for leg_artist, mapped in self.leg_map.items():
            if mapped is line:
                leg_artist.set_alpha(1.0 if line.get_visible() else 0.25)

        self.canvas.draw_idle()

    def _remove_cross(self, store):
        for k in ("v", "h"):
            if store.get(k) is not None:
                try:
                    store[k].remove()
                except Exception:
                    pass
                store[k] = None

    def _update_crosshairs(self):
        # crosshair vertical en ax1 (común), horizontal en el eje de la curva (ax1 o ax2)
        if self.ax1 is None:
            return

        def draw_for(probe, store):
            self._remove_cross(store)
            if not probe:
                return
            x, y = probe["x"], probe["y"]
            axh = probe.get("ax", self.ax1)

            store["v"] = self.ax1.axvline(x, linestyle="--", alpha=0.7, linewidth=1.0)
            store["h"] = axh.axhline(y, linestyle="--", alpha=0.7, linewidth=1.0)

        draw_for(self.probeA, self.crossA)
        draw_for(self.probeB, self.crossB)

    def highlight_line(self, line):
        if line is None:
            return

        # guardar lw original una vez
        for l in self.lines:
            if not hasattr(l, "_orig_lw"):
                l._orig_lw = l.get_linewidth()

        # toggle off
        if self.highlighted is line:
            for l in self.lines:
                l.set_alpha(1.0)
                l.set_linewidth(getattr(l, "_orig_lw", l.get_linewidth()))
            self.highlighted = None
            self.canvas.draw_idle()
            return

        self.highlighted = line
        for l in self.lines:
            if l is line:
                l.set_alpha(1.0)
                l.set_linewidth(getattr(l, "_orig_lw", l.get_linewidth()) * 2.5)
            else:
                l.set_alpha(0.25)
                l.set_linewidth(getattr(l, "_orig_lw", l.get_linewidth()))
        self.canvas.draw_idle()

    def plot(self):
        if self.x1 is None:
            QMessageBox.warning(self, "Falta", "Cargá archivo 1")
            return

        xsc = self._xscale()
        y1sc = self._scale(self.cmb_y1, self.y1)
        y2sc = 1.0 if self.y2 is None else self._scale(self.cmb_y2, self.y2)

        self.canvas.fig.clear()
        ax1 = self.canvas.fig.add_subplot(111)
        self.ax1 = ax1
        self.ax2 = None

        ax1.set_title(self.tit.text())
        ax1.set_xlabel(f"{self.xlab.text()} [{self.cmb_x.currentText()}]")
        ax1.set_ylabel(f"{self.y1lab.text()} ({scale_suffix(y1sc)})")

        p1 = ax1.plot(self.x1 * xsc, self.y1 * y1sc, label=self.y1lab.text())

        mode = self.cmb_mode.currentIndex()
        self.lines = [p1[0]]

        if mode == 0:
            self.legend = self._apply_legend(ax1)
        elif mode == 1:
            if self.x2 is None:
                QMessageBox.warning(self, "Falta", "Cargá archivo 2")
                return
            p2 = ax1.plot(self.x2 * xsc, self.y2 * y2sc, label=self.y2lab.text())
            self.lines.append(p2[0])
            lines = p1 + p2
            labels = [l.get_label() for l in lines]
            self.legend = self._apply_legend(ax1, lines, labels)
        else:
            if self.x2 is None:
                QMessageBox.warning(self, "Falta", "Cargá archivo 2")
                return
            ax2 = ax1.twinx()
            self.ax2 = ax2
            ax2.set_ylabel(f"{self.y2lab.text()} ({scale_suffix(y2sc)})")
            p2 = ax2.plot(self.x2 * xsc, self.y2 * y2sc, label=self.y2lab.text())
            self.lines.append(p2[0])
            lines = p1 + p2
            labels = [l.get_label() for l in lines]
            self.legend = self._apply_legend(ax1, lines, labels)

        # Leyenda clickeable
        self._enable_legend_picking()

        # Restablecer crosshairs si había probes (re-dibujo post-clear)
        self._update_crosshairs()

        apply_layout(self.canvas.fig, self.cmb_legend.currentText())
        apply_theme(self.canvas.fig, THEMES[self.cmb_theme.currentText()])
        self.canvas.draw()

    def on_move(self, event):
        if event.inaxes is None or event.xdata is None:
            self.fbox.hide()
            return

        nearest = nearest_line_snap(self.lines, event)
        if nearest is None:
            self.fbox.hide()
            return

        line, xs, ys = nearest
        txt = f"Curva: {line.get_label()}\nX={xs:.6g}\nY={ys:.6g}"

        if self.probeA:
            A = self.probeA
            txt += f"\n\nA: {A['label']}\nXA={A['x']:.6g}\nYA={A['y']:.6g}"
        if self.probeB:
            B = self.probeB
            txt += f"\n\nB: {B['label']}\nXB={B['x']:.6g}\nYB={B['y']:.6g}"
        if self.probeA and self.probeB:
            dx = self.probeB["x"] - self.probeA["x"]
            dy = self.probeB["y"] - self.probeA["y"]
            txt += f"\n\nΔX={dx:.6g}\nΔY={dy:.6g}"

        self.fbox.set_text(txt)
        self.fbox.move(QCursor.pos() + QPoint(16, 16))
        self.fbox.show()

    def on_click(self, event):
        if event.button != 1 or event.inaxes is None or event.xdata is None:
            return

        nearest = nearest_line_snap(self.lines, event)
        if nearest is None:
            return

        line, xs, ys = nearest

        # Doble click: resaltar curva
        if getattr(event, "dblclick", False):
            self.highlight_line(line)
            return

        # Click: set A / Shift+Click: set B
        probe = {"label": line.get_label(), "x": xs, "y": ys, "ax": line.axes}
        shift = bool(QApplication.keyboardModifiers() & Qt.KeyboardModifier.ShiftModifier)
        if shift:
            self.probeB = probe
        else:
            self.probeA = probe

        self._update_crosshairs()
        self.canvas.draw_idle()

    def reset_probes(self):
        self.probeA = None
        self.probeB = None
        self._remove_cross(self.crossA)
        self._remove_cross(self.crossB)
        self.canvas.draw_idle()

    def save_png(self):
        p, _ = QFileDialog.getSaveFileName(self, "Guardar", "ltspice_plot.png", "PNG (*.png)")
        if not p:
            return
        if not p.lower().endswith(".png"):
            p += ".png"
        self.canvas.fig.savefig(p, dpi=200, bbox_inches="tight")

    def save_svg(self):
        p, _ = QFileDialog.getSaveFileName(self, "Guardar", "ltspice_plot.svg", "SVG (*.svg)")
        if not p:
            return
        if not p.lower().endswith(".svg"):
            p += ".svg"
        self.canvas.fig.savefig(p, bbox_inches="tight")

    def save_pdf(self):
        p, _ = QFileDialog.getSaveFileName(self, "Guardar", "ltspice_plot.pdf", "PDF (*.pdf)")
        if not p:
            return
        if not p.lower().endswith(".pdf"):
            p += ".pdf"
        self.canvas.fig.savefig(p, bbox_inches="tight")


def main():
    app = QApplication(sys.argv)
    w = MainWindow()
    w.resize(1200, 720)
    w.show()
    sys.exit(app.exec())


if __name__ == "__main__":
    main()
