#!/usr/bin/env python3
"""
Generate a set of individual SVG icon files based on the screenshot labels.

- Output: ./icon_svgs/*.svg
- Transparent background
- 24x24 viewBox
- 2px stroke
- Rounded linecaps / linejoins
- Uses black by default, but you can switch to currentColor easily

Usage:
    python generate_svg_icons.py
    python generate_svg_icons.py --out ./my_icons
    python generate_svg_icons.py --stroke currentColor
    python generate_svg_icons.py --stroke "#000000" --stroke-width 2
"""

from __future__ import annotations

import argparse
import json
import math
import os
from typing import Callable, Dict, List

SVG_NS = "http://www.w3.org/2000/svg"


# ---------- SVG helpers ----------

def svg_doc(body: str, stroke: str = "black", stroke_width: float = 2.0) -> str:
    return f'''<svg xmlns="{SVG_NS}" viewBox="0 0 24 24" fill="none" stroke="{stroke}" stroke-width="{stroke_width}" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true">
{body}
</svg>
'''


def line(x1, y1, x2, y2) -> str:
    return f'<line x1="{x1}" y1="{y1}" x2="{x2}" y2="{y2}"/>'


def rect(x, y, w, h, rx=None, ry=None) -> str:
    rx_attr = f' rx="{rx}"' if rx is not None else ""
    ry_attr = f' ry="{ry}"' if ry is not None else ""
    return f'<rect x="{x}" y="{y}" width="{w}" height="{h}"{rx_attr}{ry_attr}/>'


def circle(cx, cy, r) -> str:
    return f'<circle cx="{cx}" cy="{cy}" r="{r}"/>'


def ellipse(cx, cy, rx, ry) -> str:
    return f'<ellipse cx="{cx}" cy="{cy}" rx="{rx}" ry="{ry}"/>'


def polyline(points) -> str:
    pts = " ".join(f"{x},{y}" for x, y in points)
    return f'<polyline points="{pts}"/>'


def polygon(points) -> str:
    pts = " ".join(f"{x},{y}" for x, y in points)
    return f'<polygon points="{pts}"/>'


def path(d: str) -> str:
    return f'<path d="{d}"/>'


def arc_path(cx, cy, r, start_deg, end_deg) -> str:
    a1 = math.radians(start_deg)
    a2 = math.radians(end_deg)
    x1 = cx + r * math.cos(a1)
    y1 = cy + r * math.sin(a1)
    x2 = cx + r * math.cos(a2)
    y2 = cy + r * math.sin(a2)
    large_arc = 1 if abs(end_deg - start_deg) > 180 else 0
    sweep = 1 if end_deg > start_deg else 0
    return f"M {x1:.2f} {y1:.2f} A {r} {r} 0 {large_arc} {sweep} {x2:.2f} {y2:.2f}"


def dashed_circle(cx, cy, r, dash="2 2") -> str:
    return f'<circle cx="{cx}" cy="{cy}" r="{r}" stroke-dasharray="{dash}"/>'


# ---------- common shapes ----------

def corners_selection_rect() -> str:
    return "\n".join([
        line(4, 7, 4, 4), line(4, 4, 7, 4),
        line(17, 4, 20, 4), line(20, 4, 20, 7),
        line(4, 17, 4, 20), line(4, 20, 7, 20),
        line(17, 20, 20, 20), line(20, 17, 20, 20),
        rect(4, 4, 3, 3), rect(17, 4, 3, 3), rect(4, 17, 3, 3), rect(17, 17, 3, 3)
    ])


def move_arrows() -> str:
    return "\n".join([
        line(12, 3, 12, 21),
        line(3, 12, 21, 12),
        polyline([(12, 3), (9.5, 5.5), (12, 3), (14.5, 5.5)]),
        polyline([(12, 21), (9.5, 18.5), (12, 21), (14.5, 18.5)]),
        polyline([(3, 12), (5.5, 9.5), (3, 12), (5.5, 14.5)]),
        polyline([(21, 12), (18.5, 9.5), (21, 12), (18.5, 14.5)]),
    ])


