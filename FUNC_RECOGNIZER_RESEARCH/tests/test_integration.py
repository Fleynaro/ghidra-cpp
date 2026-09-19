"""Integration smoke tests for the generated executable and PyGhidra pipeline."""

from __future__ import annotations

import sys
import unittest
import re
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))


class IntegrationTests(unittest.TestCase):
    """Check repository artifacts without requiring PyGhidra during unit-only runs."""

    def test_corpus_declares_exactly_ten_exports(self) -> None:
        """The source corpus must retain exactly ten research functions by design."""
        source = (Path(__file__).resolve().parents[1] / "src" / "corpus.cpp").read_text(encoding="utf-8")
        expected = [
            "add", "sum_array", "add_to_list", "multiply", "absolute_difference",
            "count_even", "clamp_value", "dot_product", "reverse_in_place", "choose_value",
        ]
        declarations = re.findall(r"RESEARCH_API\s+(?:int|void)\s+(\w+)\s*\(", source)
        self.assertEqual(declarations, expected)

    def test_pipeline_files_exist(self) -> None:
        """The integration entry point and build wrapper must remain discoverable."""
        root = Path(__file__).resolve().parents[1]
        self.assertTrue((root / "build.bat").is_file())
        self.assertTrue((root / "scripts" / "run_pipeline.py").is_file())


if __name__ == "__main__":
    unittest.main()
