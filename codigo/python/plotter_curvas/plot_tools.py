from dataclasses import dataclass
import numpy as np
from cycler import cycler

@dataclass
class Theme:
    fig_face: str
    ax_face: str
    text: str
    grid: str
    spine: str
    grid_ls: str = "-"
    grid_alpha: float = 0.6
    color_cycle: tuple[str, ...] = ()

THEMES = {
    # Base
    "Paper": Theme("#FAF7F0", "#FAF7F0", "#1A1A1A", "#C9C2B8", "#2A2A2A", "--", 0.5,
                   ("#1f77b4", "#d62728", "#2ca02c", "#9467bd", "#ff7f0e", "#17becf", "#8c564b", "#e377c2")),
    "Light": Theme("#FFFFFF", "#FFFFFF", "#111111", "#E6E6E6", "#222222", "-", 0.6,
                   ("#1f77b4", "#d62728", "#2ca02c", "#9467bd", "#ff7f0e", "#17becf", "#8c564b", "#e377c2")),
    "Dark": Theme("#1E1E1E", "#1E1E1E", "#EAEAEA", "#3A3A3A", "#C0C0C0", "-", 0.5,
                  ("#8dd3ff", "#ff7f7f", "#a2ff9a", "#d7a9ff", "#ffcf80", "#7df2ea", "#ffc48f", "#f9a8d4")),
    "LTspice": Theme("#0B0F14", "#0B0F14", "#D7E1EA", "#2BB673", "#93A4B2", ":", 0.35,
                     ("#2BB673", "#F5A524", "#3E92CC", "#E4572E", "#A23B72", "#60A561", "#EE964B", "#DA4167")),

    # Inspirados en matplotlib-stylesheets
    "Dracula": Theme("#282A36", "#282A36", "#F8F8F2", "#44475A", "#6272A4", "-", 0.45,
                     ("#8BE9FD", "#FF79C6", "#50FA7B", "#BD93F9", "#FFB86C", "#F1FA8C", "#FF5555", "#A4FFFF")),
    "Nord": Theme("#2E3440", "#2E3440", "#ECEFF4", "#434C5E", "#81A1C1", "-", 0.45,
                  ("#88C0D0", "#BF616A", "#A3BE8C", "#B48EAD", "#EBCB8B", "#5E81AC", "#D08770", "#8FBCBB")),
    "Solarized Dark": Theme("#002B36", "#002B36", "#93A1A1", "#073642", "#586E75", "-", 0.5,
                            ("#268BD2", "#DC322F", "#859900", "#6C71C4", "#B58900", "#2AA198", "#CB4B16", "#D33682")),
    "Solarized Light": Theme("#FDF6E3", "#FDF6E3", "#586E75", "#EEE8D5", "#93A1A1", "-", 0.6,
                             ("#268BD2", "#DC322F", "#859900", "#6C71C4", "#B58900", "#2AA198", "#CB4B16", "#D33682")),
    "Monokai": Theme("#272822", "#272822", "#F8F8F2", "#49483E", "#75715E", "-", 0.45,
                     ("#66D9EF", "#F92672", "#A6E22E", "#AE81FF", "#FD971F", "#E6DB74", "#F44747", "#A1EFE4")),
    "Gruvbox Dark": Theme("#282828", "#282828", "#EBDBB2", "#3C3836", "#928374", "-", 0.45,
                          ("#83A598", "#FB4934", "#B8BB26", "#D3869B", "#FABD2F", "#8EC07C", "#FE8019", "#BDAE93")),
    "Tableau Colorblind": Theme("#FFFFFF", "#FFFFFF", "#111111", "#E1E1E1", "#333333", "-", 0.55,
                                ("#006BA4", "#FF800E", "#ABABAB", "#595959", "#5F9ED1", "#C85200", "#898989", "#A2C8EC", "#FFBC79", "#CFCFCF")),
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
        if theme.color_cycle:
            ax.set_prop_cycle(cycler(color=list(theme.color_cycle)))
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
