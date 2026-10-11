from __future__ import annotations

import re
import sys

if sys.version_info >= (3, 11):
    import tomllib
else:
    import tomli as tomllib

from nu_content.paths import category_folder, config_dir

_ID_RE = re.compile(r"^[a-z][a-z0-9_]*$")


def load_naming() -> dict:
    path = config_dir() / "naming.toml"
    with path.open("rb") as f:
        return tomllib.load(f)


def expected_prefix(category: str) -> str:
    naming = load_naming()
    prefixes = naming.get("prefixes", {})
    key = category_folder(category)
    # environment pieces often start with env_<arena>_
    return str(prefixes.get(key, ""))


def validate_id(asset_id: str, category: str) -> list[str]:
    errors: list[str] = []
    if not _ID_RE.match(asset_id):
        errors.append("id must be lowercase snake_case starting with a letter")
    prefix = expected_prefix(category)
    if prefix and not asset_id.startswith(prefix):
        # environment: allow env_arena_xxx
        if category_folder(category) == "environment":
            if not asset_id.startswith("env_"):
                errors.append(f"id should start with {prefix!r}")
        else:
            errors.append(f"id should start with {prefix!r}")
    return errors
