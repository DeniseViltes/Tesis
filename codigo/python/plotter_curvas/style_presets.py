# style_presets.py
import json
from typing import Any, Dict, Optional

def save_style_preset(path: str, slot_styles: Dict[int, Optional[dict]]) -> None:
    """Guarda estilos por slot (0/1) en un JSON."""
    if not path.lower().endswith(".json"):
        path += ".json"
    data = {
        "slot0": slot_styles.get(0),
        "slot1": slot_styles.get(1),
    }
    with open(path, "w", encoding="utf8") as f:
        json.dump(data, f, indent=2)

def load_style_preset(path: str) -> Dict[int, Optional[dict]]:
    """Carga estilos por slot (0/1) desde un JSON."""
    with open(path, "r", encoding="utf8") as f:
        data: Dict[str, Any] = json.load(f)
    return {
        0: data.get("slot0"),
        1: data.get("slot1"),
    }