def save_disk() -> str:
    return "\n".join([
        path("M6 4h10l3 3v13H6z"),
        rect(9, 4, 6, 5),
        rect(9, 14, 6, 5, 1, 1),
    ])


def file_outline() -> str:
    return "\n".join([
        path("M7 3h7l4 4v14H7z"),
        line(14, 3, 14, 7),
        line(14, 7, 18, 7),
    ])


def folder_outline() -> str:
    return path("M3 7h6l2 2h10v9a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2z")


def align_lines(yvals, widths) -> str:
    x_center = 12
    parts = []
    for y, w in zip(yvals, widths):
        x1 = x_center - w / 2
        x2 = x_center + w / 2
        parts.append(line(x1, y, x2, y))
    return "\n".join(parts)


# ---------- icon definitions ----------

def icon_tool_rect_select() -> str:
    return corners_selection_rect()

def icon_tool_ellipse_select() -> str:
    return dashed_circle(12, 12, 8)

def icon_tool_lasso() -> str:
    return "\n".join([
        path("M7 9c0-3 3-5 6-5s6 2 6 5c0 3-3 5-6 5s-6-2-6-5z"),
        path("M10 14c0 3 2 3 2 5 0 1-1 2-2 2"),
    ])

def icon_tool_magic_wand() -> str:
    return "\n".join([
        line(9, 15, 17, 7),
        line(6, 6, 6, 8),
        line(5, 7, 7, 7),
        line(16, 4, 16, 6),
        line(15, 5, 17, 5),
        line(19, 8, 19, 10),
        line(18, 9, 20, 9),
    ])

def icon_tool_move() -> str:
    return move_arrows()

def icon_tool_move_alt() -> str:
    return move_arrows()

def icon_tool_hand() -> str:
    return "\n".join([
        path("M8 21c-2 0-3-2-3-4v-5c0-1 1-2 2-2s2 1 2 2V6c0-1 1-2 2-2s2 1 2 2v5V5c0-1 1-2 2-2s2 1 2 2v7V7c0-1 1-2 2-2s2 1 2 2v9c0 3-2 5-5 5z"),
    ])

def icon_tool_zoom() -> str:
    return "\n".join([
        circle(10.5, 10.5, 5.5),
        line(15, 15, 20, 20),
    ])

def icon_tool_pencil() -> str:
    return "\n".join([
        path("M4 20l4-1 10-10-3-3L5 16l-1 4z"),
        line(13, 6, 18, 11),
    ])

def icon_layer_visible_slash() -> str:
    return "\n".join([
        path("M2 12s4-6 10-6 10 6 10 6-4 6-10 6S2 12 2 12z"),
        circle(12, 12, 3),
        line(4, 20, 20, 4),
    ])

def icon_layer_hidden_plus() -> str:
    return "\n".join([
        path("M4 13c1-3 4-5 8-5 1 0 2 .1 3 .4"),
        path("M4 13c1 3 4 5 8 5 1 0 2-.1 3-.4"),
        line(16.5, 5.5, 21, 10),
        line(18.75, 3.25, 18.75, 12.25),
        line(14.25, 7.75, 23.25, 7.75),
    ])

def icon_layer_hidden_eye_off() -> str:
    return "\n".join([
        path("M2 12s4-6 10-6 10 6 10 6-4 6-10 6S2 12 2 12z"),
        circle(12, 12, 3),
        line(3, 21, 21, 3),
    ])

def icon_tool_brush() -> str:
    return "\n".join([
        path("M14 5c2 2 2 5 0 7l-5 5"),
        path("M9 17c-1 0-2 .5-3 1.5-1 1-.5 2.5-2 3.5 2 .5 4-.2 5.2-1.4C10.5 19.4 10.4 17 9 17z"),
        path("M16 3l5 5"),
    ])

