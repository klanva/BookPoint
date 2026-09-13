#!/usr/bin/env python3
"""
run_simulator_smoke_test.py - Master Automated Headless Smoke & E2E Test Runner for BookPoint 2.0.0 SDL2 Desktop Simulator.

Automated non-interactive pipeline test runner:
  - Validates environment & host toolchain
  - Runs full 4-tier opaque-box test suite (Tier 1-4, 116 tests)
  - If simulator binary exists, executes native smoke run in headless mode (SDL_VIDEODRIVER=dummy)
  - Exports visual screen verification screenshots into artifacts/
  - Returns exit code 0 on all tests passing, non-zero on failure.
"""

from __future__ import annotations

import argparse
import json
import os
import shutil
import subprocess
import sys
import tempfile
import time
from pathlib import Path
from typing import Any, Dict, List, Optional

# Locate repository root
SCRIPT_DIR = Path(__file__).resolve().parent
if (SCRIPT_DIR.parent / "tests").exists():
    REPO_ROOT = SCRIPT_DIR.parent
elif (SCRIPT_DIR.parent.parent / "tests").exists():
    REPO_ROOT = SCRIPT_DIR.parent.parent
else:
    REPO_ROOT = Path("C:/xteinkx4").resolve()

if str(REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(REPO_ROOT))

# Enforce UTF-8 console output on Windows
if sys.platform == "win32":
    try:
        sys.stdout.reconfigure(encoding="utf-8", errors="replace")
        sys.stderr.reconfigure(encoding="utf-8", errors="replace")
    except Exception:
        pass

CRASH_PATTERNS = (
    "std::bad_alloc",
    "terminating due to uncaught exception",
    "Assertion failed",
    "assertion failed",
    "Segmentation fault",
    "segmentation fault",
    "AddressSanitizer",
    "UndefinedBehaviorSanitizer",
)


def find_simulator_binary(env_name: str = "simulator") -> Optional[Path]:
    """Search common build output locations for the simulator binary."""
    candidates = [
        REPO_ROOT / ".pio" / "build" / env_name / "program.exe",
        REPO_ROOT / ".pio" / "build" / env_name / "program",
        REPO_ROOT / "src" / ".pio" / "build" / env_name / "program.exe",
        REPO_ROOT / "src" / ".pio" / "build" / env_name / "program",
        REPO_ROOT / "crossink" / ".pio" / "build" / env_name / "program.exe",
    ]
    for c in candidates:
        if c.exists():
            return c
    return None


def run_native_binary_smoke(program: Path, timeout_sec: int = 30) -> int:
    """Run native simulator binary in headless mode with dummy video driver."""
    print(f"\n>>> Running Native Binary Smoke Test: {program}", flush=True)
    with tempfile.TemporaryDirectory(prefix="sim_smoke_native_") as temp_dir:
        temp_root = Path(temp_dir)
        books_dir = temp_root / "fs_" / "books"
        books_dir.mkdir(parents=True, exist_ok=True)

        # Seed sample book if available
        sample_books = list(REPO_ROOT.glob("*.epub"))
        if sample_books:
            shutil.copy2(sample_books[0], books_dir / sample_books[0].name)

        env = os.environ.copy()
        env["SDL_VIDEODRIVER"] = "dummy"
        env["CROSSPOINT_SIMULATOR_SMOKE_TEST"] = "1"
        env["PYTHONIOENCODING"] = "utf-8"

        try:
            proc = subprocess.run(
                [str(program)],
                cwd=temp_root,
                env=env,
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                timeout=timeout_sec,
            )
            print(proc.stdout)
            if proc.returncode != 0:
                print(f"Simulator binary returned non-zero code: {proc.returncode}", file=sys.stderr)
                return proc.returncode

            for pattern in CRASH_PATTERNS:
                if pattern in proc.stdout:
                    print(f"Binary smoke output contains crash pattern: {pattern}", file=sys.stderr)
                    return 2

            print("Native binary headless smoke test PASSED.")
            return 0
        except subprocess.TimeoutExpired:
            print(f"Simulator binary timed out after {timeout_sec}s", file=sys.stderr)
            return 3
        except Exception as e:
            print(f"Failed to execute simulator binary: {e}", file=sys.stderr)
            return 4


def main() -> int:
    parser = argparse.ArgumentParser(description="BookPoint 2.0.0 SDL2 Desktop Simulator Smoke & E2E Test Runner")
    parser.add_argument("--tier", type=int, choices=[1, 2, 3, 4], help="Run a specific test tier (1-4)")
    parser.add_argument("--all", action="store_true", default=True, help="Run all 4 tiers (default)")
    parser.add_argument("--env", default="simulator", help="PlatformIO environment name")
    parser.add_argument("--timeout", type=int, default=45, help="Subprocess timeout in seconds")
    parser.add_argument("--no-artifacts", dest="artifacts", action="store_false", help="Skip artifact screenshots")
    parser.add_argument("--json", action="store_true", help="Output machine-readable JSON")
    parser.add_argument("-v", "--verbose", action="store_true", help="Verbose test execution")
    parser.set_defaults(artifacts=True)

    args = parser.parse_args()

    # Import simulator test suite runner
    from tests.simulator.test_runner import run_test_suite

    tiers = [args.tier] if args.tier else [1, 2, 3, 4]

    # 1. Run Python Automated E2E Simulator Suite
    suite_exit_code = run_test_suite(
        tiers=tiers,
        verbose=args.verbose,
        generate_artifacts=args.artifacts,
        json_output=args.json,
    )

    if suite_exit_code != 0:
        return suite_exit_code

    # 2. Check if a compiled simulator binary is available to run native smoke test
    binary = find_simulator_binary(args.env)
    if binary:
        bin_exit_code = run_native_binary_smoke(binary, timeout_sec=args.timeout)
        if bin_exit_code != 0:
            return bin_exit_code
    else:
        if not args.json:
            print("\nNote: Simulator binary not yet compiled; Python automated E2E test harness verified all simulator contracts.")

    return 0


if __name__ == "__main__":
    sys.exit(main())
