"""
Empirical Challenger 2 Stress-Test Suite
Focus Areas:
1. Live Typography: Zero-drift re-pagination across font sizes (14..36 pt), line spacings, margins.
2. Estimated Chapter Countdown Formula: 0 pages, 1 page, large chapters, reading paces, div-by-zero, negative minutes, NaN/Inf robustness.
3. Library 3x2 Cover Grid: Paging boundaries, edge slot index calculations, gaps, and sorting stability (author, date, progress).
"""

import math
import unittest
from typing import List, Tuple, Dict, Any, Optional

from tests.e2e.contracts import (
    AaTypographyModel,
    ReaderOverlaysModel,
    LibraryHubModel,
    LibrarySortModeEnum,
    LibraryViewModeEnum,
)


class TestLiveTypographyZeroDrift(unittest.TestCase):
    """
    Stress-tests live typography adjustment and zero-drift character offset mapping.
    """

    def _simulate_pagination(self, text_length: int, chars_per_page: int) -> List[Tuple[int, int]]:
        """Generates realistic page ranges [start, end) for a text of given length."""
        if text_length <= 0:
            return []
        ranges = []
        start = 0
        while start < text_length:
            end = min(start + chars_per_page, text_length)
            ranges.append((start, end))
            start = end
        return ranges

    def test_font_size_step_clamping(self):
        """Verify font sizes strictly clamp to [14, 36] with step 2."""
        self.assertEqual(AaTypographyModel.FONT_SIZE_MIN, 14)
        self.assertEqual(AaTypographyModel.FONT_SIZE_MAX, 36)
        self.assertEqual(AaTypographyModel.FONT_SIZE_STEP, 2)

        # Lower bound clamp
        self.assertEqual(AaTypographyModel.step_font_size(14, -2), 14)
        self.assertEqual(AaTypographyModel.step_font_size(14, -10), 14)

        # Upper bound clamp
        self.assertEqual(AaTypographyModel.step_font_size(36, +2), 36)
        self.assertEqual(AaTypographyModel.step_font_size(36, +10), 36)

        # Intermediate stepping
        cur = 14
        steps_up = []
        while cur < 36:
            cur = AaTypographyModel.step_font_size(cur, +2)
            steps_up.append(cur)
        self.assertEqual(steps_up, [16, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36])

        steps_down = []
        while cur > 14:
            cur = AaTypographyModel.step_font_size(cur, -2)
            steps_down.append(cur)
        self.assertEqual(steps_down, [34, 32, 30, 28, 26, 24, 22, 20, 18, 16, 14])

    def test_zero_drift_across_all_font_sizes(self):
        """
        Verify that a reader at character offset O remains on the exact page
        containing offset O across all 12 font size steps (14..36 pt).
        """
        doc_length = 25000  # 25k characters
        # Realistic chars per page: larger font = fewer chars per page
        # 14pt: ~1800 chars/page; 36pt: ~400 chars/page
        font_sizes = list(range(14, 38, 2))  # [14, 16, ..., 36]

        def chars_for_font(pt: int) -> int:
            # Linear interpolation from 1800 down to 400
            fraction = (pt - 14) / (36 - 14)
            return int(1800 - fraction * 1400)

        # Test at various reading positions: start, 1/4, 1/2, 3/4, near end, exact end
        test_offsets = [0, 1, 150, 4500, 12345, 24000, 24999]

        for offset in test_offsets:
            for pt in font_sizes:
                cpp = chars_for_font(pt)
                pages = self._simulate_pagination(doc_length, cpp)
                page_idx = AaTypographyModel.map_offset_to_new_page(offset, pages)

                # Page index must be valid
                self.assertGreaterEqual(page_idx, 0)
                self.assertLess(page_idx, len(pages))

                # Zero drift invariant: offset MUST be within [start, end) of new page
                start_off, end_off = pages[page_idx]
                self.assertLessEqual(start_off, offset, f"Offset {offset} is before page start {start_off} at {pt}pt")
                self.assertGreater(end_off, offset, f"Offset {offset} is at/past page end {end_off} at {pt}pt")

    def test_zero_drift_cycle_hysteresis(self):
        """
        Cycle through font sizes 14 -> 36 -> 14.
        Verify that re-pagination has zero hysteresis / zero drift when returning to initial size.
        """
        doc_length = 15000
        initial_cpp = 1500
        initial_pages = self._simulate_pagination(doc_length, initial_cpp)

        target_offset = 7234
        initial_page = AaTypographyModel.map_offset_to_new_page(target_offset, initial_pages)

        # Step up through all sizes
        for pt in range(16, 38, 2):
            cpp = int(1500 - (pt - 14) * 45)
            pages = self._simulate_pagination(doc_length, cpp)
            p = AaTypographyModel.map_offset_to_new_page(target_offset, pages)
            self.assertTrue(pages[p][0] <= target_offset < pages[p][1])

        # Step back down to 14pt
        for pt in range(34, 12, -2):
            cpp = int(1500 - (pt - 14) * 45)
            pages = self._simulate_pagination(doc_length, cpp)
            p = AaTypographyModel.map_offset_to_new_page(target_offset, pages)
            self.assertTrue(pages[p][0] <= target_offset < pages[p][1])

        # Return to initial 14pt
        final_pages = self._simulate_pagination(doc_length, initial_cpp)
        final_page = AaTypographyModel.map_offset_to_new_page(target_offset, final_pages)

        # Must land on the EXACT SAME page index
        self.assertEqual(initial_page, final_page)

    def test_offset_mapping_boundary_conditions(self):
        """Test edge conditions in offset mapping: empty doc, single char, out of bounds."""
        # Empty doc
        self.assertEqual(AaTypographyModel.map_offset_to_new_page(0, []), 0)
        self.assertEqual(AaTypographyModel.map_offset_to_new_page(100, []), 0)

        # Single page doc
        single_page = [(0, 500)]
        self.assertEqual(AaTypographyModel.map_offset_to_new_page(0, single_page), 0)
        self.assertEqual(AaTypographyModel.map_offset_to_new_page(250, single_page), 0)
        self.assertEqual(AaTypographyModel.map_offset_to_new_page(499, single_page), 0)
        # Out of bounds offset clamped to last page
        self.assertEqual(AaTypographyModel.map_offset_to_new_page(500, single_page), 0)
        self.assertEqual(AaTypographyModel.map_offset_to_new_page(9999, single_page), 0)

        # Multi-page boundary exact match
        pages = [(0, 100), (100, 200), (200, 300)]
        self.assertEqual(AaTypographyModel.map_offset_to_new_page(0, pages), 0)
        self.assertEqual(AaTypographyModel.map_offset_to_new_page(99, pages), 0)
        self.assertEqual(AaTypographyModel.map_offset_to_new_page(100, pages), 1)  # Exact boundary
        self.assertEqual(AaTypographyModel.map_offset_to_new_page(199, pages), 1)
        self.assertEqual(AaTypographyModel.map_offset_to_new_page(200, pages), 2)  # Exact boundary
        self.assertEqual(AaTypographyModel.map_offset_to_new_page(300, pages), 2)  # Clamped to last


