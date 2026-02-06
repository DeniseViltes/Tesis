from dataclasses import dataclass
import numpy as np
import matplotlib.pyplot as plt
from cycler import cycler
from pathlib import Path

@dataclass(frozen=True)
class Theme:
    plt_style: str

_STYLE_DIR = Path(__file__).resolve().parent / "styles"

THEMES = {
    # estilos externos
    "pacoty": Theme(str(_STYLE_DIR / "pacoty.mplstyle")),
    "pitayasmoothie-dark": Theme(str(_STYLE_DIR / "pitayasmoothie-dark.mplstyle")),
    "pitayasmoothie-light": Theme(str(_STYLE_DIR / "pitayasmoothie-light.mplstyle")),

    # estilos incluidos en matplotlib
    "Solarize_Light2": Theme("Solarize_Light2"),
    "_classic_test_patch": Theme("_classic_test_patch"),
    "_mpl-gallery": Theme("_mpl-gallery"),
    "_mpl-gallery-nogrid": Theme("_mpl-gallery-nogrid"),
    "bmh": Theme("bmh"),
    "classic": Theme("classic"),
    "dark_background": Theme("dark_background"),
    "fast": Theme("fast"),
    "fivethirtyeight": Theme("fivethirtyeight"),
    "ggplot": Theme("ggplot"),
    "grayscale": Theme("grayscale"),
    "seaborn": Theme("seaborn"),
    "seaborn-bright": Theme("seaborn-bright"),
    "seaborn-colorblind": Theme("seaborn-colorblind"),
    "seaborn-dark": Theme("seaborn-dark"),
    "seaborn-dark-palette": Theme("seaborn-dark-palette"),
    "seaborn-darkgrid": Theme("seaborn-darkgrid"),
    "seaborn-deep": Theme("seaborn-deep"),
    "seaborn-muted": Theme("seaborn-muted"),
    "seaborn-notebook": Theme("seaborn-notebook"),
    "seaborn-paper": Theme("seaborn-paper"),
    "seaborn-pastel": Theme("seaborn-pastel"),
    "seaborn-poster": Theme("seaborn-poster"),
    "seaborn-talk": Theme("seaborn-talk"),
    "seaborn-ticks": Theme("seaborn-ticks"),
    "seaborn-white": Theme("seaborn-white"),
    "seaborn-whitegrid": Theme("seaborn-whitegrid"),
    "tableau-colorblind10": Theme("tableau-colorblind10"),
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

def theme_curve_colors(theme: Theme, count: int) -> list[str]:
        return []


def apply_layout(fig, legend_mode: str):
    if legend_mode == "Afuera derecha":
        fig.subplots_adjust(right=0.78)
    elif legend_mode == "Afuera abajo":
        fig.subplots_adjust(bottom=0.22)
    else:
        fig.tight_layout()
