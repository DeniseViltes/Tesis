# style_panel.py
from __future__ import annotations

from dataclasses import dataclass
from typing import Callable, Dict

from PyQt6.QtWidgets import (
    QGroupBox, QFormLayout, QComboBox, QPushButton, QLineEdit, QColorDialog
)

StyleDict = Dict[str, object]

@dataclass
class StylePanelCallbacks:
    get_slot_style: Callable[[int], StyleDict]
    set_slot_style: Callable[[int, StyleDict], None]
    apply_now: Callable[[], None]   # aplica sobre el plot actual (re-legend + draw)
    save_preset: Callable[[], None]
    load_preset: Callable[[], None]

class StylePanel(QGroupBox):
    def __init__(self, parent, callbacks: StylePanelCallbacks):
        super().__init__("Estilo de curvas", parent)
        self.cb = callbacks

        form = QFormLayout(self)

        self.cmb_curve = QComboBox()
        self.cmb_curve.addItems(["Curva 1", "Curva 2"])
        self.cmb_curve.currentIndexChanged.connect(self._sync_ui_from_slot)

        self.btn_color = QPushButton("Color…")
        self.btn_color.clicked.connect(self._pick_color)

        self.cmb_ls = QComboBox()
        self.cmb_ls.addItems(["-", "--", ":", "-."])
        self.cmb_ls.currentTextChanged.connect(self._apply_from_ui)

        self.cmb_marker = QComboBox()
        self.cmb_marker.addItems(["None", "o", "s", "^", "x", "+", "d", "*"])
        self.cmb_marker.currentTextChanged.connect(self._apply_from_ui)

        self.ed_lw = QLineEdit("2.5")
        self.ed_lw.editingFinished.connect(self._apply_from_ui)

        self.btn_save = QPushButton("Guardar preset…")
        self.btn_save.clicked.connect(self.cb.save_preset)

        self.btn_load = QPushButton("Cargar preset…")
        self.btn_load.clicked.connect(self.cb.load_preset)

        form.addRow("Curva:", self.cmb_curve)
        form.addRow("Color:", self.btn_color)
        form.addRow("Línea:", self.cmb_ls)
        form.addRow("Marcador:", self.cmb_marker)
        form.addRow("Grosor:", self.ed_lw)
        form.addRow(self.btn_save)
        form.addRow(self.btn_load)

        self._sync_ui_from_slot()

    def current_slot(self) -> int:
        return int(self.cmb_curve.currentIndex())

    def _sync_ui_from_slot(self):
        slot = self.current_slot()
        st = self.cb.get_slot_style(slot) or {}

        ls = st.get("linestyle", "-")
        mk = st.get("marker", None)
        lw = st.get("linewidth", 2.5)

        if ls in [self.cmb_ls.itemText(i) for i in range(self.cmb_ls.count())]:
            self.cmb_ls.setCurrentText(ls)

        mk_txt = "None" if (mk is None or str(mk) == "None") else str(mk)
        if mk_txt in [self.cmb_marker.itemText(i) for i in range(self.cmb_marker.count())]:
            self.cmb_marker.setCurrentText(mk_txt)

        self.ed_lw.setText(str(lw))

    def _pick_color(self):
        c = QColorDialog.getColor(parent=self)
        if not c.isValid():
            return
        slot = self.current_slot()
        st = dict(self.cb.get_slot_style(slot) or {})
        st["color"] = c.name()
        self.cb.set_slot_style(slot, st)
        self.cb.apply_now()

    def _apply_from_ui(self):
        slot = self.current_slot()
        st = dict(self.cb.get_slot_style(slot) or {})

        st["linestyle"] = self.cmb_ls.currentText()

        mk = self.cmb_marker.currentText()
        st["marker"] = "None" if mk == "None" else mk

        try:
            st["linewidth"] = float(self.ed_lw.text().replace(",", "."))
        except Exception:
            pass

        self.cb.set_slot_style(slot, st)
        self.cb.apply_now()