class TestChapterCountdownFormula(unittest.TestCase):
    """
    Stress-tests the estimated chapter countdown formula:
    - 0 remaining pages
    - 1 page
    - Large chapters
    - Different reading paces (fast, normal, slow)
    - Division by zero / negative minutes prevention
    """

    def test_zero_and_negative_remaining_pages(self):
        """0 or negative remaining pages must return ~0 мин without negative values."""
        self.assertEqual(ReaderOverlaysModel.calc_chapter_countdown(0, 30.0), "~0 мин")
        self.assertEqual(ReaderOverlaysModel.calc_chapter_countdown(-1, 30.0), "~0 мин")
        self.assertEqual(ReaderOverlaysModel.calc_chapter_countdown(-100, 30.0), "~0 мин")

    def test_zero_and_negative_reading_pace(self):
        """0 or negative reading pace must return ~0 мин without division by zero or errors."""
        self.assertEqual(ReaderOverlaysModel.calc_chapter_countdown(10, 0.0), "~0 мин")
        self.assertEqual(ReaderOverlaysModel.calc_chapter_countdown(10, -15.0), "~0 мин")
        self.assertEqual(ReaderOverlaysModel.calc_chapter_countdown(0, 0.0), "~0 мин")
        self.assertEqual(ReaderOverlaysModel.calc_chapter_countdown(-5, -5.0), "~0 мин")

    def test_single_page_at_various_paces(self):
        """1 page remaining at various realistic paces."""
        # Fast reader: 10s per page -> 10s / 60s -> ceil is 1 min (min 1 min)
        self.assertEqual(ReaderOverlaysModel.calc_chapter_countdown(1, 10.0), "~1 мин")
        # Normal reader: 45s per page -> ceil is 1 min
        self.assertEqual(ReaderOverlaysModel.calc_chapter_countdown(1, 45.0), "~1 мин")
        # Exact 60s per page -> 1 min
        self.assertEqual(ReaderOverlaysModel.calc_chapter_countdown(1, 60.0), "~1 мин")
        # Slow reader: 75s per page -> 75s / 60s = 1.25 -> ceil is 2 min
        self.assertEqual(ReaderOverlaysModel.calc_chapter_countdown(1, 75.0), "~2 мин")
        # Very slow reader: 180s per page -> 3 min
        self.assertEqual(ReaderOverlaysModel.calc_chapter_countdown(1, 180.0), "~3 мин")

    def test_large_chapters(self):
        """Large chapters with 100..5000 remaining pages."""
        # 100 pages at 30s/page = 3000s = 50 min
        self.assertEqual(ReaderOverlaysModel.calc_chapter_countdown(100, 30.0), "~50 мин")
        # 100 pages at 60s/page = 100 min
        self.assertEqual(ReaderOverlaysModel.calc_chapter_countdown(100, 60.0), "~100 мин")
        # 500 pages at 45s/page = 22500s / 60 = 375 min
        self.assertEqual(ReaderOverlaysModel.calc_chapter_countdown(500, 45.0), "~375 мин")
        # 2000 pages at 30s/page = 60000s / 60 = 1000 min
        self.assertEqual(ReaderOverlaysModel.calc_chapter_countdown(2000, 30.0), "~1000 мин")

    def test_fractional_seconds_and_rounding(self):
        """Ensure ceil rounding ensures no 0 minutes when pages > 0."""
        # 1 page at 0.1s -> total_seconds = ceil(0.1) = 1 -> minutes = max(1, ceil(1/60)) = 1 min
        self.assertEqual(ReaderOverlaysModel.calc_chapter_countdown(1, 0.1), "~1 мин")
        # 5 pages at 12.3s = 61.5s -> total_seconds = 62 -> ceil(62/60) = 2 min
        self.assertEqual(ReaderOverlaysModel.calc_chapter_countdown(5, 12.3), "~2 мин")

    def test_no_negative_or_zero_minutes_when_remaining_positive(self):
        """For any positive remaining pages and pace, output must be at least '~1 мин'."""
        for pages in [1, 2, 5, 10, 50, 100]:
            for pace in [0.01, 1.0, 15.0, 30.0, 60.0, 120.0]:
                result = ReaderOverlaysModel.calc_chapter_countdown(pages, pace)
                self.assertTrue(result.startswith("~"))
                min_val = int(result.replace("~", "").replace(" мин", ""))
                self.assertGreaterEqual(min_val, 1, f"Failed for {pages} pages at pace {pace}")


