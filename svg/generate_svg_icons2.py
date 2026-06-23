#!/usr/bin/env python3
"""
Generate a polished, professional, and minimalist set of SVG icons 
for a paint/vector illustrator software.

- Output: ./icon_svgs/*.svg
- Transparent background
- 24x24 viewBox, pixel-perfect alignment
- 2px stroke, rounded linecaps/linejoins by default
- Incorporates minimalist vector paths and color blocks
- Extensible API for gradients, clipping, and fills
"""

from __future__ import annotations

import argparse
import json
import math
import os
from typing import Callable, Dict, List, Tuple, Union

SVG_NS = "http://www.w3.org/2000/svg"


# ---------- SVG Element Helpers ----------

def _attrs(kwargs: dict) -> str:
    """Convert kwargs to SVG attributes."""
    attrs =[]
    for k, v in kwargs.items():
        if v is not None:
            # Handle special SVG attributes that clash with Python keywords/syntax
            key = k.replace("_", "-")
            if key == "class_name":
                key = "class"
            attrs.append(f'{key}="{v}"')
    return (" " + " ".join(attrs)) if attrs else ""


def svg_doc(body: str, defs: str = "", stroke: str = "currentColor", stroke_width: float = 2.0) -> str:
    defs_block = f"\n<defs>\n{defs}\n</defs>\n" if defs else "\n"
    return f'''<svg xmlns="{SVG_NS}" viewBox="0 0 24 24" fill="none" stroke="{stroke}" stroke-width="{stroke_width}" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true">{defs_block}{body}</svg>
'''


def line(x1: float, y1: float, x2: float, y2: float, **kwargs) -> str:
    return f'<line x1="{x1}" y1="{y1}" x2="{x2}" y2="{y2}"{_attrs(kwargs)}/>'


def rect(x: float, y: float, w: float, h: float, rx: float = None, ry: float = None, **kwargs) -> str:
    if rx is not None: kwargs["rx"] = rx
    if ry is not None: kwargs["ry"] = ry
    return f'<rect x="{x}" y="{y}" width="{w}" height="{h}"{_attrs(kwargs)}/>'


def circle(cx: float, cy: float, r: float, **kwargs) -> str:
    return f'<circle cx="{cx}" cy="{cy}" r="{r}"{_attrs(kwargs)}/>'


def ellipse(cx: float, cy: float, rx: float, ry: float, **kwargs) -> str:
    return f'<ellipse cx="{cx}" cy="{cy}" rx="{rx}" ry="{ry}"{_attrs(kwargs)}/>'


def polyline(points: Union[str, List[Tuple[float, float]]], **kwargs) -> str:
    if isinstance(points, list):
        pts = " ".join(f"{x},{y}" for x, y in points)
    else:
        pts = points
    return f'<polyline points="{pts}"{_attrs(kwargs)}/>'


def polygon(points: Union[str, List[Tuple[float, float]]], **kwargs) -> str:
    if isinstance(points, list):
        pts = " ".join(f"{x},{y}" for x, y in points)
    else:
        pts = points
    return f'<polygon points="{pts}"{_attrs(kwargs)}/>'


def path(d: str, **kwargs) -> str:
    return f'<path d="{d}"{_attrs(kwargs)}/>'


def g(content: str, **kwargs) -> str:
    return f'<g{_attrs(kwargs)}>\n{content}\n</g>'


# ---------- Icon Builders ----------
# Each builder returns either a single string (body) 
# or a tuple (body, defs) for complex icons.

# -- Standard Tools
def icon_tool_select() -> str:
    return path("M5 3l7 18 2.5-7.5L22 11z")

def icon_tool_direct_select() -> str:
    return "\n".join([
        path("M5 3l7 18 2.5-7.5L22 11z"),
        circle(5, 3, 1.5, fill="currentColor", stroke="none")
    ])

def icon_tool_lasso() -> str:
    return path("M9 4c-4.4 0-8 3-8 7s3.6 7 8 7c3.5 0 6.5-1.5 8-4s3-4 6-4", stroke_dasharray="3 3")

