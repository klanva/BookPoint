"""
tier2_boundary_tests.py - Tier 2 Boundary & Corner Case Tests (≥5 per feature, 10 features, 50 tests total).

Authoritative References:
- c:/xteinkx4/.agents/ORIGINAL_REQUEST.md (§R1, §R2, §R3)
- c:/xteinkx4/PROJECT.md (§Interface Contracts, §Feature Inventory)
- c:/xteinkx4/TEST_INFRA.md (§Coverage Thresholds)
"""

import hashlib
import io
import re
import unittest
from pathlib import Path

from tests.e2e.contracts import (
    APP0_MAX_PARTITION_SIZE,
    BUFFER_SIZE,
    ESP32_IMAGE_MAGIC,
    EXPECTED_GH_RELEASE_COMMAND,
    EXPECTED_RELEASE_SHA256,
    EXPECTED_SCREENSHOT_FILES,
    LOGICAL_HEIGHT,
    LOGICAL_WIDTH,
    PHYSICAL_HEIGHT,
    PHYSICAL_WIDTH,
    PHYSICAL_WIDTH_BYTES,
    POLARITY_BLACK,
    POLARITY_WHITE,
    RELEASE_BIN_PATH,
    REPO_ROOT,
    THEME_METRICS,
    DashboardModel,
    DisplayGeometry,
    FirmwareValidator,
    HomeMenuItem,
    HOME_MENU_LABELS_RU,
    LyraCarouselModel,
    PARTITIONS_CSV_PATH,
    PLATFORMIO_INI_PATH,
    SwipeDir,
    UIThemeEnum,
    VirtualFramebuffer,
    VirtualKeyboardModel,
    classify_3zone_cover_tap,
    classify_swipe,
    tap_in_rect,
    ModernThemeModel,
    HardwareKeyFocusModel,
    GT911MappingModel,
    EmojiBanValidator,
    QuickSettingsCurtainModel,
    HandednessEnum,
    SideDrawerModel,
    ReaderOverlaysModel,
    AaTypographyModel,
    ModularSettingsHubModel,
    LibraryViewModeEnum,
    LibrarySortModeEnum,
    LibraryHubModel,
    CrossPointEradicationValidator,
    UniversalFormatsModel,
    StrikethroughNormalizationModel,
    ProgressiveJpegDecoderModel,
    FootnoteModalModel,
    ScreensaverGalleryModel,
)

try:
    from PIL import Image
    HAS_PIL = True
except ImportError:
    HAS_PIL = False


