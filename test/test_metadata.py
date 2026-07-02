"""Check that the package version is consistent across all manifests.

The project version is declared in several packaging manifests (one per
language binding). They are easy to let drift apart, so this test parses
each one and asserts they all agree.

TOML files are parsed with regexes rather than tomllib because the
project supports Python 3.10, where tomllib is not in the stdlib.
"""

import json
import re
from pathlib import Path

import pytest

ROOT = Path(__file__).parent.parent

VERSION_RE = re.compile(r"^\d+\.\d+\.\d+$")


def _toml_version(path, section):
    """Extract `version = "..."` from a given [section] of a TOML file."""
    content = (ROOT / path).read_text()
    match = re.search(
        rf'^\[{re.escape(section)}\][^[]*?^version = "([^"]+)"',
        content,
        re.MULTILINE | re.DOTALL,
    )
    assert match, f"version not found in [{section}] of {path}"
    return match.group(1)


def _pyproject_version():
    return _toml_version("pyproject.toml", "project")


def _cargo_toml_version():
    return _toml_version("Cargo.toml", "package")


def _cargo_lock_version():
    content = (ROOT / "Cargo.lock").read_text()
    match = re.search(
        r'^name = "tree-sitter-rst"\nversion = "([^"]+)"',
        content,
        re.MULTILINE,
    )
    assert match, "tree-sitter-rst not found in Cargo.lock"
    return match.group(1)


def _package_json_version():
    with open(ROOT / "package.json") as f:
        return json.load(f)["version"]


def _package_lock_versions():
    with open(ROOT / "package-lock.json") as f:
        data = json.load(f)
    return {
        "package-lock.json (top-level)": data["version"],
        "package-lock.json (root package)": data["packages"][""]["version"],
    }


def _tree_sitter_json_version():
    with open(ROOT / "tree-sitter.json") as f:
        return json.load(f)["metadata"]["version"]


def _cmake_version():
    content = (ROOT / "CMakeLists.txt").read_text()
    match = re.search(r'^\s*VERSION\s+"([^"]+)"', content, re.MULTILINE)
    assert match, "project VERSION not found in CMakeLists.txt"
    return match.group(1)


def _makefile_version():
    content = (ROOT / "Makefile").read_text()
    match = re.search(r"^VERSION\s*:=\s*(\S+)", content, re.MULTILINE)
    assert match, "VERSION := not found in Makefile"
    return match.group(1)


def collect_versions():
    versions = {
        "pyproject.toml": _pyproject_version(),
        "package.json": _package_json_version(),
        "tree-sitter.json": _tree_sitter_json_version(),
        "Cargo.toml": _cargo_toml_version(),
        "Cargo.lock": _cargo_lock_version(),
        "CMakeLists.txt": _cmake_version(),
        "Makefile": _makefile_version(),
    }
    versions.update(_package_lock_versions())
    return versions


def test_versions_are_well_formed():
    for source, version in collect_versions().items():
        assert VERSION_RE.match(version), f"{source}: malformed version {version!r}"


def test_versions_are_in_sync():
    versions = collect_versions()
    distinct = set(versions.values())
    formatted = "\n".join(f"  {source}: {version}" for source, version in sorted(versions.items()))
    assert len(distinct) == 1, f"Version mismatch across manifests:\n{formatted}"


if __name__ == "__main__":
    raise SystemExit(pytest.main([__file__, "-v"]))