def icon_tool_magic_wand() -> str:
    return "\n".join([
        line(9, 15, 21, 3),
        path("M4 10l2-2"),
        path("M2 4l2-2"),
        path("M10 4l-2 2"),
        path("M5 6A1 1 0 0 0 6 5A1 1 0 0 0 5 4A1 1 0 0 0 4 5A1 1 0 0 0 5 6z")
    ])

def icon_tool_move() -> str:
    return "\n".join([
        line(12, 2, 12, 22),
        line(2, 12, 22, 12),
        polyline("12 2 8 6 M12 2 16 6"),
        polyline("12 22 8 18 M12 22 16 18"),
        polyline("2 12 6 8 M2 12 6 16"),
        polyline("22 12 18 8 M22 12 18 16")
    ])

def icon_tool_pan() -> str:
    return "\n".join([
        path("M10.5 13.5V5.5a1.5 1.5 0 0 1 3 0v6"),
        path("M13.5 11.5V4a1.5 1.5 0 0 1 3 0v7.5"),
        path("M16.5 11.5V5.5a1.5 1.5 0 0 1 3 0v9a6.5 6.5 0 0 1-13 0v-4.5a1.5 1.5 0 0 1 3 0v3.5"),
        path("M7.5 13.5V7a1.5 1.5 0 0 1 3 0v6.5")
    ])

def icon_tool_zoom() -> str:
    return "\n".join([
        circle(11, 11, 6),
        line(15, 15, 21, 21),
        line(8, 11, 14, 11),
        line(11, 8, 11, 14)
    ])

def icon_tool_crop() -> str:
    return "\n".join([
        path("M6 2v16h16"),
        path("M2 6h16v16")
    ])

# -- Drawing Tools
def icon_tool_pen() -> str:
    return "\n".join([
        path("M6 13C6 5 18 5 18 13"),
        line(6, 13, 6, 7, stroke_dasharray="1 2"),
        line(18, 13, 18, 7, stroke_dasharray="1 2"),
        rect(4, 11, 4, 4, fill="currentColor", stroke="none"),
        rect(16, 11, 4, 4, fill="currentColor", stroke="none"),
        circle(6, 7, 1.5, fill="currentColor", stroke="none"),
        circle(18, 7, 1.5, fill="currentColor", stroke="none")
    ])

def icon_tool_pencil() -> str:
    return "\n".join([
        path("M17 3a2.85 2.85 0 1 1 4 4L7.5 20.5 2 22l1.5-5.5L17 3z"),
        line(15, 5, 19, 9)
    ])

def icon_tool_brush() -> str:
    return "\n".join([
        path("M9 17c-1 0-2 .5-3 1.5-1 1-.5 2.5-2 3.5 2 .5 4-.2 5.2-1.4C10.5 19.4 10.4 17 9 17z", fill="currentColor", stroke="none"),
        path("M16 3l5 5-7.5 7.5-5-5L16 3z"),
        line(14, 5, 17, 8)
    ])

def icon_tool_eraser() -> str:
    return "\n".join([
        polygon("21 7 16 2 2 16 7 21"),
        line(12, 6, 17, 11)
    ])

def icon_tool_fill() -> str:
    return "\n".join([
        path("M16 4l4 4-10 10-4-4 10-10z"),
        path("M14 2l6 6"),
        path("M6 14l-2 2a2.82 2.82 0 0 0 4 4l2-2")
    ])

def icon_tool_eyedropper() -> str:
    return "\n".join([
        path("M15 2.5a2.5 2.5 0 0 1 3.5 3.5l-1.5 1.5-3.5-3.5L15 2.5z"),
        path("M13.5 6.5l-8 8A2.5 2.5 0 0 0 4 17.5v2.5h2.5A2.5 2.5 0 0 0 9.5 18l8-8-4-4z"),
        line(4, 20, 2, 22)
    ])

def icon_tool_gradient() -> Tuple[str, str]:
    defs = '''<linearGradient id="grad1" x1="0" y1="0" x2="1" y2="1">
        <stop offset="0%" stop-color="#EF4444"/>
        <stop offset="100%" stop-color="#3B82F6"/>
    </linearGradient>'''
    body = rect(3, 3, 18, 18, rx=3, fill="url(#grad1)", stroke="currentColor")
    return body, defs