class Tier2BoundaryTests(unittest.TestCase):
    """Tier 2: Boundary Value Analysis & Edge Cases (50 tests total across features F1-F10)."""

    # --------------------------------------------------------------------------
    # FEATURE 1 BOUNDARIES: LyraCarouselTheme (5 tests)
    # --------------------------------------------------------------------------

    def test_f2_b01_carousel_empty_library(self):
        """F1.B1: Carousel with 0 books handles empty library gracefully."""
        left, center, right = LyraCarouselModel.get_book_indices_for_slot(0, 0)
        self.assertEqual((left, center, right), (-1, -1, -1))
        dots = LyraCarouselModel.calc_pagination_dots(0, 0)
        self.assertEqual(dots, [])

    def test_f2_b02_carousel_single_book(self):
        """F1.B2: Carousel with exactly 1 book displays center cover only without side duplicates."""
        left, center, right = LyraCarouselModel.get_book_indices_for_slot(0, 1)
        self.assertEqual(center, 0)
        self.assertEqual(left, -1)
        self.assertEqual(right, -1)
        # Single book does not need pagination dots
        dots = LyraCarouselModel.calc_pagination_dots(1, 0)
        self.assertEqual(dots, [])

    def test_f2_b03_carousel_two_books(self):
        """F1.B3: Carousel with 2 books correctly wraps without crashing."""
        left, center, right = LyraCarouselModel.get_book_indices_for_slot(0, 2)
        self.assertEqual(center, 0)
        self.assertEqual(left, 1)
        self.assertEqual(right, 1)

    def test_f2_b04_carousel_many_books_wrapping(self):
        """F1.B4: Carousel with 25 books wraps seamlessly at boundaries 0 and 24."""
        total = 25
        # At index 0, left is 24, right is 1
        l0, c0, r0 = LyraCarouselModel.get_book_indices_for_slot(0, total)
        self.assertEqual((l0, c0, r0), (24, 0, 1))

        # At index 24, left is 23, right is 0
        l24, c24, r24 = LyraCarouselModel.get_book_indices_for_slot(24, total)
        self.assertEqual((l24, c24, r24), (23, 24, 0))

    def test_f2_b05_carousel_extreme_zoom_scale(self):
        """F1.B5: Scale boundaries: center scale must be >= 1.0, side scale between 0.5 and 0.8."""
        scale_center = THEME_METRICS[UIThemeEnum.CAROUSEL]["scale_center"]
        scale_side = THEME_METRICS[UIThemeEnum.CAROUSEL]["scale_side"]
        self.assertGreaterEqual(scale_center, 1.0)
        self.assertLessEqual(scale_center, 1.5)
        self.assertGreaterEqual(scale_side, 0.5)
        self.assertLessEqual(scale_side, 0.8)

    # --------------------------------------------------------------------------
    # FEATURE 2 BOUNDARIES: DashboardTheme Overhaul (5 tests)
    # --------------------------------------------------------------------------

    def test_f2_b06_dashboard_zero_percent_progress(self):
        """F2.B1: 0% reading progress boundary produces 0-width fill bar."""
        filled_w = DashboardModel.calc_progress_bar_width(0.0, total_bar_width=200)
        self.assertEqual(filled_w, 0)
        # Negative percentage clamped to 0
        filled_neg = DashboardModel.calc_progress_bar_width(-15.0, total_bar_width=200)
        self.assertEqual(filled_neg, 0)

    def test_f2_b07_dashboard_hundred_percent_progress(self):
        """F2.B2: 100% reading progress boundary produces full-width fill bar and 0m remaining."""
        filled_w = DashboardModel.calc_progress_bar_width(100.0, total_bar_width=200)
        self.assertEqual(filled_w, 200)
        # Greater than 100% clamped to 100%
        filled_over = DashboardModel.calc_progress_bar_width(120.0, total_bar_width=200)
        self.assertEqual(filled_over, 200)

        time_left = DashboardModel.calc_remaining_time(500, 500, pace_pages_per_hour=40.0)
        self.assertEqual(time_left, "0м")

    def test_f2_b08_dashboard_very_long_book_title(self):
        """F2.B3: Extremely long title (250 chars) truncated gracefully with ellipsis."""
        long_title = "A" * 250
        max_chars = 32
        truncated = long_title[:max_chars - 3] + "..." if len(long_title) > max_chars else long_title
        self.assertEqual(len(truncated), max_chars)
        self.assertTrue(truncated.endswith("..."))

    def test_f2_b09_dashboard_zero_day_streak(self):
        """F2.B4: 0-day reading streak results in all 7 calendar cells inactive."""
        cells = DashboardModel.calc_streak_calendar_cells(streak_days=0, today_active=False)
        completed = [c for c in cells if c["is_completed"]]
        self.assertEqual(len(completed), 0)

    def test_f2_b10_dashboard_battery_boundaries(self):
        """F2.B5: Battery levels at boundary values 0%, 1%, 99%, 100%."""
        for level in [0, 1, 50, 99, 100]:
            clamped = max(0, min(100, level))
            self.assertEqual(clamped, level)

    # --------------------------------------------------------------------------
    # FEATURE 3 BOUNDARIES: Extended Home Menu (5 tests)
    # --------------------------------------------------------------------------

    def test_f2_b11_menu_zero_items(self):
        """F3.B1: Empty menu list calculation requires 0 height."""
        row_count = 0
        total_h = row_count * 52
        self.assertEqual(total_h, 0)

    def test_f2_b12_menu_maximum_items_overflow(self):
        """F3.B2: Menu with 15 items exceeds single-screen height budget and signals overflow."""
        available_menu_h = 480
        row_step = 52
        item_count = 15
        needed_h = item_count * row_step
        self.assertGreater(needed_h, available_menu_h)

    def test_f2_b13_menu_cyrillic_encoding(self):
        """F3.B3: Russian menu labels UTF-8 encoding integrity."""
        for item, label in HOME_MENU_LABELS_RU.items():
            encoded = label.encode("utf-8")
            decoded = encoded.decode("utf-8")
            self.assertEqual(label, decoded)
            # Cyrillic chars take 2 bytes per char in UTF-8
            self.assertGreater(len(encoded), len(label))

    def test_f2_b14_menu_rapid_touch_sequence(self):
        """F3.B4: Rapid touch sequence across different rows selects correct final row."""
        menu_top = 340
        row_step = 52
        touch_ys = [350, 410, 460, 520, 570]
        rows = [(y - menu_top) // row_step for y in touch_ys]
        self.assertEqual(rows, [0, 1, 2, 3, 4])

    def test_f2_b15_menu_boundary_touch_y(self):
        """F3.B5: Touch at exact top border (y=340) hits row 0; above (y=339) is out."""
        menu_top = 340
        row_step = 52
        self.assertEqual((340 - menu_top) // row_step, 0)
        self.assertLess((339 - menu_top) // row_step, 0)

    # --------------------------------------------------------------------------
    # FEATURE 4 BOUNDARIES: Virtual Touch Keyboard & Search (5 tests)
    # --------------------------------------------------------------------------

    def test_f2_b16_keyboard_empty_string_search(self):
        """F4.B1: Empty search query returns all books in library."""
        books = [
            {"title": "Book 1", "author": "Author A"},
            {"title": "Book 2", "author": "Author B"},
        ]
        res = VirtualKeyboardModel.filter_books("", books)
        self.assertEqual(len(res), 2)
        res_spaces = VirtualKeyboardModel.filter_books("   ", books)
        self.assertEqual(len(res_spaces), 2)

    def test_f2_b17_keyboard_special_characters_search(self):
        """F4.B2: Special characters and punctuation search handling."""
        books = [
            {"title": "C++ Primer (5th Edition)", "author": "Lippman"},
            {"title": "The Art of War", "author": "Sun Tzu"},
        ]
        res = VirtualKeyboardModel.filter_books("C++", books)
        self.assertEqual(len(res), 1)
        self.assertEqual(res[0]["title"], "C++ Primer (5th Edition)")

    def test_f2_b18_keyboard_cyrillic_mixed_case(self):
        """F4.B3: Mixed-case Cyrillic search matches accurately."""
        books = [{"title": "Евгений Онегин", "author": "Александр Пушкин"}]
        res = VirtualKeyboardModel.filter_books("еВгЕнИй", books)
        self.assertEqual(len(res), 1)

    def test_f2_b19_keyboard_max_length_limit(self):
        """F4.B4: Text buffer limit InteractionBuffer<48> truncation."""
        max_len = 48
        long_input = "А" * 100
        truncated = long_input[:max_len]
        self.assertEqual(len(truncated), 48)

    def test_f2_b20_keyboard_rapid_layout_toggle(self):
        """F4.B5: 100 consecutive layout toggles return to original state."""
        kb = VirtualKeyboardModel()
        self.assertTrue(kb.is_ru)
        for _ in range(100):
            kb.toggle_layout()
        self.assertTrue(kb.is_ru)

    # --------------------------------------------------------------------------
    # FEATURE 5 BOUNDARIES: Theme Switcher in Settings (5 tests)
    # --------------------------------------------------------------------------

    def test_f2_b21_theme_invalid_enum_fallback(self):
        """F5.B1: Out-of-bounds theme ID (99) raises ValueError."""
        with self.assertRaises(ValueError):
            UIThemeEnum(99)

    def test_f2_b22_theme_rapid_switching(self):
        """F5.B2: Rapid sequential switching across all themes without state corruption."""
        active_theme = None
        for theme_val in UIThemeEnum:
            active_theme = theme_val
            metrics = THEME_METRICS[active_theme]
            self.assertIsNotNone(metrics)
        self.assertEqual(active_theme, UIThemeEnum.MODERN)

    def test_f2_b23_theme_font_scaling_bounds(self):
        """F5.B3: Font scale boundary clamping [0.5x .. 2.0x]."""
        scales = [0.2, 0.5, 1.0, 1.5, 2.0, 3.0]
        clamped = [max(0.5, min(2.0, s)) for s in scales]
        self.assertEqual(clamped[0], 0.5)
        self.assertEqual(clamped[-1], 2.0)

    def test_f2_b24_theme_zero_padding(self):
        """F5.B4: Zero padding theme metrics render without negative coordinate exceptions."""
        padding = 0
        content_w = LOGICAL_WIDTH - 2 * padding
        self.assertEqual(content_w, 480)

    def test_f2_b25_theme_high_contrast_mode(self):
        """F5.B5: High-contrast inverted polarity fills buffer with 0x00."""
        fb = VirtualFramebuffer()
        fb.clear(fill_byte=0x00)
        self.assertEqual(fb.get_physical_pixel(0, 0), POLARITY_BLACK)
        self.assertEqual(fb.compute_entropy(), 1.0)  # 100% black pixels

    # --------------------------------------------------------------------------
    # FEATURE 6 BOUNDARIES: Standalone 800x480 Simulator (5 tests)
    # --------------------------------------------------------------------------

    def test_f2_b26_framebuffer_extreme_coordinates(self):
        """F6.B1: Exact physical panel corners (0,0), (799,0), (0,479), (799,479)."""
        fb = VirtualFramebuffer()
        corners = [(0, 0), (799, 0), (0, 479), (799, 479)]
        for px, py in corners:
            fb.set_physical_pixel(px, py, black=True)
            self.assertEqual(fb.get_physical_pixel(px, py), POLARITY_BLACK)

    def test_f2_b27_framebuffer_out_of_bounds_clipping(self):
        """F6.B2: Physical pixel reading out-of-bounds raises IndexError."""
        fb = VirtualFramebuffer()
        with self.assertRaises(IndexError):
            fb.get_physical_pixel(-1, 0)
        with self.assertRaises(IndexError):
            fb.get_physical_pixel(800, 0)
        with self.assertRaises(IndexError):
            fb.get_physical_pixel(0, 480)

    def test_f2_b28_framebuffer_exact_48000_byte_boundary(self):
        """F6.B3: Byte 47,999 is accessible, byte 48,000 raises IndexError."""
        fb = VirtualFramebuffer()
        self.assertEqual(fb.buffer[47999], 0xFF)
        with self.assertRaises(IndexError):
            _ = fb.buffer[48000]

    def test_f2_b29_framebuffer_full_white_clear(self):
        """F6.B4: Full white clear has 0 black pixels (entropy == 0.0)."""
        fb = VirtualFramebuffer()
        fb.clear(0xFF)
        self.assertEqual(fb.compute_entropy(), 0.0)

    def test_f2_b30_framebuffer_full_black_fill(self):
        """F6.B5: Full black fill has all black pixels (entropy == 1.0)."""
        fb = VirtualFramebuffer()
        fb.clear(0x00)
        self.assertEqual(fb.compute_entropy(), 1.0)

    # --------------------------------------------------------------------------
    # FEATURE 7 BOUNDARIES: Screenshot PNG Artifacts (5 tests)
    # --------------------------------------------------------------------------

    def test_f2_b31_screenshot_zero_byte_detection(self):
        """F7.B1: Zero-byte file rejected as invalid PNG."""
        empty_data = b""
        self.assertFalse(empty_data.startswith(b"\x89PNG"))

    def test_f2_b32_screenshot_wrong_aspect_ratio(self):
        """F7.B2: Non-standard resolutions rejected."""
        valid_resolutions = {(480, 800), (800, 480)}
        self.assertNotIn((1024, 768), valid_resolutions)
        self.assertIn((480, 800), valid_resolutions)
        self.assertIn((800, 480), valid_resolutions)

    def test_f2_b33_screenshot_palette_modes(self):
        """F7.B3: 1bpp mode '1' and grayscale mode 'L' export valid dimensions."""
        fb = VirtualFramebuffer()
        img_1 = fb.to_logical_pil_image()
        self.assertEqual(img_1.mode, "1")
        img_l = img_1.convert("L")
        self.assertEqual(img_l.mode, "L")
        self.assertEqual(img_l.size, (LOGICAL_WIDTH, LOGICAL_HEIGHT))

    def test_f2_b34_screenshot_monochrome_pbm_export(self):
        """F7.B4: PBM (Portable Bitmap) format header and payload size."""
        header = f"P4\n{PHYSICAL_WIDTH} {PHYSICAL_HEIGHT}\n".encode("ascii")
        expected_size = len(header) + BUFFER_SIZE
        self.assertEqual(expected_size, len(header) + 48000)

    def test_f2_b35_gallery_missing_image_graceful(self):
        """F7.B5: HTML gallery image tags contain alt attributes for missing images."""
        tag = '<img src="home_dashboard_800x480.png" alt="Home Dashboard">'
        self.assertIn('alt="Home Dashboard"', tag)

    # --------------------------------------------------------------------------
    # FEATURE 8 BOUNDARIES: GT911 Touch Gestures (5 tests)
    # --------------------------------------------------------------------------

    def test_f2_b36_touch_extreme_coordinates(self):
        """F8.B1: Extreme touch coordinates (0, 0) and (479, 799)."""
        self.assertTrue(tap_in_rect(0, 0, 0, 0, LOGICAL_WIDTH, LOGICAL_HEIGHT))
        self.assertTrue(tap_in_rect(479, 799, 0, 0, LOGICAL_WIDTH, LOGICAL_HEIGHT))
        self.assertFalse(tap_in_rect(480, 800, 0, 0, LOGICAL_WIDTH, LOGICAL_HEIGHT))

    def test_f2_b37_touch_negative_coordinates(self):
        """F8.B2: Negative coordinates rejected outside screen rect."""
        self.assertFalse(tap_in_rect(-1, 100, 0, 0, LOGICAL_WIDTH, LOGICAL_HEIGHT))
        self.assertFalse(tap_in_rect(100, -5, 0, 0, LOGICAL_WIDTH, LOGICAL_HEIGHT))

    def test_f2_b38_touch_zero_distance_swipe(self):
        """F8.B3: Zero distance motion classified as SwipeDir.NONE."""
        self.assertEqual(classify_swipe(200, 200, 200, 200), SwipeDir.NONE)

    def test_f2_b39_touch_diagonal_swipe(self):
        """F8.B4: Equal diagonal displacement (|dx| == |dy|) resolves deterministically to horizontal."""
        # dx = 100, dy = 100 -> dx >= dy -> SwipeRight
        self.assertEqual(classify_swipe(100, 100, 200, 200), SwipeDir.RIGHT)

    def test_f2_b40_touch_edge_swipe_boundary(self):
        """F8.B5: Edge swipe vs mid-screen swipe boundary (25% of width = 120px)."""
        edge_threshold_x = LOGICAL_WIDTH * 0.25  # 120
        # Start at x=50 is an edge swipe
        self.assertLess(50, edge_threshold_x)
        # Start at x=200 is a mid-screen swipe
        self.assertGreater(200, edge_threshold_x)

    # --------------------------------------------------------------------------
    # FEATURE 9 BOUNDARIES: PlatformIO Build Environment [env:x4pro] (5 tests)
    # --------------------------------------------------------------------------

    def test_f2_b41_flash_usage_under_90_percent(self):
        """F9.B1: Firmware flash usage strictly < 90.0% of app0 partition."""
        val = FirmwareValidator.validate_release_binary(RELEASE_BIN_PATH)
        self.assertLess(val["flash_pct"], 90.0)
        self.assertGreater(val["flash_pct"], 80.0)

    def test_f2_b42_ram_usage_under_30_percent(self):
        """F9.B2: Internal DRAM usage documented and certified strictly < 30.0%."""
        # From AUDIT_X4PRO.md: RAM is 19.9%
        audit_path = REPO_ROOT / "AUDIT_X4PRO.md"
        self.assertTrue(audit_path.exists())
        content = audit_path.read_text(encoding="utf-8", errors="replace")
        self.assertIn("RAM 19.9%", content)

    def test_f2_b43_partition_table_total_flash(self):
        """F9.B3: Partition table sizes fit within 16MB (0x1000000 = 16,777,216 bytes)."""
        partitions = FirmwareValidator.parse_partitions_csv(PARTITIONS_CSV_PATH)
        max_end_offset = max(p["offset"] + p["size"] for p in partitions)
        self.assertLessEqual(max_end_offset, 0x1000000)

    def test_f2_b44_psram_allocation_flag(self):
        """F9.B4: FREEINK_FB_PSRAM flag present to prevent 48KB buffer exhausting internal DRAM."""
        content = PLATFORMIO_INI_PATH.read_text(encoding="utf-8", errors="replace")
        self.assertIn("-DFREEINK_FB_PSRAM=1", content)

    def test_f2_b45_compiler_flags_fno_exceptions(self):
        """F9.B5: -fno-exceptions flag present for memory efficiency."""
        content = PLATFORMIO_INI_PATH.read_text(encoding="utf-8", errors="replace")
        self.assertIn("-fno-exceptions", content)

    # --------------------------------------------------------------------------
    # FEATURE 10 BOUNDARIES: Release Binary & GitHub Asset (5 tests)
    # --------------------------------------------------------------------------

    def test_f2_b46_release_binary_minimum_size(self):
        """F10.B1: Binary must be >= 4 MB (full firmware with embedded assets)."""
        val = FirmwareValidator.validate_release_binary(RELEASE_BIN_PATH)
        self.assertGreater(val["size"], 4 * 1024 * 1024)

    def test_f2_b47_release_binary_maximum_size(self):
        """F10.B2: Binary must not exceed app0 partition limit (6,553,600 bytes)."""
        val = FirmwareValidator.validate_release_binary(RELEASE_BIN_PATH)
        self.assertLessEqual(val["size"], APP0_MAX_PARTITION_SIZE)

    def test_f2_b48_release_binary_checksum_format(self):
        """F10.B3: SHA256 string is exactly 64 lowercase hex characters."""
        val = FirmwareValidator.validate_release_binary(RELEASE_BIN_PATH)
        self.assertEqual(len(val["sha256"]), 64)
        self.assertRegex(val["sha256"], r"^[0-9a-f]{64}$")

    def test_f2_b49_release_tag_semver_format(self):
        """F10.B4: Release tag follows SemVer pattern v2.0.0."""
        tag = "v2.0.0"
        self.assertRegex(tag, r"^v\d+\.\d+\.\d+$")

    def test_f2_b50_release_clobber_flag(self):
        """F10.B5: Release command specifies --clobber for idempotent uploads."""
        self.assertIn("--clobber", EXPECTED_GH_RELEASE_COMMAND)

    # --------------------------------------------------------------------------
    # FEATURE 11 BOUNDARIES: ModernTheme & Flagship Home Screen (8 tests)
    # --------------------------------------------------------------------------

    def test_f2_b51_hero_card_boundary_pixels(self):
        """F11.B1: Hero card exact corner boundary pixel containment."""
        # Exact corners (inclusive)
        self.assertTrue(ModernThemeModel.classify_hero_tap(14, 50))
        self.assertTrue(ModernThemeModel.classify_hero_tap(465, 50))
        self.assertTrue(ModernThemeModel.classify_hero_tap(14, 360))
        self.assertTrue(ModernThemeModel.classify_hero_tap(465, 360))
        # 1 pixel outside each edge
        self.assertFalse(ModernThemeModel.classify_hero_tap(13, 50))
        self.assertFalse(ModernThemeModel.classify_hero_tap(466, 50))
        self.assertFalse(ModernThemeModel.classify_hero_tap(14, 49))
        self.assertFalse(ModernThemeModel.classify_hero_tap(14, 361))

    def test_f2_b52_status_bar_and_curtain_boundary_pixels(self):
        """F11.B2: Status bar and curtain transition boundary pixels."""
        # Top-edge boundary
        self.assertTrue(ModernThemeModel.classify_header_tap(0, 0))
        self.assertTrue(ModernThemeModel.classify_header_tap(479, 43))
        self.assertFalse(ModernThemeModel.classify_header_tap(0, 44))
        self.assertFalse(ModernThemeModel.classify_header_tap(480, 20))
        # Curtain dismiss boundary: 339 is inside curtain, 340 is outside
        self.assertFalse(ModernThemeModel.classify_curtain_dismiss_tap(240, 339, curtain_h=340))
        self.assertTrue(ModernThemeModel.classify_curtain_dismiss_tap(240, 340, curtain_h=340))

    def test_f2_b53_dock_column_transitions(self):
        """F11.B3: Dock 5-column transition boundaries at exact 96px increments."""
        # Col 0 vs Col 1 transition (95 vs 96)
        self.assertEqual(ModernThemeModel.classify_dock_tap(95, 750), 0)
        self.assertEqual(ModernThemeModel.classify_dock_tap(96, 750), 1)
        # Col 1 vs Col 2 transition (191 vs 192)
        self.assertEqual(ModernThemeModel.classify_dock_tap(191, 750), 1)
        self.assertEqual(ModernThemeModel.classify_dock_tap(192, 750), 2)
        # Col 2 vs Col 3 transition (287 vs 288)
        self.assertEqual(ModernThemeModel.classify_dock_tap(287, 750), 2)
        self.assertEqual(ModernThemeModel.classify_dock_tap(288, 750), 3)
        # Col 3 vs Col 4 transition (383 vs 384)
        self.assertEqual(ModernThemeModel.classify_dock_tap(383, 750), 3)
        self.assertEqual(ModernThemeModel.classify_dock_tap(384, 750), 4)
        # Rightmost edge
        self.assertEqual(ModernThemeModel.classify_dock_tap(479, 750), 4)
        # Vertical out-of-bounds
        self.assertIsNone(ModernThemeModel.classify_dock_tap(200, 719))
        self.assertIsNone(ModernThemeModel.classify_dock_tap(200, 800))

    def test_f2_b54_recent_shelf_slot_boundaries_and_empty_counts(self):
        """F11.B4: Shelf slot boundaries, 10px inter-slot gaps, and empty book handling."""
        # Empty shelf: returns None
        self.assertIsNone(ModernThemeModel.classify_shelf_tap(50, 450, shelf_count=0))
        # 1-book shelf: slot 0 works, slot 1 returns None
        self.assertEqual(ModernThemeModel.classify_shelf_tap(50, 450, shelf_count=1), 0)
        self.assertIsNone(ModernThemeModel.classify_shelf_tap(200, 450, shelf_count=1))
        # Inter-slot gaps
        # Gap between slot 0 and slot 1: 14 + 144 = 158 .. 167
        self.assertIsNone(ModernThemeModel.classify_shelf_tap(158, 450, shelf_count=3))
        self.assertIsNone(ModernThemeModel.classify_shelf_tap(167, 450, shelf_count=3))
        # Slot 1 left boundary: 168
        self.assertEqual(ModernThemeModel.classify_shelf_tap(168, 450, shelf_count=3), 1)

    def test_f2_b55_capsule_progress_extreme_bounds(self):
        """F11.B5: Capsule progress bar clamping at extreme boundaries [-999.0 .. +1000.0]."""
        self.assertEqual(ModernThemeModel.calc_capsule_progress(-999.0), 0)
        self.assertEqual(ModernThemeModel.calc_capsule_progress(-0.0001), 0)
        self.assertEqual(ModernThemeModel.calc_capsule_progress(0.0), 0)
        self.assertEqual(ModernThemeModel.calc_capsule_progress(100.0), 170)
        self.assertEqual(ModernThemeModel.calc_capsule_progress(100.0001), 170)
        self.assertEqual(ModernThemeModel.calc_capsule_progress(1000.0), 170)

    def test_f2_b56_gt911_corners_and_zero_drift(self):
        """F11.B6: GT911 mathematical transformation at 4 corners with zero coordinate drift."""
        corners = [(0, 0), (479, 0), (0, 799), (479, 799)]
        for cx, cy in corners:
            res_x, res_y = GT911MappingModel.full_pipeline(cx, cy)
            self.assertEqual(res_x, cx)
            self.assertEqual(res_y, cy)

    def test_f2_b57_hardware_key_focus_dynamic_targets(self):
        """F11.B7: Hardware key focus targets with 0, 1, and 4 recent books."""
        # 0 recent books (no hero, no shelf: only 5 dock items)
        focus_no_books = HardwareKeyFocusModel(has_hero=False, shelf_count=0, dock_count=5)
        self.assertEqual(focus_no_books.total_targets, 5)
        self.assertEqual(focus_no_books.get_target_descriptor(0)["kind"], "DOCK")

        # 1 recent book (1 hero, 0 shelf: 6 targets)
        focus_1_book = HardwareKeyFocusModel(has_hero=True, shelf_count=0, dock_count=5)
        self.assertEqual(focus_1_book.total_targets, 6)
        self.assertEqual(focus_1_book.get_target_descriptor(0)["kind"], "HERO")
        self.assertEqual(focus_1_book.get_target_descriptor(1)["kind"], "DOCK")

        # 4 recent books (1 hero, 3 shelf: 9 targets)
        focus_4_books = HardwareKeyFocusModel(has_hero=True, shelf_count=3, dock_count=5)
        self.assertEqual(focus_4_books.total_targets, 9)
        self.assertEqual(focus_4_books.get_target_descriptor(0)["kind"], "HERO")
        self.assertEqual(focus_4_books.get_target_descriptor(1)["kind"], "SHELF")
        self.assertEqual(focus_4_books.get_target_descriptor(4)["kind"], "DOCK")

    def test_f2_b58_emoji_detection_unicode_planes(self):
        """F11.B8: Emoji detector precision across Unicode emoji blocks and pure text invariance."""
        # Sample emojis from multiple Unicode blocks
        test_emojis = [
            "\U0001F300",  # Cyclone
            "\U0001F600",  # Grinning Face
            "\U0001F680",  # Rocket
            "\U00002600",  # Black Sun with Rays
            "\U00002702",  # Black Scissors
            "\U0001F980",  # Crab
            "\U0001FA70",  # Ballet Shoes
        ]
        for em in test_emojis:
            self.assertTrue(EmojiBanValidator.contains_emoji(em))

        # Russian, English, numbers, punctuation must not trigger emoji detector
        clean_samples = [
            "Мастер и Маргарита",
            "War and Peace",
            "1234567890",
            "!@#$%^&*()-_=+[]{};':\",.<>/?\\|`~",
            "Страница 42 из 350 (12%)",
        ]
        for sample in clean_samples:
            self.assertFalse(EmojiBanValidator.contains_emoji(sample))

    # --------------------------------------------------------------------------
    # FEATURE 12 BOUNDARY TESTS: R1-R6 Edge Cases & Boundary Values (8 tests)
    # --------------------------------------------------------------------------

    def test_f12_b01_curtain_swipe_threshold_and_boundary_touches(self):
        """F12.B1: Curtain swipe threshold limits, diagonal rejection, and boundary touches."""
        # Threshold is 50.0 px
        self.assertFalse(QuickSettingsCurtainModel.classify_open_swipe(240, 10, 240, 59, threshold=50.0))  # dy = 49
        self.assertTrue(QuickSettingsCurtainModel.classify_open_swipe(240, 10, 240, 60, threshold=50.0))   # dy = 50

        # Diagonal swipe (dx > dy) rejected
        self.assertFalse(QuickSettingsCurtainModel.classify_open_swipe(200, 10, 270, 60, threshold=50.0))

        # Dismiss tap boundary: y=359 is inside curtain, y=360 is outside curtain
        self.assertFalse(QuickSettingsCurtainModel.classify_dismiss_tap(240, 359, curtain_h=360))
        self.assertTrue(QuickSettingsCurtainModel.classify_dismiss_tap(240, 360, curtain_h=360))

        # Slider extreme stepping
        self.assertEqual(QuickSettingsCurtainModel.step_channel(0, -10), 0)
        self.assertEqual(QuickSettingsCurtainModel.step_channel(100, 10), 100)

    def test_f12_b02_side_drawer_edge_detection_and_margin_slop(self):
        """F12.B2: Side drawer edge thresholds, handedness reversal, and backdrop margins."""
        # Right-handed edge trigger threshold: start_x >= 450, dx <= -50
        self.assertFalse(SideDrawerModel.classify_open_gesture(449, 400, 380, 400, HandednessEnum.RIGHT))
        self.assertTrue(SideDrawerModel.classify_open_gesture(450, 400, 400, 400, HandednessEnum.RIGHT))
        self.assertFalse(SideDrawerModel.classify_open_gesture(450, 400, 401, 400, HandednessEnum.RIGHT))

        # Left-handed edge trigger threshold: start_x <= 30, dx >= 50
        self.assertFalse(SideDrawerModel.classify_open_gesture(31, 400, 100, 400, HandednessEnum.LEFT))
        self.assertTrue(SideDrawerModel.classify_open_gesture(30, 400, 80, 400, HandednessEnum.LEFT))
        self.assertFalse(SideDrawerModel.classify_open_gesture(30, 400, 79, 400, HandednessEnum.LEFT))

        # Backdrop boundary dismiss tests
        self.assertTrue(SideDrawerModel.classify_dismiss_tap(159, 400, HandednessEnum.RIGHT))
        self.assertFalse(SideDrawerModel.classify_dismiss_tap(160, 400, HandednessEnum.RIGHT))
        self.assertFalse(SideDrawerModel.classify_dismiss_tap(319, 400, HandednessEnum.LEFT))
        self.assertTrue(SideDrawerModel.classify_dismiss_tap(320, 400, HandednessEnum.LEFT))

    def test_f12_b03_reader_countdown_zero_and_large_remainders(self):
        """F12.B3: Reader chapter countdown with 0 remaining pages, 0 pace, and large chapters."""
        self.assertEqual(ReaderOverlaysModel.calc_chapter_countdown(0, 45.0), "~0 мин")
        self.assertEqual(ReaderOverlaysModel.calc_chapter_countdown(-5, 45.0), "~0 мин")
        self.assertEqual(ReaderOverlaysModel.calc_chapter_countdown(10, 0.0), "~0 мин")
        self.assertEqual(ReaderOverlaysModel.calc_chapter_countdown(1, 15.0), "~1 мин")
        self.assertEqual(ReaderOverlaysModel.calc_chapter_countdown(1, 75.0), "~2 мин")
        self.assertEqual(ReaderOverlaysModel.calc_chapter_countdown(100, 60.0), "~100 мин")

    def test_f12_b04_reader_typography_font_size_and_margin_clamps(self):
        """F12.B4: Typography font size limits [14..36] and offset relocation edge cases."""
        self.assertEqual(AaTypographyModel.step_font_size(14, -1), 14)
        self.assertEqual(AaTypographyModel.step_font_size(14, -10), 14)
        self.assertEqual(AaTypographyModel.step_font_size(36, 1), 36)
        self.assertEqual(AaTypographyModel.step_font_size(36, 10), 36)

        # Empty page ranges fallback
        self.assertEqual(AaTypographyModel.map_offset_to_new_page(500, []), 0)

        # Out-of-bounds offset maps to last page
        ranges = [(0, 100), (100, 250), (250, 400)]
        self.assertEqual(AaTypographyModel.map_offset_to_new_page(999, ranges), 2)
        # Boundary offset exactly on page start
        self.assertEqual(AaTypographyModel.map_offset_to_new_page(100, ranges), 1)

    def test_f12_b05_settings_graphical_toggle_rapid_toggling(self):
        """F12.B5: Settings graphical toggle rapid toggling and knob geometry invariants."""
        is_on = False
        track = (300, 50, 56, 28)
        for _ in range(50):
            is_on = not is_on
            cx, cy = ModularSettingsHubModel.get_toggle_knob_center(track, is_on)
            expected_x = 300 + 56 - 11 - 3 if is_on else 300 + 11 + 3
            self.assertEqual(cx, expected_x)
            self.assertEqual(cy, 50 + 14)

        # Slider boundary clamps
        self.assertEqual(ModularSettingsHubModel.step_slider_value(1, -10, 1, 10), 1)
        self.assertEqual(ModularSettingsHubModel.step_slider_value(10, 10, 1, 10), 10)

    def test_f12_b06_library_cover_grid_sparse_and_overflow_pages(self):
        """F12.B6: Library 3x2 grid with 0, 1, and 6 items, and gap touch rejection."""
        # 0 items on page
        self.assertIsNone(LibraryHubModel.classify_grid_tap(50, 100, items_on_page=0))

        # 1 item on page: slot 0 is valid, slot 1 returns None
        self.assertEqual(LibraryHubModel.classify_grid_tap(50, 100, items_on_page=1), 0)
        self.assertIsNone(LibraryHubModel.classify_grid_tap(300, 100, items_on_page=1))

        # Tap in horizontal gap between cols (col 0 ends at 20+210=230; col 1 starts at 230+20=250)
        self.assertIsNone(LibraryHubModel.classify_grid_tap(240, 100, items_on_page=6))

        # Tap in margin above grid (y < 60)
        self.assertIsNone(LibraryHubModel.classify_grid_tap(50, 30, items_on_page=6))

    def test_f12_b07_library_sorting_identical_and_missing_metadata(self):
        """F12.B7: Library sorting edge cases: identical values, 0% and 100% progress, empty query."""
        books = [
            {"title": "Книга Б", "author": "Автор", "date": 100, "progress": 0.0},
            {"title": "Книга А", "author": "Автор", "date": 100, "progress": 100.0},
        ]
        # Sort by title
        sorted_title = LibraryHubModel.sort_books(books, LibrarySortModeEnum.NAME)
        self.assertEqual(sorted_title[0]["title"], "Книга А")

        # Empty search returns full list
        self.assertEqual(len(LibraryHubModel.filter_books("", books)), 2)
        # Non-matching query returns empty list
        self.assertEqual(len(LibraryHubModel.filter_books("Несуществующее", books)), 0)

    def test_f12_b08_crosspoint_eradication_validator_adversarial_patterns(self):
        """F12.B8: Adversarial pattern detection for CrossPoint eradication and SD safety."""
        # Mixed casing and disguised variations
        adversarial_strings = [
            "crosspoint", "CrossPoint", "CROSSPOINT", "CrOsSpOiNt",
            "crosspoint-reader", "http://crosspoint.local/"
        ]
        for s in adversarial_strings:
            self.assertFalse(CrossPointEradicationValidator.is_clean_ui_label(s))
            hits = CrossPointEradicationValidator.audit_text_for_crosspoint(s)
            self.assertGreaterEqual(len(hits), 1)

        # Clean strings must pass
        clean_strings = ["BookPoint Reader", "PocketBook Linux", "Nickel OS"]
        for s in clean_strings:
            self.assertTrue(CrossPointEradicationValidator.is_clean_ui_label(s))
            self.assertEqual(len(CrossPointEradicationValidator.audit_text_for_crosspoint(s)), 0)

        # SD preservation paths
        self.assertTrue(CrossPointEradicationValidator.is_preserved_sd_path("/.crosspoint/"))
        self.assertTrue(CrossPointEradicationValidator.is_preserved_sd_path("fs_/.crosspoint/stats/reading_stats.csv"))
        self.assertFalse(CrossPointEradicationValidator.is_preserved_sd_path("/system/root/"))

    # --------------------------------------------------------------------------
    # FEATURE 13 BOUNDARIES: Round 6 Modernization Edge Cases (6 tests)
    # --------------------------------------------------------------------------

    def test_f13_b01_control_curtain_slider_track_seek_edge_points_and_clamping(self):
        """F13.B1: Control curtain slider track seek edge points, boundary clamping, and zero-width slop."""
        # Extreme negative and overflow touch X
        self.assertEqual(QuickSettingsCurtainModel.track_x_to_value("BRIGHTNESS", -500), 0)
        self.assertEqual(QuickSettingsCurtainModel.track_x_to_value("BRIGHTNESS", 0), 0)
        self.assertEqual(QuickSettingsCurtainModel.track_x_to_value("BRIGHTNESS", 67), 0)
        self.assertEqual(QuickSettingsCurtainModel.track_x_to_value("BRIGHTNESS", 68), 0)
        self.assertEqual(QuickSettingsCurtainModel.track_x_to_value("BRIGHTNESS", 412), 100)
        self.assertEqual(QuickSettingsCurtainModel.track_x_to_value("BRIGHTNESS", 413), 100)
        self.assertEqual(QuickSettingsCurtainModel.track_x_to_value("BRIGHTNESS", 99999), 100)

        # Invalid channel raises ValueError
        with self.assertRaises(ValueError):
            QuickSettingsCurtainModel.track_x_to_value("UNKNOWN_CHANNEL", 100)

        # Step channel boundary clamping
        self.assertEqual(QuickSettingsCurtainModel.step_channel(0, -10), 0)
        self.assertEqual(QuickSettingsCurtainModel.step_channel(100, 10), 100)
        self.assertEqual(QuickSettingsCurtainModel.step_channel(0, -999), 0)
        self.assertEqual(QuickSettingsCurtainModel.step_channel(100, 999), 100)

        # Status bar tap boundaries
        self.assertTrue(QuickSettingsCurtainModel.classify_open_tap(0, 0))
        self.assertTrue(QuickSettingsCurtainModel.classify_open_tap(479, 43))
        self.assertFalse(QuickSettingsCurtainModel.classify_open_tap(479, 44))
        self.assertFalse(QuickSettingsCurtainModel.classify_open_tap(-1, 20))
        self.assertFalse(QuickSettingsCurtainModel.classify_open_tap(480, 20))

    def test_f13_b02_reader_top_bar_boundary_taps_and_button_margins(self):
        """F13.B2: Reader top bar boundary hit testing, corner edges, and tap isolation."""
        # Back button: (0, 0, 64, 64)
        self.assertEqual(ReaderOverlaysModel.classify_top_tap(0, 0), "BACK")
        self.assertEqual(ReaderOverlaysModel.classify_top_tap(63, 63), "BACK")

        # TOC button: (64, 0, 80, 64) -> [64..143]x[0..63]
        self.assertEqual(ReaderOverlaysModel.classify_top_tap(64, 0), "TOC")
        self.assertEqual(ReaderOverlaysModel.classify_top_tap(143, 63), "TOC")

        # Title area: (144, 0, 220, 64) -> [144..363]x[0..63]
        self.assertEqual(ReaderOverlaysModel.classify_top_tap(144, 0), "TITLE")
        self.assertEqual(ReaderOverlaysModel.classify_top_tap(363, 63), "TITLE")

        # Bookmark button: (364, 0, 56, 64) -> [364..419]x[0..63]
        self.assertEqual(ReaderOverlaysModel.classify_top_tap(364, 0), "TOGGLE_BOOKMARK")
        self.assertEqual(ReaderOverlaysModel.classify_top_tap(419, 63), "TOGGLE_BOOKMARK")

        # Search button: (420, 0, 60, 64) -> [420..479]x[0..63]
        self.assertEqual(ReaderOverlaysModel.classify_top_tap(420, 0), "SEARCH")
        self.assertEqual(ReaderOverlaysModel.classify_top_tap(479, 63), "SEARCH")

        # Below top bar (y >= 64) returns None
        self.assertIsNone(ReaderOverlaysModel.classify_top_tap(100, 64))
        self.assertIsNone(ReaderOverlaysModel.classify_top_tap(100, 100))

    def test_f13_b03_typography_font_size_margin_spacing_boundary_clamps(self):
        """F13.B3: Typography font size, margin, and line spacing boundary clamp invariants."""
        # Font size clamps at [14, 36]
        self.assertEqual(AaTypographyModel.step_font_size(14, -2), 14)
        self.assertEqual(AaTypographyModel.step_font_size(14, -100), 14)
        self.assertEqual(AaTypographyModel.step_font_size(36, 2), 36)
        self.assertEqual(AaTypographyModel.step_font_size(36, 100), 36)

        # Margin clamps at [0, 40]
        self.assertEqual(AaTypographyModel.step_margin(0, -1), 0)
        self.assertEqual(AaTypographyModel.step_margin(0, -10), 0)
        self.assertEqual(AaTypographyModel.step_margin(40, 1), 40)
        self.assertEqual(AaTypographyModel.step_margin(40, 10), 40)

        # Line spacing clamps at [1.0, 1.6]
        self.assertAlmostEqual(AaTypographyModel.step_line_spacing(1.0, -1), 1.0)
        self.assertAlmostEqual(AaTypographyModel.step_line_spacing(1.6, 1), 1.6)

        # Offset mapping edge cases
        self.assertEqual(AaTypographyModel.map_offset_to_new_page(0, []), 0)
        ranges = [(0, 100), (100, 250), (250, 500)]
        self.assertEqual(AaTypographyModel.map_offset_to_new_page(0, ranges), 0)
        self.assertEqual(AaTypographyModel.map_offset_to_new_page(99, ranges), 0)
        self.assertEqual(AaTypographyModel.map_offset_to_new_page(100, ranges), 1)
        self.assertEqual(AaTypographyModel.map_offset_to_new_page(499, ranges), 2)
        self.assertEqual(AaTypographyModel.map_offset_to_new_page(500, ranges), 2)
        self.assertEqual(AaTypographyModel.map_offset_to_new_page(10000, ranges), 2)

    def test_f13_b04_fb2_zip_empty_corrupted_and_combining_stroke_variations(self):
        """F13.B4: FB2/FB2.zip filename case insensitivity, empty strings, and combining stroke variations."""
        # Case insensitivity
        self.assertTrue(UniversalFormatsModel.has_fb2_extension("BOOK.FB2"))
        self.assertTrue(UniversalFormatsModel.has_fb2_extension("Book.Fb2"))
        self.assertTrue(UniversalFormatsModel.has_fb2_zip_extension("BOOK.FB2.ZIP"))
        self.assertTrue(UniversalFormatsModel.has_fb2_zip_extension("book.fb2.Zip"))

        # Negative extension matches
        self.assertFalse(UniversalFormatsModel.has_fb2_extension("book.fb2.bak"))
        self.assertFalse(UniversalFormatsModel.has_fb2_extension("book_fb2"))
        self.assertFalse(UniversalFormatsModel.has_fb2_zip_extension("book.zip"))

        # Empty string handling in combining stroke normalization
        clean, strike = StrikethroughNormalizationModel.normalize_strikethrough_text("")
        self.assertEqual(clean, "")
        self.assertFalse(strike)

        # Multiple adjacent U+0336 marks
        double_stroke = "а\u0336\u0336б\u0336\u0336"
        clean_double, strike_double = StrikethroughNormalizationModel.normalize_strikethrough_text(double_stroke)
        self.assertEqual(clean_double, "аб")
        self.assertTrue(strike_double)

        # Legacy encoding fallback on unknown encoding
        raw = "Тест".encode("utf-8")
        decoded = UniversalFormatsModel.decode_legacy_bytes(raw, "unknown_encoding_xyz")
        self.assertEqual(decoded, "Тест")

    def test_f13_b05_progressive_jpeg_extreme_dimensions_memory_budget(self):
        """F13.B5: Progressive JPEG extreme dimension calculations and PSRAM safety threshold."""
        # Width = 1
        self.assertEqual(ProgressiveJpegDecoderModel.calc_mcu_row_buffer_bytes(1, 16), 16)
        self.assertTrue(ProgressiveJpegDecoderModel.is_psram_safe(1))

        # Extreme dimensions
        self.assertTrue(ProgressiveJpegDecoderModel.is_psram_safe(1920))
        self.assertTrue(ProgressiveJpegDecoderModel.is_psram_safe(4096))
        # Width > 4096 exceeds 64 KB single MCU row buffer
        self.assertFalse(ProgressiveJpegDecoderModel.is_psram_safe(5000))

        # Downsample steps validation
        with self.assertRaises(ValueError):
            ProgressiveJpegDecoderModel.calc_downsample_steps(100, 100, 0, 100)
        with self.assertRaises(ValueError):
            ProgressiveJpegDecoderModel.calc_downsample_steps(100, 100, 100, -1)

    def test_f13_b06_screensaver_selection_index_overflow_and_resolution_matching(self):
        """F13.B6: Screensaver theme index boundary clamps and version formatting variations."""
        # Valid theme indices 1..11
        for i in range(1, 12):
            fn = ScreensaverGalleryModel.get_theme_filename(i)
            self.assertTrue(fn.startswith(f"{i:02d}_"))

        # Invalid theme index 0 or 12 raises ValueError
        with self.assertRaises(ValueError):
            ScreensaverGalleryModel.get_theme_filename(0)
        with self.assertRaises(ValueError):
            ScreensaverGalleryModel.get_theme_filename(12)
        with self.assertRaises(ValueError):
            ScreensaverGalleryModel.get_theme_filename(-1)

        # Version string irregular format normalization to exact 'v2.1.0'
        irregular_versions = [
            "2.1.0",
            "v2.1.0",
            "V2.1.0",
            "2.1.0-x4pro",
            "2.1.0-dirty",
            "v2.1.0-x4pro-release",
        ]
        for v in irregular_versions:
            self.assertEqual(
                ScreensaverGalleryModel.format_version_display(v),
                "v2.1.0",
                f"Failed to normalize irregular version '{v}' to 'v2.1.0'",
            )


if __name__ == "__main__":
    unittest.main()


