from dataclasses import dataclass, field
import numpy as np
import matplotlib.pyplot as plt
from cycler import cycler

@dataclass
class Theme:
    plt_style: str
    fig_face: str
    ax_face: str
    text: str
    grid: str
    spine: str
    grid_ls: str = "-"
    grid_alpha: float = 0.6
    curve_colors: tuple[str, ...] = ()
    extra_rc: dict = field(default_factory=dict)

THEMES = {
    "Paper": Theme(
        "seaborn-v0_8-paper", "#FAF7F0", "#FAF7F0", "#1A1A1A", "#C9C2B8", "#2A2A2A", "--", 0.5,
        ("#4C78A8", "#F58518", "#54A24B", "#E45756", "#72B7B2", "#B279A2", "#FF9DA6", "#9D755D")
    ),
    "Light": Theme(
        "default", "#FFFFFF", "#FFFFFF", "#111111", "#E6E6E6", "#222222", "-", 0.6,
        ("#1f77b4", "#ff7f0e", "#2ca02c", "#d62728", "#9467bd", "#8c564b", "#e377c2", "#17becf")
    ),
    "Dark": Theme(
        "dark_background", "#1E1E1E", "#1E1E1E", "#EAEAEA", "#3A3A3A", "#C0C0C0", "-", 0.5,
        ("#8dd3c7", "#ffffb3", "#bebada", "#fb8072", "#80b1d3", "#fdb462", "#b3de69", "#fccde5")
    ),
    "LTspice": Theme(
        "dark_background", "#0B0F14", "#0B0F14", "#D7E1EA", "#2BB673", "#93A4B2", ":", 0.35,
        ("#00E676", "#FFEA00", "#40C4FF", "#FF6E40", "#B388FF", "#69F0AE", "#FFFF8D", "#EA80FC")
    ),
    "Solarized": Theme(
        "Solarized_Light2", "#FDF6E3", "#FDF6E3", "#586E75", "#EEE8D5", "#657B83", "-", 0.5,
        ("#268BD2", "#DC322F", "#859900", "#B58900", "#6C71C4", "#2AA198", "#CB4B16", "#D33682")
    ),
    "Dracula": Theme(
        "dark_background", "#282A36", "#282A36", "#F8F8F2", "#44475A", "#6272A4", "-", 0.45,
        ("#8BE9FD", "#FFB86C", "#50FA7B", "#FF5555", "#BD93F9", "#FF79C6", "#F1FA8C", "#69FF94"),
        {"axes.titleweight": "bold"}
    ),
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

def use_theme_style(theme: Theme):
    """Carga un estilo base de matplotlib para el tema seleccionado."""
    plt.style.use(theme.plt_style)
    if theme.extra_rc:
        plt.rcParams.update(theme.extra_rc)

def apply_theme(fig, theme: Theme):
    # Ajustes por figura/ejes para conservar identidad visual propia.
    fig.set_facecolor(theme.fig_face)
    for ax in fig.axes:
        ax.set_facecolor(theme.ax_face)
        ax.tick_params(colors=theme.text)
        ax.xaxis.label.set_color(theme.text)
        ax.yaxis.label.set_color(theme.text)
        ax.title.set_color(theme.text)
        if theme.curve_colors:
            ax.set_prop_cycle(cycler(color=theme.curve_colors))
        ax.grid(True, color=theme.grid, linestyle=theme.grid_ls, alpha=theme.grid_alpha)
        for sp in ax.spines.values():
            sp.set_color(theme.spine)

def theme_curve_colors(theme: Theme, count: int) -> list[str]:
    if count <= 0:
        return []
    if not theme.curve_colors:
        return []
    colors = list(theme.curve_colors)
    out = []
    for i in range(count):
        out.append(colors[i % len(colors)])
    return out

def apply_layout(fig, legend_mode: str):
    if legend_mode == "Afuera derecha":
        fig.subplots_adjust(right=0.78)
    elif legend_mode == "Afuera abajo":
        fig.subplots_adjust(bottom=0.22)
    else:
        fig.tight_layout()
