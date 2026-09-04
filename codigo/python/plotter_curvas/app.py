import os
import sys
import shutil
import ast
import numpy as np
import pickle
from PyQt6.QtCore import Qt, QPoint
from PyQt6.QtGui import QCursor
from PyQt6.QtWidgets import (
    QApplication, QMainWindow, QWidget, QFileDialog, QVBoxLayout, QHBoxLayout,
    QPushButton, QLabel, QComboBox, QMessageBox, QLineEdit, QGroupBox, QFormLayout, QFrame, QScrollArea, QColorDialog,
    QListWidget, QListWidgetItem, QCheckBox, QDialog, QDialogButtonBox, QTabWidget
)
import json
import matplotlib as mpl
from matplotlib.backends.backend_qtagg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.backends.backend_qtagg import NavigationToolbar2QT as NavigationToolbar
from matplotlib.figure import Figure
from matplotlib.ticker import ScalarFormatter
from pathlib import Path


from ltspice_io import read_ltspice_table, read_ltspice_steps
from plot_tools import THEMES, SCALE_MAP, pick_auto_scale, apply_layout, theme_curve_colors, use_theme_style
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
        self._line_keys=[]
        self._signal_names = {}
        self._line_name_overrides = {}
        self._line_transforms = {}
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
        self.axes = []
        self.legends = []
        self.secondary_axes = {}

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
        self._latex_warning_shown = False
        self._theme_change_pending = False
        self._manual_panel_assignments = {}

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

        btn_save_project = QPushButton("Guardar proyecto...")
        btn_save_project.clicked.connect(self.save_project)

        btn_open_project = QPushButton("Abrir proyecto...")
        btn_open_project.clicked.connect(self.open_project)

        self.l1 = QLabel("Archivos: (ninguno seleccionado)")

        fl.addWidget(b1)
        fl.addWidget(btn_save_project)
        fl.addWidget(btn_open_project)
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
        self.cmb_theme.setCurrentText("ggplot")
        self.cmb_theme.currentTextChanged.connect(self._on_theme_changed)


        self.cmb_mode = QComboBox()
        self.cmb_mode.addItems(
            ["1 curva", "N curvas (mismo eje Y)"]
        )
        self.cmb_mode.currentIndexChanged.connect(self._sync_plot_selection_ui)

        self.cmb_subplot_layout = QComboBox()
        self.cmb_subplot_layout.addItems(["1x1", "1x2", "2x1", "2x2"])
        self.cmb_subplot_layout.currentTextChanged.connect(self._replot_if_data_loaded)

        self.ed_subplot_titles = QLineEdit()
        self.ed_subplot_titles.setPlaceholderText("Panel 1; Panel 2; Panel 3; Panel 4")

        self.chk_manual_panels = QCheckBox("Asignación manual de paneles")
        self.chk_manual_panels.stateChanged.connect(self._on_manual_panels_toggled)
        self.btn_panel_assignments = QPushButton("Seleccionar curvas por panel...")
        self.btn_panel_assignments.clicked.connect(self.edit_panel_assignments)

        self.cmb_x = QComboBox()
        self.cmb_x.addItems(["x1", "x1e3", "x1e-3", "x1e6", "x1e-6", "x1e9", "x1e-9"])
        self.cmb_x.setCurrentText("x1")
        self.cmb_x.currentTextChanged.connect(self._replot_if_data_loaded)

        self.cmb_y1 = QComboBox()
        self.cmb_y1.addItems(["Auto"] + list(SCALE_MAP.keys()))
        self.cmb_y1.currentIndexChanged.connect(self._replot_if_data_loaded)

        self.chk_y2 = QCheckBox("Usar eje Y derecho")
        self.chk_y2.stateChanged.connect(self._replot_if_data_loaded)
        self.chk_y2.stateChanged.connect(self._sync_y2_ui)

        self.cmb_y2_signal = QComboBox()
        self.cmb_y2_signal.addItem("(ninguna)", None)
        self.cmb_y2_signal.currentIndexChanged.connect(self._replot_if_data_loaded)

        self.cmb_y2 = QComboBox()
        self.cmb_y2.addItems(["Auto"] + list(SCALE_MAP.keys()))
        self.cmb_y2.currentIndexChanged.connect(self._replot_if_data_loaded)
        self.cmb_y2.currentTextChanged.connect(self._update_factor_labels)

        self.y2lab = QLineEdit("Y2")

        # info de escala aplicada (SIN notación científica)
        self.lbl_y1_factor = QLabel("x1")
        self.lbl_y2_factor = QLabel("x1")

        # actualizar factor mostrado al cambiar selección
        self.cmb_y1.currentTextChanged.connect(self._update_factor_labels)

        self.cmb_legend = QComboBox()
        self.cmb_legend.addItems(
            ["Auto (best)", "Afuera derecha", "Afuera abajo", "Arriba derecha", "Arriba izquierda", "Abajo derecha", "Abajo izquierda"]
        )
        self.cmb_legend.currentTextChanged.connect(self._replot_if_data_loaded)

        self.btn_best_legend = QPushButton("Ubicar leyenda automaticamente")
        self.btn_best_legend.clicked.connect(self._set_best_legend_location)

        self.chk_cursors = QCheckBox("Habilitar cursores A/B")
        self.chk_cursors.setChecked(True)
        self.chk_cursors.stateChanged.connect(self._on_cursors_toggled)

        self.chk_usetex = QCheckBox("Usar LaTeX")
        self.chk_usetex.stateChanged.connect(self._on_text_rendering_changed)

        self.tit = QLineEdit("LTspice plot")
        self.xlab = QLineEdit("time")
        self.y1lab = QLineEdit("Y1")

        # Nombres de curvas (independientes de los labels de ejes)
        self.c1name = QLineEdit("Curva 1")
        self.c1name.editingFinished.connect(self.apply_curve_names_now)

        self.cmb_curve_name = QComboBox()
        self.cmb_curve_name.currentIndexChanged.connect(self._on_curve_name_selection_changed)
        self.ed_curve_name = QLineEdit()
        self.ed_curve_name.editingFinished.connect(self.rename_selected_curve)
        self.btn_curve_name = QPushButton("Aplicar nombre")
        self.btn_curve_name.clicked.connect(self.rename_selected_curve)

        self.ed_shift_x = QLineEdit("0")
        self.ed_shift_points = QLineEdit("0")
        self.ed_shift_y = QLineEdit("0")
        self.ed_y_expression = QLineEdit("curva")
        self.ed_y_expression.setPlaceholderText("Ej.: 3 - curva")
        self.ed_y_expression.setToolTip(
            "Usá 'curva' o 'y' con +, -, *, /, ** y paréntesis. Ejemplo: 3 - curva"
        )
        self.ed_trim_start = QLineEdit("0")
        self.ed_trim_end = QLineEdit("0")
        self.btn_curve_adjust = QPushButton("Aplicar ajuste")
        self.btn_curve_adjust.clicked.connect(self.apply_selected_curve_adjustment)
        self.btn_curve_adjust_reset = QPushButton("Reset ajuste")
        self.btn_curve_adjust_reset.clicked.connect(self.reset_selected_curve_adjustment)

        self.lst_signals = QListWidget()
        self.lst_signals.itemChanged.connect(self._on_signal_item_changed)

        # Operaciones entre curvas
        self.cmb_op_a = QComboBox()
        self.cmb_op_b = QComboBox()
        self.cmb_op = QComboBox()
        self.cmb_op.addItems(["Suma (+)", "Resta (-)"])
        self.chk_only_result = QCheckBox("Solo graficar resultado")
        self.btn_op_apply = QPushButton("Aplicar operación")
        self.btn_op_apply.clicked.connect(self.apply_curve_operation)

        form.addRow("Tema:", self.cmb_theme)
        form.addRow("Modo:", self.cmb_mode)
        form.addRow("Subfiguras:", self.cmb_subplot_layout)
        form.addRow("Titulos de paneles:", self.ed_subplot_titles)
        form.addRow(self.chk_manual_panels)
        form.addRow(self.btn_panel_assignments)
        form.addRow("Escala X:", self.cmb_x)
        form.addRow("Escala Y1:", self.cmb_y1)
        form.addRow("Factor Y1 aplicado:", self.lbl_y1_factor)
        form.addRow(self.chk_y2)
        form.addRow("Señal Y2:", self.cmb_y2_signal)
        form.addRow("Escala Y2:", self.cmb_y2)
        form.addRow("Factor Y2 aplicado:", self.lbl_y2_factor)
        form.addRow("Y2 label:", self.y2lab)
        form.addRow("Leyenda:", self.cmb_legend)
        form.addRow(self.btn_best_legend)
        form.addRow(self.chk_cursors)
        form.addRow("Texto:", self.chk_usetex)
        form.addRow("Título:", self.tit)
        form.addRow("X label:", self.xlab)
        form.addRow("Y1 label:", self.y1lab)
        form.addRow("Nombre curva:", self.c1name)
        form.addRow("Curva a renombrar:", self.cmb_curve_name)
        form.addRow("Nuevo nombre:", self.ed_curve_name)
        form.addRow(self.btn_curve_name)
        form.addRow("Mover X:", self.ed_shift_x)
        form.addRow("Mover X puntos:", self.ed_shift_points)
        form.addRow("Mover Y:", self.ed_shift_y)
        form.addRow("Ecuación Y:", self.ed_y_expression)
        form.addRow("Quitar puntos inicio:", self.ed_trim_start)
        form.addRow("Quitar puntos fin:", self.ed_trim_end)
        form.addRow(self.btn_curve_adjust)
        form.addRow(self.btn_curve_adjust_reset)
        form.addRow("Señales a graficar (modo N):", self.lst_signals)
        form.addRow("Curva A:", self.cmb_op_a)
        form.addRow("Operación:", self.cmb_op)
        form.addRow("Curva B:", self.cmb_op_b)
        form.addRow("Opciones operación:", self.chk_only_result)
        form.addRow(self.btn_op_apply)
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
        self._sync_y2_ui()
        self._refresh_operation_curve_combos()
        self._refresh_curve_name_controls()

        # Botones
        bp = QPushButton("Graficar")
        bp.clicked.connect(self.plot)

        bs_editable = QPushButton("Guardar figura editable…")
        bs_editable.clicked.connect(self.save_editable)

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
        controls.addWidget(bs_editable)
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
    def _configure_text_rendering(self):
        use_tex = self.chk_usetex.isChecked()
        if use_tex and shutil.which("latex") is None:
            if not self._latex_warning_shown:
                QMessageBox.warning(
                    self,
                    "LaTeX no disponible",
                    "No encontré una instalación de LaTeX. Podés usar formato tipo LaTeX entre $...$ igualmente.",
                )
                self._latex_warning_shown = True
            self.chk_usetex.blockSignals(True)
            self.chk_usetex.setChecked(False)
            self.chk_usetex.blockSignals(False)
            use_tex = False

        mpl.rcParams["text.usetex"] = use_tex

    def _on_text_rendering_changed(self):
        self._configure_text_rendering()
        if self.ax1 is not None:
            self.refresh_legend()
            self.canvas.draw_idle()

    def _replot_if_data_loaded(self, *_):
        if getattr(self, "data1", None) is not None or bool(getattr(self, "steps", None)):
            self.plot()

    def _on_theme_changed(self, *_):
        """Descarta colores automáticos anteriores y aplica la nueva paleta."""
        self._theme_change_pending = True
        self._replot_if_data_loaded()

    def _xscale(self):
        return {
            "x1": 1.0,
            "x1e3": 1e3,
            "x1e-3": 1e-3,
            "x1e6": 1e6,
            "x1e-6": 1e-6,
            "x1e9": 1e9,
            "x1e-9": 1e-9,
        }.get(self.cmb_x.currentText(), 1.0)

    def _disable_axis_factor_text(self, ax):
        if ax is None:
            return
        for axis in (ax.xaxis, ax.yaxis):
            fmt = ScalarFormatter(useOffset=False)
            fmt.set_scientific(False)
            axis.set_major_formatter(fmt)
            axis.get_offset_text().set_visible(False)

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
        y2_txt = self.cmb_y2.currentText()
        self.lbl_y2_factor.setText("Auto" if y2_txt.startswith("Auto") else self._factor_str(SCALE_MAP.get(y2_txt, 1.0)))

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
        self._refresh_y2_signal_combo()

    def _refresh_y2_signal_combo(self):
        """Actualiza las señales disponibles para el eje Y derecho."""
        previous = self.cmb_y2_signal.currentData()
        self.cmb_y2_signal.blockSignals(True)
        self.cmb_y2_signal.clear()
        self.cmb_y2_signal.addItem("(ninguna)", None)
        restore_index = 0
        for ds_idx, ds in enumerate(self.datasets):
            colnames = ds.get("colnames") or ()
            for col_idx, name in enumerate(colnames[1:], start=1):
                key = (ds_idx, col_idx)
                display_name = self._signal_names.get(key, name)
                self.cmb_y2_signal.addItem(f"[{ds.get('name', 'archivo')}] {display_name}", key)
                if previous is not None and tuple(previous) == key:
                    restore_index = self.cmb_y2_signal.count() - 1
        self.cmb_y2_signal.setCurrentIndex(restore_index)
        self.cmb_y2_signal.blockSignals(False)

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



    def _refresh_operation_curve_combos(self):
        self.cmb_op_a.blockSignals(True)
        self.cmb_op_b.blockSignals(True)
        self.cmb_op_a.clear()
        self.cmb_op_b.clear()

        if not self.lines:
            self.cmb_op_a.addItem("(sin curvas)")
            self.cmb_op_b.addItem("(sin curvas)")
            self.cmb_op_a.setEnabled(False)
            self.cmb_op_b.setEnabled(False)
            self.chk_only_result.setEnabled(False)
            self.btn_op_apply.setEnabled(False)
        else:
            for i, line in enumerate(self.lines):
                label = line.get_label() or f"Curva {i+1}"
                txt = f"{i+1}: {label}"
                self.cmb_op_a.addItem(txt, i)
                self.cmb_op_b.addItem(txt, i)
            self.cmb_op_a.setEnabled(True)
            self.cmb_op_b.setEnabled(True)
            self.chk_only_result.setEnabled(True)
            self.btn_op_apply.setEnabled(len(self.lines) >= 2)
            if len(self.lines) >= 2:
                self.cmb_op_b.setCurrentIndex(1)

        self.cmb_op_a.blockSignals(False)
        self.cmb_op_b.blockSignals(False)

    def _refresh_curve_name_controls(self, keep_index: int | None = None):
        self.cmb_curve_name.blockSignals(True)
        self.cmb_curve_name.clear()

        lines = [line for line in (self.lines or []) if line is not None]
        if not lines:
            self.cmb_curve_name.addItem("(sin curvas)", None)
            self.cmb_curve_name.setEnabled(False)
            self.ed_curve_name.setEnabled(False)
            self.btn_curve_name.setEnabled(False)
            self.ed_shift_x.setEnabled(False)
            self.ed_shift_points.setEnabled(False)
            self.ed_shift_y.setEnabled(False)
            self.ed_y_expression.setEnabled(False)
            self.ed_trim_start.setEnabled(False)
            self.ed_trim_end.setEnabled(False)
            self.btn_curve_adjust.setEnabled(False)
            self.btn_curve_adjust_reset.setEnabled(False)
            self.ed_curve_name.clear()
            self.cmb_curve_name.blockSignals(False)
            return

        for i, line in enumerate(lines):
            label = line.get_label() or f"Curva {i+1}"
            self.cmb_curve_name.addItem(f"{i+1}: {label}", i)

        self.cmb_curve_name.setEnabled(True)
        self.ed_curve_name.setEnabled(True)
        self.btn_curve_name.setEnabled(True)
        self.ed_shift_x.setEnabled(True)
        self.ed_shift_points.setEnabled(True)
        self.ed_shift_y.setEnabled(True)
        self.ed_y_expression.setEnabled(True)
        self.ed_trim_start.setEnabled(True)
        self.ed_trim_end.setEnabled(True)
        self.btn_curve_adjust.setEnabled(True)
        self.btn_curve_adjust_reset.setEnabled(True)

        if keep_index is not None and 0 <= keep_index < self.cmb_curve_name.count():
            self.cmb_curve_name.setCurrentIndex(keep_index)
        self.cmb_curve_name.blockSignals(False)
        self._on_curve_name_selection_changed()

    def _on_curve_name_selection_changed(self, *_):
        idx = self.cmb_curve_name.currentData()
        if idx is None or not self.lines or int(idx) >= len(self.lines):
            self.ed_curve_name.clear()
            self.ed_shift_x.setText("0")
            self.ed_shift_points.setText("0")
            self.ed_shift_y.setText("0")
            self.ed_y_expression.setText("curva")
            self.ed_trim_start.setText("0")
            self.ed_trim_end.setText("0")
            return
        self.ed_curve_name.setText(self.lines[int(idx)].get_label() or f"Curva {int(idx)+1}")
        tr = self._transform_for_line_index(int(idx))
        self.ed_shift_x.setText(str(tr.get("dx", 0.0)))
        self.ed_shift_points.setText(str(tr.get("shift_points", 0)))
        self.ed_shift_y.setText(str(tr.get("dy", 0.0)))
        self.ed_y_expression.setText(str(tr.get("y_expression", "curva")))
        self.ed_trim_start.setText(str(tr.get("trim_start", 0)))
        self.ed_trim_end.setText(str(tr.get("trim_end", 0)))

    def rename_selected_curve(self):
        idx = self.cmb_curve_name.currentData()
        if idx is None or not self.lines or int(idx) >= len(self.lines):
            return

        idx = int(idx)
        new_name = self.ed_curve_name.text().strip()
        if not new_name:
            new_name = f"Curva {idx+1}"

        self.lines[idx].set_label(new_name)
        if idx < len(self._line_keys):
            self._line_name_overrides[self._line_keys[idx]] = new_name

        if idx == 0:
            self.c1name.setText(new_name)

        self.refresh_legend()
        self._refresh_operation_curve_combos()
        self._refresh_curve_name_controls(keep_index=idx)
        self.canvas.draw_idle()

    def _parse_float_field(self, field: QLineEdit, default: float = 0.0) -> float:
        txt = field.text().strip().replace(",", ".")
        if not txt:
            return default
        return float(txt)

    def _parse_int_field(self, field: QLineEdit, default: int = 0) -> int:
        txt = field.text().strip()
        if not txt:
            return default
        return max(0, int(float(txt.replace(",", "."))))

    def _parse_signed_int_field(self, field: QLineEdit, default: int = 0) -> int:
        txt = field.text().strip()
        if not txt:
            return default
        return int(float(txt.replace(",", ".")))

    def _transform_for_line_index(self, idx: int) -> dict:
        if idx < len(self._line_keys):
            return dict(self._line_transforms.get(self._line_keys[idx], {}))
        return {}

    def _remember_line_base_data(self, line):
        if line is None:
            return
        line._base_xdata = np.asarray(line.get_xdata(orig=False), dtype=float).copy()
        line._base_ydata = np.asarray(line.get_ydata(orig=False), dtype=float).copy()

    def _evaluate_curve_expression(self, expression: str, y: np.ndarray) -> np.ndarray:
        """Evalúa una expresión aritmética segura usando la curva como variable."""
        expression = (expression or "curva").strip().replace(",", ".")
        tree = ast.parse(expression, mode="eval")

        binary_ops = {
            ast.Add: np.add,
            ast.Sub: np.subtract,
            ast.Mult: np.multiply,
            ast.Div: np.divide,
            ast.Pow: np.power,
        }
        unary_ops = {ast.UAdd: lambda value: value, ast.USub: np.negative}

        def evaluate(node):
            if isinstance(node, ast.Expression):
                return evaluate(node.body)
            if isinstance(node, ast.Constant) and isinstance(node.value, (int, float)):
                return float(node.value)
            if isinstance(node, ast.Name) and node.id.lower() in {"curva", "y"}:
                return y
            if isinstance(node, ast.BinOp) and type(node.op) in binary_ops:
                return binary_ops[type(node.op)](evaluate(node.left), evaluate(node.right))
            if isinstance(node, ast.UnaryOp) and type(node.op) in unary_ops:
                return unary_ops[type(node.op)](evaluate(node.operand))
            raise ValueError("Solo se permiten curva/y, números, +, -, *, /, ** y paréntesis.")

        result = np.asarray(evaluate(tree), dtype=float)
        if result.ndim == 0:
            result = np.full_like(y, float(result), dtype=float)
        if result.shape != y.shape:
            raise ValueError("La ecuación debe producir un valor por cada punto de la curva.")
        return result

    def _apply_transform_to_line(self, line, transform: dict) -> bool:
        if line is None:
            return False
        if not hasattr(line, "_base_xdata") or not hasattr(line, "_base_ydata"):
            self._remember_line_base_data(line)

        x = np.asarray(line._base_xdata, dtype=float)
        y = np.asarray(line._base_ydata, dtype=float)
        trim_start = max(0, int(transform.get("trim_start", 0) or 0))
        trim_end = max(0, int(transform.get("trim_end", 0) or 0))

        if trim_start + trim_end >= x.size:
            return False

        dx = float(transform.get("dx", 0.0) or 0.0)
        shift_points = int(transform.get("shift_points", 0) or 0)
        if shift_points:
            diffs = np.diff(x[np.isfinite(x)])
            diffs = diffs[np.isfinite(diffs) & (diffs != 0)]
            if diffs.size:
                dx += shift_points * float(np.median(diffs))

        end = x.size - trim_end if trim_end else x.size
        x = x[trim_start:end] + dx
        y = y[trim_start:end]
        y = self._evaluate_curve_expression(transform.get("y_expression", "curva"), y)
        y = y + float(transform.get("dy", 0.0) or 0.0)
        line.set_data(x, y)
        return True

    def _apply_line_transforms(self):
        for i, line in enumerate(getattr(self, "lines", []) or []):
            self._remember_line_base_data(line)
            if i < len(self._line_keys):
                transform = self._line_transforms.get(self._line_keys[i])
                if transform:
                    self._apply_transform_to_line(line, transform)

    def _refresh_axes_after_curve_adjustment(self):
        for ax in dict.fromkeys(line.axes for line in self.lines if line is not None):
            ax.relim()
            ax.autoscale_view()
            self._disable_axis_factor_text(ax)
        self.reset_probes()

    def apply_selected_curve_adjustment(self):
        idx = self.cmb_curve_name.currentData()
        if idx is None or not self.lines or int(idx) >= len(self.lines):
            return

        idx = int(idx)
        try:
            transform = {
                "dx": self._parse_float_field(self.ed_shift_x),
                "shift_points": self._parse_signed_int_field(self.ed_shift_points),
                "dy": self._parse_float_field(self.ed_shift_y),
                "y_expression": self.ed_y_expression.text().strip() or "curva",
                "trim_start": self._parse_int_field(self.ed_trim_start),
                "trim_end": self._parse_int_field(self.ed_trim_end),
            }
            self._evaluate_curve_expression(transform["y_expression"], np.asarray([1.0]))
        except (ValueError, SyntaxError) as exc:
            QMessageBox.warning(self, "Ajuste de curva", f"Revisá los valores o la ecuación:\n{exc}")
            return

        if not self._apply_transform_to_line(self.lines[idx], transform):
            QMessageBox.warning(self, "Ajuste de curva", "El recorte deja la curva sin puntos.")
            return

        if idx < len(self._line_keys):
            self._line_transforms[self._line_keys[idx]] = transform

        self._refresh_axes_after_curve_adjustment()
        self._refresh_operation_curve_combos()
        self._refresh_curve_name_controls(keep_index=idx)
        self.canvas.draw_idle()

    def reset_selected_curve_adjustment(self):
        idx = self.cmb_curve_name.currentData()
        if idx is None or not self.lines or int(idx) >= len(self.lines):
            return

        idx = int(idx)
        if idx < len(self._line_keys):
            self._line_transforms.pop(self._line_keys[idx], None)

        self._apply_transform_to_line(self.lines[idx], {})
        self._refresh_axes_after_curve_adjustment()
        self._refresh_curve_name_controls(keep_index=idx)
        self.canvas.draw_idle()

    def _align_curves(self, a, b):
        xa = np.asarray(a.get_xdata(), dtype=float)
        ya = np.asarray(a.get_ydata(), dtype=float)
        xb = np.asarray(b.get_xdata(), dtype=float)
        yb = np.asarray(b.get_ydata(), dtype=float)

        if xa.size == xb.size and np.allclose(xa, xb):
            return xa, ya, yb

        order = np.argsort(xb)
        xb_sorted = xb[order]
        yb_sorted = yb[order]
        # Dedup x for interpolation stability
        xb_unique, idx = np.unique(xb_sorted, return_index=True)
        yb_unique = yb_sorted[idx]
        yb_interp = np.interp(xa, xb_unique, yb_unique)
        return xa, ya, yb_interp

    def apply_curve_operation(self):
        if not self.lines or len(self.lines) < 2 or self.ax1 is None:
            QMessageBox.warning(self, "Operación", "Necesitás al menos 2 curvas graficadas.")
            return

        ia = self.cmb_op_a.currentData()
        ib = self.cmb_op_b.currentData()
        if ia is None or ib is None or ia == ib:
            QMessageBox.warning(self, "Operación", "Elegí dos curvas distintas.")
            return

        la = self.lines[int(ia)]
        lb = self.lines[int(ib)]
        target_ax = la.axes
        x, ya, yb = self._align_curves(la, lb)

        op = self.cmb_op.currentText()
        if op.startswith("Suma"):
            y = ya + yb
            symbol = "+"
        else:
            y = ya - yb
            symbol = "-"

        self._configure_text_rendering()
        label = f"({la.get_label()}) {symbol} ({lb.get_label()})"
        if self.chk_only_result.isChecked():
            # Reemplazar solo las curvas involucradas en la operacion. Las demas
            # curvas seleccionadas deben seguir visibles aunque se pida mostrar
            # unicamente el resultado de A y B.
            involved = {int(ia), int(ib)}
            remaining_lines = []
            remaining_keys = []
            for idx, (line, key) in enumerate(zip(self.lines, self._line_keys)):
                if idx in involved:
                    line.remove()
                else:
                    remaining_lines.append(line)
                    remaining_keys.append(key)

            p = target_ax.plot(x, y, label=label)
            self.lines = remaining_lines + [p[0]]
            self._line_keys = remaining_keys + [("OP_ONLY", int(ia), symbol, int(ib))]
            self._remember_line_base_data(p[0])
            for ax in dict.fromkeys(line.axes for line in self.lines if line is not None):
                ax.relim()
                ax.autoscale_view()
        else:
            p = target_ax.plot(x, y, label=label)
            self.lines.append(p[0])
            self._line_keys.append(("OP", int(ia), symbol, int(ib), len(self.lines)))
            self._remember_line_base_data(p[0])

        self._disable_axis_factor_text(target_ax)
        self.refresh_legend()
        self._refresh_operation_curve_combos()
        self._refresh_curve_name_controls()
        self.canvas.draw_idle()

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

            key = (
                repr(self._line_keys[i])
                if i < len(self._line_keys)
                else (line.get_label() or f"__idx_{i}")
            )
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

    def _persist_line_name_overrides(self):
        """Guarda nombres actuales de curvas para reusar en próximos re-plots."""
        for i, line in enumerate(getattr(self, "lines", []) or []):
            if line is None:
                continue
            if i >= len(self._line_keys):
                continue
            key = self._line_keys[i]
            self._line_name_overrides[key] = line.get_label()

    def _label_for_key(self, key, default_label: str) -> str:
        return self._line_name_overrides.get(key, default_label)

    def _sync_marker_color_to_line(self, line):
        if line is None:
            return
        marker = line.get_marker()
        if marker in (None, "None", "none", "", " "):
            return
        color = line.get_color()
        line.set_markerfacecolor(color)
        line.set_markeredgecolor(color)

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
        self._sync_marker_color_to_line(line)

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

            line_key = repr(self._line_keys[i]) if i < len(self._line_keys) else None
            key = line.get_label() or f"__idx_{i}"
            st = None

            if self._style_cache:
                st = (
                    self._style_cache.get(line_key)
                    or self._style_cache.get(key)
                    or self._style_cache.get(f"__idx_{i}")
                )

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

    def _load_project_datasets(self, paths):
        """Carga los archivos de datos referenciados por un proyecto."""
        self.datasets = []
        for p in paths:
            _, cols, steps = read_ltspice_steps(p, "auto")
            if steps:
                self.datasets.append({
                    "path": p, "name": os.path.basename(p), "data": None,
                    "steps": steps, "colnames": cols,
                })
            else:
                data, _, cols = read_ltspice_table(p, "auto")
                self.datasets.append({
                    "path": p, "name": os.path.basename(p), "data": data,
                    "steps": None, "colnames": cols,
                })

        if not self.datasets:
            raise ValueError("El proyecto no contiene archivos de datos.")

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
            self.l1.setText(f"Archivo: {first['name']}")
        else:
            self.l1.setText(f"Archivos: {len(self.datasets)} cargados")
        self._refresh_signal_list()

    def save_project(self):
        """Guarda las fuentes y la configuracion editable de la figura."""
        if not self.datasets:
            QMessageBox.warning(self, "Sin datos", "Primero tenes que cargar algun archivo.")
            return

        self._commit_signal_names_from_list()
        self._persist_line_name_overrides()
        self._capture_line_styles()
        p, _ = QFileDialog.getSaveFileName(
            self, "Guardar proyecto", "proyecto.ltplot.json",
            "Proyecto LTspice Plotter (*.ltplot.json)",
        )
        if not p:
            return
        if not p.lower().endswith(".ltplot.json"):
            p += ".ltplot.json"

        project_dir = os.path.dirname(os.path.abspath(p))
        dataset_paths = []
        for ds in self.datasets:
            try:
                dataset_paths.append(os.path.relpath(ds["path"], project_dir))
            except ValueError:
                dataset_paths.append(os.path.abspath(ds["path"]))

        selected_signals = []
        for i in range(self.lst_signals.count()):
            item = self.lst_signals.item(i)
            payload = item.data(Qt.ItemDataRole.UserRole)
            if payload and payload[0] == "DSCOL" and item.checkState() == Qt.CheckState.Checked:
                selected_signals.append({"dataset": int(payload[1]), "column": int(payload[2])})

        project = {
            "format": "ltspice-plotter-project",
            "version": 1,
            "datasets": dataset_paths,
            "controls": {
                "theme": self.cmb_theme.currentText(),
                "mode": self.cmb_mode.currentIndex(),
                "subplot_layout": self.cmb_subplot_layout.currentText(),
                "subplot_titles": self.ed_subplot_titles.text(),
                "manual_panels": self.chk_manual_panels.isChecked(),
                "x_scale": self.cmb_x.currentText(),
                "y_scale_index": self.cmb_y1.currentIndex(),
                "use_y2": self.chk_y2.isChecked(),
                "y2_signal": list(self.cmb_y2_signal.currentData()) if self.cmb_y2_signal.currentData() is not None else None,
                "y2_scale_index": self.cmb_y2.currentIndex(),
                "y2_label": self.y2lab.text(),
                "legend": self.cmb_legend.currentText(),
                "cursors_enabled": self.chk_cursors.isChecked(),
                "use_tex": self.chk_usetex.isChecked(),
                "title": self.tit.text(),
                "x_label": self.xlab.text(),
                "y_label": self.y1lab.text(),
                "single_curve_column": self.cmb_file1.currentIndex(),
                "single_curve_name": self.c1name.text(),
                "operation": self.cmb_op.currentText(),
                "only_operation_result": self.chk_only_result.isChecked(),
            },
            "selected_signals": selected_signals,
            "panel_assignments": [
                {
                    "panel": panel_index,
                    "signals": [list(key) for key in sorted(signals)],
                }
                for panel_index, signals in sorted(self._manual_panel_assignments.items())
            ],
            "signal_names": [
                {"dataset": key[0], "column": key[1], "name": name}
                for key, name in self._signal_names.items()
            ],
            "curve_style": self._curve_style,
            "curve_color_custom": self._curve_color_custom,
            "style_cache": self._style_cache,
            "line_names": [
                {"key": list(key), "name": name}
                for key, name in self._line_name_overrides.items()
            ],
            "line_transforms": [
                {"key": list(key), "transform": transform}
                for key, transform in self._line_transforms.items()
            ],
        }
        try:
            with open(p, "w", encoding="utf-8") as archivo:
                json.dump(project, archivo, indent=2, ensure_ascii=False)
            QMessageBox.information(self, "Proyecto guardado", "El proyecto se guardo correctamente.")
        except Exception as exc:
            QMessageBox.critical(self, "Error al guardar", str(exc))

    def open_project(self):
        """Recarga los datos y reconstruye una figura desde un proyecto JSON."""
        p, _ = QFileDialog.getOpenFileName(
            self, "Abrir proyecto", "",
            "Proyecto LTspice Plotter (*.ltplot.json);;JSON (*.json)",
        )
        if not p:
            return
        try:
            with open(p, "r", encoding="utf-8") as archivo:
                project = json.load(archivo)
            if project.get("format") != "ltspice-plotter-project":
                raise ValueError("El archivo seleccionado no es un proyecto de LTspice Plotter.")
            if project.get("version") != 1:
                raise ValueError("La version del proyecto no es compatible.")

            project_dir = os.path.dirname(os.path.abspath(p))
            paths = [
                os.path.normpath(saved if os.path.isabs(saved) else os.path.join(project_dir, saved))
                for saved in project.get("datasets", [])
            ]
            missing = [path for path in paths if not os.path.isfile(path)]
            if missing:
                QMessageBox.critical(
                    self, "Archivos no encontrados",
                    "No se encontraron estos archivos:\n\n" + "\n".join(missing),
                )
                return

            self._signal_names = {}
            self._line_name_overrides = {}
            self._line_transforms = {}
            self._style_cache = {}
            self.lines = []
            self._line_keys = []
            self.canvas.fig.clear()
            self._load_project_datasets(paths)

            controls = project.get("controls", {})
            widgets = [
                self.cmb_theme, self.cmb_mode, self.cmb_subplot_layout, self.cmb_x, self.cmb_y1,
                self.cmb_legend, self.cmb_file1, self.chk_usetex,
                self.cmb_op, self.chk_only_result, self.chk_y2,
                self.cmb_y2_signal, self.cmb_y2, self.chk_cursors,
                self.chk_manual_panels,
            ]
            for widget in widgets:
                widget.blockSignals(True)
            try:
                self.cmb_theme.setCurrentText(controls.get("theme", "ggplot"))
                self.cmb_mode.setCurrentIndex(int(controls.get("mode", 0)))
                self.cmb_subplot_layout.setCurrentText(controls.get("subplot_layout", "1x1"))
                self.chk_manual_panels.setChecked(bool(controls.get("manual_panels", False)))
                self.cmb_x.setCurrentText(controls.get("x_scale", "x1"))
                self.cmb_y1.setCurrentIndex(int(controls.get("y_scale_index", 0)))
                self.chk_y2.setChecked(bool(controls.get("use_y2", False)))
                saved_y2 = controls.get("y2_signal")
                y2_index = 0
                if saved_y2 is not None:
                    saved_y2 = tuple(saved_y2)
                    for index in range(self.cmb_y2_signal.count()):
                        value = self.cmb_y2_signal.itemData(index)
                        if value is not None and tuple(value) == saved_y2:
                            y2_index = index
                            break
                self.cmb_y2_signal.setCurrentIndex(y2_index)
                self.cmb_y2.setCurrentIndex(int(controls.get("y2_scale_index", 0)))
                self.cmb_legend.setCurrentText(controls.get("legend", "Auto (best)"))
                self.chk_cursors.setChecked(bool(controls.get("cursors_enabled", True)))
                self.chk_usetex.setChecked(bool(controls.get("use_tex", False)))
                self.cmb_file1.setCurrentIndex(int(controls.get("single_curve_column", 0)))
                self.cmb_op.setCurrentText(controls.get("operation", "Suma (+)"))
                self.chk_only_result.setChecked(bool(controls.get("only_operation_result", False)))
            finally:
                for widget in widgets:
                    widget.blockSignals(False)

            self._on_file1_column_changed()
            self.tit.setText(controls.get("title", "LTspice plot"))
            self.ed_subplot_titles.setText(controls.get("subplot_titles", ""))
            self.xlab.setText(controls.get("x_label", "time"))
            self.y1lab.setText(controls.get("y_label", "Y1"))
            self.y2lab.setText(controls.get("y2_label", "Y2"))
            self.c1name.setText(controls.get("single_curve_name", "Curva 1"))
            self._signal_names = {
                (int(item["dataset"]), int(item["column"])): item["name"]
                for item in project.get("signal_names", [])
            }
            self._manual_panel_assignments = {
                int(item["panel"]): {tuple(signal) for signal in item.get("signals", [])}
                for item in project.get("panel_assignments", [])
            }
            self._line_name_overrides = {
                tuple(item["key"]): item["name"] for item in project.get("line_names", [])
            }
            self._line_transforms = {
                tuple(item["key"]): item["transform"]
                for item in project.get("line_transforms", [])
            }
            self._curve_style = project.get("curve_style") or self._curve_style
            self._curve_color_custom = bool(project.get("curve_color_custom", False))
            saved_style_cache = project.get("style_cache", {})

            self._refresh_signal_list()
            selected = {
                (int(item["dataset"]), int(item["column"]))
                for item in project.get("selected_signals", [])
            }
            self.lst_signals.blockSignals(True)
            try:
                for i in range(self.lst_signals.count()):
                    item = self.lst_signals.item(i)
                    payload = item.data(Qt.ItemDataRole.UserRole)
                    if payload and payload[0] == "DSCOL":
                        item.setCheckState(
                            Qt.CheckState.Checked
                            if (payload[1], payload[2]) in selected
                            else Qt.CheckState.Unchecked
                        )
            finally:
                self.lst_signals.blockSignals(False)

            self._sync_style_ui_from_curve()
            self._sync_plot_selection_ui()
            self._sync_y2_ui()
            self._configure_text_rendering()
            self.plot()
            self._style_cache = saved_style_cache
            self._apply_line_styles(allow_color=True)
            self.refresh_legend()
            self.canvas.draw_idle()
            self._capture_line_styles()
            QMessageBox.information(self, "Proyecto abierto", "El proyecto se restauro correctamente.")
        except Exception as exc:
            QMessageBox.critical(self, "Error al abrir", str(exc))

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
        self.leg_map = {}
        for legend in getattr(self, "legends", []) or ([self.legend] if self.legend else []):
            try:
                legend.remove()
            except Exception:
                pass
        self.legend = None
        self.legends = []

        axes = self.axes or [self.ax1]
        for ax in axes:
            panel_axes = {ax}
            if ax in self.secondary_axes:
                panel_axes.add(self.secondary_axes[ax])
            handles = [
                line for line in (self.lines or [])
                if line is not None and line.axes in panel_axes
            ]
            if not handles:
                continue
            labels = [line.get_label() for line in handles]
            legend = self._apply_legend(ax, handles, labels)
            self.legends.append(legend)
            if self.legend is None:
                self.legend = legend

            for artist, line in zip(legend.get_lines(), handles):
                artist.set_picker(True)
                artist.set_pickradius(5)
                artist.set_alpha(1.0 if line.get_visible() else 0.25)
                self.leg_map[artist] = line
            for artist, line in zip(legend.get_texts(), handles):
                artist.set_picker(True)
                artist.set_alpha(1.0 if line.get_visible() else 0.25)
                self.leg_map[artist] = line

    def _set_best_legend_location(self):
        """Vuelve a calcular automaticamente la mejor ubicacion de la leyenda."""
        self.cmb_legend.blockSignals(True)
        self.cmb_legend.setCurrentText("Auto (best)")
        self.cmb_legend.blockSignals(False)
        if self.lines:
            self.refresh_legend()
            if len(self.axes) == 1:
                apply_layout(self.canvas.fig, "Auto (best)")
            else:
                self.canvas.fig.tight_layout(rect=(0, 0, 1, 0.96))
            self.canvas.draw_idle()

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

    def _on_cursors_toggled(self, *_):
        if not self.chk_cursors.isChecked():
            self.fbox.hide()
            self.reset_probes()
        else:
            self._update_crosshairs()
            self.canvas.draw_idle()

    def _update_crosshairs(self):
        if not self.chk_cursors.isChecked():
            self._remove_cross(self.crossA)
            self._remove_cross(self.crossB)
            return
        # crosshair vertical en ax1 (común), horizontal en el eje de la curva (ax1 o ax2)
        if self.ax1 is None:
            return

        def draw_for(probe, store):
            self._remove_cross(store)
            if not probe:
                return
            x, y = probe["x"], probe["y"]
            axh = probe.get("ax", self.ax1)

            store["v"] = axh.axvline(x, linestyle="--", alpha=0.7, linewidth=1.0)
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
            if self._line_keys:
                self._line_name_overrides[self._line_keys[0]] = self.lines[0].get_label()
        # refrescar leyenda para reflejar los nuevos nombres
        self.refresh_legend()
        self._refresh_operation_curve_combos()
        self._refresh_curve_name_controls(keep_index=0)
        self.canvas.draw_idle()


    def _subplot_shape(self):
        return {
            "1x1": (1, 1), "1x2": (1, 2),
            "2x1": (2, 1), "2x2": (2, 2),
        }.get(self.cmb_subplot_layout.currentText(), (1, 1))

    def _on_manual_panels_toggled(self, *_):
        if self.chk_manual_panels.isChecked() and not any(self._manual_panel_assignments.values()):
            return
        self._replot_if_data_loaded()

    def edit_panel_assignments(self):
        """Permite repetir libremente una señal en uno o más paneles."""
        if not self.datasets:
            QMessageBox.warning(self, "Sin datos", "Primero tenés que cargar algún archivo.")
            return

        rows, cols = self._subplot_shape()
        panel_count = rows * cols
        dialog = QDialog(self)
        dialog.setWindowTitle("Seleccionar curvas por panel")
        dialog.resize(520, 480)
        layout = QVBoxLayout(dialog)
        tabs = QTabWidget(dialog)
        panel_lists = []

        for panel_index in range(panel_count):
            signal_list = QListWidget()
            assigned = self._manual_panel_assignments.get(panel_index, set())
            for ds_idx, ds in enumerate(self.datasets):
                colnames = ds.get("colnames") or ()
                for col_idx, name in enumerate(colnames[1:], start=1):
                    key = (ds_idx, col_idx)
                    display_name = self._signal_names.get(key, name)
                    item = QListWidgetItem(f"[{ds.get('name', 'archivo')}] {display_name}")
                    item.setFlags(item.flags() | Qt.ItemFlag.ItemIsUserCheckable)
                    item.setData(Qt.ItemDataRole.UserRole, key)
                    item.setCheckState(
                        Qt.CheckState.Checked if key in assigned else Qt.CheckState.Unchecked
                    )
                    signal_list.addItem(item)
            panel_lists.append(signal_list)
            tabs.addTab(signal_list, f"Panel {panel_index + 1}")

        layout.addWidget(tabs)
        buttons = QDialogButtonBox(
            QDialogButtonBox.StandardButton.Ok | QDialogButtonBox.StandardButton.Cancel
        )
        buttons.accepted.connect(dialog.accept)
        buttons.rejected.connect(dialog.reject)
        layout.addWidget(buttons)

        if dialog.exec() != QDialog.DialogCode.Accepted:
            return

        assignments = {}
        for panel_index, signal_list in enumerate(panel_lists):
            selected = set()
            for item_index in range(signal_list.count()):
                item = signal_list.item(item_index)
                if item.checkState() == Qt.CheckState.Checked:
                    selected.add(tuple(item.data(Qt.ItemDataRole.UserRole)))
            assignments[panel_index] = selected

        self._manual_panel_assignments = assignments
        self.chk_manual_panels.blockSignals(True)
        self.chk_manual_panels.setChecked(True)
        self.chk_manual_panels.blockSignals(False)
        self.plot()

    def _panel_plot_requests(self, selected):
        """Retorna pares (panel, señal), admitiendo la misma señal más de una vez."""
        rows, cols = self._subplot_shape()
        panel_count = rows * cols
        if self.chk_manual_panels.isChecked():
            requests = []
            for panel_index in range(panel_count):
                for ds_idx, col_idx in sorted(self._manual_panel_assignments.get(panel_index, set())):
                    ds = self.datasets[ds_idx]
                    colnames = ds.get("colnames") or ()
                    name = colnames[col_idx] if col_idx < len(colnames) else f"Columna {col_idx}"
                    requests.append((panel_index, ("DSCOL", ds_idx, col_idx, name)))
            return requests
        return [
            (index % panel_count, payload)
            for index, payload in enumerate(selected)
            if payload and payload[0] == "DSCOL"
        ]

    def _dataset_column_values(self, ds_idx, col_idx):
        ds = self.datasets[ds_idx]
        if ds.get("steps"):
            values = [
                step["data"][:, col_idx]
                for step in ds["steps"]
                if col_idx < step["data"].shape[1]
            ]
            return np.concatenate(values) if values else None
        data = ds.get("data")
        if data is not None and col_idx < data.shape[1]:
            return data[:, col_idx]
        return None

    def plot(self):
        if self.data1 is None and not self.steps:
            QMessageBox.warning(self, "Falta", "Cargá un archivo")
            return

        xsc = self._xscale()
        mode = self.cmb_mode.currentIndex()
        if mode == 1:
            self._commit_signal_names_from_list()
        selected = self._selected_columns() if mode == 1 else []
        panel_requests = self._panel_plot_requests(selected) if mode == 1 else []

        use_theme_style(THEMES[self.cmb_theme.currentText()])
        self._configure_text_rendering()

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

        y2_key = self.cmb_y2_signal.currentData() if self.chk_y2.isChecked() else None
        y2_key = tuple(y2_key) if y2_key is not None else None
        y2src = self._dataset_column_values(*y2_key) if y2_key is not None else None

        # En modo N se escala según las señales realmente asignadas a paneles.
        # Las repeticiones no afectan el resultado y Y2 se calcula por separado.
        if mode == 1:
            y1_values = []
            seen_y1 = set()
            for _, (kind, ds_idx, col_idx, _) in panel_requests:
                if kind == "DSCOL" and (ds_idx, col_idx) != y2_key:
                    if (ds_idx, col_idx) in seen_y1:
                        continue
                    seen_y1.add((ds_idx, col_idx))
                    values = self._dataset_column_values(ds_idx, col_idx)
                    if values is not None:
                        y1_values.append(values)
            y1src = np.concatenate(y1_values) if y1_values else None

        y1sc = self._scale(self.cmb_y1, y1src, self.lbl_y1_factor) if y1src is not None else 1.0
        y2sc = self._scale(self.cmb_y2, y2src, self.lbl_y2_factor) if y2src is not None else 1.0

        # Guardar estilos actuales antes de borrar el figure (para no perder cambios)
        self._persist_line_name_overrides()
        self._capture_line_styles()
        if self._theme_change_pending:
            self._style_cache = {}
            self._theme_change_pending = False

        self.canvas.fig.clear()
        rows, cols = self._subplot_shape()
        axes_grid = self.canvas.fig.subplots(rows, cols, squeeze=False)
        self.axes = list(axes_grid.flat)
        ax1 = self.axes[0]
        self.ax1 = ax1
        self.ax2 = None
        self.secondary_axes = {}
        if y2_key is not None:
            self.ax2 = ax1.twinx()
            self.ax2.set_ylabel(self.y2lab.text())
            self.secondary_axes[ax1] = self.ax2
        self.legends = []

        panel_titles = [title.strip() for title in self.ed_subplot_titles.text().split(";")]
        if len(self.axes) == 1:
            ax1.set_title(self.tit.text())
        else:
            self.canvas.fig.suptitle(self.tit.text())
        for index, ax in enumerate(self.axes):
            if len(self.axes) > 1 and index < len(panel_titles) and panel_titles[index]:
                ax.set_title(panel_titles[index])
            ax.set_xlabel(self.xlab.text())

        # IMPORTANTE: el usuario pidió sin notación/escala en el label => SOLO el texto del usuario
        for ax in self.axes:
            ax.set_ylabel(self.y1lab.text())

        self.lines = []
        self._line_keys = []

        if mode == 1:
            if not panel_requests:
                QMessageBox.warning(self, "Falta", "Seleccioná al menos una señal en modo N.")
                return

            total = 0
            for _, (kind, ds_idx, col_idx, _) in panel_requests:
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
            for panel_index, (kind, ds_idx, col_idx, name) in panel_requests:
                if kind != "DSCOL":
                    continue
                if y2_key is not None and (ds_idx, col_idx) == y2_key:
                    continue
                target_ax = self.axes[panel_index]
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
                        step_label = st.get("label", "Step")
                        key = ("DSCOL_STEP", ds_idx, col_idx, step_label)
                        if self.chk_manual_panels.isChecked():
                            key = ("PANEL", panel_index) + key
                        default_label = f"[{file_name}] {base_name} | {step_label}"
                        label = self._label_for_key(key, default_label)
                        p = target_ax.plot(x * xsc, y * y1sc, label=label)
                        if colors:
                            p[0].set_color(colors[color_idx])
                        color_idx += 1
                        self.lines.append(p[0])
                        self._line_keys.append(key)
                elif ds.get("data") is not None and col_idx < ds["data"].shape[1]:
                    arr = ds["data"]
                    x = arr[:, 0]
                    y = arr[:, col_idx]
                    key = ("DSCOL", ds_idx, col_idx)
                    if self.chk_manual_panels.isChecked():
                        key = ("PANEL", panel_index) + key
                    default_label = f"[{file_name}] {base_name}"
                    label = self._label_for_key(key, default_label)
                    p = target_ax.plot(x * xsc, y * y1sc, label=label)
                    if colors:
                        p[0].set_color(colors[color_idx])
                    color_idx += 1
                    self.lines.append(p[0])
                    self._line_keys.append(key)
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
                self._line_keys.append(("SINGLE_STEP", col1, 0))
            else:
                p1 = ax1.plot(self.x1 * xsc, self.y1 * y1sc, label=(self.c1name.text().strip() or "Curva 1"))
                self.lines = [p1[0]]
                self._line_keys = [("SINGLE", self.cmb_file1.currentIndex() + 1)]

        # La señal Y2 se superpone en el primer panel usando un eje derecho.
        if y2_key is not None and self.ax2 is not None:
            ds_idx, col_idx = y2_key
            ds = self.datasets[ds_idx]
            colnames = ds.get("colnames") or ()
            raw_name = colnames[col_idx] if col_idx < len(colnames) else f"Columna {col_idx}"
            base_name = self._signal_names.get((ds_idx, col_idx), raw_name)
            file_name = ds.get("name", f"archivo_{ds_idx+1}")
            if ds.get("steps"):
                for step in ds["steps"]:
                    arr = step["data"]
                    if col_idx >= arr.shape[1]:
                        continue
                    step_label = step.get("label", "Step")
                    key = ("Y2_STEP", ds_idx, col_idx, step_label)
                    default_label = f"[Y2] [{file_name}] {base_name} | {step_label}"
                    line = self.ax2.plot(
                        arr[:, 0] * xsc, arr[:, col_idx] * y2sc,
                        label=self._label_for_key(key, default_label),
                    )[0]
                    self.lines.append(line)
                    self._line_keys.append(key)
            else:
                arr = ds.get("data")
                if arr is not None and col_idx < arr.shape[1]:
                    key = ("Y2", ds_idx, col_idx)
                    default_label = f"[Y2] [{file_name}] {base_name}"
                    line = self.ax2.plot(
                        arr[:, 0] * xsc, arr[:, col_idx] * y2sc,
                        label=self._label_for_key(key, default_label),
                    )[0]
                    self.lines.append(line)
                    self._line_keys.append(key)
        # Un único ciclo de color para todos los ejes evita que twinx() vuelva
        # a empezar por el primer color del tema.
        color_identities = []
        for index, line in enumerate(self.lines):
            key = self._line_keys[index] if index < len(self._line_keys) else ("LINE", index)
            # Copias de la misma señal en distintos paneles conservan el mismo color.
            identity = key[2:] if key and key[0] == "PANEL" else key
            color_identities.append(identity)
        unique_identities = list(dict.fromkeys(color_identities))
        identity_colors = dict(zip(unique_identities, self._distinct_colors(len(unique_identities))))
        for line, identity in zip(self.lines, color_identities):
            color = identity_colors.get(identity)
            if color is None:
                continue
            line.set_color(color)
            self._sync_marker_color_to_line(line)

        self._apply_line_styles(allow_color=(mode == 0))
        self._apply_line_transforms()
        for ax in self.axes:
            self._disable_axis_factor_text(ax)
        if self.ax2 is not None:
            self._disable_axis_factor_text(self.ax2)
        # Leyenda siempre sincronizada con estilos reales + clickeable
        self.refresh_legend()
        self._refresh_operation_curve_combos()
        self._refresh_curve_name_controls()

        # Crosshairs persistentes (si había probes)
        self._update_crosshairs()

        if len(self.axes) == 1:
            apply_layout(self.canvas.fig, self.cmb_legend.currentText())
        else:
            self.canvas.fig.tight_layout(rect=(0, 0, 1, 0.96))
        self.canvas.draw()

    # -------------------------
    # Probe UI
    # -------------------------
    def on_move(self, event):
        if not self.chk_cursors.isChecked():
            self.fbox.hide()
            return
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
        if not self.chk_cursors.isChecked():
            return
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
        path = self.datasets[0]["name"]
        archivo =  Path(path)
        nombre = archivo.with_suffix(".png")
        p, _ = QFileDialog.getSaveFileName(self, "Guardar",str(nombre), "PNG (*.png)")
        if not p:
            return
        if not p.lower().endswith(".png"):
            p += ".png"
        self.canvas.fig.savefig(p, dpi=200, bbox_inches="tight")

    def save_editable(self):
        if not self.canvas.fig.axes:
            QMessageBox.warning(
                self,
                "Sin figura",
                "Primero tenés que generar una figura."
            )
            return

        p, _ = QFileDialog.getSaveFileName(
            self,
            "Guardar figura editable",
            "ltspice_plot.mplfig",
            "Figura Matplotlib (*.mplfig)"
        )

        if not p:
            return

        if not p.lower().endswith(".mplfig"):
            p += ".mplfig"

        with open(p, "wb") as archivo:
            pickle.dump(self.canvas.fig, archivo)

        QMessageBox.information(
            self,
            "Figura guardada",
            "La figura editable se guardó correctamente."
        )


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

    def _sync_y2_ui(self, *_):
        enabled = self.chk_y2.isChecked()
        self.cmb_y2_signal.setEnabled(enabled)
        self.cmb_y2.setEnabled(enabled)
        self.y2lab.setEnabled(enabled)
        self.lbl_y2_factor.setEnabled(enabled)

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