def icon_tool_eraser() -> str:
    return "\n".join([
        polygon([(6, 15), (12, 9), (18, 15), (12, 21)]),
        line(9, 18, 18, 18),
    ])

def icon_tool_fill() -> str:
    return "\n".join([
        polygon([(8, 8), (13, 3), (18, 8), (13, 13)]),
        line(10, 10, 17, 17),
        path("M17 20c1.5 0 2.5-1 2.5-2.2 0-1.4-1.3-2.4-2.5-4.3-1.2 1.9-2.5 2.9-2.5 4.3C14.5 19 15.5 20 17 20z"),
    ])

def icon_tool_eyedropper() -> str:
    return "\n".join([
        path("M14 4l6 6"),
        path("M16 2a2 2 0 0 1 3 0l1 1a2 2 0 0 1 0 3l-3 3-4-4z"),
        line(9, 13, 4, 20),
        path("M4 20l2-1"),
    ])

def icon_tool_line() -> str:
    return line(5, 19, 19, 5)

def icon_tool_rectangle() -> str:
    return rect(5, 7, 14, 10, 1, 1)

def icon_tool_ellipse() -> str:
    return ellipse(12, 12, 7, 5)

def icon_tool_polygon() -> str:
    return polygon([(12, 4), (18, 7), (20, 13), (16, 19), (8, 19), (4, 13), (6, 7)])

def icon_opt_brush_size() -> str:
    return icon_tool_brush()

def icon_opt_tolerance() -> str:
    return "\n".join([
        path("M7 17V7"),
        path("M10 17V7"),
        line(6, 12, 11, 12),
        line(13, 17, 18, 7),
    ])

def icon_opt_anti_alias() -> str:
    return "\n".join([
        path("M8 17V7"),
        path("M13 17V7"),
        line(7, 12, 14, 12),
    ])

def icon_opt_text_bold() -> str:
    return "\n".join([
        path("M8 5h5a3 3 0 0 1 0 6H8z"),
        path("M8 11h6a4 4 0 0 1 0 8H8z"),
        line(8, 5, 8, 19),
    ])

def icon_tool_text() -> str:
    return "\n".join([
        line(5, 5, 19, 5),
        line(12, 5, 12, 19),
    ])

def icon_tool_crop() -> str:
    return "\n".join([
        line(7, 4, 7, 17),
        line(4, 7, 17, 7),
        line(11, 20, 11, 11),
        line(20, 11, 11, 11),
    ])

def icon_tool_transform() -> str:
    return "\n".join([
        rect(7, 7, 8, 8),
        line(15, 5, 19, 5), line(19, 5, 19, 9),
        line(5, 15, 5, 19), line(5, 19, 9, 19),
    ])

def icon_layer_visible_stack() -> str:
    return "\n".join([
        polygon([(12, 4), (19, 8), (12, 12), (5, 8)]),
        polygon([(12, 9), (19, 13), (12, 17), (5, 13)]),
        polygon([(12, 14), (19, 18), (12, 22), (5, 18)]),
    ])

def icon_layer_hidden_stack() -> str:
    return "\n".join([
        polygon([(12, 4), (19, 8), (12, 12), (5, 8)]),
        polygon([(12, 9), (19, 13), (12, 17), (5, 13)]),
        line(5, 19, 19, 5),
    ])

def icon_layer_locked() -> str:
    return "\n".join([
        rect(7, 11, 10, 9, 2, 2),
        path("M9 11V8a3 3 0 0 1 6 0v3"),
    ])

def icon_layer_unlocked() -> str:
    return "\n".join([
        rect(7, 11, 10, 9, 2, 2),
        path("M10 11V8a3 3 0 0 1 6 0"),
    ])

def icon_layer_add() -> str:
    return "\n".join([
        circle(12, 12, 8),
        line(12, 8, 12, 16),
        line(8, 12, 16, 12),
    ])

