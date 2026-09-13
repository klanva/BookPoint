#!/usr/bin/env python3
"""
run_e2e_tests.py - Master E2E Test Runner for BookPoint 2.0.0 Visual Redesign.

Supports:
  --tier 1      : Run Tier 1 Feature Coverage Tests (≥50 tests)
  --tier 2      : Run Tier 2 Boundary & Corner Case Tests (≥50 tests)
  --tier 3      : Run Tier 3 Cross-Feature Combination Tests (≥10 tests)
  --tier 4      : Run Tier 4 Real-World Application Scenarios (≥6 tests)
  --all         : Run all 4 Tiers (175 tests total)
  --json        : Output structured JSON results

Exit Codes:
  0 : All tests passed cleanly
  1 : One or more tests failed or encountered errors
  2 : CLI argument parsing error
"""

from __future__ import annotations

import argparse
import json
import os
import sys
import time
import unittest
from pathlib import Path
from typing import Any, Dict, List, Optional

# Ensure repository root is on sys.path
REPO_ROOT = Path(__file__).resolve().parent.parent.parent
if str(REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(REPO_ROOT))

# Ensure stdout handles UTF-8 on Windows consoles
if sys.platform == "win32":
    try:
        sys.stdout.reconfigure(encoding="utf-8", errors="replace")
        sys.stderr.reconfigure(encoding="utf-8", errors="replace")
    except Exception:
        pass

from tests.e2e.tier1_feature_tests import Tier1FeatureTests
from tests.e2e.tier2_boundary_tests import Tier2BoundaryTests
from tests.e2e.tier3_combination_tests import Tier3CombinationTests
from tests.e2e.tier4_realworld_tests import Tier4RealWorldTests

# ANSI Terminal Colors
class Colors:
    RESET = "\033[0m"
    BOLD = "\033[1m"
    DIM = "\033[2m"
    GREEN = "\033[92m"
    RED = "\033[91m"
    YELLOW = "\033[93m"
    CYAN = "\033[96m"
    BLUE = "\033[94m"
    MAGENTA = "\033[95m"


def supports_color() -> bool:
    """Check if current terminal supports ANSI escape codes."""
    if os.environ.get("NO_COLOR"):
        return False
    if sys.platform == "win32":
        # Enable Windows VT100 mode if possible
        try:
            import ctypes
            kernel32 = ctypes.windll.kernel32
            handle = kernel32.GetStdHandle(-11)  # STD_OUTPUT_HANDLE
            mode = ctypes.c_ulong()
            if kernel32.GetConsoleMode(handle, ctypes.byref(mode)):
                kernel32.SetConsoleMode(handle, mode.value | 0x0004)  # ENABLE_VIRTUAL_TERMINAL_PROCESSING
                return True
        except Exception:
            pass
    return hasattr(sys.stdout, "isatty") and sys.stdout.isatty()


USE_COLOR = supports_color()


def color(text: str, code: str) -> str:
    return f"{code}{text}{Colors.RESET}" if USE_COLOR else text


class DetailedTestResult(unittest.TestResult):
    """Custom TestResult capturing timing and failure details per test."""

    def __init__(self) -> None:
        super().__init__()
        self.records: List[Dict[str, Any]] = []
        self._test_start_time: float = 0.0

    def startTest(self, test: unittest.TestCase) -> None:
        super().startTest(test)
        self._test_start_time = time.perf_counter()

    def addSuccess(self, test: unittest.TestCase) -> None:
        super().addSuccess(test)
        duration = time.perf_counter() - self._test_start_time
        doc = (test.shortDescription() or "").strip()
        self.records.append({
            "name": test.id(),
            "method": test._testMethodName,
            "doc": doc,
            "status": "PASS",
            "duration_sec": round(duration, 4),
            "error": None,
        })

    def addFailure(self, test: unittest.TestCase, err: Any) -> None:
        super().addFailure(test, err)
        duration = time.perf_counter() - self._test_start_time
        doc = (test.shortDescription() or "").strip()
        self.records.append({
            "name": test.id(),
            "method": test._testMethodName,
            "doc": doc,
            "status": "FAIL",
            "duration_sec": round(duration, 4),
            "error": self._exc_info_to_string(err, test),
        })

    def addError(self, test: unittest.TestCase, err: Any) -> None:
        super().addError(test, err)
        duration = time.perf_counter() - self._test_start_time
        doc = (test.shortDescription() or "").strip()
        self.records.append({
            "name": test.id(),
            "method": test._testMethodName,
            "doc": doc,
            "status": "ERROR",
            "duration_sec": round(duration, 4),
            "error": self._exc_info_to_string(err, test),
        })


def get_tier_suite(tier: int) -> unittest.TestSuite:
    """Returns test suite for specified tier."""
    loader = unittest.TestLoader()
    suite = unittest.TestSuite()
    if tier == 1:
        suite.addTests(loader.loadTestsFromTestCase(Tier1FeatureTests))
    elif tier == 2:
        suite.addTests(loader.loadTestsFromTestCase(Tier2BoundaryTests))
    elif tier == 3:
        suite.addTests(loader.loadTestsFromTestCase(Tier3CombinationTests))
    elif tier == 4:
        suite.addTests(loader.loadTestsFromTestCase(Tier4RealWorldTests))
    return suite


def run_tier(tier: int, verbose: bool = False) -> Tuple[DetailedTestResult, float]:
    """Runs a single test tier and returns result with execution time."""
    suite = get_tier_suite(tier)
    result = DetailedTestResult()
    start = time.perf_counter()
    suite.run(result)
    elapsed = time.perf_counter() - start
    return result, elapsed


