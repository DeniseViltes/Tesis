from dataclasses import dataclass
import numpy as np

@dataclass
class Theme:
    fig_face: str
    ax_face: str
    text: str
    grid: str
    spine: str
    grid_ls: str = "-"
    grid_alpha: float = 0.6

THEMES = {
    "Paper": Theme("#FAF7F0", "#FAF7F0", "#1A1A1A", "#C9C2B8", "#2A2A2A", "--", 0.5),
    "Light": Theme("#FFFFFF", "#FFFFFF", "#111111", "#E6E6E6", "#222222", "-", 0.6),
    "Dark":  Theme("#1E1E1E", "#1E1E1E", "#EAEAEA", "#3A3A3A", "#C0C0C0", "-", 0.5),
    "LTspice":Theme("#0B0F14", "#0B0F14", "#D7E1EA", "#2BB673", "#93A4B2", ":", 0.35),
}

SCALE_MAP = {"x1":1.0, "x1e3":1e3, "x1e-3":1e-3, "x1e6":1e6, "x1e-6":1e-6}

def pick_auto_scale(y: np.ndarray) -> float:
    y = np.asarray(y)
    y = y[np.isfinite(y)]
    if y.size == 0:
        return 1.0
    m = float(np.max(np.abs(y)))
    if m == 0:
        return 1.0
    if m < 1e-6: return 1e6
    if m < 1e-3: return 1e3
    if m < 1:    return 1e3
    if m >= 1e3: return 1e-3
    return 1.0

def scale_suffix(f: float) -> str:
    return "x1" if f == 1.0 else f"x{f:g}"

def apply_theme(fig, theme: Theme):
    fig.set_facecolor(theme.fig_face)
    for ax in fig.axes:
        ax.set_facecolor(theme.ax_face)
        ax.tick_params(colors=theme.text)
        ax.xaxis.label.set_color(theme.text)
        ax.yaxis.label.set_color(theme.text)
        ax.title.set_color(theme.text)
        ax.grid(True, color=theme.grid, linestyle=theme.grid_ls, alpha=theme.grid_alpha)
        for sp in ax.spines.values():
            sp.set_color(theme.spine)

def apply_layout(fig, legend_mode: str):
    if legend_mode == "Afuera derecha":
        fig.subplots_adjust(right=0.78)
    elif legend_mode == "Afuera abajo":
        fig.subplots_adjust(bottom=0.22)
    else:
        fig.tight_layout()