def icon_opt_ranti_alias() -> str:
    return "\n".join([
        line(5, 18, 19, 6),
        line(8, 19, 18, 9),
        line(5, 15, 15, 5),
    ])

def icon_opt_contiguous() -> str:
    return "\n".join([
        circle(10, 12, 4),
        circle(15, 12, 4),
    ])

def icon_opt_text_bold_alt() -> str:
    return icon_opt_text_bold()

def icon_opt_text_italic() -> str:
    return "\n".join([
        line(10, 5, 16, 5),
        line(8, 19, 14, 19),
        line(13, 5, 11, 19),
    ])

def icon_layer_delete() -> str:
    return "\n".join([
        rect(8, 6, 8, 2),
        path("M6 8h12l-1 11a2 2 0 0 1-2 2H9a2 2 0 0 1-2-2z"),
    ])

def icon_layer_duplicate() -> str:
    return "\n".join([
        rect(9, 8, 9, 11, 1, 1),
        rect(6, 5, 9, 11, 1, 1),
    ])

def icon_layer_merge() -> str:
    return "\n".join([
        path("M12 4v10"),
        polyline([(8, 10), (12, 14), (16, 10)]),
        path("M6 18h12"),
    ])

def icon_layer_type_raster() -> str:
    return "\n".join([
        polygon([(12, 4), (19, 8), (12, 12), (5, 8)]),
        polyline([(12, 12), (12, 19)]),
        polyline([(9, 16), (12, 19), (15, 16)]),
    ])

def icon_layer_type_vector() -> str:
    return "\n".join([
        polygon([(12, 4), (19, 8), (12, 12), (5, 8)]),
        polygon([(12, 10), (19, 14), (12, 18), (5, 14)]),
    ])

def icon_layer_type_text() -> str:
    return "\n".join([
        line(6, 6, 18, 6),
        line(12, 6, 12, 18),
    ])

def icon_layer_type_brush() -> str:
    return icon_tool_brush()

def icon_layer_group() -> str:
    return "\n".join([
        rect(6, 6, 8, 8, 1, 1),
        path("M10 10l8-4"),
        line(18, 3, 18, 7),
        line(16, 5, 20, 5),
    ])

def icon_color_swap() -> str:
    return "\n".join([
        path("M7 8h10"),
        polyline([(13, 5), (17, 8), (13, 11)]),
        path("M17 16H7"),
        polyline([(11, 13), (7, 16), (11, 19)]),
    ])

def icon_color_reset() -> str:
    return "\n".join([
        path(arc_path(12, 12, 7, 45, 320)),
        polyline([(15.5, 4.5), (18.5, 4.5), (18.5, 7.5)]),
    ])

def icon_color_wheel() -> str:
    return "\n".join([
        circle(12, 12, 7),
        path("M12 12L12 5"),
        path("M12 12L18 9"),
        path("M12 12L17 17"),
        path("M12 12L7 17"),
        path("M12 12L6 9"),
    ])

def icon_color_palette() -> str:
    return "\n".join([
        path("M12 4c-4.5 0-8 3.1-8 7.5S7.5 20 12 20h1.5a1.5 1.5 0 0 0 0-3h-.5a2 2 0 0 1 0-4h1c3.3 0 6-2.4 6-5.5C20 5 16.4 4 12 4z"),
        circle(8, 10, 1),
        circle(11, 8, 1),
        circle(15, 8.5, 1),
        circle(16, 12, 1),
    ])

def icon_opt_align_left() -> str:
    return "\n".join([
        line(5, 6, 17, 6),
        line(5, 10, 15, 10),
        line(5, 14, 18, 14),
        line(5, 18, 16, 18),
    ])

def icon_opt_align_center() -> str:
    return align_lines([6, 10, 14, 18], [12, 8, 10, 6])

def icon_opt_align_right() -> str:
    return "\n".join([
        line(7, 6, 19, 6),
        line(9, 10, 19, 10),
        line(6, 14, 19, 14),
        line(8, 18, 19, 18),
    ])