# -- Shape / Type Tools
def icon_tool_text() -> str:
    return "\n".join([
        line(4, 5, 20, 5, stroke_width=3),
        line(12, 5, 12, 19, stroke_width=3),
        line(9, 19, 15, 19, stroke_width=2),
        line(4, 5, 4, 8, stroke_width=2),
        line(20, 5, 20, 8, stroke_width=2)
    ])

def icon_tool_shape_rect() -> str:
    return rect(4, 4, 16, 16, rx=2)

def icon_tool_shape_ellipse() -> str:
    return circle(12, 12, 8)

def icon_tool_shape_polygon() -> str:
    return polygon("12 3 20 9 17 19 7 19 4 9")

def icon_tool_line() -> str:
    return "\n".join([
        line(5, 19, 19, 5),
        circle(5, 19, 1.5, fill="currentColor", stroke="none"),
        circle(19, 5, 1.5, fill="currentColor", stroke="none")
    ])

# -- Colors
def icon_color_palette() -> str:
    return "\n".join([
        path("M12 4c-4.5 0-8 3.1-8 7.5S7.5 20 12 20h1.5a1.5 1.5 0 0 0 0-3h-.5a2 2 0 0 1 0-4h1c3.3 0 6-2.4 6-5.5C20 5 16.4 4 12 4z"),
        circle(7.5, 14, 1.5, fill="#EF4444", stroke="none"),
        circle(10.5, 8, 1.5, fill="#F59E0B", stroke="none"),
        circle(15.5, 10.5, 1.5, fill="#3B82F6", stroke="none")
    ])

def icon_color_wheel() -> Tuple[str, str]:
    defs = '''<clipPath id="wheel-clip">
        <path d="M12 22a10 10 0 1 0 0-20 10 10 0 1 0 0 20zM12 16a4 4 0 1 1 0-8 4 4 0 1 1 0 8z" fill-rule="evenodd" clip-rule="evenodd"/>
    </clipPath>'''
    body = "\n".join([
        g("\n".join([
            path("M12 12L12 2A10 10 0 0 1 20.66 7L12 12z", fill="#EF4444", stroke="none"),
            path("M12 12L20.66 7A10 10 0 0 1 20.66 17L12 12z", fill="#F59E0B", stroke="none"),
            path("M12 12L20.66 17A10 10 0 0 1 12 22L12 12z", fill="#10B981", stroke="none"),
            path("M12 12L12 22A10 10 0 0 1 3.34 17L12 12z", fill="#3B82F6", stroke="none"),
            path("M12 12L3.34 17A10 10 0 0 1 3.34 7L12 12z", fill="#6366F1", stroke="none"),
            path("M12 12L3.34 7A10 10 0 0 1 12 2L12 12z", fill="#EC4899", stroke="none"),
        ]), clip_path="url(#wheel-clip)"),
        circle(12, 12, 10, fill="none", stroke="currentColor"),
        circle(12, 12, 4, fill="none", stroke="currentColor")
    ])
    return body, defs

def icon_color_swatches() -> str:
    return "\n".join([
        rect(8, 8, 12, 12, rx=2, fill="#3B82F6", stroke="currentColor"),
        rect(4, 4, 12, 12, rx=2, fill="#EF4444", stroke="currentColor")
    ])

def icon_color_swap() -> str:
    return "\n".join([
        path("M7 8h10"),
        polyline("13 5 17 8 13 11"),
        path("M17 16H7"),
        polyline("11 13 7 16 11 19")
    ])

# -- Layers
def icon_layer_stack() -> str:
    return "\n".join([
        polygon("12 3 20 7 12 11 4 7"),
        polyline("4 11 12 15 20 11"),
        polyline("4 15 12 19 20 15")
    ])

def icon_layer_new() -> str:
    return "\n".join([
        polygon("12 3 20 7 12 11 4 7"),
        polyline("4 11 12 15 20 11"),
        polyline("4 15 12 19 20 15"),
        line(16, 17, 22, 17, stroke_width=2),
        line(19, 14, 19, 20, stroke_width=2)
    ])

