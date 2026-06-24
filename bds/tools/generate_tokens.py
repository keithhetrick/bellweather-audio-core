#!/usr/bin/env python3
"""Generate typed token constants from BDS per-product JSON token files.

Reads bds/tokens/products/**/*.tokens.json (recursive) and emits:
  - C++ header   (constexpr uint32_t colors, float/int scalars)
  - TypeScript    (exported const objects with full types)
  - CSS           (:root custom properties with --bw-* naming)

Usage:
  python3 bds/tools/generate_tokens.py \
    --products bds/tokens/products/ \
    --output-cpp modules/bw_ui/include/bw_ui/generated/BwTokens.h \
    --output-ts  packages/bellweather-ui/generated/tokens.ts \
    --output-css packages/bellweather-ui/generated/tokens.css
"""

import argparse
import json
import re
import sys
from pathlib import Path


# Color parsing

_RGBA_RE = re.compile(
    r"rgba?\(\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*(?:,\s*([\d.]+))?\s*\)"
)


def hex_to_argb(s):
    """Convert #RRGGBB or #AARRGGBB to 0xAARRGGBB integer."""
    h = s.lstrip("#")
    if len(h) == 6:
        return 0xFF000000 | int(h, 16)
    if len(h) == 8:
        return int(h, 16)
    return None


def rgba_to_argb(s):
    """Convert rgba(r, g, b, a) to 0xAARRGGBB integer."""
    m = _RGBA_RE.match(s)
    if not m:
        return None
    r, g, b = int(m.group(1)), int(m.group(2)), int(m.group(3))
    a = float(m.group(4)) if m.group(4) else 1.0
    return (round(a * 255) << 24) | (r << 16) | (g << 8) | b


def parse_color(v):
    """Try to parse a color string to 0xAARRGGBB."""
    if not isinstance(v, str):
        return None
    if v.startswith("#"):
        return hex_to_argb(v)
    if v.startswith("rgb"):
        return rgba_to_argb(v)
    return None


def is_color(v):
    return isinstance(v, str) and (v.startswith("#") or v.startswith("rgb"))


# Name helpers

def deep_merge(base, override):
    """Deep-merge two dicts. Override wins on conflicts."""
    result = dict(base)
    for k, v in override.items():
        if k in result and isinstance(result[k], dict) and isinstance(v, dict):
            result[k] = deep_merge(result[k], v)
        else:
            result[k] = v
    return result


_ALIAS_RE = re.compile(r"^\{([A-Za-z0-9_.]+)\}$")


def _alias_lookup(root, path):
    """Walk a dotted path from a product root to its value; fail loudly if any
    segment is missing."""
    node = root
    for seg in path.split("."):
        if not isinstance(node, dict) or seg not in node:
            raise SystemExit(f"alias path '{path}' unresolved at segment '{seg}'")
        node = node[seg]
    return node


def _resolve_alias_value(root, value, trail):
    """Follow a `{dotted.path}` reference transitively to its terminal literal.
    Non-reference values pass through. A path re-encountered along the current
    trail is a cycle and fails loudly."""
    if not isinstance(value, str):
        return value
    m = _ALIAS_RE.match(value)
    if not m:
        return value
    path = m.group(1)
    if path in trail:
        raise SystemExit(f"cyclic alias reference: {' -> '.join(trail + [path])}")
    return _resolve_alias_value(root, _alias_lookup(root, path), trail + [path])


def resolve_aliases(root):
    """Rewrite every `{dotted.path}` string reference in a product dict to its
    literal target, in place. Resolution is transitive (chains follow to the
    terminal literal), acyclic (a cycle raises), and order-independent (each
    value resolves by walking from the root, never from sibling order)."""
    def walk(node):
        if isinstance(node, dict):
            for k, v in node.items():
                if isinstance(v, (dict, list)):
                    walk(v)
                else:
                    node[k] = _resolve_alias_value(root, v, [])
        elif isinstance(node, list):
            for i, v in enumerate(node):
                if isinstance(v, (dict, list)):
                    walk(v)
                else:
                    node[i] = _resolve_alias_value(root, v, [])
    walk(root)
    return root


def camel_to_upper_snake(s):
    """camelCase -> UPPER_SNAKE."""
    s = s.replace("-", "_")
    s = re.sub(r"([a-z0-9])([A-Z])", r"\1_\2", s)
    s = re.sub(r"([a-zA-Z])(\d)", r"\1_\2", s)
    return s.upper()