def icon_opt_stroke_weight() -> str:
    return "\n".join([
        line(6, 8, 18, 8),
        line(5, 16, 19, 16),
    ])

def icon_doc_new() -> str:
    return file_outline()

def icon_doc_open() -> str:
    return folder_outline()

def icon_doc_save_file() -> str:
    return file_outline()

def icon_doc_save_disk() -> str:
    return save_disk()

def icon_doc_export() -> str:
    return folder_outline()

def icon_ui_undo() -> str:
    return "\n".join([
        path("M9 7L4 12l5 5"),
        path("M4 12h8a6 6 0 0 1 6 6"),
    ])

def icon_ui_redo() -> str:
    return "\n".join([
        path("M15 7l5 5-5 5"),
        path("M20 12h-8a6 6 0 0 0-6 6"),
    ])

def icon_ui_redo_alt() -> str:
    return icon_ui_redo()

def icon_ui_history() -> str:
    return "\n".join([
        circle(12, 12, 7),
        line(12, 8, 12, 12),
        line(12, 12, 15, 14),
        polyline([(7, 6), (5, 6), (5, 8)]),
    ])

def icon_ui_settings() -> str:
    return "\n".join([
        circle(12, 12, 2.5),
        path("M12 4v2"),
        path("M12 18v2"),
        path("M4 12h2"),
        path("M18 12h2"),
        path("M6.3 6.3l1.4 1.4"),
        path("M16.3 16.3l1.4 1.4"),
        path("M17.7 6.3l-1.4 1.4"),
        path("M7.7 16.3l-1.4 1.4"),
    ])

def icon_ui_theme_dark() -> str:
    return "\n".join([
        circle(12, 12, 7),
        path("M12 5a7 7 0 0 0 0 14a5 5 0 0 1 0-14z"),
    ])

def icon_ui_theme_light() -> str:
    return "\n".join([
        circle(12, 12, 4),
        line(12, 2.5, 12, 5),
        line(12, 19, 12, 21.5),
        line(2.5, 12, 5, 12),
        line(19, 12, 21.5, 12),
        line(5, 5, 6.8, 6.8),
        line(17.2, 17.2, 19, 19),
        line(17.2, 6.8, 19, 5),
        line(5, 19, 6.8, 17.2),
    ])

def icon_ui_close() -> str:
    return "\n".join([
        line(6, 6, 18, 18),
        line(18, 6, 6, 18),
    ])

def icon_vector_anchor_add() -> str:
    return "\n".join([
        circle(6, 6, 1.5),
        rect(16.5, 4.5, 3, 3),
        rect(4.5, 16.5, 3, 3),
        line(7.5, 6, 16.5, 6),
        path("M7.5 18c3-5 6-8 9-12"),
        line(20, 14, 20, 18),
        line(18, 16, 22, 16),
    ])

def icon_vector_anchor_del() -> str:
    return "\n".join([
        circle(6, 6, 1.5),
        rect(16.5, 4.5, 3, 3),
        rect(4.5, 16.5, 3, 3),
        line(7.5, 6, 16.5, 6),
        path("M7.5 18c3-5 6-8 9-12"),
        line(18, 14, 22, 18),
        line(22, 14, 18, 18),
    ])

def icon_vector_path_close() -> str:
    return "\n".join([
        circle(12, 12, 7),
        line(9, 9, 15, 15),
        line(15, 9, 9, 15),
    ])

def icon_vector_bring_front() -> str:
    return "\n".join([
        rect(8, 8, 9, 9, 1, 1),
        rect(5, 5, 9, 9, 1, 1),
    ])

def icon_vector_send_back() -> str:
    return "\n".join([
        polygon([(12, 3), (19, 7), (12, 11), (5, 7)]),
        polygon([(12, 9), (19, 13), (12, 17), (5, 13)]),
        polygon([(12, 15), (19, 19), (12, 23), (5, 19)]),
    ])