def icon_layer_delete() -> str:
    return "\n".join([
        polygon("12 3 20 7 12 11 4 7"),
        polyline("4 11 12 15 20 11"),
        polyline("4 15 12 19 20 15"),
        line(16, 14, 22, 20, stroke_width=2),
        line(22, 14, 16, 20, stroke_width=2)
    ])

def icon_layer_group() -> str:
    return "\n".join([
        path("M4 4h4l2 2h10v14H4V4z"),
        path("M8 10h4v4H8z"),
        path("M12 14h4v4h-4z")
    ])

def icon_layer_visible() -> str:
    return "\n".join([
        path("M2 12c0 0 4-7 10-7s10 7 10 7-4 7-10 7-10-7-10-7z"),
        circle(12, 12, 3)
    ])

def icon_layer_hidden() -> str:
    return "\n".join([
        path("M2 12c0 0 4-7 10-7s10 7 10 7-4 7-10 7-10-7-10-7z"),
        circle(12, 12, 3),
        line(3, 3, 21, 21)
    ])

def icon_layer_lock() -> str:
    return "\n".join([
        rect(7, 11, 10, 9, rx=2),
        path("M9 11V8a3 3 0 0 1 6 0v3")
    ])

def icon_layer_unlock() -> str:
    return "\n".join([
        rect(7, 11, 10, 9, rx=2),
        path("M10 11V8a2 2 0 0 1 4 0")
    ])

# -- Alignment Tools
def icon_align_left() -> str:
    return "\n".join([
        line(4, 4, 4, 20),
        rect(8, 6, 12, 4, rx=1),
        rect(8, 14, 8, 4, rx=1)
    ])

def icon_align_center() -> str:
    return "\n".join([
        line(12, 4, 12, 20),
        rect(6, 6, 12, 4, rx=1),
        rect(8, 14, 8, 4, rx=1)
    ])

def icon_align_right() -> str:
    return "\n".join([
        line(20, 4, 20, 20),
        rect(4, 6, 12, 4, rx=1),
        rect(8, 14, 8, 4, rx=1)
    ])

def icon_align_top() -> str:
    return "\n".join([
        line(4, 4, 20, 4),
        rect(6, 8, 4, 12, rx=1),
        rect(14, 8, 4, 8, rx=1)
    ])

def icon_align_middle() -> str:
    return "\n".join([
        line(4, 12, 20, 12),
        rect(6, 6, 4, 12, rx=1),
        rect(14, 8, 4, 8, rx=1)
    ])

def icon_align_bottom() -> str:
    return "\n".join([
        line(4, 20, 20, 20),
        rect(6, 4, 4, 12, rx=1),
        rect(14, 8, 4, 8, rx=1)
    ])

# -- Pathfinder / Boolean Operations
def icon_path_union() -> str:
    return path("M4 4h10v6h6v10H10v-6H4V4z", fill="currentColor", stroke="currentColor")

def icon_path_subtract() -> str:
    return "\n".join([
        path("M4 4h10v6h-6v6H4V4z", fill="currentColor", stroke="currentColor"),
        rect(10, 10, 10, 10, fill="none", stroke="currentColor", stroke_dasharray="2 2")
    ])

def icon_path_intersect() -> str:
    return "\n".join([
        rect(10, 10, 4, 4, fill="currentColor", stroke="currentColor"),
        rect(4, 4, 10, 10, fill="none", stroke="currentColor", stroke_dasharray="2 2"),
        rect(10, 10, 10, 10, fill="none", stroke="currentColor", stroke_dasharray="2 2")
    ])

def icon_path_exclude() -> str:
    return path("M4 4h10v6h6v10H10v-6H4V4z M10 10h4v4h-4v-4z", fill_rule="evenodd", fill="currentColor", stroke="currentColor")

# -- UI Icons
def icon_ui_grid() -> str:
    return "\n".join([
        path("M3 3h18v18H3z"),
        path("M9 3v18"),
        path("M15 3v18"),
        path("M3 9h18"),
        path("M3 15h18")
    ])

