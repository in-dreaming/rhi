#!/usr/bin/env python3
"""Offline shader compiler: .slang -> SPIRV / DXIL / MSL / WGSL + reflection JSON."""

import argparse
import subprocess
import sys
import os
from pathlib import Path


def find_slangc():
    slangc = os.environ.get("SLANGC_PATH")
    if slangc and Path(slangc).is_file():
        return slangc
    for candidate in ["slangc", "slangc.exe"]:
        try:
            result = subprocess.run([candidate, "--version"],
                                   capture_output=True, text=True)
            if result.returncode == 0:
                return candidate
        except FileNotFoundError:
            continue
    print("Error: slangc not found. Install Slang or set SLANGC_PATH.", file=sys.stderr)
    sys.exit(1)


def compile_shader(slangc, input_path, output_dir, entry, targets):
    input_path = Path(input_path)
    output_dir = Path(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    stem = input_path.stem
    entry_suffix = f"-{entry}" if entry else ""
    target_map = {
        "spirv":  (["-target", "spirv", "-profile", "spirv_1_3"], f"{stem}{entry_suffix}.spv"),
        "dxil":   (["-target", "dxil", "-profile", "sm_6_0"],     f"{stem}{entry_suffix}.dxil"),
        "msl":    (["-target", "metal", "-profile", "metallib_3_0"], f"{stem}{entry_suffix}.msl"),
        "wgsl":   (["-target", "wgsl"],                          f"{stem}{entry_suffix}.wgsl"),
    }

    for t in targets:
        if t not in target_map:
            print(f"Warning: unknown target '{t}', skipping", file=sys.stderr)
            continue
        extra_args, out_name = target_map[t]
        out_path = output_dir / out_name
        cmd = [slangc, str(input_path), *extra_args, "-o", str(out_path)]
        if entry:
            cmd.extend(["-entry", entry])
        print(f"  {t}: {out_path}")
        result = subprocess.run(cmd, capture_output=True, text=True)
        if result.returncode != 0:
            print(f"Error compiling {t}:\n{result.stderr}", file=sys.stderr)
        if result.stderr:
            print(f"  warnings:\n{result.stderr}")

    reflection_path = output_dir / f"{stem}.reflection.json"
    cmd = [slangc, str(input_path), "-emit-reflection-json",
           "-o", str(reflection_path)]
    if entry:
        cmd.extend(["-entry", entry])
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode == 0:
        print(f"  reflection: {reflection_path}")
    elif result.stderr:
        print(f"  reflection failed: {result.stderr}", file=sys.stderr)


def main():
    parser = argparse.ArgumentParser(description="Offline Slang shader compiler")
    parser.add_argument("input", help="Input .slang file")
    parser.add_argument("-o", "--output-dir", default=".", help="Output directory")
    parser.add_argument("-e", "--entry", default="", help="Entry point name (comma-separated for multiple)")
    parser.add_argument("-t", "--targets", nargs="+",
                       default=["spirv", "dxil", "msl", "wgsl"],
                       help="Target formats (spirv dxil msl wgsl)")
    args = parser.parse_args()

    slangc = find_slangc()
    entries = [e.strip() for e in args.entry.split(",") if e.strip()]
    if not entries:
        entries = [""]

    for entry in entries:
        label = f"entry '{entry}'" if entry else "default"
        print(f"Compiling {label}:")
        compile_shader(slangc, args.input, args.output_dir, entry, args.targets)


if __name__ == "__main__":
    main()
