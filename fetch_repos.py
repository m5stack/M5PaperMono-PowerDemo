#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
#
# SPDX-License-Identifier: MIT

"""Fetch component repositories at the revisions locked in repos.json."""

from __future__ import annotations

import argparse
import json
import os
import shutil
import stat
import subprocess
import sys
import tempfile
from dataclasses import dataclass
from pathlib import Path
from typing import Any


SCRIPT_DIR = Path(__file__).resolve().parent
DEFAULT_CONFIG = SCRIPT_DIR / "repos.json"


class FetchError(RuntimeError):
    """Raised when a component cannot be validated or installed."""


@dataclass(frozen=True)
class Repository:
    name: str
    path: Path
    source: str
    url: str | None
    ref: str | None
    commit: str | None
    component: str | None
    version: str | None
    component_hash: str | None
    source_subdir: Path | None
    patches: tuple[Path, ...]
    remove: tuple[Path, ...]


def run(command: list[str], *, cwd: Path | None = None, capture: bool = False) -> str:
    result = subprocess.run(
        command,
        cwd=cwd,
        check=False,
        text=True,
        stdout=subprocess.PIPE if capture else None,
        stderr=subprocess.PIPE if capture else None,
    )
    if result.returncode != 0:
        details = (result.stderr or result.stdout or "").strip()
        suffix = f"\n{details}" if details else ""
        raise FetchError(f"Command failed ({result.returncode}): {' '.join(command)}{suffix}")
    return (result.stdout or "").strip()