def icon_ui_snap() -> str:
    return "\n".join([
        path("M6 9a6 6 0 1 1 12 0v5a2 2 0 0 1-2 2h-1a2 2 0 0 1-2-2v-4a1 1 0 0 0-2 0v4a2 2 0 0 1-2 2H8a2 2 0 0 1-2-2V9z"),
        path("M6 9h3"),
        path("M15 9h3")
    ])

def icon_ui_undo() -> str:
    return "\n".join([
        path("M9 14L4 9l5-5"),
        path("M4 9h11a5 5 0 0 1 0 10H11")
    ])

def icon_ui_redo() -> str:
    return "\n".join([
        path("M15 14l5-5-5-5"),
        path("M20 9H9a5 5 0 0 0 0 10h4")
    ])

def icon_ui_settings() -> str:
    return "\n".join([
        circle(12, 12, 3),
        path("M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 0 1 0 2.83 2 2 0 0 1-2.83 0l-.06-.06a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 0-1 1.51V21a2 2 0 0 1-2 2 2 2 0 0 1-2-2v-.09A1.65 1.65 0 0 0 9 19.4a1.65 1.65 0 0 0-1.82.33l-.06.06a2 2 0 0 1-2.83 0 2 2 0 0 1 0-2.83l.06-.06a1.65 1.65 0 0 0 .33-1.82 1.65 1.65 0 0 0-1.51-1H3a2 2 0 0 1-2-2 2 2 0 0 1 2-2h.09A1.65 1.65 0 0 0 4.6 9a1.65 1.65 0 0 0-.33-1.82l-.06-.06a2 2 0 0 1 0-2.83 2 2 0 0 1 2.83 0l.06.06a1.65 1.65 0 0 0 1.82.33H9a1.65 1.65 0 0 0 1-1.51V3a2 2 0 0 1 2-2 2 2 0 0 1 2 2v.09a1.65 1.65 0 0 0 1 1.51 1.65 1.65 0 0 0 1.82-.33l.06-.06a2 2 0 0 1 2.83 0 2 2 0 0 1 0 2.83l-.06.06a1.65 1.65 0 0 0-.33 1.82V9a1.65 1.65 0 0 0 1.51 1H21a2 2 0 0 1 2 2 2 2 0 0 1-2 2h-.09a1.65 1.65 0 0 0-1.51 1z")
    ])

def icon_ui_close() -> str:
    return "\n".join([
        line(6, 6, 18, 18),
        line(18, 6, 6, 18)
    ])

def icon_ui_menu() -> str:
    return "\n".join([
        line(3, 6, 21, 6),
        line(3, 12, 21, 12),
        line(3, 18, 21, 18)
    ])

def icon_ui_export() -> str:
    return "\n".join([
        path("M12 15V3"),
        polyline("8 7 12 3 16 7"),
        path("M20 16v4a2 2 0 0 1-2 2H6a2 2 0 0 1-2-2v-4")
    ])

def icon_ui_open() -> str:
    return "\n".join([
        path("M12 3v12"),
        polyline("8 11 12 15 16 11"),
        path("M20 16v4a2 2 0 0 1-2 2H6a2 2 0 0 1-2-2v-4")
    ])

def icon_ui_save() -> str:
    return "\n".join([
        path("M19 21H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h11l5 5v11a2 2 0 0 1-2 2z"),
        path("M17 21v-8H7v8"),
        path("M7 3v5h8V3")
    ])


# ---------- Registry & Generator ----------