# ---------- registry ----------

ICON_BUILDERS: Dict[str, Callable[[], str]] = {
    "tool-rect-select": icon_tool_rect_select,
    "tool-ellipse-select": icon_tool_ellipse_select,
    "tool-lasso": icon_tool_lasso,
    "tool-magic-wand": icon_tool_magic_wand,
    "tool-move": icon_tool_move,
    "tool-move-alt": icon_tool_move_alt,
    "tool-hand": icon_tool_hand,
    "tool-zoom": icon_tool_zoom,
    "tool-pencil": icon_tool_pencil,

    "layer-visible-slash": icon_layer_visible_slash,
    "layer-hidden-plus": icon_layer_hidden_plus,
    "layer-hidden-eye-off": icon_layer_hidden_eye_off,

    "tool-brush": icon_tool_brush,
    "tool-eraser": icon_tool_eraser,
    "tool-fill": icon_tool_fill,
    "tool-eyedropper": icon_tool_eyedropper,
    "tool-line": icon_tool_line,
    "tool-rectangle": icon_tool_rectangle,
    "tool-ellipse": icon_tool_ellipse,
    "tool-polygon": icon_tool_polygon,

    "opt-brush-size": icon_opt_brush_size,
    "opt-tolerance": icon_opt_tolerance,
    "opt-anti-alias": icon_opt_anti_alias,
    "opt-text-bold": icon_opt_text_bold,

    "tool-text": icon_tool_text,
    "tool-crop": icon_tool_crop,
    "tool-transform": icon_tool_transform,

    "layer-visible": icon_layer_visible_stack,
    "layer-hidden": icon_layer_hidden_stack,
    "layer-locked": icon_layer_locked,
    "layer-unlocked": icon_layer_unlocked,
    "layer-add": icon_layer_add,

    "opt-ranti-alias": icon_opt_ranti_alias,
    "opt-contiguous": icon_opt_contiguous,
    "opt-text-bold-alt": icon_opt_text_bold_alt,
    "opt-text-italic": icon_opt_text_italic,

    "layer-delete": icon_layer_delete,
    "layer-duplicate": icon_layer_duplicate,
    "layer-merge": icon_layer_merge,
    "layer-type-raster": icon_layer_type_raster,
    "layer-type-vector": icon_layer_type_vector,
    "layer-type-text": icon_layer_type_text,
    "layer-type-brush": icon_layer_type_brush,
    "layer-group": icon_layer_group,

    "color-swap": icon_color_swap,
    "color-reset": icon_color_reset,
    "color-wheel": icon_color_wheel,
    "color-palette": icon_color_palette,

    "opt-align-left": icon_opt_align_left,
    "opt-align-center": icon_opt_align_center,
    "opt-align-right": icon_opt_align_right,
    "opt-stroke-weight": icon_opt_stroke_weight,
    "color-wheel-alt": icon_color_wheel,

    "doc-new": icon_doc_new,
    "doc-open": icon_doc_open,
    "doc-save-file": icon_doc_save_file,
    "doc-save-disk": icon_doc_save_disk,
    "doc-export": icon_doc_export,

    "ui-undo": icon_ui_undo,
    "ui-rodo": icon_ui_redo,   # matches screenshot typo/labeling
    "ui-redo": icon_ui_redo_alt,
    "ui-history-disabled": icon_ui_history,
    "ui-history": icon_ui_history,
    "ui-settings": icon_ui_settings,
    "ui-theme-dark": icon_ui_theme_dark,
    "ui-theme-light": icon_ui_theme_light,
    "ui-close": icon_ui_close,

    "vector-anchor-add": icon_vector_anchor_add,
    "vector-anchor-del": icon_vector_anchor_del,
    "vector-path-close": icon_vector_path_close,
    "vector-bring-front": icon_vector_bring_front,
    "vector-send-back": icon_vector_send_back,
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
