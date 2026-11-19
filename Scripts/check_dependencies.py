#!/usr/bin/env python3
"""Phase 0 dependency, licensing and secret-hygiene checks.

Runs in CI and locally. Exits non-zero when policy is violated.
"""
from __future__ import annotations

import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
REQUIRED_FILES = [
    "README.md",
    "LICENSE",
    "NOTICE",
    "CONTRIBUTING.md",
    "THIRD_PARTY.md",
    "CMakeLists.txt",
    ".github/workflows/windows-msvc.yml",
    ".github/workflows/sanitizers.yml",
    "Scripts/run_sanitizers.sh",
]
FORBIDDEN_PATHS = ["third_party/vst3", "third_party/asio"]
SECRET_PATTERNS = [re.compile(r"ghp_[A-Za-z0-9]{20,}"), re.compile(r"github_pat_[A-Za-z0-9_]{20,}")]


def main() -> int:
    failures: list[str] = []

    for relative in REQUIRED_FILES:
        if not (ROOT / relative).is_file():
            failures.append(f"missing required file: {relative}")

    for relative in FORBIDDEN_PATHS:
        path = ROOT / relative
        if path.is_dir() and any(path.iterdir()):
            failures.append(f"proprietary SDK must not be vendored: {relative}")

    for path in ROOT.rglob("*"):
        if not path.is_file() or any(part in {".git", "build", "_deps"} for part in path.parts):
            continue
        if path.suffix.lower() in {".png", ".jpg", ".pdf", ".zip", ".exe", ".dll", ".a", ".o"}:
            continue
        try:
            text = path.read_text(encoding="utf-8")
        except (UnicodeDecodeError, OSError):
            continue
        for pattern in SECRET_PATTERNS:
            if pattern.search(text):
                failures.append(f"possible credential in tracked file: {path.relative_to(ROOT)}")

    if failures:
        print("Dependency policy check FAILED:")
        for failure in failures:
            print(f"  - {failure}")
        return 1

    print("Dependency policy check: PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