ICON_REGISTRY: Dict[str, Callable[[], Union[str, Tuple[str, str]]]] = {
    # Tools
    "tool-select": icon_tool_select,
    "tool-direct-select": icon_tool_direct_select,
    "tool-lasso": icon_tool_lasso,
    "tool-magic-wand": icon_tool_magic_wand,
    "tool-move": icon_tool_move,
    "tool-pan": icon_tool_pan,
    "tool-zoom": icon_tool_zoom,
    "tool-crop": icon_tool_crop,
    
    # Drawing
    "tool-pen": icon_tool_pen,
    "tool-pencil": icon_tool_pencil,
    "tool-brush": icon_tool_brush,
    "tool-eraser": icon_tool_eraser,
    "tool-fill": icon_tool_fill,
    "tool-eyedropper": icon_tool_eyedropper,
    "tool-gradient": icon_tool_gradient,
    
    # Shapes
    "tool-text": icon_tool_text,
    "tool-shape-rect": icon_tool_shape_rect,
    "tool-shape-ellipse": icon_tool_shape_ellipse,
    "tool-shape-polygon": icon_tool_shape_polygon,
    "tool-line": icon_tool_line,
    
    # Colors
    "color-palette": icon_color_palette,
    "color-wheel": icon_color_wheel,
    "color-swatches": icon_color_swatches,
    "color-swap": icon_color_swap,
    
    # Layers
    "layer-stack": icon_layer_stack,
    "layer-new": icon_layer_new,
    "layer-delete": icon_layer_delete,
    "layer-group": icon_layer_group,
    "layer-visible": icon_layer_visible,
    "layer-hidden": icon_layer_hidden,
    "layer-lock": icon_layer_lock,
    "layer-unlock": icon_layer_unlock,
    
    # Alignment
    "align-left": icon_align_left,
    "align-center": icon_align_center,
    "align-right": icon_align_right,
    "align-top": icon_align_top,
    "align-middle": icon_align_middle,
    "align-bottom": icon_align_bottom,
    
    # Pathfinder
    "path-union": icon_path_union,
    "path-subtract": icon_path_subtract,
    "path-intersect": icon_path_intersect,
    "path-exclude": icon_path_exclude,
    
    # UI
    "ui-grid": icon_ui_grid,
    "ui-snap": icon_ui_snap,
    "ui-undo": icon_ui_undo,
    "ui-redo": icon_ui_redo,
    "ui-settings": icon_ui_settings,
    "ui-close": icon_ui_close,
    "ui-menu": icon_ui_menu,
    "ui-export": icon_ui_export,
    "ui-open": icon_ui_open,
    "ui-save": icon_ui_save,
}


def write_icons(out_dir: str, stroke: str, stroke_width: float) -> List[str]:
    os.makedirs(out_dir, exist_ok=True)
    written = []

    for name, builder in ICON_BUILDERS.items():
        body = builder()
        svg = svg_doc(body, stroke=stroke, stroke_width=stroke_width)
        path_out = os.path.join(out_dir, f"{name}.svg")
        with open(path_out, "w", encoding="utf-8") as f:
            f.write(svg)
        written.append(path_out)

    manifest = {
        "count": len(ICON_BUILDERS),
        "viewBox": "0 0 24 24",
        "stroke": stroke,
        "strokeWidth": stroke_width,
        "background": "transparent",
        "icons": sorted(ICON_BUILDERS.keys()),
    }
    with open(os.path.join(out_dir, "manifest.json"), "w", encoding="utf-8") as f:
        json.dump(manifest, f, indent=2)

    readme = f"""# SVG Icon Set

Generated icons: {len(ICON_BUILDERS)}
ViewBox: 0 0 24 24
Stroke: {stroke}
Stroke width: {stroke_width}
Background: transparent

Notes:
- These are clean SVG recreations based on the screenshot labels.
- File names match the visible labels as closely as possible.
- A few ambiguous/duplicate labels use suffixes like `-alt`, `-file`, or `-disk`.
"""
    with open(os.path.join(out_dir, "README.md"), "w", encoding="utf-8") as f:
        f.write(readme)

    return written


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", default="icon_svgs", help="Output directory")
    parser.add_argument("--stroke", default="black", help='SVG stroke color, e.g. "black" or "currentColor"')
    parser.add_argument("--stroke-width", type=float, default=2.0, help="Stroke width")
    args = parser.parse_args()

    written = write_icons(args.out, args.stroke, args.stroke_width)
    print(f"Wrote {len(written)} SVG icons to: {os.path.abspath(args.out)}")


if __name__ == "__main__":
    main()