def camel_to_lower_snake(s):
    """camelCase -> lower_snake (C++ namespace)."""
    s = s.replace("-", "_")
    s = re.sub(r"([a-z0-9])([A-Z])", r"\1_\2", s)
    return s.lower()


def camel_to_kebab(s):
    """camelCase -> kebab-case (CSS)."""
    s = re.sub(r"([a-z0-9])([A-Z])", r"\1-\2", s)
    s = re.sub(r"([a-zA-Z])(\d)", r"\1-\2", s)
    return s.lower().replace("_", "-")


def to_pascal(s):
    """any_case -> PascalCase (C++ struct names)."""
    parts = camel_to_lower_snake(s).split("_")
    return "".join(p.capitalize() for p in parts if p)


# C++ generation

_SKIP_CPP = frozenset({
    "$schema", "meta", "references", "cssTokens", "cssImports",
})

# C++ reserved words and common macro names to avoid
_CPP_RESERVED = frozenset({"MIN", "MAX", "NULL", "TRUE", "FALSE", "EOF"})


def _safe_cpp_name(name):
    """Avoid collisions with C/C++ macros."""
    if name in _CPP_RESERVED:
        return name + "_VAL"
    return name


def _cpp_float_literal(val):
    """Single source for float-to-C++ literal spelling, shared by the scalar and
    struct paths so a struct field and its sibling namespace constant cannot
    diverge."""
    return f"{val}f"


def _cpp_struct_field_type(sample):
    if is_color(sample):
        return "uint32_t"
    if isinstance(sample, bool):
        return "bool"
    if isinstance(sample, int):
        return "int"
    return "float"


def _cpp_struct_field_value(val):
    if is_color(val):
        argb = parse_color(val)
        return f"0x{argb:08x}" if argb is not None else "0u"
    if isinstance(val, bool):
        return "true" if val else "false"
    if isinstance(val, int):
        return str(val)
    if isinstance(val, float):
        return _cpp_float_literal(val)
    return "0"


def _cpp_emit_struct_group(group, indent, lines):
    """Emit `struct <$struct> { ... };` plus one constexpr instance per variant.

    A struct-shaped group carries a `$struct` marker naming the type; each
    sibling variant becomes a constexpr instance. Aggregate init is positional,
    so all variants must share an identical ordered field set. The generator
    fails when fields differ instead of silently shifting aggregate values."""
    pad = "    " * indent
    struct_name = group["$struct"]
    variants = [(k, v) for k, v in group.items()
                if k != "$struct" and isinstance(v, dict)]
    if not variants:
        return
    field_order = list(variants[0][1].keys())
    for vname, vdata in variants:
        if list(vdata.keys()) != field_order:
            raise SystemExit(
                f"$struct group '{struct_name}': variant '{vname}' field set/order "
                f"differs from '{variants[0][0]}'; aggregate init requires identical "
                f"ordered fields ({field_order} vs {list(vdata.keys())})")
    decls = " ".join(
        f"{_cpp_struct_field_type(variants[0][1][f])} {f};" for f in field_order)
    lines.append(f"{pad}struct {struct_name} {{ {decls} }};")
    for vname, vdata in variants:
        inst = _safe_cpp_name(camel_to_upper_snake(vname))
        vals = ", ".join(_cpp_struct_field_value(vdata[f]) for f in field_order)
        lines.append(f"{pad}constexpr {struct_name} {inst} = {{ {vals} }};")


def _cpp_emit(data, indent, lines):
    """Recursively emit constexpr declarations for a dict."""
    pad = "    " * indent

    # A `$struct` marker on this dict emits a struct + constexpr instances
    # alongside the flat per-variant constants emitted below.
    if "$struct" in data:
        _cpp_emit_struct_group(data, indent, lines)

    for key, val in data.items():
        if key == "$struct":
            continue
        if key == "note":
            lines.append(f"{pad}// {val}")
            continue

        # Nested dict -> sub-namespace (skip if it would be empty)
        if isinstance(val, dict):
            sub_lines = []
            _cpp_emit(val, indent + 1, sub_lines)
            if sub_lines:
                ns = camel_to_lower_snake(key)
                lines.append(f"{pad}namespace {ns} {{")
                lines.extend(sub_lines)
                lines.append(f"{pad}}}")
            continue

        # Array handling
        if isinstance(val, list):
            _cpp_emit_array(key, val, indent, lines)
            continue

        # Color -> constexpr uint32_t
        if is_color(val):
            argb = parse_color(val)
            if argb is not None:
                name = _safe_cpp_name(camel_to_upper_snake(key))
                lines.append(f"{pad}constexpr uint32_t {name} = 0x{argb:08x};")
            continue

        # Numeric -> constexpr int/float
        if isinstance(val, bool):
            continue
        if isinstance(val, int):
            name = _safe_cpp_name(camel_to_upper_snake(key))
            lines.append(f"{pad}constexpr int {name} = {val};")
            continue
        if isinstance(val, float):
            name = _safe_cpp_name(camel_to_upper_snake(key))
            lines.append(f"{pad}constexpr float {name} = {_cpp_float_literal(val)};")
            continue

        # Strings -> skip in C++ (font families, etc.)