def print_banner() -> None:
    print(color("=" * 80, Colors.CYAN))
    print(color("  BookPoint 2.0.0 -- Opaque-Box E2E Test Harness", Colors.BOLD + Colors.CYAN))
    print(color("  Platform: ESP32-S3 (x4pro) | Display: SSD1677 800x480 | Controller: GT911", Colors.DIM))
    print(color("=" * 80, Colors.CYAN))


def main() -> int:
    parser = argparse.ArgumentParser(
        description="BookPoint 2.0.0 E2E Test Suite Runner",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        "--tier",
        type=str,
        choices=["1", "2", "3", "4", "all"],
        default=None,
        help="Run specific test tier (1: Feature, 2: Boundary, 3: Combination, 4: Real-World, all: All Tiers)",
    )
    parser.add_argument(
        "--all",
        action="store_true",
        help="Run all 4 test tiers (equivalent to --tier all)",
    )
    parser.add_argument(
        "--json",
        action="store_true",
        help="Output structured JSON results to stdout",
    )
    parser.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        help="Verbose test reporting with individual docstrings",
    )

    args = parser.parse_args()

    # Determine which tiers to run
    if args.all or args.tier == "all" or (args.tier is None and not args.json):
        tiers_to_run = [1, 2, 3, 4]
    elif args.tier:
        tiers_to_run = [int(args.tier)]
    else:
        tiers_to_run = [1, 2, 3, 4]

    tier_names = {
        1: "Tier 1: Feature Coverage (>=50 tests across F1-F12)",
        2: "Tier 2: Boundary Value Analysis & Corner Cases (>=50 tests across F1-F12)",
        3: "Tier 3: Cross-Feature Integration Tests (>=10 tests)",
        4: "Tier 4: Real-World Application Workloads (>=6 tests)",
    }

    if not args.json:
        print_banner()

    total_passed = 0
    total_failed = 0
    total_errors = 0
    total_tests = 0
    start_total_time = time.perf_counter()

    all_records: List[Dict[str, Any]] = []
    tier_summaries: List[Dict[str, Any]] = []

    for tier_idx in tiers_to_run:
        name = tier_names.get(tier_idx, f"Tier {tier_idx}")
        if not args.json:
            print(f"\n{color(f'>>> Executing {name}', Colors.BOLD + Colors.BLUE)}")

        result, elapsed = run_tier(tier_idx, verbose=args.verbose)

        passed = len(result.records) - len(result.failures) - len(result.errors)
        failed = len(result.failures)
        errors = len(result.errors)
        tier_count = len(result.records)

        total_passed += passed
        total_failed += failed
        total_errors += errors
        total_tests += tier_count
        all_records.extend(result.records)

        tier_summaries.append({
            "tier": tier_idx,
            "name": name,
            "total": tier_count,
            "passed": passed,
            "failed": failed,
            "errors": errors,
            "duration_sec": round(elapsed, 4),
        })

        if not args.json:
            for rec in result.records:
                status = rec["status"]
                m_name = rec["method"]
                doc = rec["doc"]
                dur = rec["duration_sec"]
                if status == "PASS":
                    status_str = color("[PASS]", Colors.GREEN)
                elif status == "FAIL":
                    status_str = color("[FAIL]", Colors.RED)
                else:
                    status_str = color("[ERR!]", Colors.MAGENTA)

                if args.verbose or status != "PASS":
                    desc = f" - {doc}" if doc else ""
                    print(f"  {status_str} {m_name} ({dur:.3f}s){desc}")
                    if rec["error"]:
                        print(color(f"      {rec['error']}", Colors.RED))

            # Tier summary line
            summary_color = Colors.GREEN if (failed == 0 and errors == 0) else Colors.RED
            status_text = "PASSED" if (failed == 0 and errors == 0) else "FAILED"
            print(
                color(
                    f"  Status: {status_text} | Total: {tier_count} | Passed: {passed} | "
                    f"Failed: {failed} | Errors: {errors} | Time: {elapsed:.3f}s",
                    summary_color,
                )
            )

    total_elapsed = time.perf_counter() - start_total_time

    # Output structured JSON if requested
    if args.json:
        json_output = {
            "suite": "BookPoint 2.0.0 E2E Test Suite",
            "overall_status": "PASSED" if (total_failed == 0 and total_errors == 0) else "FAILED",
            "total_tests": total_tests,
            "passed": total_passed,
            "failed": total_failed,
            "errors": total_errors,
            "duration_sec": round(total_elapsed, 4),
            "tier_summaries": tier_summaries,
            "test_records": all_records,
        }
        print(json.dumps(json_output, indent=2, ensure_ascii=False))
    else:
        # Final summary box
        print("\n" + color("=" * 80, Colors.CYAN))
        overall_color = Colors.BOLD + Colors.GREEN if (total_failed == 0 and total_errors == 0) else Colors.BOLD + Colors.RED
        verdict = "ALL TESTS PASSED (100% SUCCESS)" if (total_failed == 0 and total_errors == 0) else "TEST FAILURES DETECTED"
        print(color(f"  FINAL VERDICT: {verdict}", overall_color))
        print(
            f"  Executed: {color(str(total_tests), Colors.BOLD)} tests | "
            f"Passed: {color(str(total_passed), Colors.GREEN)} | "
            f"Failed: {color(str(total_failed), Colors.RED if total_failed > 0 else Colors.DIM)} | "
            f"Errors: {color(str(total_errors), Colors.MAGENTA if total_errors > 0 else Colors.DIM)} | "
            f"Total Time: {total_elapsed:.3f}s"
        )
        print(color("=" * 80, Colors.CYAN) + "\n")

    return 0 if (total_failed == 0 and total_errors == 0) else 1


if __name__ == "__main__":
    sys.exit(main())
