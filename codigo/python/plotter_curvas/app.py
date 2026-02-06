import os
import sys
import numpy as np
from PyQt6.QtCore import Qt, QPoint
from PyQt6.QtGui import QCursor
from PyQt6.QtWidgets import (
    QApplication, QMainWindow, QWidget, QFileDialog, QVBoxLayout, QHBoxLayout,
    QPushButton, QLabel, QComboBox, QMessageBox, QLineEdit, QGroupBox, QFormLayout, QFrame, QScrollArea, QColorDialog,
    QListWidget, QListWidgetItem
)
import json
from matplotlib.backends.backend_qtagg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.backends.backend_qtagg import NavigationToolbar2QT as NavigationToolbar
from matplotlib.figure import Figure

from ltspice_io import read_ltspice_table, read_ltspice_steps
from plot_tools import THEMES, SCALE_MAP, pick_auto_scale, apply_theme, apply_layout, theme_curve_colors, use_theme_style
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

        self.data1 = None  # tabla NxM (sin .step)
        self.steps = None  # lista de bloques (con .step)
        self.file1_path = None
        self.x1 = self.y1 = None
        self.colnames1 = None
        self.lines = []
        self._signal_names = {}
        self.datasets = []

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

        # Estilos (persistencia)
        self._style_cache = {}  # re-plot inmediato (mismo dataset)
        self._curve_style = {
            "color": None,
            "linewidth": 2.5,
            "linestyle": "-",
            "marker": None,
            "alpha": 1.0,
            "visible": True,
        }
        self._curve_color_custom = False

        root = QWidget()
        self.setCentralWidget(root)
        layout = QHBoxLayout(root)
        controls = QVBoxLayout()
        controls_widget = QWidget()
        controls_widget.setLayout(controls)
        scroll = QScrollArea()
        scroll.setWidgetResizable(True)
        scroll.setWidget(controls_widget)
        scroll.setMinimumWidth(320)
        layout.addWidget(scroll, 0)

        # Archivos
        grp_files = QGroupBox("Archivos")
        fl = QVBoxLayout(grp_files)

        b1 = QPushButton("Abrir archivo(s)…")
        b1.clicked.connect(self.open1)

        self.l1 = QLabel("Archivos: (ninguno seleccionado)")

        fl.addWidget(b1)
        fl.addWidget(self.l1)

        self.cmb_file1 = QComboBox()
        self.cmb_file1.currentIndexChanged.connect(self._on_file1_column_changed)
        self.cmb_file1.addItem("(sin datos)")
        self.cmb_file1.setEnabled(False)

        fl.addWidget(QLabel("Curva (columna):"))
        fl.addWidget(self.cmb_file1)
        controls.addWidget(grp_files)

        # Opciones
        grp_opts = QGroupBox("Opciones")
        form = QFormLayout(grp_opts)

        self.cmb_theme = QComboBox()
        self.cmb_theme.addItems(list(THEMES.keys()))
        self.cmb_theme.setCurrentText("Paper")


        self.cmb_mode = QComboBox()
        self.cmb_mode.addItems(
            ["1 curva", "N curvas (mismo eje Y)"]
        )
        self.cmb_mode.currentIndexChanged.connect(self._sync_plot_selection_ui)

        self.cmb_x = QComboBox()
        self.cmb_x.addItems(["s", "ms"])
        self.cmb_x.setCurrentText("ms")

        self.cmb_y1 = QComboBox()
        self.cmb_y1.addItems(["Auto"] + list(SCALE_MAP.keys()))

        # info de escala aplicada (SIN notación científica)
        self.lbl_y1_factor = QLabel("x1")

        # actualizar factor mostrado al cambiar selección
        self.cmb_y1.currentTextChanged.connect(self._update_factor_labels)

        self.cmb_legend = QComboBox()
        self.cmb_legend.addItems(
            ["Auto (best)", "Afuera derecha", "Afuera abajo", "Arriba derecha", "Arriba izquierda", "Abajo derecha", "Abajo izquierda"]
        )

        self.tit = QLineEdit("LTspice plot")
        self.xlab = QLineEdit("time")
        self.y1lab = QLineEdit("Y1")

        # Nombres de curvas (independientes de los labels de ejes)
        self.c1name = QLineEdit("Curva 1")
        self.c1name.editingFinished.connect(self.apply_curve_names_now)

        self.lst_signals = QListWidget()
        self.lst_signals.itemChanged.connect(self._on_signal_item_changed)

        form.addRow("Tema:", self.cmb_theme)
        form.addRow("Modo:", self.cmb_mode)
        form.addRow("X:", self.cmb_x)
        form.addRow("Escala Y1:", self.cmb_y1)
        form.addRow("Factor Y1 aplicado:", self.lbl_y1_factor)
        form.addRow("Leyenda:", self.cmb_legend)
        form.addRow("Título:", self.tit)
        form.addRow("X label:", self.xlab)
        form.addRow("Y1 label:", self.y1lab)
        form.addRow("Nombre curva:", self.c1name)
        form.addRow("Señales a graficar (modo N):", self.lst_signals)
        controls.addWidget(grp_opts)
        grp_style = QGroupBox("Estilo de curvas")
        form_s = QFormLayout(grp_style)

        self.btn_color = QPushButton("Color…")
        self.btn_color.clicked.connect(self.edit_color)

        self.cmb_ls = QComboBox()
        self.cmb_ls.addItems(["-", "--", ":", "-."])
        self.cmb_ls.currentTextChanged.connect(self.edit_style)

        self.cmb_marker = QComboBox()
        self.cmb_marker.addItems(["None", "o", "s", "^", "x", "+"])
        self.cmb_marker.currentTextChanged.connect(self.edit_style)

        self.spin_lw = QLineEdit("2.5")
        self.spin_lw.editingFinished.connect(self.edit_style)

        btn_save = QPushButton("Guardar preset…")
        btn_save.clicked.connect(self.save_preset)

        btn_load = QPushButton("Cargar preset…")
        btn_load.clicked.connect(self.load_preset)

        form_s.addRow("Color:", self.btn_color)
        form_s.addRow("Línea:", self.cmb_ls)
        form_s.addRow("Marcador:", self.cmb_marker)
        form_s.addRow("Grosor:", self.spin_lw)
        form_s.addRow(btn_save)
        form_s.addRow(btn_load)

        controls.addWidget(grp_style)
        self._sync_style_ui_from_curve()

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

        bh = QPushButton("Ayuda")
        bh.clicked.connect(self.show_help)

        controls.addWidget(bp)
        controls.addWidget(bs_png)
        controls.addWidget(bs_svg)
        controls.addWidget(bs_pdf)
        controls.addWidget(br)
        controls.addWidget(bh)
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

        # Initialize factor labels
        self._update_factor_labels()
        self._sync_plot_selection_ui()


    # -------------------------
    # Helpers: escala + estilos
    # -------------------------
    def _xscale(self):
        return 1e3 if self.cmb_x.currentText() == "ms" else 1.0

    def _distinct_colors(self, count: int):
        if count <= 0:
            return []
        theme = THEMES[self.cmb_theme.currentText()]
        colors = theme_curve_colors(theme, count)
        if colors:
            return colors
        return []

    def _factor_str(self, f: float) -> str:
        """Formato SIN notación científica: x1000 / x0.001 / x1"""
        if f is None:
            return "x1"
        f = float(f)
        if f == 1.0:
            return "x1"

        # Si es casi entero, mostrar entero
        if abs(f - round(f)) < 1e-12:
            return f"x{int(round(f))}"

        # Decimal fijo (sin exponentes), recortando ceros
        s = format(f, "f")
        if "." in s:
            s = s.rstrip("0").rstrip(".")
        return f"x{s}"

    def _update_factor_labels(self):
        # Por ahora sólo refleja lo que diga el combo si no hay datos aún
        y1_txt = self.cmb_y1.currentText()
        self.lbl_y1_factor.setText("Auto" if y1_txt.startswith("Auto") else self._factor_str(SCALE_MAP.get(y1_txt, 1.0)))

    def _populate_column_combo(self, combo: QComboBox, colnames: tuple[str, ...] | None):
        """
        Llena el combo con columnas Y (colnames[1:]).
        """
        combo.blockSignals(True)
        combo.clear()

        if not colnames or len(colnames) < 2:
            combo.addItem("(sin datos)")
            combo.setEnabled(False)
            combo.blockSignals(False)
            return

        for name in colnames[1:]:
            combo.addItem(name)

        combo.setEnabled(True)
        combo.blockSignals(False)

    def _refresh_signal_list(self):
        """
        Modo N:
          - lista = columnas de todos los archivos cargados
        """
        self.lst_signals.blockSignals(True)
        self.lst_signals.clear()
        default_checked = False

        for ds_idx, ds in enumerate(self.datasets):
            colnames = ds.get("colnames")
            if not colnames or len(colnames) < 2:
                continue
            fname = ds.get("name", f"archivo_{ds_idx+1}")
            for col_idx, name in enumerate(colnames[1:], start=1):
                key = (ds_idx, col_idx)
                base_name = self._signal_names.get(key, name)
                label = f"[{fname}] {base_name}"
                if ds.get("steps"):
                    label += " (todos los steps)"
                item = QListWidgetItem(label)
                item.setFlags(
                    item.flags()
                    | Qt.ItemFlag.ItemIsUserCheckable
                    | Qt.ItemFlag.ItemIsEditable
                )
                if not default_checked:
                    item.setCheckState(Qt.CheckState.Checked)
                    default_checked = True
                else:
                    item.setCheckState(Qt.CheckState.Unchecked)
                item.setData(Qt.ItemDataRole.UserRole, ("DSCOL", ds_idx, col_idx, name))
                self.lst_signals.addItem(item)

        if self.lst_signals.count() == 0:
            item = QListWidgetItem("(sin datos)")
            item.setFlags(item.flags() & ~Qt.ItemFlag.ItemIsUserCheckable)
            self.lst_signals.addItem(item)

        self.lst_signals.setEnabled(True)
        self.lst_signals.blockSignals(False)

    def _on_signal_item_changed(self, item: QListWidgetItem):
        payload = item.data(Qt.ItemDataRole.UserRole)
        if not payload:
            return
        kind = payload[0]
        if kind != "DSCOL":
            return
        _, ds_idx, col_idx, default_name = payload
        raw = item.text().replace(" (todos los steps)", "").strip()
        if "] " in raw:
            new_name = raw.split("] ", 1)[1].strip()
        else:
            new_name = raw
        if not new_name:
            new_name = default_name
        self._signal_names[(ds_idx, col_idx)] = new_name


    def _selected_columns(self) -> list[tuple[str, int, str]]:
        selected = []
        for i in range(self.lst_signals.count()):
            item = self.lst_signals.item(i)
            if not item.flags() & Qt.ItemFlag.ItemIsUserCheckable:
                continue
            if item.checkState() == Qt.CheckState.Checked:
                payload = item.data(Qt.ItemDataRole.UserRole)
                if payload:
                    selected.append(payload)
        return selected

    def _commit_signal_names_from_list(self):
        """Sincroniza nombres editados en la lista (modo N), incluso si no disparó itemChanged."""
        for i in range(self.lst_signals.count()):
            item = self.lst_signals.item(i)
            payload = item.data(Qt.ItemDataRole.UserRole) if item else None
            if not payload or payload[0] != "DSCOL":
                continue

            _, ds_idx, col_idx, default_name = payload
            raw = item.text().replace(" (todos los steps)", "").strip()
            if "] " in raw:
                new_name = raw.split("] ", 1)[1].strip()
            else:
                new_name = raw
            self._signal_names[(ds_idx, col_idx)] = new_name or default_name

    def _on_file1_column_changed(self):
        if self.colnames1 is None:
            return

        idx = self.cmb_file1.currentIndex()
        col_idx = idx + 1  # porque el combo lista colnames[1:]

        # Fuente de datos: tabla normal o primer step
        base = None
        if self.data1 is not None:
            base = self.data1
        elif self.steps:
            base = self.steps[0]["data"]

        if base is None or col_idx >= base.shape[1]:
            return

        self.x1 = base[:, 0]
        self.y1 = base[:, col_idx]

        if len(self.colnames1) > col_idx:
            self.y1lab.setText(self.colnames1[col_idx])
            if self.c1name.text().strip() in ("", "Curva 1"):
                self.c1name.setText(self.colnames1[col_idx])



    def _scale(self, combo: QComboBox, y, info_label: QLabel):
        t = combo.currentText()
        if t.startswith("Auto"):
            f = pick_auto_scale(y)
            combo.setItemText(0, "Auto" if f == 1.0 else f"Auto ({self._factor_str(f)})")
            info_label.setText("Auto" if f == 1.0 else self._factor_str(f))
            return f

        f = SCALE_MAP.get(t, 1.0)
        info_label.setText(self._factor_str(f))
        return f

    def _capture_line_styles(self):
        """
        Guarda el estilo actual de las curvas:
        - cache por label/índice (para re-plot inmediato)
        """
        cache = {}
        for i, line in enumerate(getattr(self, "lines", []) or []):
            if line is None:
                continue

            key = line.get_label() or f"__idx_{i}"
            st = {
                "color": line.get_color(),
                "linestyle": line.get_linestyle(),
                "linewidth": line.get_linewidth(),
                "marker": line.get_marker(),
                "markersize": line.get_markersize(),
                "markerfacecolor": line.get_markerfacecolor(),
                "markeredgecolor": line.get_markeredgecolor(),
                "alpha": line.get_alpha(),
                "visible": line.get_visible(),
            }
            cache[key] = st

        self._style_cache = cache

    def _apply_style_dict_to_line(self, line, st: dict, allow_color: bool = True):
        if line is None or not st:
            return

        if allow_color and st.get("color") is not None:
            line.set_color(st["color"])
        if st.get("linestyle") is not None:
            line.set_linestyle(st["linestyle"])
        if st.get("linewidth") is not None:
            line.set_linewidth(st["linewidth"])

        # marker: "None" (string) lo quita siempre
        mk = st.get("marker", "None")
        if mk is None:
            mk = "None"
        line.set_marker(mk)

        if st.get("markersize") is not None:
            line.set_markersize(st["markersize"])
        if st.get("markerfacecolor") is not None:
            line.set_markerfacecolor(st["markerfacecolor"])
        if st.get("markeredgecolor") is not None:
            line.set_markeredgecolor(st["markeredgecolor"])
        if st.get("alpha") is not None:
            line.set_alpha(st["alpha"])
        if st.get("visible") is not None:
            line.set_visible(st["visible"])

    def _apply_line_styles(self, allow_color: bool = True):
        """
        Reaplica estilos a las curvas recién creadas.
        Prioridad:
        1) _style_cache (re-plot inmediato)
        2) _curve_style (preset actual)
        """
        for i, line in enumerate(getattr(self, "lines", []) or []):
            if line is None:
                continue

            key = line.get_label() or f"__idx_{i}"
            st = None

            if self._style_cache:
                st = self._style_cache.get(key) or self._style_cache.get(f"__idx_{i}")

            if st is None:
                st = self._curve_style

            if not st:
                continue
            use_color = allow_color
            if st is self._curve_style and not self._curve_color_custom:
                use_color = False
            self._apply_style_dict_to_line(line, st, allow_color=use_color)

    # -------------------------
    # File I/O
    # -------------------------
    def open1(self):
        paths, _ = QFileDialog.getOpenFileNames(self, "Abrir", "", "Text files (*.txt *.csv);;All (*.*)")
        if not paths:
            return
        try:
            self.datasets = []
            for p in paths:
                _, cols, steps = read_ltspice_steps(p, "auto")
                if steps:
                    self.datasets.append({
                        "path": p,
                        "name": os.path.basename(p),
                        "data": None,
                        "steps": steps,
                        "colnames": cols,
                    })
                else:
                    data, _, cols = read_ltspice_table(p, "auto")
                    self.datasets.append({
                        "path": p,
                        "name": os.path.basename(p),
                        "data": data,
                        "steps": None,
                        "colnames": cols,
                    })

            first = self.datasets[0]
            self.file1_path = first["path"]
            self.steps = first["steps"]
            self.data1 = first["data"]
            self.colnames1 = first["colnames"]

            self._populate_column_combo(self.cmb_file1, self.colnames1)
            self._on_file1_column_changed()

            if self.colnames1:
                self.xlab.setText(self.colnames1[0])

            if len(self.datasets) == 1:
                self.l1.setText(f"Archivo: {self.datasets[0]['name']}")
            else:
                self.l1.setText(f"Archivos: {len(self.datasets)} cargados")

            self._refresh_signal_list()

        except Exception as e:
            QMessageBox.critical(self, "Error", str(e))

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

    def refresh_legend(self):
        """Recrea la leyenda usando los Line2D actuales, para que copie color/estilo real."""
        if self.ax1 is None:
            return

        # limpiar mapping de leyenda anterior (artistas viejos)
        self.leg_map = {}

        # borrar leyenda previa
        if self.legend is not None:
            try:
                self.legend.remove()
            except Exception:
                pass
            self.legend = None

        handles = [l for l in (self.lines or []) if l is not None]
        if not handles:
            return
        labels = [h.get_label() for h in handles]

        self.legend = self._apply_legend(self.ax1, handles, labels)
        self._enable_legend_picking()

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

        # persistir visibilidad al slot
        if line is self.lines[0]:
            self._curve_style["visible"] = line.get_visible()

        self.canvas.draw_idle()

    # -------------------------
    # Crosshairs
    # -------------------------
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

    # -------------------------
    # Highlight
    # -------------------------
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

    # -------------------------
    # Plot
    # -------------------------

    def apply_curve_names_now(self):
        """Aplica el nombre de la curva actual sin re-graficar."""
        if not getattr(self, "lines", None):
            return
        if len(self.lines) >= 1:
            self.lines[0].set_label(self.c1name.text().strip() or "Curva 1")
        # refrescar leyenda para reflejar los nuevos nombres
        self.refresh_legend()
        self.canvas.draw_idle()


    def plot(self):
        if self.data1 is None and not self.steps:
            QMessageBox.warning(self, "Falta", "Cargá un archivo")
            return

        xsc = self._xscale()
        mode = self.cmb_mode.currentIndex()
        if mode == 1:
            self._commit_signal_names_from_list()

        use_theme_style(THEMES[self.cmb_theme.currentText()])

        y1src = None
        if self.steps:
            if mode == 0:
                col1 = self.cmb_file1.currentIndex() + 1
                arr = self.steps[0]["data"]
                if col1 < arr.shape[1]:
                    y1src = arr[:, col1]
            else:
                selected = self._selected_columns()
                ys1 = []
                for kind, ds_idx, col_idx, _ in selected:
                    if kind != "DSCOL":
                        continue
                    ds = self.datasets[ds_idx]
                    if ds.get("steps"):
                        for st in ds["steps"]:
                            arr = st["data"]
                            if col_idx < arr.shape[1]:
                                ys1.append(arr[:, col_idx])
                    elif ds.get("data") is not None and col_idx < ds["data"].shape[1]:
                        ys1.append(ds["data"][:, col_idx])
                y1src = np.concatenate(ys1) if ys1 else None
        else:
            if mode == 0:
                y1src = self.y1
            else:
                selected = self._selected_columns()
                ys1 = []
                for kind, ds_idx, col_idx, _ in selected:
                    if kind != "DSCOL":
                        continue
                    ds = self.datasets[ds_idx]
                    if ds.get("steps"):
                        for st in ds["steps"]:
                            arr = st["data"]
                            if col_idx < arr.shape[1]:
                                ys1.append(arr[:, col_idx])
                    elif ds.get("data") is not None and col_idx < ds["data"].shape[1]:
                        ys1.append(ds["data"][:, col_idx])
                y1src = np.concatenate(ys1) if ys1 else None

        y1sc = self._scale(self.cmb_y1, y1src, self.lbl_y1_factor) if y1src is not None else 1.0

        # Guardar estilos actuales antes de borrar el figure (para no perder cambios)
        self._capture_line_styles()

        self.canvas.fig.clear()
        ax1 = self.canvas.fig.add_subplot(111)
        self.ax1 = ax1
        self.ax2 = None

        ax1.set_title(self.tit.text())
        ax1.set_xlabel(f"{self.xlab.text()} [{self.cmb_x.currentText()}]")

        # IMPORTANTE: el usuario pidió sin notación/escala en el label => SOLO el texto del usuario
        ax1.set_ylabel(self.y1lab.text())

        self.lines = []

        if mode == 1:
            selected = self._selected_columns()
            if not selected:
                QMessageBox.warning(self, "Falta", "Seleccioná al menos una señal en modo N.")
                return

            total = 0
            for kind, ds_idx, col_idx, _ in selected:
                if kind != "DSCOL":
                    continue
                ds = self.datasets[ds_idx]
                if ds.get("steps"):
                    for st in ds["steps"]:
                        arr = st["data"]
                        if col_idx < arr.shape[1]:
                            total += 1
                elif ds.get("data") is not None and col_idx < ds["data"].shape[1]:
                    total += 1

            colors = self._distinct_colors(total)
            color_idx = 0
            for kind, ds_idx, col_idx, name in selected:
                if kind != "DSCOL":
                    continue
                ds = self.datasets[ds_idx]
                base_name = self._signal_names.get((ds_idx, col_idx), name)
                file_name = ds.get("name", f"archivo_{ds_idx+1}")

                if ds.get("steps"):
                    for st in ds["steps"]:
                        arr = st["data"]
                        if col_idx >= arr.shape[1]:
                            continue
                        x = arr[:, 0]
                        y = arr[:, col_idx]
                        label = f"[{file_name}] {base_name} | {st.get('label', 'Step')}"
                        p = ax1.plot(x * xsc, y * y1sc, label=label)
                        if colors:
                            p[0].set_color(colors[color_idx])
                        color_idx += 1
                        self.lines.append(p[0])
                elif ds.get("data") is not None and col_idx < ds["data"].shape[1]:
                    arr = ds["data"]
                    x = arr[:, 0]
                    y = arr[:, col_idx]
                    label = f"[{file_name}] {base_name}"
                    p = ax1.plot(x * xsc, y * y1sc, label=label)
                    if colors:
                        p[0].set_color(colors[color_idx])
                    color_idx += 1
                    self.lines.append(p[0])
        else:
            if self.steps:
                base1 = self.c1name.text().strip() or "Curva 1"
                col1 = self.cmb_file1.currentIndex() + 1
                arr = self.steps[0]["data"]
                if col1 >= arr.shape[1]:
                    QMessageBox.warning(self, "Falta", "Seleccioná una columna válida.")
                    return
                x = arr[:, 0]
                y = arr[:, col1]
                label = f"{base1} | {self.steps[0].get('label','Step')}"
                p = ax1.plot(x * xsc, y * y1sc, label=label)
                self.lines.append(p[0])
            else:
                p1 = ax1.plot(self.x1 * xsc, self.y1 * y1sc, label=(self.c1name.text().strip() or "Curva 1"))
                self.lines = [p1[0]]
        self._apply_line_styles(allow_color=(mode == 0))
        # Leyenda siempre sincronizada con estilos reales + clickeable
        self.refresh_legend()

        # Crosshairs persistentes (si había probes)
        self._update_crosshairs()

        apply_layout(self.canvas.fig, self.cmb_legend.currentText())
        apply_theme(self.canvas.fig, THEMES[self.cmb_theme.currentText()])
        self.canvas.draw()

    # -------------------------
    # Probe UI
    # -------------------------
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

    # -------------------------
    # Export
    # -------------------------
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

    #-------------------------
    # Logica del editor de curvas
    #-------------------------

    def _sync_style_ui_from_curve(self):
        """Actualiza los controles UI según el estilo guardado actual."""
        st = self._curve_style or {}

        ls = st.get("linestyle", "-")
        mk = st.get("marker", None)
        lw = st.get("linewidth", 2.5)

        # setear combos sin romper si el valor no existe
        if ls in [self.cmb_ls.itemText(i) for i in range(self.cmb_ls.count())]:
            self.cmb_ls.setCurrentText(ls)

        mk_txt = "None" if (mk is None or mk == "None") else str(mk)
        if mk_txt in [self.cmb_marker.itemText(i) for i in range(self.cmb_marker.count())]:
            self.cmb_marker.setCurrentText(mk_txt)

        self.spin_lw.setText(str(lw))

    def edit_color(self):
        c = QColorDialog.getColor(parent=self)
        if not c.isValid():
            return
        self._curve_style["color"] = c.name()
        self._curve_color_custom = True

        if self.lines:
            allow_color = self.cmb_mode.currentIndex() == 0
            for line in self.lines:
                self._apply_style_dict_to_line(line, self._curve_style, allow_color=allow_color)

        self._style_cache = {}
        self.refresh_legend()
        self.canvas.draw_idle()
        self._capture_line_styles()

    def edit_style(self):
        st = self._curve_style

        st["linestyle"] = self.cmb_ls.currentText()

        mk = self.cmb_marker.currentText()
        st["marker"] = "None" if mk == "None" else mk

        try:
            st["linewidth"] = float(self.spin_lw.text().replace(",", "."))
        except Exception:
            pass

        self._curve_style = st

        if self.lines:
            allow_color = self.cmb_mode.currentIndex() == 0
            for line in self.lines:
                self._apply_style_dict_to_line(line, st, allow_color=allow_color)

        self._style_cache = {}
        self.refresh_legend()
        self.canvas.draw_idle()
        self._capture_line_styles()

    def save_preset(self):
        p, _ = QFileDialog.getSaveFileName(self, "Guardar preset", "style.json", "JSON (*.json)")
        if not p:
            return
        if not p.lower().endswith(".json"):
            p += ".json"

        data = {"curve": self._curve_style}
        with open(p, "w", encoding="utf8") as f:
            json.dump(data, f, indent=2)

    def load_preset(self):
        p, _ = QFileDialog.getOpenFileName(self, "Cargar preset", "", "JSON (*.json)")
        if not p:
            return

        with open(p, "r", encoding="utf8") as f:
            data = json.load(f)

        self._curve_style = data.get("curve") or self._curve_style
        self._curve_color_custom = bool(
            isinstance(self._curve_style, dict) and self._curve_style.get("color")
        )

        if isinstance(self._curve_style, dict) and self._curve_style.get("marker", "__missing__") is None:
            self._curve_style["marker"] = "None"

        self._sync_style_ui_from_curve()

        self._style_cache = {}
        self._apply_line_styles(allow_color=(self.cmb_mode.currentIndex() == 0))
        self.refresh_legend()
        self.canvas.draw_idle()
        self._capture_line_styles()

    def _sync_plot_selection_ui(self, *_):
        """
        Habilita/deshabilita controles según el modo seleccionado.
        Modos:
          0: 1 curva
          1: N curvas (mismo Y)
        """
        mode = self.cmb_mode.currentIndex()
        has_data = (self.data1 is not None) or bool(self.steps)

        # combos de columnas
        self.cmb_file1.setEnabled(has_data)

        if mode == 0:
            # 1 curva
            self.lst_signals.setEnabled(False)

        else:
            # N curvas
            self.lst_signals.setEnabled(has_data)
            if has_data:
                self._refresh_signal_list()

    # -------------------------
    # Help
    # -------------------------
    def show_help(self):
        from PyQt6.QtWidgets import QDialog, QTextEdit, QVBoxLayout

        dlg = QDialog(self)
        dlg.setWindowTitle("Ayuda — LTspice Plotter")
        dlg.resize(700, 600)

        text = QTextEdit()
        text.setReadOnly(True)

        help_text = """
<h2>LTspice Plotter — Manual rápido</h2>

<b>Cargar archivos</b><br>
Abrir archivo(s) (uno o más)<br><br>

<b>Selección de señales</b><br>
Columnas → elegí la señal para curva única<br>
Modo N → tildá múltiples señales en la lista<br><br>

<b>Archivos con .step</b><br>
En modo N, cada señal seleccionada se grafica para todos los steps<br><br>

<b>Modos de gráfico</b><br>
1 curva → una sola señal<br>
N curvas mismo eje → múltiples señales de uno o más archivos<br><br>

<b>Escalas</b><br>
Elegí la escala en “Escala Y1”.<br>
El <i>factor aplicado</i> se muestra a la izquierda y no modifica el texto del label.<br><br>

<b>Cursores A/B</b><br>
Click = fija A<br>
Shift + Click = fija B<br>
Muestra ΔX y ΔY automáticamente<br><br>

<b>Resaltar curva</b><br>
Doble click sobre una curva<br><br>

<b>Leyenda interactiva</b><br>
Click en la leyenda para ocultar/mostrar curvas<br>
Se puede arrastrar<br><br>

<b>Crosshair</b><br>
Al fijar A/B se dibujan líneas guía<br><br>

<b>Exportación</b><br>
PNG → imagen<br>
SVG/PDF → vectorial (papers / tesis)<br>
"""

        text.setHtml(help_text)

        lay = QVBoxLayout(dlg)
        lay.addWidget(text)

        dlg.exec()


def main():
    app = QApplication(sys.argv)
    w = MainWindow()
    w.resize(1200, 720)
    w.show()
    sys.exit(app.exec())


if __name__ == "__main__":
    main()
