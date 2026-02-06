from dataclasses import dataclass
import numpy as np
from matplotlib import cycler
import matplotlib.pyplot as plt

@dataclass
class Theme:
    fig_face: str
    ax_face: str
    text: str
    grid: str
    spine: str
    grid_ls: str = "-"
    grid_alpha: float = 0.6
    curve_colors: tuple[str, ...] = ()


BUILTIN_THEMES = {
    "Paper": Theme("#FAF7F0", "#FAF7F0", "#1A1A1A", "#C9C2B8", "#2A2A2A", "--", 0.5,
                   ("#4C78A8", "#F58518", "#54A24B", "#E45756", "#72B7B2", "#B279A2", "#FF9DA6", "#9D755D")),
    "Light": Theme("#FFFFFF", "#FFFFFF", "#111111", "#E6E6E6", "#222222", "-", 0.6,
                   ("#1f77b4", "#ff7f0e", "#2ca02c", "#d62728", "#9467bd", "#8c564b", "#e377c2", "#17becf")),
    "Dark":  Theme("#1E1E1E", "#1E1E1E", "#EAEAEA", "#3A3A3A", "#C0C0C0", "-", 0.5,
                   ("#8dd3c7", "#ffffb3", "#bebada", "#fb8072", "#80b1d3", "#fdb462", "#b3de69", "#fccde5")),
    "LTspice":Theme("#0B0F14", "#0B0F14", "#D7E1EA", "#2BB673", "#93A4B2", ":", 0.35,
                    ("#00E676", "#FFEA00", "#40C4FF", "#FF6E40", "#B388FF", "#69F0AE", "#FFFF8D", "#EA80FC")),
}

# Estilos del repositorio pedido por el usuario (dhaitz/matplotlib-stylesheets)
REMOTE_STYLE_THEMES = {
    "pitayasmoothie-dark": "https://github.com/dhaitz/matplotlib-stylesheets/raw/master/pitayasmoothie-dark.mplstyle",
    "pitayasmoothie-light": "https://github.com/dhaitz/matplotlib-stylesheets/raw/master/pitayasmoothie-light.mplstyle",
    "pacoty": "https://github.com/dhaitz/matplotlib-stylesheets/raw/master/pacoty.mplstyle",
}

THEME_NAMES = list(BUILTIN_THEMES.keys()) + list(REMOTE_STYLE_THEMES.keys())

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


def set_active_theme(theme_name: str):
    """Aplica el estilo activo con la misma API que ejemplifica matplotlib: plt.style.use(...)."""
    if theme_name in REMOTE_STYLE_THEMES:
        plt.style.use("default")
        try:
            plt.style.use(REMOTE_STYLE_THEMES[theme_name])
        except Exception:
            # fallback seguro si no hay red o la URL no está disponible
            plt.style.use("default")
        return
    plt.style.use("default")


def apply_theme(fig, theme_name: str):
    """Ajustes visuales extra para temas internos; en temas remotos delega en mplstyle."""
    if theme_name in REMOTE_STYLE_THEMES:
        return

    theme = BUILTIN_THEMES.get(theme_name)
    if theme is None:
        return
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


def theme_curve_colors(theme_name: str, count: int) -> list[str]:
    if count <= 0:
        return []

    if theme_name in BUILTIN_THEMES:
        colors = list(BUILTIN_THEMES[theme_name].curve_colors)
    else:
        colors = plt.rcParams.get("axes.prop_cycle", cycler(color=[])).by_key().get("color", [])

    if not colors:
        return []

    return [colors[i % len(colors)] for i in range(count)]


def apply_layout(fig, legend_mode: str):
    if legend_mode == "Afuera derecha":
        fig.subplots_adjust(right=0.78)
    elif legend_mode == "Afuera abajo":
        fig.subplots_adjust(bottom=0.22)
    else:
        fig.tight_layout()
