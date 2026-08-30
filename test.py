#!/usr/bin/env python3

import argparse
import json
import os
import pathlib
import stat
import subprocess as sp
import sys
import tempfile
from enum import Enum

ALERON = "./aleron"
QBE = "qbe"
CC = "clang"


class Stage(str, Enum):
    AST = "ast"
    IR = "ir"
    BIN = "bin"


def main() -> None:
    run_make()
    parser = argparser()
    args = parser.parse_args()

    match args.command:
        case "all":
            handle_all(args)
        case "unit":
            handle_unit(args)
        case "snapshot":
            handle_snapshot(args)


def argparser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="The testing tool for Aleron.")

    subcommands = parser.add_subparsers(
        dest="command", required=True, description="Which kinds of tests to execute"
    )

    snapshot_parent = argparse.ArgumentParser(add_help=False)
    _ = snapshot_parent.add_argument(
        "-sd",
        "--snap-dir",
        type=pathlib.Path,
        default=pathlib.Path("tests/integration/_snapshots/"),
        help="Which dir to look for and update snapshots",
    )
    _ = snapshot_parent.add_argument(
        "-fd",
        "--file-dir",
        type=pathlib.Path,
        default=pathlib.Path("tests/integration/"),
        help="Which dir to look for the files to run for the snapshot",
    )
    _ = snapshot_parent.add_argument(
        "-st",
        "--stage",
        choices=("all", "ast", "ir", "bin"),
        default="all",
        help="Which stage of the compiler to test",
    )

    unit_parent = argparse.ArgumentParser(add_help=False)
    _ = unit_parent.add_argument(
        "-r",
        "--runner",
        type=pathlib.Path,
        default=pathlib.Path("./tests/test_runner"),
        help="Path to the 'test_runner' file",
    )
    _ = unit_parent.add_argument(
        "-s",
        "--suite",
        default="*",
        help="Which suite to run. Use a regex pattern e.g.: parser.* or *.ast*",
    )

    _ = subcommands.add_parser(
        "all", help="Runs *all* of the tests", parents=[snapshot_parent, unit_parent]
    )

    parser_unit = subcommands.add_parser(
        "unit", help="Runs only the unit tests", parents=[unit_parent]
    )
    _ = parser_unit.add_argument(
        "-l",
        "--list",
        action="store_true",
        help="Whether to list all the available tests",
    )

    parser_snapshot = subcommands.add_parser(
        "snapshot",
        help="Runs the snapshot tests to guarantee no regression",
        parents=[snapshot_parent],
    )
    snapshot_group = parser_snapshot.add_mutually_exclusive_group()
    _ = snapshot_group.add_argument(
        "-u",
        "--update",
        type=pathlib.Path,
        help="A file to run and update its snapshot",
    )
    _ = snapshot_group.add_argument(
        "-ua",
        "--update-all",
        choices=["all", "new"],
        help="Update all snapshots or only missing ones",
    )

    return parser


def handle_all(args: argparse.Namespace) -> None:
    args.update = None
    args.update_all = None
    args.list = False

    handle_unit(args)
    handle_snapshot(args)


def handle_unit(args: argparse.Namespace) -> None:
    runner: pathlib.Path = args.runner
    if not runner.is_file():
        print(f"{runner} is not a file. Please provide a valid path.")
        sys.exit(1)

    if args.list:
        _ = sp.run([runner, "--list-tests"], check=True)
        return
    _ = sp.run([runner, f"--filter={args.suite}"], check=True)