def _cpp_emit_array(key, arr, indent, lines):
    """Emit C++ for array values."""
    pad = "    " * indent

    if not arr:
        return

    # Array of plain numbers -> constexpr int[]/float[]
    if all(isinstance(x, int) for x in arr):
        name = camel_to_upper_snake(key)
        items = ", ".join(str(x) for x in arr)
        lines.append(f"{pad}constexpr int {name}[] = {{ {items} }};")
        return

    if all(isinstance(x, (int, float)) for x in arr):
        name = camel_to_upper_snake(key)
        items = ", ".join(f"{x}f" for x in arr)
        lines.append(f"{pad}constexpr float {name}[] = {{ {items} }};")
        return

    # Array of gradient-like objects: { position/stop/gr, color }
    if isinstance(arr[0], dict):
        sample = arr[0]
        has_color = "color" in sample
        pos_key = next((k for k in ("position", "stop", "gr") if k in sample), None)

        if has_color and pos_key:
            struct = to_pascal(key) + "Stop"
            name = camel_to_upper_snake(key)
            lines.append(f"{pad}struct {struct} {{ float position; uint32_t color; }};")
            lines.append(f"{pad}constexpr {struct} {name}[] = {{")
            for entry in arr:
                pos = float(entry[pos_key])
                argb = parse_color(entry["color"])
                if argb is not None:
                    lines.append(f"{pad}    {{ {pos}f, 0x{argb:08x} }},")
            lines.append(f"{pad}}};")
            return

    # Complex arrays: emit as a pointer back to the token source.
    lines.append(f"{pad}// {key}: [{len(arr)} entries] (complex array; see token JSON)")


def generate_cpp(products):
    """Generate full C++ header from product token dicts."""
    lines = [
        "// Copyright (c) 2026 Bellweather Studios.",
        "// SPDX-License-Identifier: Apache-2.0",
        "//",
        "// BwTokens.h - Auto-generated from bds/tokens/products/*.tokens.json",
        "// DO NOT EDIT - regenerate with: python3 bds/tools/generate_tokens.py",
        "",
        "#pragma once",
        "",
        "#include <cstdint>",
        "",
        "namespace bws::tokens {",
        "",
    ]

    cpp_products = {k: v for k, v in products.items()
                    if v.get("meta", {}).get("platform") == "cpp"}

    for product, data in sorted(cpp_products.items()):
        ns = camel_to_lower_snake(product)
        lines.append(f"namespace {ns} {{")

        for section, section_data in data.items():
            if section in _SKIP_CPP:
                continue

            # Promote colors sub-categories directly into product namespace
            # so usage is bws::tokens::<product>::bg::DEEP (not ::colors::bg::DEEP)
            if section == "colors" and isinstance(section_data, dict):
                _cpp_emit(section_data, 1, lines)
                continue

            # Regular dict section -> namespace (skip if empty)
            if isinstance(section_data, dict):
                sub_lines = []
                _cpp_emit(section_data, 2, sub_lines)
                if sub_lines:
                    sns = camel_to_lower_snake(section)
                    lines.append(f"    namespace {sns} {{")
                    lines.extend(sub_lines)
                    lines.append(f"    }}")
                continue

            # Top-level array (meterGradient, shadows, zoneLayout)
            if isinstance(section_data, list):
                _cpp_emit_array(section, section_data, 1, lines)
                continue

            # Top-level scalar (rare)
            if isinstance(section_data, (int, float)):
                name = camel_to_upper_snake(section)
                if isinstance(section_data, float):
                    lines.append(f"    constexpr float {name} = {section_data}f;")
                else:
                    lines.append(f"    constexpr int {name} = {section_data};")

        lines.append(f"}} // namespace {ns}")
        lines.append("")

    lines.append("} // namespace bws::tokens")
    lines.append("")
    return "\n".join(lines)


