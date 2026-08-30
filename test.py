#!/usr/bin/env python3

import argparse
import json
import os
import pathlib
import stat
import subprocess as sp
import sys
import tempfile

ALERON = "./aleron"
QBE = "qbe"
CC = "clang"


def main() -> None:
    run_make()
    parser = argparser()
    args = parser.parse_args()

    command: str = args.command
    match command:
        case "all":
            handle_all(args)
        case "unit":
            handle_unit(args)
        case "snapshot":
            handle_snapshot(args)

        case _:
            pass


def argparser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="The testing tool for Aleron.")

    subcommands = parser.add_subparsers(
        dest="command", required=True, description="Which kinds of tests to execute"
    )

    snapshot_parent = argparse.ArgumentParser(add_help=False)
    _ = snapshot_parent.add_argument(
        "-sd",
        "--snap-dir",
        metavar=("DIRECTORY"),
        help="Which dir to look for and update snapshots",
        default="tests/integration/_snapshots/",
    )
    _ = snapshot_parent.add_argument(
        "-fd",
        "--file-dir",
        metavar=("DIRECTORY"),
        help="Which dir to look for the files to run for the snapshot",
        default="tests/integration/",
    )

    unit_parent = argparse.ArgumentParser(add_help=False)
    _ = unit_parent.add_argument(
        "-r",
        "--runner",
        metavar=("PATH"),
        help="Path to the 'test_runner' file",
        default="./tests/test_runner",
    )
    _ = unit_parent.add_argument(
        "-s",
        "--suite",
        metavar=("SUITE"),
        help="Which suite to run. Use a regex pattern e.g.: parser.* or *.ast*",
        default="*",
    )

    _parser_all = subcommands.add_parser(
        "all", help="Runs *all* of the tests", parents=[snapshot_parent, unit_parent]
    )

    parser_unit = subcommands.add_parser(
        "unit", help="Runs only the unit tests", parents=[unit_parent]
    )
    _ = parser_unit.add_argument(
        "-l",
        "--list",
        action="store_true",
        help="Wheter to list all the available tests",
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
        metavar=("UPDATE"),
        help="A file to run and update its snapshot",
    )
    _ = snapshot_group.add_argument(
        "-ua",
        "--update-all",
        help="Update all snapshots or only the files without ones already",
        choices=["all", "new"],
    )

    return parser


def handle_all(args: argparse.Namespace):
    args.update = None
    args.update_all = None
    args.suite = None

    handle_unit(args)
    handle_snapshot(args)


def handle_unit(args: argparse.Namespace):
    runner: pathlib.Path = pathlib.Path(args.runner)
    if not runner.is_file():
        print(f"{runner} is not a file. Please provide a valid path.")
        sys.exit(1)
    list: bool = args.list
    suite: str = args.suite

    if list:
        _ = sp.run([runner, "--list-tests"], check=True)
        return
    _ = sp.run([runner, f"--filter={suite}"], check=True)


def handle_snapshot(args: argparse.Namespace):
    snap_dir: pathlib.Path = pathlib.Path(args.snap_dir).resolve()
    file_dir: pathlib.Path = pathlib.Path(args.file_dir).resolve()
    update: pathlib.Path | None = (
        file_dir.joinpath(pathlib.Path(args.update)).resolve() if args.update else None
    )
    update_all: str | None = (
        args.update_all if args.update_all else None
    )  # to shut pyright up

    snap_dir.mkdir(parents=True, exist_ok=True)
    file_dir.mkdir(parents=True, exist_ok=True)

    files_and_snaps: dict[pathlib.Path, pathlib.Path] = {}
    for file in file_dir.rglob("*.ale"):
        if not file.is_file():
            continue
        snap = snap_dir.joinpath(file.name).with_suffix(".json")
        files_and_snaps[file] = snap

    def update_snapshot(file: pathlib.Path, snap: pathlib.Path):
        print(f"Updating snapshot for file: {file.name}")
        info = run_aleron(file)
        with open(snap, "w") as f:
            json.dump(info, f, indent=2)

    if update:
        file = update
        snap = files_and_snaps[file]

        update_snapshot(file, snap)
        print("Done.")
        return
    if update_all:
        all = update_all == "all"
        for file, snap in files_and_snaps.items():
            if not all and snap.exists():
                continue
            update_snapshot(file, snap)
        print("Done.")
        return

    for file, snap in files_and_snaps.items():
        if not snap.exists():
            print(
                f'No snapshot found for: {file.name}. Please run "{sys.argv[0]} snapshot -ua new"'
            )
            continue
        print(f"Running: {file.name}")
        info = run_aleron(file)
        with open(snap, "r") as f:
            snapshot: dict[str, int | str] = json.load(f)
        assert info["code"] == snapshot["code"], f"Exit code mismatch on {file.name}"
        assert info["stdout"] == snapshot["stdout"], f"stdout mismatch on {file.name}"
        assert info["stderr"] == snapshot["stderr"], f"stderr mismatch on {file.name}"

    print("All snapshot tests passed successfully.")


def run_make():
    _ = sp.run(["make", "all"], check=True)


def run_aleron(path_raw: pathlib.Path | str) -> dict[str, int | str]:
    path: str = ""
    if isinstance(path_raw, pathlib.Path):
        path = str(path_raw.resolve())
    else:
        path = path_raw
    assert path.endswith(".ale")

    contents: str = ""
    with open(path, "r") as f:
        contents = f.read()

    process = sp.run([ALERON, contents], capture_output=True, check=True)
    qbe_ssa = process.stdout.decode()
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
        process = sp.run(
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
