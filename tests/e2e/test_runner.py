"""
test_runner.py - Module entrypoint and alias for run_e2e_tests.py.
"""

import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent.parent
if str(REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(REPO_ROOT))

from tests.e2e.run_e2e_tests import DetailedTestResult, main, run_tier

if __name__ == "__main__":
    sys.exit(main())