# CSS generation

_SKIP_CSS = frozenset({
    "$schema", "meta", "references", "cssTokens", "cssImports",
})


def _css_emit(data, prefix, lines):
    """Recursively emit CSS custom properties."""
    for key, val in data.items():
        if key in ("note", "$struct"):
            continue
        prop = f"{prefix}-{camel_to_kebab(key)}"
        if isinstance(val, dict):
            _css_emit(val, prop, lines)
        elif isinstance(val, list):
            continue  # skip arrays in CSS
        elif isinstance(val, bool):
            continue
        elif isinstance(val, str):
            lines.append(f"  {prop}: {val};")
        elif isinstance(val, (int, float)):
            lines.append(f"  {prop}: {val};")


def generate_css(products):
    """Generate CSS custom properties from web product token dicts."""
    lines = [
        "/* tokens.css - Auto-generated from bds/tokens/products/*.tokens.json */",
        "/* DO NOT EDIT - regenerate with: npm run tokens */",
        "",
    ]

    web_products = {k: v for k, v in products.items()
                    if v.get("meta", {}).get("platform") == "web"}

    for product, data in sorted(web_products.items()):
        slug = camel_to_kebab(product)
        divider = "-" * max(1, 60 - len(product))
        lines.append(f"/* -- {product} {divider} */")
        lines.append(":root {")

        for section, section_data in data.items():
            if section in _SKIP_CSS:
                continue

            # Promote colors sub-categories (same logic as C++)
            if section == "colors" and isinstance(section_data, dict):
                for cat, cat_data in section_data.items():
                    if isinstance(cat_data, dict):
                        lines.append(f"  /* {cat} */")
                        _css_emit(cat_data, f"--bw-{slug}-{camel_to_kebab(cat)}", lines)
                continue

            if isinstance(section_data, dict):
                lines.append(f"  /* {section} */")
                _css_emit(section_data, f"--bw-{slug}-{camel_to_kebab(section)}", lines)
                continue

        lines.append("}")
        lines.append("")

    return "\n".join(lines)


# TypeScript generation

_SKIP_TS = frozenset({"$schema", "meta", "references"})


def _ts_value(val):
    """Format a value as a TypeScript literal (inline)."""
    if isinstance(val, str):
        return f'"{val}"'
    if isinstance(val, bool):
        return "true" if val else "false"
    if isinstance(val, (int, float)):
        return str(val)
    if isinstance(val, list):
        items = ", ".join(_ts_value(x) for x in val)
        return f"[{items}]"
    if isinstance(val, dict):
        pairs = []
        for k, v in val.items():
            safe_key = k if re.match(r"^[a-zA-Z_]\w*$", k) else f'"{k}"'
            pairs.append(f"{safe_key}: {_ts_value(v)}")
        return "{ " + ", ".join(pairs) + " }"
    return str(val)


def _ts_emit(data, indent, lines):
    """Recursively emit TypeScript object properties with indentation."""
    pad = "  " * indent
    for key, val in data.items():
        if key in ("note", "$struct"):
            continue
        safe_key = key if re.match(r"^[a-zA-Z_]\w*$", key) else f'"{key}"'
        if isinstance(val, dict):
            lines.append(f"{pad}{safe_key}: {{")
            _ts_emit(val, indent + 1, lines)
            lines.append(f"{pad}}},")
        elif isinstance(val, list):
            # Format arrays: short arrays inline, long arrays multiline
            inline = _ts_value(val)
            if len(inline) < 80:
                lines.append(f"{pad}{safe_key}: {inline},")
            else:
                lines.append(f"{pad}{safe_key}: [")
                for item in val:
                    lines.append(f"{pad}  {_ts_value(item)},")
                lines.append(f"{pad}],")
        else:
            lines.append(f"{pad}{safe_key}: {_ts_value(val)},")