def command_succeeds(command: list[str], *, cwd: Path) -> bool:
    result = subprocess.run(command, cwd=cwd, check=False, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    return result.returncode == 0


def remove_tree(path: Path) -> None:
    def make_writable_and_retry(function: Any, target: str, _: Any) -> None:
        os.chmod(target, stat.S_IWRITE)
        function(target)

    shutil.rmtree(path, onerror=make_writable_and_retry)


def confined_path(root: Path, relative: Path, field: str) -> Path:
    if relative.is_absolute() or ".." in relative.parts:
        raise FetchError(f"Unsafe {field}: {relative}")
    path = (root / relative).resolve()
    try:
        path.relative_to(root.resolve())
    except ValueError as error:
        raise FetchError(f"{field} escapes its root: {relative}") from error
    return path


def load_repositories(config_path: Path) -> list[Repository]:
    try:
        raw = json.loads(config_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        raise FetchError(f"Cannot read {config_path}: {error}") from error
    if not isinstance(raw, list) or not raw:
        raise FetchError("Repository config must be a non-empty JSON array")

    repositories: list[Repository] = []
    names: set[str] = set()
    paths: set[Path] = set()
    for index, item in enumerate(raw):
        if not isinstance(item, dict):
            raise FetchError(f"Repository entry {index} must be an object")
        try:
            name = str(item["name"])
            path = Path(str(item["path"]))
        except KeyError as error:
            raise FetchError(f"Repository entry {index} is missing {error.args[0]}") from error
        source = str(item.get("source", "git"))
        if source not in ("git", "registry"):
            raise FetchError(f"Unsupported source for {name}: {source}")
        url = str(item["url"]) if "url" in item else None
        ref = str(item["ref"]) if "ref" in item else None
        commit = str(item["commit"]).lower() if "commit" in item else None
        component = str(item["component"]) if "component" in item else None
        version = str(item["version"]) if "version" in item else None
        component_hash = str(item["component_hash"]).lower() if "component_hash" in item else None
        if name in names or path in paths:
            raise FetchError(f"Duplicate component name or path: {name}")
        if source == "git":
            if not url or not ref or not commit:
                raise FetchError(f"Git source for {name} requires url, ref, and commit")
            if len(commit) != 40 or any(char not in "0123456789abcdef" for char in commit):
                raise FetchError(f"Commit for {name} must be a full 40-character SHA")
        else:
            if not component or not version or not component_hash:
                raise FetchError(
                    f"Registry source for {name} requires component, version, and component_hash"
                )
            if len(component_hash) != 64 or any(
                char not in "0123456789abcdef" for char in component_hash
            ):
                raise FetchError(f"Component hash for {name} must be a full SHA-256 value")

        confined_path(SCRIPT_DIR, path, f"destination for {name}")
        source_subdir_value = item.get("source_subdir")
        source_subdir = Path(str(source_subdir_value)) if source_subdir_value else None
        if source_subdir is not None:
            confined_path(SCRIPT_DIR, source_subdir, f"source_subdir for {name}")
        patches = tuple(
            confined_path(SCRIPT_DIR, Path(str(value)), f"patch for {name}")
            for value in item.get("patches", [])
        )
        remove = tuple(Path(str(value)) for value in item.get("remove", []))
        for relative in remove:
            confined_path(SCRIPT_DIR, relative, f"remove path for {name}")

        names.add(name)
        paths.add(path)
        repositories.append(
            Repository(
                name,
                path,
                source,
                url,
                ref,
                commit,
                component,
                version,
                component_hash,
                source_subdir,
                patches,
                remove,
            )
        )
    return repositories


def select_repositories(repositories: list[Repository], selected: list[str]) -> list[Repository]:
    if not selected:
        return repositories
    by_name = {repository.name: repository for repository in repositories}
    unknown = sorted(set(selected) - by_name.keys())
    if unknown:
        raise FetchError(f"Unknown repositories: {', '.join(unknown)}")
    selected_names = set(selected)
    return [repository for repository in repositories if repository.name in selected_names]


def validate_assets(repositories: list[Repository]) -> None:
    for repository in repositories:
        for patch in repository.patches:
            if not patch.is_file():
                raise FetchError(f"Patch does not exist for {repository.name}: {patch}")


def repository_url(repository: Repository, transport: str) -> str:
    if repository.url is None:
        raise FetchError(f"Git URL is missing for {repository.name}")
    if transport == "https":
        return repository.url
    prefix = "https://github.com/"
    if not repository.url.startswith(prefix):
        raise FetchError(f"SSH transport is only supported for GitHub URLs: {repository.url}")
    return f"git@github.com:{repository.url.removeprefix(prefix)}"


def prepare_repository(repository: Repository, staging_root: Path, transport: str) -> Path:
    if repository.source == "registry":
        return prepare_registry_component(repository, staging_root)

    assert repository.ref is not None
    assert repository.commit is not None
    checkout = staging_root / repository.name
    print(f"Fetching {repository.name} ({repository.ref}, {repository.commit[:12]})", flush=True)
    checkout.mkdir()
    run(["git", "init"], cwd=checkout)
    run(["git", "remote", "add", "origin", repository_url(repository, transport)], cwd=checkout)
    run(["git", "fetch", "--depth", "1", "origin", repository.ref], cwd=checkout)
    fetched_head = run(["git", "rev-parse", "FETCH_HEAD"], cwd=checkout, capture=True).lower()
    if fetched_head == repository.commit:
        run(["git", "checkout", "--detach", "FETCH_HEAD"], cwd=checkout)
    else:
        print("  Locked commit is older than the current ref; fetching full ref history", flush=True)
        run(["git", "fetch", "--unshallow", "origin", repository.ref], cwd=checkout)
        run(["git", "checkout", "--detach", repository.commit], cwd=checkout)
    head = run(["git", "rev-parse", "HEAD"], cwd=checkout, capture=True).lower()
    if head != repository.commit:
        raise FetchError(f"Revision mismatch for {repository.name}: expected {repository.commit}, got {head}")
    for patch in repository.patches:
        print(f"  Applying {patch.relative_to(SCRIPT_DIR)}")
        run(["git", "apply", "--check", str(patch)], cwd=checkout)
        run(["git", "apply", str(patch)], cwd=checkout)
    for relative in repository.remove:
        target = confined_path(checkout, relative, f"remove path for {repository.name}")
        if target.is_dir():
            remove_tree(target)
        elif target.exists():
            target.unlink()

    source = checkout
    if repository.source_subdir is not None:
        source = confined_path(checkout, repository.source_subdir, f"source_subdir for {repository.name}")
        if not source.is_dir():
            raise FetchError(f"Source subdirectory does not exist for {repository.name}: {repository.source_subdir}")
    return source


def prepare_registry_component(repository: Repository, staging_root: Path) -> Path:
    try:
        from idf_component_tools.manifest import SolvedComponent
        from idf_component_tools.sources.web_service import WebServiceSource
        from idf_component_tools.utils import ComponentVersion
    except ImportError as error:
        raise FetchError(
            "Registry components require the ESP-IDF Python environment. "
            "Run fetch_repos.py from an ESP-IDF shell."
        ) from error

    assert repository.component is not None
    assert repository.version is not None
    assert repository.component_hash is not None
    checkout = staging_root / repository.name
    print(
        f"Fetching {repository.name} "
        f"({repository.component}@{repository.version}, {repository.component_hash[:12]})",
        flush=True,
    )
    source = WebServiceSource()
    component = SolvedComponent(
        name=repository.component,
        version=ComponentVersion(repository.version),
        component_hash=repository.component_hash,
        source=source,
        dependencies=[],
    )
    try:
        source.download(component, str(checkout))
    except Exception as error:
        raise FetchError(f"Cannot fetch registry component {repository.name}: {error}") from error
    (checkout / ".component_hash").write_text(repository.component_hash, encoding="utf-8")
    return checkout


def install_repository(repository: Repository, source: Path, output_root: Path, replace: bool) -> None:
    destination = confined_path(output_root, repository.path, f"destination for {repository.name}")
    if destination.exists() and not replace:
        raise FetchError(f"Destination exists for {repository.name}: {destination}. Use --replace to replace it.")
    backup = output_root / "_component_backups" / repository.path
    if destination.exists():
        if backup.exists():
            remove_tree(backup)
        backup.parent.mkdir(parents=True, exist_ok=True)
        shutil.copytree(destination, backup)
        remove_tree(destination)
    try:
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copytree(source, destination)
        metadata = {
            "name": repository.name,
            "source": repository.source,
            "url": repository.url,
            "ref": repository.ref,
            "commit": repository.commit,
            "component": repository.component,
            "version": repository.version,
            "component_hash": repository.component_hash,
            "source_subdir": repository.source_subdir.as_posix() if repository.source_subdir else None,
        }
        metadata_path = destination / ".powerdemo-source.json"
        metadata_path.write_text(json.dumps(metadata, indent=2) + "\n", encoding="utf-8")
    except Exception:
        if destination.exists():
            remove_tree(destination)
        if backup.exists():
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copytree(backup, destination)
        raise


def check_repository(repository: Repository, output_root: Path) -> bool:
    destination = confined_path(output_root, repository.path, f"destination for {repository.name}")
    if repository.source == "registry":
        hash_path = destination / ".component_hash"
        if not destination.is_dir() or not hash_path.is_file():
            print(f"MISSING   {repository.name:<32} {repository.path}")
            return False
        if hash_path.read_text(encoding="utf-8").strip() != repository.component_hash:
            print(f"MISMATCH  {repository.name:<32} component hash")
            return False
        print(f"MATCH     {repository.name:<32} {repository.component_hash[:12]}")
        return True

    metadata_path = destination / ".powerdemo-source.json"
    if not destination.is_dir() or not metadata_path.is_file():
        print(f"MISSING   {repository.name:<24} {repository.path}")
        return False
    try:
        metadata = json.loads(metadata_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError):
        print(f"INVALID   {repository.name:<24} source metadata")
        return False
    if metadata.get("commit") != repository.commit or metadata.get("url") != repository.url:
        print(f"MISMATCH  {repository.name:<24} source metadata")
        return False
    git_marker = destination / ".git"
    if git_marker.exists():
        head = run(["git", "rev-parse", "HEAD"], cwd=destination, capture=True).lower()
        if head != repository.commit:
            print(f"MISMATCH  {repository.name:<24} {head[:12]}")
            return False
        if not all(
            command_succeeds(["git", "apply", "--reverse", "--check", str(patch)], cwd=destination)
            for patch in repository.patches
        ):
            print(f"MISMATCH  {repository.name:<24} required patches")
            return False
    assert repository.commit is not None
    print(f"MATCH     {repository.name:<24} {repository.commit[:12]}")
    return True


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", nargs="?", choices=("fetch", "check"), default="fetch")
    parser.add_argument("--config", type=Path, default=DEFAULT_CONFIG)
    parser.add_argument("--repo", action="append", default=[], help="Process only this component (repeatable)")
    parser.add_argument("--output-root", type=Path, default=SCRIPT_DIR)
    parser.add_argument("--replace", action="store_true", help="Back up and replace existing component directories")
    parser.add_argument(
        "--skip-existing",
        action="store_true",
        help="Skip component destinations that already exist instead of failing",
    )
    parser.add_argument("--transport", choices=("https", "ssh"), default="https")
    return parser.parse_args()


def main() -> int:
    arguments = parse_arguments()
    try:
        repositories = select_repositories(load_repositories(arguments.config.resolve()), arguments.repo)
        validate_assets(repositories)
        output_root = arguments.output_root.resolve()
        if arguments.command == "check":
            return 0 if all(check_repository(repository, output_root) for repository in repositories) else 1

        existing = [repository.name for repository in repositories if (output_root / repository.path).exists()]
        if arguments.skip_existing and existing:
            existing_names = set(existing)
            repositories = [repository for repository in repositories if repository.name not in existing_names]
            print(f"Skipping existing components: {', '.join(existing)}", flush=True)
            if not repositories:
                print("All selected components already exist.")
                return 0
            existing = []
        if existing and not arguments.replace:
            names = ", ".join(existing)
            raise FetchError(f"Destinations already exist: {names}. Use --replace to replace them.")
        output_root.mkdir(parents=True, exist_ok=True)
        staging_root = Path(tempfile.mkdtemp(prefix=".fetch_repos-", dir=output_root))
        try:
            prepared = {
                repository.name: prepare_repository(repository, staging_root, arguments.transport)
                for repository in repositories
            }
            for repository in repositories:
                install_repository(repository, prepared[repository.name], output_root, arguments.replace)
        finally:
            if staging_root.exists():
                remove_tree(staging_root)
        print("All selected repositories were installed successfully.")
        return 0
    except (FetchError, OSError, ValueError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
