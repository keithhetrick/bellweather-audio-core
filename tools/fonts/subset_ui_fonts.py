#!/usr/bin/env python3
# Copyright (c) 2026 Bellweather Studios.
# SPDX-License-Identifier: Apache-2.0

# Subset Bellweather UI fonts (Inter + JetBrains Mono NL) for in-binary embedding.
#
# Why: every Bellweather plugin ships the full TTFs (~1.19 MB) embedded via
# juce_add_binary_data. Inter alone ships full Latin + Cyrillic + Greek +
# Vietnamese + 1000+ symbol glyphs, of which the plugin UI uses ASCII + Latin
# Extended + a small set of UI/math symbols. CJK localizations (ja/ko/zh) fall
# back to system fonts at runtime - no embedded CJK font ships.
#
# Subset target ("Mid" level, chosen 2026-05-30):
#   - Unicode: ASCII, Latin-1 Supp, Latin Extended-A/B/Additional, General
#     Punctuation, super/subscripts, Currency, Letterlike, Number Forms,
#     Math Operators, Geometric Shapes, Misc Symbols, Dingbats.
#   - OT features kept: kern, ccmp, locl, mark, mkmk, tnum (tabular nums for
#     meter readouts), calt (contextual alts incl. JetBrains Mono ligatures).
#   - Hinting kept (preserves small-size rendering on low-DPI displays).
#   - Discretionary features dropped: dlig, salt, ss01-ss20, onum, hist, etc.
#
# Outputs go to ${output_dir}/<basename>.ttf with the SAME filename as the
# source, so juce_add_binary_data generates the same C++ identifiers (the
# FontManifest.cpp lookup table doesn't need to change).
#
# Run manually:
#   python3 tools/fonts/subset_ui_fonts.py \
#       --source-dir modules/bw_ui/stylekit/assets/fonts \
#       --output-dir /tmp/font_subset_out
#
# CMake invokes this automatically - see modules/bw_ui/CMakeLists.txt
# "juce_add_binary_data(bw_ui_FontAssets ...)" block.

import argparse
import sys
from pathlib import Path


UNICODE_RANGES = ",".join([
    "U+0020-007E",  # Basic Latin (ASCII printable)
    "U+00A0-00FF",  # Latin-1 Supplement
    "U+0100-017F",  # Latin Extended-A
    "U+0180-024F",  # Latin Extended-B
    "U+1E00-1EFF",  # Latin Extended Additional (qps-ploc pseudo-loc uses these)
    "U+2000-206F",  # General Punctuation (em-dash, curly quotes, etc.)
    "U+2070-209F",  # Superscripts and Subscripts
    "U+20A0-20CF",  # Currency Symbols
    "U+2100-214F",  # Letterlike Symbols (™, ℗, ℅, dB-adjacent)
    "U+2150-218F",  # Number Forms
    "U+2200-22FF",  # Mathematical Operators (±, ×, ÷, ≤, ≥, ≠, ∞)
    "U+25A0-25FF",  # Geometric Shapes (▶, ●, ◀)
    "U+2600-26FF",  # Miscellaneous Symbols (☀, ☁, ♭, ♯, ⚠)
    "U+2700-27BF",  # Dingbats (✓, ✗, ✶)
])

KEPT_LAYOUT_FEATURES = ["kern", "ccmp", "locl", "mark", "mkmk", "tnum", "calt"]

# Hinting tables - TrueType bytecode instructions for small-size rendering.
# Modern macOS ignores entirely (subpixel AA since 10.6); modern Windows
# DirectWrite ignores at sizes >= ~13px; FreeType (Linux) still uses them.
# "tight" level drops these for ~170 KB extra savings per plugin.
HINTING_TABLES = ["cvt ", "fpgm", "prep", "gasp", "hdmx", "LTSH", "VDMX"]

SOURCE_TO_OUTPUT = {
    "JetBrainsMono/JetBrainsMonoNL-Regular.ttf": "JetBrainsMonoNL-Regular.ttf",
    "JetBrainsMono/JetBrainsMonoNL-Medium.ttf":  "JetBrainsMonoNL-Medium.ttf",
    "Inter/Inter-Regular.ttf":                   "Inter-Regular.ttf",
    "Inter/Inter-SemiBold.ttf":                  "Inter-SemiBold.ttf",
}


def subset_one(src: Path, dst: Path, level: str) -> tuple[int, int]:
    from fontTools.subset import Subsetter, Options, load_font, save_font

    options = Options()
    options.layout_features = KEPT_LAYOUT_FEATURES
    options.name_IDs = ["*"]
    options.name_legacy = True
    options.name_languages = ["*"]
    options.glyph_names = False
    options.notdef_glyph = True
    options.notdef_outline = True
    options.recommended_glyphs = False
    if level == "tight":
        options.drop_tables = HINTING_TABLES
        options.hinting = False
    else:
        options.drop_tables = []  # keep hinting tables (cvt, fpgm, prep, gasp)

    font = load_font(str(src), options)
    subsetter = Subsetter(options=options)
    subsetter.populate(unicodes=parse_unicode_ranges(UNICODE_RANGES))
    subsetter.subset(font)
    save_font(font, str(dst), options)

    return src.stat().st_size, dst.stat().st_size


def parse_unicode_ranges(spec: str) -> list[int]:
    out: list[int] = []
    for token in spec.split(","):
        token = token.strip()
        if not token:
            continue
        if token.upper().startswith("U+"):
            token = token[2:]
        if "-" in token:
            lo_s, hi_s = token.split("-", 1)
            lo, hi = int(lo_s, 16), int(hi_s, 16)
            out.extend(range(lo, hi + 1))
        else:
            out.append(int(token, 16))
    return out


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Subset Bellweather UI fonts for in-binary embedding.")
    parser.add_argument("--source-dir", required=True, type=Path,
                        help="bw_ui stylekit fonts root (contains Inter/ and JetBrainsMono/)")
    parser.add_argument("--output-dir", required=True, type=Path,
                        help="Where to write subset TTFs (typically build-tree)")
    parser.add_argument("--level", choices=["mid", "tight"], default="mid",
                        help="mid (default): keep hinting. tight: drop hinting tables "
                             "for ~170 KB extra savings per plugin.")
    args = parser.parse_args()

    try:
        import fontTools  # noqa: F401
    except ImportError:
        sys.stderr.write(
            "ERROR: fontTools is not installed. Install with:\n"
            "    python3 -m pip install fonttools\n"
            "or via Homebrew:\n"
            "    brew install fonttools\n"
        )
        return 1

    args.output_dir.mkdir(parents=True, exist_ok=True)

    total_in = total_out = 0
    for rel_src, out_name in SOURCE_TO_OUTPUT.items():
        src = args.source_dir / rel_src
        dst = args.output_dir / out_name
        if not src.is_file():
            sys.stderr.write(f"ERROR: source font missing: {src}\n")
            return 2
        in_b, out_b = subset_one(src, dst, args.level)
        total_in += in_b
        total_out += out_b
        pct = (out_b / in_b * 100.0) if in_b else 0.0
        print(f"  {out_name:40s}  {in_b:7d} -> {out_b:7d}  ({pct:5.1f}%)")

    saved = total_in - total_out
    print(f"  TOTAL: {total_in} -> {total_out}  (saved {saved} bytes, "
          f"{saved / 1024:.1f} KB)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
