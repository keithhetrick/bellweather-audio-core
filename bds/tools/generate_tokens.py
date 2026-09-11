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
import copy
import hashlib
import json
import re
import unicodedata
from pathlib import Path

TARGET_ORDER = ("cpp", "web", "swift")
_APPROVED_META_KEYS = frozenset(
    {
        "product",
        "version",
        "targets",
        "extends",
        "source",
        "note",
    }
)
_SEMVER_RE = re.compile(r"^\d+\.\d+\.\d+$")
MAX_SOURCE_BYTES = 1024 * 1024
MAX_TOKEN_DEPTH = 32
MAX_TOKEN_LEAVES = 10000
MAX_INHERITANCE_DEPTH = 8
MAX_ALIAS_CHAIN = 64


def _reject_duplicate_pairs(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise SystemExit(f"duplicate JSON key: {key}")
        result[key] = value
    return result


def _strict_json_loads(text):
    def reject_nonfinite(value):
        raise SystemExit(f"non-finite JSON number: {value}")

    return json.loads(
        text,
        object_pairs_hook=_reject_duplicate_pairs,
        parse_constant=reject_nonfinite,
    )


def _product_targets(data):
    return tuple(data.get("meta", {}).get("targets", ()))


def validate_product_metadata(filename_stem, data):
    """Validate the BDS-MULTI-TARGET-1.A public source interface."""
    meta = data.get("meta")
    if not isinstance(meta, dict):
        raise SystemExit(f"{filename_stem}: meta must be an object")
    unknown = sorted(set(meta) - _APPROVED_META_KEYS)
    if unknown:
        raise SystemExit(f"{filename_stem}: unknown metadata field(s): {', '.join(unknown)}")
    for field in ("product", "version", "targets", "extends"):
        if field not in meta:
            raise SystemExit(f"{filename_stem}: meta.{field} is required")
    if meta["product"] != filename_stem:
        raise SystemExit(f"{filename_stem}: meta.product must equal filename stem")
    if not isinstance(meta["version"], str) or not _SEMVER_RE.fullmatch(meta["version"]):
        raise SystemExit(f"{filename_stem}: meta.version must be semantic X.Y.Z")
    targets = meta["targets"]
    if not isinstance(targets, list) or not targets:
        raise SystemExit(f"{filename_stem}: meta.targets must be a nonempty array")
    if any(not isinstance(target, str) or target not in TARGET_ORDER for target in targets):
        raise SystemExit(f"{filename_stem}: meta.targets contains an unknown target")
    if len(set(targets)) != len(targets):
        raise SystemExit(f"{filename_stem}: meta.targets contains duplicates")
    canonical = [target for target in TARGET_ORDER if target in targets]
    if targets != canonical:
        raise SystemExit(f"{filename_stem}: meta.targets must use canonical cpp, web, swift order")
    extends = meta["extends"]
    if not isinstance(extends, list) or any(not isinstance(parent, str) for parent in extends):
        raise SystemExit(f"{filename_stem}: meta.extends must be an array of product IDs")


def validate_token_tree(filename_stem, data):
    """Validate bounded selected-target values before any emitter can skip them."""
    targets = _product_targets(data)
    leaf_count = 0

    def walk(node, path, depth):
        nonlocal leaf_count
        if depth > MAX_TOKEN_DEPTH:
            raise SystemExit(f"{filename_stem}: token tree exceeds depth {MAX_TOKEN_DEPTH}")
        if isinstance(node, dict):
            for key, value in node.items():
                if depth == 0 and key in ("$schema", "meta"):
                    continue
                if key in ("note", "$struct"):
                    continue
                walk(value, f"{path}.{key}" if path else key, depth + 1)
            return
        if isinstance(node, list):
            for index, value in enumerate(node):
                walk(value, f"{path}[{index}]", depth + 1)
            return
        leaf_count += 1
        if leaf_count > MAX_TOKEN_LEAVES:
            raise SystemExit(f"{filename_stem}: token tree exceeds {MAX_TOKEN_LEAVES} leaves")
        if isinstance(node, str) and is_color(node) and parse_color(node) is None:
            raise SystemExit(f"{filename_stem}: invalid color at '{path}'")
        if len(targets) > 1:
            if isinstance(node, str):
                alias = _ALIAS_RE.fullmatch(node)
                if not alias and not is_color(node):
                    raise SystemExit(
                        f"{filename_stem}: multi-target string at '{path}' is not a color or alias"
                    )
                if node.startswith("#") and len(node) == 9:
                    raise SystemExit(
                        f"{filename_stem}: ambiguous eight-digit multi-target color at '{path}'"
                    )
            elif isinstance(node, bool):
                raise SystemExit(
                    f"{filename_stem}: boolean at '{path}' is unsupported by every selected emitter"
                )

    walk(data, "", 0)


# Color parsing

_RGBA_RE = re.compile(r"rgba?\(\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*(?:,\s*([\d.]+))?\s*\)")


def hex_to_argb(s):
    """Convert #RRGGBB or #AARRGGBB to 0xAARRGGBB integer."""
    h = s.lstrip("#")
    if len(h) == 6 and re.fullmatch(r"[0-9A-Fa-f]{6}", h):
        return 0xFF000000 | int(h, 16)
    if len(h) == 8 and re.fullmatch(r"[0-9A-Fa-f]{8}", h):
        return int(h, 16)
    return None


def rgba_to_argb(s):
    """Convert rgba(r, g, b, a) to 0xAARRGGBB integer."""
    m = _RGBA_RE.fullmatch(s)
    if not m:
        return None
    r, g, b = int(m.group(1)), int(m.group(2)), int(m.group(3))
    a = float(m.group(4)) if m.group(4) else 1.0
    if any(channel > 255 for channel in (r, g, b)) or not 0.0 <= a <= 1.0:
        return None
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


def _merge_token_tree(base, override, path=""):
    """Merge compatible token nodes while keeping leaf overrides authoritative."""
    result = copy.deepcopy(base)
    for key, value in override.items():
        child_path = f"{path}.{key}" if path else key
        if key not in result:
            result[key] = copy.deepcopy(value)
            continue
        existing = result[key]
        if isinstance(existing, dict) and isinstance(value, dict):
            result[key] = _merge_token_tree(existing, value, child_path)
            continue
        if isinstance(existing, dict) != isinstance(value, dict):
            raise SystemExit(f"incompatible inheritance override at '{child_path}'")
        existing_kind = _token_kind(existing)
        override_kind = _token_kind(value)
        if existing_kind != override_kind and "alias" not in (existing_kind, override_kind):
            raise SystemExit(
                f"incompatible inheritance override at '{child_path}': "
                f"{existing_kind} cannot become {override_kind}"
            )
        result[key] = copy.deepcopy(value)
    return result


def _token_kind(value):
    if isinstance(value, dict):
        return "object"
    if isinstance(value, list):
        return "array"
    if isinstance(value, bool):
        return "boolean"
    if isinstance(value, (int, float)):
        return "number"
    if isinstance(value, str) and _ALIAS_RE.fullmatch(value):
        return "alias"
    if is_color(value):
        return "color"
    if isinstance(value, str):
        return "string"
    return type(value).__name__


def resolve_product_inheritance(products, foundations):
    """Return product token trees with ordered foundations applied.

    The returned mapping is independent of caller-owned dictionaries. Metadata
    always belongs to the leaf product and is never inherited as token data.
    """
    resolved_foundations = {}

    def resolve_foundation(name, trail):
        if name in resolved_foundations:
            return resolved_foundations[name]
        if name not in foundations:
            raise SystemExit(f"missing token foundation: {name}")
        if name in trail:
            raise SystemExit(f"cyclic token inheritance: {' -> '.join([*trail, name])}")
        if len(trail) >= MAX_INHERITANCE_DEPTH:
            raise SystemExit(f"token inheritance exceeds depth {MAX_INHERITANCE_DEPTH}")
        foundation = foundations[name]
        validate_product_metadata(name, foundation)
        merged = {}
        for parent in foundation["meta"]["extends"]:
            parent_data = resolve_foundation(parent, [*trail, name])
            merged = _merge_token_tree(merged, parent_data)
        own_tokens = {key: value for key, value in foundation.items() if key != "meta"}
        merged = _merge_token_tree(merged, own_tokens)
        resolved_foundations[name] = merged
        return merged

    resolved_products = {}
    for name, product in products.items():
        validate_product_metadata(name, product)
        merged = {}
        child_targets = set(product["meta"]["targets"])
        for parent in product["meta"]["extends"]:
            if parent not in foundations:
                raise SystemExit(f"missing token foundation: {parent}")
            parent_targets = set(foundations[parent]["meta"]["targets"])
            if not child_targets.issubset(parent_targets):
                raise SystemExit(f"{name}: targets are not supported by foundation '{parent}'")
            merged = _merge_token_tree(merged, resolve_foundation(parent, [name]))
        own_tokens = {key: value for key, value in product.items() if key != "meta"}
        merged = _merge_token_tree(merged, own_tokens)
        resolved_products[name] = {"meta": copy.deepcopy(product["meta"]), **merged}
    return resolved_products


_ALIAS_RE = re.compile(r"^\{([A-Za-z0-9_.]+)\}$")


def _alias_lookup(root, path):
    """Walk a dotted path from a product root to its value; fail loudly if any
    segment is missing."""
    if path.split(".", 1)[0] in {"meta", "$schema", "references"}:
        raise SystemExit(f"alias path '{path}' refers to metadata")
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
        raise SystemExit(f"cyclic alias reference: {' -> '.join([*trail, path])}")
    if len(trail) >= MAX_ALIAS_CHAIN:
        raise SystemExit(f"alias chain exceeds {MAX_ALIAS_CHAIN} references")
    target = _alias_lookup(root, path)
    if isinstance(target, (dict, list)):
        raise SystemExit(f"alias path '{path}' refers to a container")
    return _resolve_alias_value(root, target, [*trail, path])


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


def _normalized_source_name(name):
    return unicodedata.normalize("NFKC", name)


def _web_product_identifier(product):
    words = _normalized_source_name(product).replace("-", " ").split()
    return words[0].lower() + "".join(word.capitalize() for word in words[1:])


def _assert_unique_normalized(scope, entries):
    seen = {}
    for authored, normalized in entries:
        previous = seen.get(normalized)
        if previous is not None and previous != authored:
            raise SystemExit(
                f"normalized identifier collision in {scope}: "
                f"'{previous}' and '{authored}' both become '{normalized}'"
            )
        seen[normalized] = authored


def validate_normalized_identifiers(products):
    """Reject target API collisions before any emitter writes output."""
    product_entries = {target: [] for target in TARGET_ORDER}
    for product, data in products.items():
        targets = _product_targets(data)
        for target in targets:
            if target == "cpp":
                normalized = camel_to_lower_snake(_normalized_source_name(product))
            elif target == "swift":
                normalized = _swift_type_name(_normalized_source_name(product))
            else:
                normalized = _web_product_identifier(product)
            product_entries[target].append((product, normalized))

        def walk_scopes(node, path, targets=targets, product=product):
            if not isinstance(node, dict):
                return
            authored_items = [
                (key, value)
                for key, value in node.items()
                if key not in {"$schema", "meta", "note", "$struct"}
            ]
            for target in targets:
                if target == "cpp":
                    entries = [
                        (
                            key,
                            camel_to_lower_snake(_normalized_source_name(key))
                            if isinstance(value, dict)
                            else camel_to_upper_snake(_normalized_source_name(key)),
                        )
                        for key, value in authored_items
                    ]
                elif target == "swift":
                    entries = [
                        (
                            key,
                            _swift_type_name(_normalized_source_name(key))
                            if isinstance(value, dict)
                            else _swift_value_name(_normalized_source_name(key)).strip("`"),
                        )
                        for key, value in authored_items
                    ]
                else:
                    entries = [(key, _normalized_source_name(key)) for key, _ in authored_items]
                _assert_unique_normalized(
                    f"{target} product '{product}' at '{path or '<root>'}'", entries
                )
            for key, value in authored_items:
                if isinstance(value, dict):
                    walk_scopes(value, f"{path}.{key}" if path else key)

        walk_scopes(data, "")

        if "web" in targets:
            css_paths = []

            def collect_css(node, segments, css_paths=css_paths):
                for key, value in node.items():
                    if key in {"$schema", "meta", "note", "$struct"}:
                        continue
                    next_segments = [*segments, camel_to_kebab(_normalized_source_name(key))]
                    if isinstance(value, dict):
                        collect_css(value, next_segments)
                    elif isinstance(value, (str, int, float)) and not isinstance(value, bool):
                        css_paths.append((".".join([*segments, key]), "-".join(next_segments)))

            for section, section_data in data.items():
                if section in _SKIP_CSS or not isinstance(section_data, dict):
                    continue
                if section == "colors":
                    collect_css(section_data, [])
                else:
                    collect_css(section_data, [camel_to_kebab(section)])
            _assert_unique_normalized(f"CSS product '{product}'", css_paths)

    for target, entries in product_entries.items():
        _assert_unique_normalized(f"{target} product exports", entries)


# C++ generation

_SKIP_CPP = frozenset(
    {
        "$schema",
        "meta",
        "references",
        "cssTokens",
        "cssImports",
    }
)

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
    variants = [(k, v) for k, v in group.items() if k != "$struct" and isinstance(v, dict)]
    if not variants:
        return
    field_order = list(variants[0][1].keys())
    for vname, vdata in variants:
        if list(vdata.keys()) != field_order:
            raise SystemExit(
                f"$struct group '{struct_name}': variant '{vname}' field set/order "
                f"differs from '{variants[0][0]}'; aggregate init requires identical "
                f"ordered fields ({field_order} vs {list(vdata.keys())})"
            )
    decls = " ".join(f"{_cpp_struct_field_type(variants[0][1][f])} {f};" for f in field_order)
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

    cpp_products = {k: v for k, v in products.items() if "cpp" in _product_targets(v)}

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
                    lines.append("    }")
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

_SKIP_CSS = frozenset(
    {
        "$schema",
        "meta",
        "references",
        "cssTokens",
        "cssImports",
    }
)


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

    web_products = {k: v for k, v in products.items() if "web" in _product_targets(v)}

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
        return json.dumps(val, ensure_ascii=False)
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
            safe_key = k if re.match(r"^[a-zA-Z_]\w*$", k) else json.dumps(k, ensure_ascii=False)
            pairs.append(f"{safe_key}: {_ts_value(v)}")
        return "{ " + ", ".join(pairs) + " }"
    return str(val)


def _ts_emit(data, indent, lines):
    """Recursively emit TypeScript object properties with indentation."""
    pad = "  " * indent
    for key, val in data.items():
        if key in ("note", "$struct"):
            continue
        safe_key = key if re.match(r"^[a-zA-Z_]\w*$", key) else json.dumps(key, ensure_ascii=False)
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

    web_products = {k: v for k, v in products.items() if "web" in _product_targets(v)}

    for product, data in sorted(web_products.items()):
        # Convert product names to valid JS identifiers.
        ident = _web_product_identifier(product)

        lines.append(f"export const {ident} = {{")
        for section, section_data in data.items():
            if section in _SKIP_TS:
                continue
            safe_key = (
                section
                if re.match(r"^[a-zA-Z_]\w*$", section)
                else json.dumps(section, ensure_ascii=False)
            )
            if isinstance(section_data, dict):
                lines.append(f"  {safe_key}: {{")
                _ts_emit(section_data, 2, lines)
                lines.append("  },")
            elif isinstance(section_data, list):
                inline = _ts_value(section_data)
                if len(inline) < 80:
                    lines.append(f"  {safe_key}: {inline},")
                else:
                    lines.append(f"  {safe_key}: [")
                    for item in section_data:
                        lines.append(f"    {_ts_value(item)},")
                    lines.append("  ],")
            else:
                lines.append(f"  {safe_key}: {_ts_value(section_data)},")

        lines.append("} as const;")
        lines.append("")

    return "\n".join(lines)


# Swift generation

_SKIP_SWIFT = frozenset(
    {
        "$schema",
        "meta",
        "references",
        "cssTokens",
        "cssImports",
        "note",
        "$struct",
    }
)
_SWIFT_RESERVED = frozenset(
    {
        "associatedtype",
        "class",
        "deinit",
        "enum",
        "extension",
        "fileprivate",
        "func",
        "import",
        "init",
        "inout",
        "internal",
        "let",
        "open",
        "operator",
        "private",
        "protocol",
        "public",
        "rethrows",
        "static",
        "struct",
        "subscript",
        "typealias",
        "var",
        "break",
        "case",
        "continue",
        "default",
        "defer",
        "do",
        "else",
        "fallthrough",
        "for",
        "guard",
        "if",
        "in",
        "repeat",
        "return",
        "switch",
        "where",
        "while",
        "as",
        "Any",
        "catch",
        "false",
        "is",
        "nil",
        "super",
        "self",
        "Self",
        "throw",
        "throws",
        "true",
        "try",
    }
)


def _swift_type_name(name):
    ident = to_pascal(name)
    if not ident or ident[0].isdigit():
        ident = "Token" + ident
    return ident


def _swift_value_name(name):
    words = [part for part in camel_to_lower_snake(name).split("_") if part]
    ident = words[0] + "".join(word.capitalize() for word in words[1:]) if words else "token"
    if ident[0].isdigit():
        ident = "token" + ident
    if ident in _SWIFT_RESERVED:
        ident = f"`{ident}`"
    return ident


def _swift_scalar(value):
    if is_color(value):
        argb = parse_color(value)
        if argb is None:
            raise SystemExit(f"unsupported Swift color: {value}")
        return "UInt32", f"0x{argb:08x}"
    if isinstance(value, bool):
        return "Bool", "true" if value else "false"
    if isinstance(value, int):
        return "Int", str(value)
    if isinstance(value, float):
        return "Double", repr(value)
    if isinstance(value, str):
        return "String", json.dumps(value, ensure_ascii=False)
    raise SystemExit(f"unsupported Swift scalar: {type(value).__name__}")


def _swift_emit(data, indent, lines):
    pad = "    " * indent
    for key, value in data.items():
        if key in _SKIP_SWIFT:
            continue
        if isinstance(value, dict):
            lines.append(f"{pad}public enum {_swift_type_name(key)} {{")
            _swift_emit(value, indent + 1, lines)
            lines.append(f"{pad}}}")
            continue
        if isinstance(value, list):
            if not value:
                raise SystemExit(f"Swift array '{key}' must not be empty")
            typed = [_swift_scalar(item) for item in value]
            element_type = typed[0][0]
            if any(item_type != element_type for item_type, _ in typed):
                raise SystemExit(f"Swift array '{key}' must be homogeneous")
            literals = ", ".join(literal for _, literal in typed)
            lines.append(
                f"{pad}public static let {_swift_value_name(key)}: [{element_type}] = [{literals}]"
            )
            continue
        value_type, literal = _swift_scalar(value)
        lines.append(f"{pad}public static let {_swift_value_name(key)}: {value_type} = {literal}")


def generate_swift(products):
    lines = [
        "// BellweatherTokens.generated.swift - Auto-generated from BDS token sources",
        "// DO NOT EDIT - regenerate with: npm run tokens",
        "",
        "public enum BellweatherTokens {",
    ]
    swift_products = {
        key: value for key, value in products.items() if "swift" in _product_targets(value)
    }
    for product, data in sorted(swift_products.items()):
        lines.append(f"    public enum {_swift_type_name(product)} {{")
        for section, section_data in data.items():
            if section in _SKIP_SWIFT:
                continue
            if isinstance(section_data, dict):
                lines.append(f"        public enum {_swift_type_name(section)} {{")
                _swift_emit(section_data, 3, lines)
                lines.append("        }")
            elif isinstance(section_data, list):
                _swift_emit({section: section_data}, 2, lines)
            else:
                _swift_emit({section: section_data}, 2, lines)
        lines.append("    }")
    lines.append("}")
    lines.append("")
    return "\n".join(lines)


# Main

_EMITTERS = {
    "cpp": generate_cpp,
    "typescript": generate_ts,
    "css": generate_css,
    "swift": generate_swift,
}


def _load_token_file(path):
    try:
        if path.stat().st_size > MAX_SOURCE_BYTES:
            raise SystemExit(f"{path}: source exceeds {MAX_SOURCE_BYTES} bytes")
        data = _strict_json_loads(path.read_text(encoding="utf-8"))
    except (json.JSONDecodeError, OSError) as exc:
        raise SystemExit(f"{path}: {exc}") from exc
    name = path.name.replace(".tokens.json", "")
    validate_product_metadata(name, data)
    validate_token_tree(name, data)
    return name, data


def _load_products(products_dir):
    if not products_dir.is_dir():
        raise SystemExit(f"{products_dir} is not a directory")
    products = {}
    for path in sorted(products_dir.glob("**/*.tokens.json")):
        name, data = _load_token_file(path)
        if name in products:
            raise SystemExit(f"duplicate product filename stem: {name}")
        products[name] = data
    if not products:
        raise SystemExit(f"no *.tokens.json files found in {products_dir}")
    return products


def _load_foundations(paths):
    foundations = {}
    for path in paths:
        name, data = _load_token_file(path)
        if name in foundations:
            raise SystemExit(f"duplicate token foundation: {name}")
        foundations[name] = data
    return foundations


def _render_output_set(products, outputs):
    rendered = {}
    for emitter, destination in outputs.items():
        if emitter not in _EMITTERS:
            raise SystemExit(f"unknown token emitter: {emitter}")
        rendered[destination] = _EMITTERS[emitter](products)
    return rendered


def _replace_output_set(rendered):
    """Render-first, per-file atomic replacement for BDS-MULTI-TARGET-1.D."""
    staged = []
    try:
        for destination, content in rendered.items():
            destination.parent.mkdir(parents=True, exist_ok=True)
            temporary = destination.with_name(destination.name + ".bds-tmp")
            temporary.write_text(content, encoding="utf-8")
            staged.append((temporary, destination))
        for temporary, destination in staged:
            temporary.replace(destination)
    finally:
        for temporary, _ in staged:
            if temporary.exists():
                temporary.unlink()


def _resolved_inside(root, candidate):
    try:
        candidate.relative_to(root)
        return True
    except ValueError:
        return False


def _manifest_generation(manifest_path):
    try:
        manifest_bytes = manifest_path.read_bytes()
        manifest = _strict_json_loads(manifest_bytes.decode("utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise SystemExit(f"{manifest_path}: {exc}") from exc
    if manifest.get("schemaVersion") != "bds-token-generation/v1":
        raise SystemExit("unsupported token generation manifest schemaVersion")
    required = {"repositoryRoot", "schema", "products", "foundations", "outputs"}
    if set(manifest) != required | {"schemaVersion"}:
        raise SystemExit("token generation manifest has missing or unknown fields")
    root = (manifest_path.parent / manifest["repositoryRoot"]).resolve()
    schema_path = (root / manifest["schema"]).resolve()
    if not _resolved_inside(root, schema_path) or not schema_path.is_file():
        raise SystemExit("manifest schema is missing or outside repository root")
    products_dir = (root / manifest["products"]).resolve()
    foundation_paths = [(root / value).resolve() for value in manifest["foundations"]]
    if not _resolved_inside(root, products_dir) or not products_dir.is_dir():
        raise SystemExit("manifest products directory is missing or outside repository root")
    if any(not _resolved_inside(root, path) or not path.is_file() for path in foundation_paths):
        raise SystemExit("manifest foundation is missing or outside repository root")
    output_spec = manifest["outputs"]
    if not isinstance(output_spec, dict) or "integrity" not in output_spec:
        raise SystemExit("manifest outputs must contain an integrity destination")
    output_paths = {}
    for emitter, relative in output_spec.items():
        destination = (root / relative).resolve()
        if not _resolved_inside(root, destination):
            raise SystemExit(f"manifest output escapes repository root: {relative}")
        if destination in output_paths.values():
            raise SystemExit(f"duplicate manifest output destination: {relative}")
        output_paths[emitter] = destination
    integrity_path = output_paths.pop("integrity")
    products = _load_products(products_dir)
    foundations = _load_foundations(foundation_paths)
    resolved = resolve_product_inheritance(products, foundations)
    for data in resolved.values():
        resolve_aliases(data)
    validate_normalized_identifiers(resolved)
    selected_targets = {target for data in resolved.values() for target in _product_targets(data)}
    required_emitters = {"integrity"}
    if "cpp" in selected_targets:
        required_emitters.add("cpp")
    if "web" in selected_targets:
        required_emitters.update({"typescript", "css"})
    if "swift" in selected_targets:
        required_emitters.add("swift")
    if set(output_spec) != required_emitters:
        raise SystemExit(
            "manifest output set does not exactly match selected targets: "
            f"expected {sorted(required_emitters)}, got {sorted(output_spec)}"
        )
    rendered = _render_output_set(resolved, output_paths)
    source_hash = hashlib.sha256()
    source_hash.update(manifest_bytes)
    compiler_bytes = Path(__file__).read_bytes()
    source_hash.update(b"bds/tools/generate_tokens.py")
    source_hash.update(compiler_bytes)
    for path in sorted([schema_path, *products_dir.glob("**/*.tokens.json"), *foundation_paths]):
        source_hash.update(path.relative_to(root).as_posix().encode("utf-8"))
        source_hash.update(path.read_bytes())
    lock_outputs = {}
    for destination, content in sorted(rendered.items(), key=lambda item: str(item[0])):
        relative = destination.relative_to(root).as_posix()
        lock_outputs[relative] = hashlib.sha256(content.encode("utf-8")).hexdigest()
    lock = {
        "schemaVersion": "bds-token-output-set/v1",
        "compilerFingerprint": hashlib.sha256(compiler_bytes).hexdigest(),
        "sourceFingerprint": source_hash.hexdigest(),
        "outputs": lock_outputs,
    }
    rendered[integrity_path] = json.dumps(lock, indent=2, sort_keys=True) + "\n"
    _replace_output_set(rendered)
    for destination in rendered:
        print(f"wrote {destination.relative_to(root)}")


def _explicit_generation(args):
    products = _load_products(Path(args.products))
    foundations = _load_foundations([Path(args.base)]) if args.base else {}
    if foundations:
        products = resolve_product_inheritance(products, foundations)
    for data in products.values():
        resolve_aliases(data)
    validate_normalized_identifiers(products)
    outputs = {}
    if args.output_cpp:
        outputs["cpp"] = Path(args.output_cpp)
    if args.output_ts:
        outputs["typescript"] = Path(args.output_ts)
    if args.output_css:
        outputs["css"] = Path(args.output_css)
    if args.output_swift:
        outputs["swift"] = Path(args.output_swift)
    if not outputs:
        print("WARNING: no output flags given")
        return
    _replace_output_set(_render_output_set(products, outputs))
    for emitter, destination in outputs.items():
        print(f"wrote {emitter} -> {destination}")


def main():
    ap = argparse.ArgumentParser(
        description="Generate token constants from BDS per-product JSON files."
    )
    ap.add_argument(
        "--products",
        help="Directory containing *.tokens.json files",
    )
    ap.add_argument("--manifest", help="Canonical BDS generation manifest")
    ap.add_argument(
        "--base",
        help="Optional base.tokens.json for shared tokens",
    )
    ap.add_argument("--output-cpp", help="Output path for C++ header")
    ap.add_argument("--output-ts", help="Output path for TypeScript module")
    ap.add_argument("--output-css", help="Output path for CSS custom properties")
    ap.add_argument("--output-swift", help="Output path for Swift constants")
    args = ap.parse_args()
    explicit_values = [
        args.products,
        args.base,
        args.output_cpp,
        args.output_ts,
        args.output_css,
        args.output_swift,
    ]
    if args.manifest:
        if any(value is not None for value in explicit_values):
            ap.error("--manifest is mutually exclusive with explicit generation flags")
        _manifest_generation(Path(args.manifest).resolve())
        return 0
    if not args.products:
        ap.error("--products is required in explicit generation mode")
    _explicit_generation(args)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