<b>Subfiguras</b><br>
Elegí 1x1, 1x2, 2x1 o 2x2.<br>
En modo N las señales se distribuyen sucesivamente entre los paneles.<br>
Los títulos de panel se separan con punto y coma.<br><br>
Para decidir manualmente qué aparece en cada panel, usá
"Seleccionar curvas por panel". La misma señal puede marcarse en varios paneles.<br>
Desactivá "Asignación manual de paneles" para volver a la distribución automática.<br><br>

<b>Dos ejes Y</b><br>
Activá "Usar eje Y derecho" y elegí la señal Y2.<br>
Y1 e Y2 tienen escala y etiqueta independientes.<br>
Con subfiguras, el eje Y2 se superpone en el primer panel.<br><br>

<b>Escalas</b><br>
“Escala X” y “Escala Y1” son multiplicadores de datos.<br>
Las unidades se escriben manualmente en “X label” e “Y1 label”.<br>
El <i>factor aplicado</i> no modifica el texto del label.<br><br>

<b>Cursores A/B</b><br>
El control "Habilitar cursores A/B" permite activarlos o desactivarlos.<br>
Click = fija A<br>
Shift + Click = fija B<br>
Muestra ΔX y ΔY automáticamente<br><br>

<b>Resaltar curva</b><br>
Doble click sobre una curva<br><br>

<b>Leyenda interactiva</b><br>
"Ubicar leyenda automáticamente" busca la zona con menor superposición.<br>
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