def generate_ts(products):
    """Generate TypeScript const exports from web product token dicts."""
    lines = [
        "// tokens.ts - Auto-generated from bds/tokens/products/*.tokens.json",
        "// DO NOT EDIT - regenerate with: npm run tokens",
        "",
    ]

    web_products = {k: v for k, v in products.items()
                    if v.get("meta", {}).get("platform") == "web"}

    for product, data in sorted(web_products.items()):
        # Convert product names to valid JS identifiers.
        words = product.replace("-", " ").split()
        ident = words[0].lower() + "".join(w.capitalize() for w in words[1:])

        lines.append(f"export const {ident} = {{")
        for section, section_data in data.items():
            if section in _SKIP_TS:
                continue
            safe_key = section if re.match(r"^[a-zA-Z_]\w*$", section) else f'"{section}"'
            if isinstance(section_data, dict):
                lines.append(f"  {safe_key}: {{")
                _ts_emit(section_data, 2, lines)
                lines.append(f"  }},")
            elif isinstance(section_data, list):
                inline = _ts_value(section_data)
                if len(inline) < 80:
                    lines.append(f"  {safe_key}: {inline},")
                else:
                    lines.append(f"  {safe_key}: [")
                    for item in section_data:
                        lines.append(f"    {_ts_value(item)},")
                    lines.append(f"  ],")
            else:
                lines.append(f"  {safe_key}: {_ts_value(section_data)},")

        lines.append("} as const;")
        lines.append("")

    return "\n".join(lines)


# Main

def main():
    ap = argparse.ArgumentParser(
        description="Generate token constants from BDS per-product JSON files."
    )
    ap.add_argument(
        "--products", required=True,
        help="Directory containing *.tokens.json files",
    )
    ap.add_argument(
        "--base",
        help="Optional base.tokens.json for shared tokens",
    )
    ap.add_argument("--output-cpp", help="Output path for C++ header")
    ap.add_argument("--output-ts", help="Output path for TypeScript module")
    ap.add_argument("--output-css", help="Output path for CSS custom properties")
    args = ap.parse_args()

    products_dir = Path(args.products)
    if not products_dir.is_dir():
        print(f"ERROR: {products_dir} is not a directory", file=sys.stderr)
        return 1

    # Load all product token files
    products = {}
    for f in sorted(products_dir.glob("**/*.tokens.json")):
        name = f.stem.replace(".tokens", "")
        try:
            products[name] = json.loads(f.read_text(encoding="utf-8"))
        except (json.JSONDecodeError, OSError) as e:
            print(f"ERROR: {f}: {e}", file=sys.stderr)
            return 1

    if not products:
        print(f"ERROR: no *.tokens.json files found in {products_dir}", file=sys.stderr)
        return 1

    cpp_count = sum(1 for v in products.values() if v.get("meta", {}).get("platform") == "cpp")
    web_count = sum(1 for v in products.values() if v.get("meta", {}).get("platform") == "web")
    print(f"loaded {len(products)} products ({cpp_count} C++, {web_count} web)")

    # Load base tokens and merge into web products. Base provides defaults;
    # product values override. C++ products are unaffected.
    if args.base:
        base_path = Path(args.base)
        if base_path.exists():
            try:
                base = json.loads(base_path.read_text(encoding="utf-8"))
                base_platform = base.get("meta", {}).get("platform", "web")
                merged = 0
                for name, data in products.items():
                    if data.get("meta", {}).get("platform") == base_platform:
                        products[name] = deep_merge(base, data)
                        merged += 1
                print(f"merged base tokens into {merged} {base_platform} products")
            except (json.JSONDecodeError, OSError) as e:
                print(f"WARNING: could not load base tokens: {e}", file=sys.stderr)

    # Resolve {group.path} alias references to literals before emit, so the
    # generated constants are pre-resolved constexpr (no runtime resolver).
    for data in products.values():
        resolve_aliases(data)

    # Generate outputs
    written = []

    if args.output_cpp:
        dst = Path(args.output_cpp)
        dst.parent.mkdir(parents=True, exist_ok=True)
        content = generate_cpp(products)
        dst.write_text(content, encoding="utf-8")
        written.append(f"C++ -> {dst}")

    if args.output_ts:
        dst = Path(args.output_ts)
        dst.parent.mkdir(parents=True, exist_ok=True)
        content = generate_ts(products)
        dst.write_text(content, encoding="utf-8")
        written.append(f"TS  -> {dst}")

    if args.output_css:
        dst = Path(args.output_css)
        dst.parent.mkdir(parents=True, exist_ok=True)
        content = generate_css(products)
        dst.write_text(content, encoding="utf-8")
        written.append(f"CSS -> {dst}")

    for w in written:
        print(f"wrote {w}")

    if not written:
        print("WARNING: no output flags given (use --output-cpp, --output-ts, --output-css)")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