def handle_snapshot(args: argparse.Namespace) -> None:
    if args.stage == "all":
        for s in Stage:
            args.stage = s.value
            handle_snapshot(args)
        return

    stage = Stage(args.stage)
    snap_dir: pathlib.Path = args.snap_dir.resolve()
    file_dir: pathlib.Path = args.file_dir.resolve()

    update: pathlib.Path | None = (
        (file_dir / args.update).resolve() if args.update else None
    )
    update_all: str | None = args.update_all

    snap_dir.mkdir(parents=True, exist_ok=True)
    file_dir.mkdir(parents=True, exist_ok=True)

    files_and_snaps: dict[pathlib.Path, pathlib.Path] = {
        f.resolve(): snap_dir / stage.value / f.with_suffix(".json").name
        for f in file_dir.rglob("*.ale")
        if f.is_file()
    }

    def update_snapshot(file: pathlib.Path, snap: pathlib.Path) -> None:
        print(f"[{stage.value.upper()}] Updating snapshot for file: {file.name}")
        info = run_aleron(file, stage)
        snap.parent.mkdir(parents=True, exist_ok=True)
        with open(snap, "w") as f:
            json.dump(info, f, indent=2)

    if update:
        snap = files_and_snaps.get(update)
        if not snap:
            print(f"Error: {update} not found in {file_dir}")
            sys.exit(1)

        update_snapshot(update, snap)
        print(f"[{stage.value.upper()}] Done.")
        return

    if update_all:
        is_all = update_all == "all"
        for file, snap in files_and_snaps.items():
            if not is_all and snap.exists():
                continue
            update_snapshot(file, snap)
        print(f"[{stage.value.upper()}] Done.")
        return

    for file, snap in files_and_snaps.items():
        if not snap.exists():
            print(
                f"[{stage.value.upper()}] Missing snapshot for {file.name}. Generate via: -ua new"
            )
            continue
        print(f"[{stage.value.upper()}] Testing: {file.name}")
        info = run_aleron(file, stage)
        with open(snap, "r") as f:
            snapshot: dict[str, int | str] = json.load(f)
        assert info["code"] == snapshot["code"], f"Exit code mismatch on {file.name}"
        assert info["stdout"] == snapshot["stdout"], f"stdout mismatch on {file.name}"
        assert info["stderr"] == snapshot["stderr"], f"stderr mismatch on {file.name}"

    print(f"[{stage.value.upper()}] All snapshot tests passed successfully.")


def run_make() -> None:
    _ = sp.run(["make", "all"], check=True)


def run_aleron(path: pathlib.Path, stage: Stage) -> dict[str, int | str]:
    path_str = str(path.resolve())
    assert path_str.endswith(".ale")

    with open(path_str, "r") as f:
        contents = f.read()

    if stage in (Stage.AST, Stage.IR):
        process = sp.run(
            [ALERON, f"-e={stage.value}", contents],
            capture_output=True,
            check=False,
        )
        return {
            "code": process.returncode,
            "stdout": process.stdout.decode(),
            "stderr": process.stderr.decode(),
        }

    process = sp.run([ALERON, contents], capture_output=True, check=True)
    qbe_ssa = process.stdout.decode()

    with tempfile.NamedTemporaryFile("w+") as f:
        _ = f.write(qbe_ssa)
        f.flush()
        process = sp.run(
            [QBE, f.name],
            capture_output=True,
            check=True,
        )
        qbe_asm = process.stdout.decode()

    temp_path: str = ""
    with (
        tempfile.NamedTemporaryFile("w+") as inp,
        tempfile.NamedTemporaryFile(delete=False) as out,
    ):
        _ = inp.write(qbe_asm)
        inp.flush()
        _ = sp.run(
            [CC, "-x", "assembler", "-o", out.name, inp.name],
            capture_output=False,
            check=True,
        )
        st = os.stat(out.name)
        os.chmod(out.name, st.st_mode | stat.S_IEXEC)
        temp_path = out.name

    try:
        process = sp.run(
            [temp_path],
            check=False,
            capture_output=True,
        )
    finally:
        if os.path.exists(temp_path):
            os.remove(temp_path)

    return {
        "code": process.returncode,
        "stdout": process.stdout.decode(),
        "stderr": process.stderr.decode(),
    }


if __name__ == "__main__":
    main()