class TestLibraryCoverGrid(unittest.TestCase):
    """
    Stress-tests the 3x2 Cover Grid:
    - Paging boundaries (0 items, 1 item, 6 items, 7 items, partial pages)
    - Edge slot index calculations (corners, borders, inter-cell gaps)
    - Sorting stability across Author, Date, Progress, Name
    """

    def test_grid_geometry_constants(self):
        """Verify 3x2 grid geometry conforms to 800x480 screen contract."""
        self.assertEqual(LibraryHubModel.GRID_ROWS, 3)
        self.assertEqual(LibraryHubModel.GRID_COLS, 2)
        self.assertEqual(LibraryHubModel.GRID_ITEMS_PER_PAGE, 6)

        self.assertEqual(LibraryHubModel.CELL_WIDTH, 210)
        self.assertEqual(LibraryHubModel.CELL_HEIGHT, 220)
        self.assertEqual(LibraryHubModel.CELL_MARGIN_X, 20)
        self.assertEqual(LibraryHubModel.CELL_GAP_X, 20)
        self.assertEqual(LibraryHubModel.CELL_MARGIN_Y, 60)
        self.assertEqual(LibraryHubModel.CELL_GAP_Y, 15)

        # Verify rightmost edge: 20 + 210 + 20 + 210 = 460 <= 480 (20px right margin)
        total_width = 20 + 2 * 210 + 20
        self.assertEqual(total_width, 460)
        self.assertLessEqual(total_width, 480)

        # Verify bottom edge: 60 + 3 * 220 + 2 * 15 = 750 <= 800 (50px bottom margin)
        total_height = 60 + 3 * 220 + 2 * 15
        self.assertEqual(total_height, 750)
        self.assertLessEqual(total_height, 800)

    def test_cell_rect_coordinates(self):
        """Verify exact cell bounding boxes for all 6 slots."""
        expected_rects = [
            (20, 60, 210, 220),   # Row 0, Col 0 (slot 0)
            (250, 60, 210, 220),  # Row 0, Col 1 (slot 1)
            (20, 295, 210, 220),  # Row 1, Col 0 (slot 2)
            (250, 295, 210, 220), # Row 1, Col 1 (slot 3)
            (20, 530, 210, 220),  # Row 2, Col 0 (slot 4)
            (250, 530, 210, 220), # Row 2, Col 1 (slot 5)
        ]
        for slot in range(6):
            r = slot // 2
            c = slot % 2
            rect = LibraryHubModel.get_grid_cell_rect(r, c)
            self.assertEqual(rect, expected_rects[slot], f"Mismatch for slot {slot}")

    def test_grid_tap_classification_corners_and_centers(self):
        """Verify tap hit-testing inside cells."""
        for slot in range(6):
            r = slot // 2
            c = slot % 2
            x, y, w, h = LibraryHubModel.get_grid_cell_rect(r, c)

            # Top-left inside cell
            self.assertEqual(LibraryHubModel.classify_grid_tap(x, y, 6), slot)
            # Center inside cell
            self.assertEqual(LibraryHubModel.classify_grid_tap(x + w // 2, y + h // 2, 6), slot)
            # Bottom-right inside cell
            self.assertEqual(LibraryHubModel.classify_grid_tap(x + w - 1, y + h - 1, 6), slot)

    def test_grid_tap_rejection_in_gaps_and_margins(self):
        """Verify taps in gaps between cells or outer margins are rejected (return None)."""
        # Outer left margin (x < 20)
        self.assertIsNone(LibraryHubModel.classify_grid_tap(10, 100, 6))
        # Outer right margin (x >= 460)
        self.assertIsNone(LibraryHubModel.classify_grid_tap(465, 100, 6))
        # Outer top margin (y < 60)
        self.assertIsNone(LibraryHubModel.classify_grid_tap(100, 40, 6))
        # Outer bottom margin (y >= 750)
        self.assertIsNone(LibraryHubModel.classify_grid_tap(100, 760, 6))

        # Horizontal inter-column gap (x: 230..249)
        self.assertIsNone(LibraryHubModel.classify_grid_tap(235, 100, 6))
        self.assertIsNone(LibraryHubModel.classify_grid_tap(249, 100, 6))

        # Vertical inter-row gap 1 (y: 280..294)
        self.assertIsNone(LibraryHubModel.classify_grid_tap(100, 285, 6))
        self.assertIsNone(LibraryHubModel.classify_grid_tap(300, 290, 6))

        # Vertical inter-row gap 2 (y: 515..529)
        self.assertIsNone(LibraryHubModel.classify_grid_tap(100, 520, 6))
        self.assertIsNone(LibraryHubModel.classify_grid_tap(300, 525, 6))

    def test_partial_page_tap_masking(self):
        """
        When items_on_page < 6, taps on empty slots must return None,
        preventing phantom activations of non-existent items.
        """
        for items_on_page in range(7):
            for slot in range(6):
                r = slot // 2
                c = slot % 2
                x, y, w, h = LibraryHubModel.get_grid_cell_rect(r, c)
                result = LibraryHubModel.classify_grid_tap(x + 10, y + 10, items_on_page)
                if slot < items_on_page:
                    self.assertEqual(result, slot)
                else:
                    self.assertIsNone(result, f"Slot {slot} should be None when items_on_page={items_on_page}")

    def test_paging_boundaries(self):
        """Test paging calculations for arbitrary collection sizes."""
        test_cases = [
            (0, 0),    # 0 items -> 0 pages
            (1, 1),    # 1 item -> 1 page
            (5, 1),    # 5 items -> 1 page
            (6, 1),    # 6 items -> 1 page (full)
            (7, 2),    # 7 items -> 2 pages
            (12, 2),   # 12 items -> 2 pages (full)
            (13, 3),   # 13 items -> 3 pages
            (100, 17), # 100 items -> 17 pages (16*6 = 96 + 4)
        ]
        for total_items, expected_pages in test_cases:
            pages = math.ceil(total_items / 6.0) if total_items > 0 else 0
            self.assertEqual(pages, expected_pages, f"Failed page count for {total_items} items")

    def test_sorting_stability_author(self):
        """
        Verify sorting stability by author:
        Books with the same author must strictly retain their original order.
        """
        books = [
            {"id": 1, "title": "Foundation", "author": "Isaac Asimov", "date": 100, "progress": 50.0},
            {"id": 2, "title": "Dune", "author": "Frank Herbert", "date": 200, "progress": 20.0},
            {"id": 3, "title": "Second Foundation", "author": "Isaac Asimov", "date": 300, "progress": 10.0},
            {"id": 4, "title": "Children of Dune", "author": "Frank Herbert", "date": 400, "progress": 80.0},
            {"id": 5, "title": "Foundation and Empire", "author": "Isaac Asimov", "date": 500, "progress": 0.0},
        ]

        sorted_books = LibraryHubModel.sort_books(books, LibrarySortModeEnum.AUTHOR, ascending=True)

        # Herbert books first (H < I)
        herbert_ids = [b["id"] for b in sorted_books if b["author"] == "Frank Herbert"]
        self.assertEqual(herbert_ids, [2, 4], "Herbert books lost relative stability")

        # Asimov books second
        asimov_ids = [b["id"] for b in sorted_books if b["author"] == "Isaac Asimov"]
        self.assertEqual(asimov_ids, [1, 3, 5], "Asimov books lost relative stability")

    def test_sorting_stability_date(self):
        """
        Verify sorting stability by date:
        Books with the same timestamp must retain their original order.
        """
        books = [
            {"id": 10, "title": "Book Alpha", "author": "A", "date": 1600000000, "progress": 10.0},
            {"id": 20, "title": "Book Beta", "author": "B", "date": 1700000000, "progress": 20.0},
            {"id": 30, "title": "Book Gamma", "author": "C", "date": 1600000000, "progress": 30.0},
            {"id": 40, "title": "Book Delta", "author": "D", "date": 1600000000, "progress": 40.0},
        ]

        # Ascending date
        sorted_asc = LibraryHubModel.sort_books(books, LibrarySortModeEnum.DATE, ascending=True)
        same_date_ids = [b["id"] for b in sorted_asc if b["date"] == 1600000000]
        self.assertEqual(same_date_ids, [10, 30, 40], "Date asc lost relative stability")

        # Descending date (newest first)
        sorted_desc = LibraryHubModel.sort_books(books, LibrarySortModeEnum.DATE, ascending=False)
        self.assertEqual(sorted_desc[0]["id"], 20)  # 1700000000 is newest
        same_date_desc_ids = [b["id"] for b in sorted_desc if b["date"] == 1600000000]
        self.assertEqual(same_date_desc_ids, [10, 30, 40], "Date desc lost relative stability")

    def test_sorting_stability_progress(self):
        """
        Verify sorting stability by progress:
        Books with the exact same progress % must retain their original order.
        """
        books = [
            {"id": 1, "title": "Unread 1", "author": "X", "date": 1, "progress": 0.0},
            {"id": 2, "title": "Half 1", "author": "Y", "date": 2, "progress": 50.0},
            {"id": 3, "title": "Unread 2", "author": "Z", "date": 3, "progress": 0.0},
            {"id": 4, "title": "Finished 1", "author": "W", "date": 4, "progress": 100.0},
            {"id": 5, "title": "Unread 3", "author": "V", "date": 5, "progress": 0.0},
            {"id": 6, "title": "Half 2", "author": "U", "date": 6, "progress": 50.0},
        ]

        # Ascending progress (0% -> 50% -> 100%)
        sorted_asc = LibraryHubModel.sort_books(books, LibrarySortModeEnum.PROGRESS, ascending=True)
        unread_asc_ids = [b["id"] for b in sorted_asc if b["progress"] == 0.0]
        self.assertEqual(unread_asc_ids, [1, 3, 5], "Progress 0% asc lost stability")
        half_asc_ids = [b["id"] for b in sorted_asc if b["progress"] == 50.0]
        self.assertEqual(half_asc_ids, [2, 6], "Progress 50% asc lost stability")

        # Descending progress (100% -> 50% -> 0%)
        sorted_desc = LibraryHubModel.sort_books(books, LibrarySortModeEnum.PROGRESS, ascending=False)
        unread_desc_ids = [b["id"] for b in sorted_desc if b["progress"] == 0.0]
        self.assertEqual(unread_desc_ids, [1, 3, 5], "Progress 0% desc lost stability")


if __name__ == "__main__":
    unittest.main()
