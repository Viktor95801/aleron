#!/usr/bin/env python3

# Negative testing was vibecoded cuz i dont have a lot of free time

import argparse
import difflib
import functools
import json
import os
import shlex
import stat
import subprocess as sp
import sys
import tempfile
from ast import literal_eval
from enum import Enum
from pathlib import Path
from posix import mkdir
from time import sleep
from typing import Any, Optional

ALERON: str = "./aleron"
QBE: str = "qbe"
CC: str = "clang"


class Stage(str, Enum):
    LEX = "lex"
    AST = "ast"
    IR = "ir"
    BIN = "bin"


@functools.cache
def stage_is_failable(stage: Stage) -> bool:
    return bool(stage == Stage.LEX or stage == Stage.AST)


def main() -> None:
    run_make()
    parser: argparse.ArgumentParser = argparser()
    args: argparse.Namespace = parser.parse_args()

    match args.command:
        case "all":
            handle_all(args)
        case "unit":
            handle_unit(args)
        case "snapshot":
            handle_snapshot(args)


def argparser() -> argparse.ArgumentParser:
    parser: argparse.ArgumentParser = argparse.ArgumentParser(
        description="The testing tool for Aleron."
    )

    subcommands: argparse._SubParsersAction[argparse.ArgumentParser] = (
        parser.add_subparsers(
            dest="command",
            required=True,
            description="Which kinds of tests to execute",
        )
    )

    snapshot_parent: argparse.ArgumentParser = argparse.ArgumentParser(add_help=False)
    _ = snapshot_parent.add_argument(
        "-sd",
        "--snap-dir",
        type=Path,
        default=Path("tests/integration/_snapshots/"),
        help="Which dir to look for and update snapshots",
    )
    _ = snapshot_parent.add_argument(
        "-fd",
        "--file-dir",
        type=Path,
        default=Path("tests/integration/"),
        help="Which dir to look for the files to run for the snapshot",
    )
    _ = snapshot_parent.add_argument(
        "-st",
        "--stage",
        choices=("all", "lex", "ast", "ir", "bin"),
        default="all",
        help="Which stage of the compiler to test",
    )

    unit_parent: argparse.ArgumentParser = argparse.ArgumentParser(add_help=False)
    _ = unit_parent.add_argument(
        "-r",
        "--runner",
        type=Path,
        default=Path("./tests/test_runner"),
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

    parser_unit: argparse.ArgumentParser = subcommands.add_parser(
        "unit", help="Runs only the unit tests", parents=[unit_parent]
    )
    _ = parser_unit.add_argument(
        "-l",
        "--list",
        action="store_true",
        help="Whether to list all the available tests",
    )

    parser_snapshot: argparse.ArgumentParser = subcommands.add_parser(
        "snapshot",
        help="Runs the snapshot tests to guarantee no regression",
        parents=[snapshot_parent],
    )
    _ = parser_snapshot.add_argument(
        "-f",
        "--from-file",
        type=Path,
        metavar=("FILE[.txt]"),
        help='A file to generate snapshots from with the format (assumes correct syntax): <stage> <binding path from file dir> <exit code:int> ["stdout"] ["stderr"];',
    )

    snapshot_group: argparse._MutuallyExclusiveGroup = (
        parser_snapshot.add_mutually_exclusive_group()
    )
    _ = snapshot_group.add_argument(
        "-u",
        "--update",
        type=Path,
        help="A file to run and update its snapshot",
    )
    _ = snapshot_group.add_argument(
        "-ub",
        "--update-by",
        help='Recursively update files in the file dir by a "regex" string, e.g.: basics/*.ale',
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
    args.update_by = None
    args.from_file = None
    args.list = False

    handle_unit(args)
    handle_snapshot(args)


def handle_unit(args: argparse.Namespace) -> None:
    runner: Path = args.runner
    if not runner.is_file():
        print(f"{runner} is not a file. Please provide a valid path.")
        sys.exit(1)

    if args.list:
        _ = sp.run([str(runner), "--list-tests"], check=True)
        return
    _ = sp.run([str(runner), f"--filter={args.suite}"], check=True)


@functools.lru_cache(maxsize=256)
def should_fail_compilation(file: Path) -> bool:
    """Returns True if test is marked to fail via filename or parent directory name."""
    if ".fail." in file.name or file.name.startswith("fail_"):
        return True

    part: str
    for part in file.parts[:-1]:
        if part.startswith("fail_") or part == "fail":
            return True

    return False


def execute_test(file: Path, stage: Stage) -> dict[str, Any] | None:
    """Executes Aleron safely, handling expected and unexpected compilation passes/failures."""
    expects_failure: bool = should_fail_compilation(file)

    if expects_failure and not stage_is_failable(stage):
        return None

    try:
        info: dict[str, Any] = run_aleron(file, stage)
    except (sp.CalledProcessError, RuntimeError) as e:
        if not expects_failure:
            print(
                f"[{stage.value.upper()}] WARNING: Expected successful compilation, but failed on {file.name}"
            )
            print(f"Details: {e}")

        raw_stdout: Any = getattr(e, "stdout", b"")
        raw_stderr: Any = getattr(e, "stderr", str(e))

        stdout_str: str = (
            raw_stdout.decode()
            if isinstance(raw_stdout, bytes)
            else str(raw_stdout or "")
        )
        stderr_str: str = (
            raw_stderr.decode()
            if isinstance(raw_stderr, bytes)
            else str(raw_stderr or "")
        )
        code_val: int = int(getattr(e, "returncode", 1))

        return {
            "code": code_val,
            "stdout": stdout_str,
            "stderr": stderr_str,
        }

    if expects_failure and info["code"] == 0:
        print(
            f"[{stage.value.upper()}] WARNING: Expected compilation failure, but passed on {file.name}"
        )

    return info


def update_snapshot(file: Path, snap: Path, stage: Stage) -> None:
    info: dict[str, Any] | None = execute_test(file, stage)
    if info is None:
        return

    print(f"[{stage.value.upper()}] Updating snapshot for file: {file.name}")
    snap.parent.mkdir(parents=True, exist_ok=True)
    with open(snap, "w") as f:
        json.dump(info, f, indent=2)


def test_single_snapshot(file: Path, snap: Path, stage: Stage) -> bool:
    if not snap.exists():
        if should_fail_compilation(file) and not stage_is_failable(stage):
            return False
        print(
            f"[{stage.value.upper()}] Missing snapshot for {file.name}. Generate via: -ua new"
        )
        return False

    print(f"[{stage.value.upper()}] Testing: {file.name}")
    info: dict[str, Any] | None = execute_test(file, stage)
    if info is None:
        return False

    snapshot: dict[str, Any] = {}
    with open(snap, "r") as f:
        snapshot = json.load(f)

    had_error: bool = False
    if info["code"] != snapshot["code"]:
        print(f"Exit code mismatch on {file.name}")
        print(f"Expected {snapshot['code']}, got {info['code']}")
        had_error = True

    if info["stdout"] != snapshot["stdout"]:
        print(f"stdout mismatch on {file.name}")
        diff = difflib.unified_diff(
            str(snapshot["stdout"]).splitlines(),
            str(info["stdout"]).splitlines(),
            "expected",
            "got",
            lineterm="",
        )
        line: str
        for line in diff:
            print(line)
        had_error = True

    if info["stderr"] != snapshot["stderr"]:
        print(f"stderr mismatch on {file.name}")
        diff = difflib.unified_diff(
            str(snapshot["stderr"]).splitlines(),
            str(info["stderr"]).splitlines(),
            "expected",
            "got",
            lineterm="",
        )
        line: str
        for line in diff:
            print(line)
        had_error = True

    if had_error:
        print("----------- ERROR on test -----------")
        sleep(1)

    return had_error


def snapshot_from(from_file: Path, file_dir: Path, snap_dir: Path):
    lexer = shlex.shlex(from_file.read_text(), str(from_file))

    def tok_to_str(tok: str | None) -> str:
        return "<eof>" if tok == lexer.eof else str(tok)

    def write_snap(stage: Stage, path: Path, code: int, stdout: str, stderr: str):
        snap = snap_dir / stage.value / path.relative_to(file_dir).with_suffix(".json")
        snap.parent.mkdir(parents=True, exist_ok=True)
        with open(snap, "w") as f:
            json.dump(
                {
                    "code": code,
                    "stdout": stdout,
                    "stderr": stderr,
                },
                f,
            )

    # goto would make this a lot easier
    # goto is life
    while True:
        tok = lexer.get_token()
        if tok == lexer.eof:
            break
        if tok not in Stage:
            print(
                f"ERROR: expected 'stage' name (choose from lex, ast, ir or bin), got: {tok_to_str(tok)}"
            )
            sys.exit(1)
        from_stage = Stage(tok)

        tok = lexer.get_token()
        if tok == lexer.eof or tok == ";":
            print(f"ERROR: expected a binding path, got: {tok_to_str(tok)}")
            sys.exit(1)

        from_path: Path = file_dir / Path(tok.strip("\"'"))
        if not from_path.exists():
            print(f"ERROR: {from_path} not found in {file_dir}")
            sys.exit(1)

        tok = lexer.get_token()
        if tok == lexer.eof or tok == ";":
            print(f"ERROR: expected an exit code, got: {tok_to_str(tok)}")
            sys.exit(1)
        try:
            from_code: int = int(tok)
        except ValueError as e:
            print(f"ERROR: expected a valid integer: {e}")
            sys.exit(1)

        tok = lexer.get_token()
        if tok == lexer.eof:
            print(f"ERROR: expected a ';' or the stdout, got: {tok_to_str(tok)}")
            sys.exit(1)
        if tok == ";":
            write_snap(from_stage, from_path, from_code, "", "")
            continue
        from_stdout = str(tok)

        tok = lexer.get_token()
        if tok == lexer.eof:
            print(f"ERROR: expected a ';' or the stderr, got: {tok_to_str(tok)}")
            sys.exit(1)
        if tok == ";":
            write_snap(from_stage, from_path, from_code, from_stdout, "")
        from_stderr = str(tok)

        tok = lexer.get_token()
        if tok != ";":
            print(f"ERROR: expected a ';', got: {tok_to_str(tok)}")
            sys.exit(1)

        write_snap(from_stage, from_path, from_code, from_stdout, from_stderr)

    print(f"Done parsing: {from_file.name}")


def handle_snapshot(args: argparse.Namespace) -> None:
    snap_dir: Path = args.snap_dir.resolve()
    file_dir: Path = args.file_dir.resolve()
    from_file: Path | None = args.from_file
    if from_file:
        if not from_file.exists():
            print(f"ERROR: {from_file.name} not found")
            sys.exit(1)

        snapshot_from(from_file, file_dir, snap_dir)
        return

    if args.stage == "all":
        s: Stage
        for s in Stage:
            args.stage = s.value
            handle_snapshot(args)
        return

    stage: Stage = Stage(args.stage)

    update: Path | None = (file_dir / args.update).resolve() if args.update else None
    update_by: str | None = args.update_by
    update_all: str | None = args.update_all

    snap_dir.mkdir(parents=True, exist_ok=True)
    file_dir.mkdir(parents=True, exist_ok=True)

    files_and_snaps: dict[Path, Path] = {
        f.resolve(): snap_dir
        / stage.value
        / f.relative_to(file_dir).with_suffix(".json")
        for f in file_dir.rglob("*.ale")
        if f.is_file()
    }

    if update:
        snap: Path | None = files_and_snaps.get(update)
        if not snap:
            print(f"[{stage.value.upper()}] ERROR: {update} not found in {file_dir}")
            sys.exit(1)
        update_snapshot(update, snap, stage)
        print(f"[{stage.value.upper()}] Done.")
        return

    if update_by:
        file_item: Path
        for file_item in file_dir.rglob(update_by):
            if file_item.is_relative_to(snap_dir):
                continue
            snap_item: Path | None = files_and_snaps.get(file_item)
            if not snap_item:
                start: str = f"[{stage.value.upper()}]"
                print(f"{start} File not found: {file_item.name}, skipping.")
                print(
                    f"{' ' * len(start)} Try adding '.ale' to pattern, e.g.: {update_by + '.ale'}"
                )
                continue
            update_snapshot(file_item, snap_item, stage)
        print(f"[{stage.value.upper()}] Done.")
        return

    if update_all:
        is_all: bool = update_all == "all"
        target_file: Path
        target_snap: Path
        for target_file, target_snap in files_and_snaps.items():
            if not is_all and target_snap.exists():
                continue
            update_snapshot(target_file, target_snap, stage)
        print(f"[{stage.value.upper()}] Done.")
        return

    error_on: list[str] = []
    test_file: Path
    test_snap: Path
    for test_file, test_snap in files_and_snaps.items():
        if test_single_snapshot(test_file, test_snap, stage):
            error_on.append(test_file.name)

    if not error_on:
        print(f"[{stage.value.upper()}] All snapshot tests passed successfully.")
        return

    stage_str: str = stage.value.upper()
    print(f"[{stage_str}] ERROR: Had error on files:")
    err_file: str
    for err_file in error_on:
        print(" " * len(stage_str) + f" - {err_file}")


def run_make() -> None:
    _ = sp.run(["make", "all"], check=True)


def run_aleron(path: Path, stage: Stage) -> dict[str, Any]:
    path_str: str = str(path.resolve())
    assert path_str.endswith(".ale")

    contents: str = ""
    with open(path_str, "r") as f:
        contents = f.read()

    if stage in (Stage.LEX, Stage.AST, Stage.IR):
        process: sp.CompletedProcess[bytes] = sp.run(
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
    qbe_ssa: str = process.stdout.decode()

    qbe_asm: str = ""
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
        st: os.stat_result = os.stat(out.name)
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
