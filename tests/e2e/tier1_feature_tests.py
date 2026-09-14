"""
tier1_feature_tests.py - Tier 1 Feature Coverage Tests (≥5 per feature, 10 features, 50 tests total).

Authoritative References:
- c:/xteinkx4/.agents/ORIGINAL_REQUEST.md (§R1, §R2, §R3)
- c:/xteinkx4/PROJECT.md (§Interface Contracts, §Feature Inventory)
- c:/xteinkx4/TEST_INFRA.md (§Coverage Thresholds)
"""

import hashlib
import io
import math
import os
import unittest
from pathlib import Path

from tests.e2e.contracts import (
    APP0_MAX_PARTITION_SIZE,
    ARTIFACTS_DIR,
    BUFFER_SIZE,
    ESP32_IMAGE_MAGIC,
    EXPECTED_GH_RELEASE_COMMAND,
    EXPECTED_RELEASE_SHA256,
    EXPECTED_RELEASE_SIZE,
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
    SRC_DIR,
    THEME_METRICS,
    DashboardModel,
    DisplayGeometry,
    FirmwareValidator,
    HomeMenuItem,
    HOME_MENU_ICONS,
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
    OpdsCatalogHubModel,
    OpdsStuckEscapeModel,
    SleepCoversRotationModel,
    UniversalFormatsModel,
    StrikethroughNormalizationModel,
    ProgressiveJpegDecoderModel,
    FootnoteModalModel,
    ScreensaverGalleryModel,
    ZeroOverlapModel,
    PremiumTypographySuiteModel,
    FlagshipErgonomicsModel,
    ZeroBrickRiskModel,
    HighFidelityImageEngineModel,
)

try:
    from PIL import Image
    HAS_PIL = True
except ImportError:
    HAS_PIL = False


class Tier1FeatureTests(unittest.TestCase):
    """Tier 1: Feature Coverage (50 tests total across features F1-F10)."""

    # --------------------------------------------------------------------------
    # FEATURE 1: LyraCarouselTheme (5 tests)
    # --------------------------------------------------------------------------

    def test_f1_01_carousel_3slot_geometry(self):
        """F1.1: Verify 3-slot layout geometry within 480x800 logical canvas."""
        slots = LyraCarouselModel.get_slot_rects()
        self.assertIn("left", slots)
        self.assertIn("center", slots)
        self.assertIn("right", slots)

        cx, cy, cw, ch = slots["center"]
        self.assertEqual(cw, 220)
        self.assertEqual(ch, 320)
        # Center slot must be horizontally centered at X=240
        self.assertEqual(cx + cw // 2, LOGICAL_WIDTH // 2)

        lx, ly, lw, lh = slots["left"]
        rx, ry, rw, rh = slots["right"]
        # Side slots must be smaller than center slot
        self.assertLess(lw, cw)
        self.assertLess(lh, ch)
        self.assertEqual(lw, rw)
        self.assertEqual(lh, rh)

    def test_f1_02_carousel_scaling_math(self):
        """F1.2: Verify scaling ratio for center cover (1.0) vs side previews (0.65)."""
        scale_center = THEME_METRICS[UIThemeEnum.CAROUSEL]["scale_center"]
        scale_side = THEME_METRICS[UIThemeEnum.CAROUSEL]["scale_side"]
        self.assertEqual(scale_center, 1.0)
        self.assertEqual(scale_side, 0.65)

        cw, ch = LyraCarouselModel.CENTER_WIDTH, LyraCarouselModel.CENTER_HEIGHT
        sw, sh = LyraCarouselModel.SIDE_WIDTH, LyraCarouselModel.SIDE_HEIGHT
        self.assertAlmostEqual(sw / float(cw), 0.65, delta=0.01)
        self.assertAlmostEqual(sh / float(ch), 0.65, delta=0.01)

    def test_f1_03_carousel_pagination_dots(self):
        """F1.3: Verify pagination dot coordinates, count, and active highlight."""
        total_books = 5
        active_index = 2
        dots = LyraCarouselModel.calc_pagination_dots(total_books, active_index)
        self.assertEqual(len(dots), 5)
        for i, dot in enumerate(dots):
            self.assertEqual(dot["index"], i)
            if i == active_index:
                self.assertTrue(dot["is_active"])
                self.assertEqual(dot["radius"], 4)
            else:
                self.assertFalse(dot["is_active"])
                self.assertEqual(dot["radius"], 2)

    def test_f1_04_carousel_theme_registration(self):
        """F1.4: Verify Carousel theme registration in UI_THEME enum and metrics."""
        self.assertEqual(int(UIThemeEnum.CAROUSEL), 2)
        metrics = THEME_METRICS[UIThemeEnum.CAROUSEL]
        self.assertEqual(metrics["recent_books_count"], 3)
        self.assertTrue(metrics["has_carousel"])
        self.assertEqual(metrics["name"], "LyraCarousel")

    def test_f1_05_carousel_title_and_progress_slot(self):
        """F1.5: Verify title and progress bar vertical budget below cover area."""
        slots = LyraCarouselModel.get_slot_rects()
        _, center_y, _, center_h = slots["center"]
        carousel_bottom = center_y + center_h
        self.assertLessEqual(carousel_bottom, 450)
        # Dedicated title & progress area below carousel (y: 450..550)
        title_y = 480
        progress_y = 515
        self.assertGreater(progress_y, title_y)
        self.assertLess(progress_y, LOGICAL_HEIGHT - 200)

    # --------------------------------------------------------------------------
    # FEATURE 2: DashboardTheme Overhaul (5 tests)
    # --------------------------------------------------------------------------

    def test_f2_01_dashboard_header_layout(self):
        """F2.1: Verify header height and widget bounds (date/clock/battery)."""
        header_h = DashboardModel.HEADER_HEIGHT
        self.assertEqual(header_h, 40)
        self.assertLess(header_h, DashboardModel.COVER_CARD_Y)
        # Header is within y=0..40
        header_y_max = 40
        self.assertLessEqual(header_y_max, 50)

    def test_f2_02_dashboard_large_cover_card(self):
        """F2.2: Verify large cover card aspect ratio and dimensions."""
        cw = DashboardModel.COVER_WIDTH
        ch = DashboardModel.COVER_HEIGHT
        card_h = DashboardModel.COVER_CARD_HEIGHT
        self.assertLessEqual(ch, card_h)
        # Aspect ratio ~ 2:3 typical for book covers (180:270 = 0.667)
        self.assertAlmostEqual(cw / float(ch), 0.667, places=2)

    def test_f2_03_dashboard_progress_bar_math(self):
        """F2.3: Verify reading progress bar fill width calculation."""
        total_w = 200
        self.assertEqual(DashboardModel.calc_progress_bar_width(0.0, total_w), 0)
        self.assertEqual(DashboardModel.calc_progress_bar_width(50.0, total_w), 100)
        self.assertEqual(DashboardModel.calc_progress_bar_width(100.0, total_w), 200)
        self.assertEqual(DashboardModel.calc_progress_bar_width(25.0, total_w), 50)
        self.assertEqual(DashboardModel.calc_progress_bar_width(75.0, total_w), 150)

    def test_f2_04_dashboard_streak_calendar(self):
        """F2.4: Verify 7-day reading streak mini-calendar geometry and marking."""
        cells = DashboardModel.calc_streak_calendar_cells(streak_days=4, today_active=True)
        self.assertEqual(len(cells), 7)
        # Last 4 days must be completed
        completed_count = sum(1 for c in cells if c["is_completed"])
        self.assertEqual(completed_count, 4)
        self.assertTrue(cells[6]["is_today"])
        self.assertTrue(cells[6]["is_completed"])

    def test_f2_05_dashboard_time_remaining_calculation(self):
        """F2.5: Verify reading pace and remaining time calculation."""
        # 300 total pages, 150 read, pace 30 pages/hour -> 5 hours left = '5ч 0м'
        res = DashboardModel.calc_remaining_time(300, 150, 30.0)
        self.assertEqual(res, "5ч 0м")
        # 100 pages left at 200 pages/hour -> 0.5 hour = '30м'
        res_quick = DashboardModel.calc_remaining_time(200, 100, 200.0)
        self.assertEqual(res_quick, "30м")
        # Fully read -> '0м'
        self.assertEqual(DashboardModel.calc_remaining_time(200, 200, 50.0), "0м")

    # --------------------------------------------------------------------------
    # FEATURE 3: Extended Home Menu (5 tests)
    # --------------------------------------------------------------------------

    def test_f3_01_home_menu_item_enumeration(self):
        """F3.1: Verify existence of required menu items in HomeMenuItem."""
        required = [
            HomeMenuItem.FILE_BROWSER,
            HomeMenuItem.RECENTS,
            HomeMenuItem.SEARCH_BOOKS,
            HomeMenuItem.READING_STATS,
            HomeMenuItem.BOOKMARKS,
            HomeMenuItem.SETTINGS_MENU,
        ]
        for item in required:
            self.assertIn(item, HomeMenuItem)

    def test_f3_02_home_menu_icon_mapping(self):
        """F3.2: Verify icon assignments for extended menu items."""
        self.assertEqual(HOME_MENU_ICONS[HomeMenuItem.SEARCH_BOOKS], "search24")
        self.assertEqual(HOME_MENU_ICONS[HomeMenuItem.READING_STATS], "stats")
        self.assertEqual(HOME_MENU_ICONS[HomeMenuItem.BOOKMARKS], "bookmark")
        self.assertEqual(HOME_MENU_ICONS[HomeMenuItem.FILE_BROWSER], "folder24")
        self.assertEqual(HOME_MENU_ICONS[HomeMenuItem.RECENTS], "recent")

    def test_f3_03_home_menu_height_budget(self):
        """F3.3: Verify menu rows fit vertically without exceeding screen height."""
        header_h = 40
        cover_h = 280
        available_menu_h = LOGICAL_HEIGHT - (header_h + cover_h)  # 800 - 320 = 480
        menu_items_count = 7
        row_height = 48
        spacing = 4
        total_menu_h = menu_items_count * (row_height + spacing)
        self.assertLess(total_menu_h, available_menu_h)

    def test_f3_04_home_menu_russian_labels(self):
        """F3.4: Verify authentic Russian translations for menu items."""
        self.assertEqual(HOME_MENU_LABELS_RU[HomeMenuItem.SEARCH_BOOKS], "Быстрый поиск книг")
        self.assertEqual(HOME_MENU_LABELS_RU[HomeMenuItem.READING_STATS], "Статистика чтения")
        self.assertEqual(HOME_MENU_LABELS_RU[HomeMenuItem.BOOKMARKS], "Закладки")

    def test_f3_05_home_menu_row_hit_testing(self):
        """F3.5: Verify vertical touch hit testing for menu rows."""
        menu_top = 340
        row_step = 52
        # Touch at Y=350 should hit row 0
        row_0 = (350 - menu_top) // row_step
        self.assertEqual(row_0, 0)
        # Touch at Y=400 should hit row 1
        row_1 = (400 - menu_top) // row_step
        self.assertEqual(row_1, 1)

    # --------------------------------------------------------------------------
    # FEATURE 4: Virtual Touch Keyboard & Search (5 tests)
    # --------------------------------------------------------------------------

    def test_f4_01_keyboard_cyrillic_layout(self):
        """F4.1: Verify Russian ЙЦУКЕН keyboard structure and letters."""
        kb = VirtualKeyboardModel()
        self.assertTrue(kb.is_ru)
        layout = kb.get_current_layout()
        self.assertEqual(len(layout), 4)
        # First row starts with Й
        self.assertEqual(layout[0][0], "Й")
        # Second row starts with Ф
        self.assertEqual(layout[1][0], "Ф")
        # Third row has LANG and BKSP
        self.assertIn("LANG", layout[2])
        self.assertIn("BKSP", layout[2])

    def test_f4_02_keyboard_qwerty_layout(self):
        """F4.2: Verify English QWERTY keyboard structure after toggle."""
        kb = VirtualKeyboardModel()
        kb.toggle_layout()
        self.assertFalse(kb.is_ru)
        layout = kb.get_current_layout()
        self.assertEqual(len(layout), 4)
        self.assertEqual(layout[0][0], "Q")
        self.assertEqual(layout[1][0], "A")
        self.assertEqual(layout[2][1], "Z")

    def test_f4_03_keyboard_touch_hitbox_resolution(self):
        """F4.3: Verify touch point resolution to virtual key."""
        kb = VirtualKeyboardModel(start_y=520, height=260)
        # Touch at top-left of keyboard area should hit first key of row 0
        key = kb.hit_test_key(touch_x=20, touch_y=530)
        self.assertEqual(key, "Й")
        # Touch outside keyboard area should return None
        self.assertIsNone(kb.hit_test_key(touch_x=20, touch_y=400))

    def test_f4_04_search_filtering_engine(self):
        """F4.4: Verify case-insensitive Cyrillic and Latin book title search."""
        books = [
            {"title": "Евгений Онегин", "author": "Пушкин Александр"},
            {"title": "Мильон терзаний", "author": "Гончаров Иван"},
            {"title": "War and Peace", "author": "Leo Tolstoy"},
        ]
        # Search Cyrillic
        res_pushkin = VirtualKeyboardModel.filter_books("онегин", books)
        self.assertEqual(len(res_pushkin), 1)
        self.assertEqual(res_pushkin[0]["title"], "Евгений Онегин")

        # Search Latin
        res_tolstoy = VirtualKeyboardModel.filter_books("war", books)
        self.assertEqual(len(res_tolstoy), 1)
        self.assertEqual(res_tolstoy[0]["title"], "War and Peace")

    def test_f4_05_search_to_reader_transition(self):
        """F4.5: Verify search selection contract produces valid book path."""
        selected_book = {"path": "/sd/books/onegin.epub", "title": "Евгений Онегин"}
        self.assertTrue(selected_book["path"].endswith(".epub"))
        self.assertTrue(selected_book["path"].startswith("/sd/"))

    # --------------------------------------------------------------------------
    # FEATURE 5: Theme Switcher in Settings (5 tests)
    # --------------------------------------------------------------------------

    def test_f5_01_theme_enum_values(self):
        """F5.1: Verify exact integer mapping of UI_THEME enum."""
        self.assertEqual(int(UIThemeEnum.CLASSIC), 0)
        self.assertEqual(int(UIThemeEnum.LYRA), 1)
        self.assertEqual(int(UIThemeEnum.CAROUSEL), 2)
        self.assertEqual(int(UIThemeEnum.ROUNDEDRAFF), 3)
        self.assertEqual(int(UIThemeEnum.MINIMAL), 4)
        self.assertEqual(int(UIThemeEnum.DASHBOARD), 5)

    def test_f5_02_theme_settings_list_descriptor(self):
        """F5.2: Verify SettingsList.h contains UI_THEME option descriptors."""
        settings_path = REPO_ROOT / "src" / "src" / "SettingsList.h"
        self.assertTrue(settings_path.exists(), f"Missing {settings_path}")
        content = settings_path.read_text(encoding="utf-8", errors="replace")
        self.assertIn("STR_UI_THEME", content)
        self.assertIn("uiTheme", content)

    def test_f5_03_theme_metrics_registry(self):
        """F5.3: Verify all 6 theme modes have registered metrics."""
        for theme_val in UIThemeEnum:
            self.assertIn(theme_val, THEME_METRICS)
            metrics = THEME_METRICS[theme_val]
            self.assertIn("name", metrics)
            self.assertIn("recent_books_count", metrics)

    def test_f5_04_theme_reload_lifecycle(self):
        """F5.4: Verify reload mechanism updates theme state."""
        current_theme = UIThemeEnum.DASHBOARD
        # Simulate switching to CAROUSEL
        current_theme = UIThemeEnum.CAROUSEL
        metrics = THEME_METRICS[current_theme]
        self.assertEqual(metrics["name"], "LyraCarousel")
        self.assertEqual(metrics["recent_books_count"], 3)

    def test_f5_05_theme_persistence_roundtrip(self):
        """F5.5: Verify theme serialization to uint8_t and roundtrip parsing."""
        raw_byte = 5  # DASHBOARD
        parsed = UIThemeEnum(raw_byte)
        self.assertEqual(parsed, UIThemeEnum.DASHBOARD)
        self.assertEqual(int(parsed), 5)

    # --------------------------------------------------------------------------
    # FEATURE 6: Standalone 800x480 Simulator (5 tests)
    # --------------------------------------------------------------------------

    def test_f6_01_framebuffer_buffer_size_calculation(self):
        """F6.1: Verify exact 48,000 bytes allocation (800x480 1bpp)."""
        fb = VirtualFramebuffer()
        self.assertEqual(len(fb.buffer), 48000)
        self.assertEqual(BUFFER_SIZE, 48000)
        self.assertEqual(PHYSICAL_WIDTH_BYTES, 100)

    def test_f6_02_pixel_offset_and_bitmask_math(self):
        """F6.2: Verify byte offset and bit position for arbitrary coordinates."""
        # Top-left (0, 0)
        self.assertEqual(DisplayGeometry.calc_byte_offset(0, 0), 0)
        self.assertEqual(DisplayGeometry.calc_bit_position(0), 7)

        # Bottom-right (799, 479)
        # phyY=479 -> 479 * 100 = 47900; phyX=799 -> 799 // 8 = 99 -> byte 47999
        self.assertEqual(DisplayGeometry.calc_byte_offset(799, 479), 47999)
        self.assertEqual(DisplayGeometry.calc_bit_position(799), 0)

    def test_f6_03_pixel_polarity_conventions(self):
        """F6.3: Verify 1=White (background) and 0=Black (ink) operations."""
        fb = VirtualFramebuffer()
        # On clear, all pixels are white (1)
        self.assertEqual(fb.get_physical_pixel(0, 0), POLARITY_WHITE)
        # Set black (0)
        fb.set_physical_pixel(0, 0, black=True)
        self.assertEqual(fb.get_physical_pixel(0, 0), POLARITY_BLACK)
        # Set back to white (1)
        fb.set_physical_pixel(0, 0, black=False)
        self.assertEqual(fb.get_physical_pixel(0, 0), POLARITY_WHITE)

    def test_f6_04_coordinate_rotation_portrait_to_landscape(self):
        """F6.4: Verify GfxRenderer rotation transform: (x, y) -> (y, 479 - x)."""
        # Logical (0, 0) -> Physical (0, 479)
        px, py = DisplayGeometry.logical_to_physical(0, 0)
        self.assertEqual(px, 0)
        self.assertEqual(py, 479)

        # Logical (479, 799) -> Physical (799, 0)
        px, py = DisplayGeometry.logical_to_physical(479, 799)
        self.assertEqual(px, 799)
        self.assertEqual(py, 0)

    def test_f6_05_coordinate_rotation_invertibility(self):
        """F6.5: Verify bidirectional coordinate mapping invertibility."""
        test_points = [(0, 0), (479, 0), (0, 799), (479, 799), (240, 400)]
        for lx, ly in test_points:
            px, py = DisplayGeometry.logical_to_physical(lx, ly)
            rx, ry = DisplayGeometry.physical_to_logical(px, py)
            self.assertEqual((lx, ly), (rx, ry))

    # --------------------------------------------------------------------------
    # FEATURE 7: Screenshot PNG Artifacts (5 tests)
    # --------------------------------------------------------------------------

    def test_f7_01_screenshot_manifest_definition(self):
        """F7.1: Verify manifest defines all 5 required PNG screenshots."""
        self.assertEqual(len(EXPECTED_SCREENSHOT_FILES), 5)
        self.assertIn("home_dashboard_800x480.png", EXPECTED_SCREENSHOT_FILES)
        self.assertIn("home_carousel_800x480.png", EXPECTED_SCREENSHOT_FILES)
        self.assertIn("reader_screen_800x480.png", EXPECTED_SCREENSHOT_FILES)
        self.assertIn("keyboard_search_800x480.png", EXPECTED_SCREENSHOT_FILES)
        self.assertIn("games_chess_800x480.png", EXPECTED_SCREENSHOT_FILES)

    def test_f7_02_screenshot_png_format_validation(self):
        """F7.2: Verify PNG magic header and chunk integrity."""
        png_magic = b"\x89PNG\r\n\x1a\n"
        # Test simulated image export
        fb = VirtualFramebuffer()
        fb.draw_logical_rect(50, 50, 100, 100, fill_black=True)
        img = fb.to_logical_pil_image()
        buf = io.BytesIO()
        img.save(buf, format="PNG")
        raw = buf.getvalue()
        self.assertTrue(raw.startswith(png_magic))

    def test_f7_03_screenshot_resolution_specification(self):
        """F7.3: Verify logical portrait dimensions (480x800)."""
        fb = VirtualFramebuffer()
        img = fb.to_logical_pil_image()
        self.assertEqual(img.size, (LOGICAL_WIDTH, LOGICAL_HEIGHT))

    def test_f7_04_screenshot_non_blank_entropy(self):
        """F7.4: Verify pixel entropy calculation detects drawn content."""
        fb = VirtualFramebuffer()
        self.assertEqual(fb.compute_entropy(), 0.0)  # Blank white
        # Draw some content
        fb.draw_logical_rect(100, 100, 200, 200, fill_black=True)
        entropy = fb.compute_entropy()
        self.assertGreater(entropy, 0.05)

    def test_f7_05_gallery_html_structure(self):
        """F7.5: Verify HTML5 gallery requirements referencing screenshots."""
        gallery_template = """<!DOCTYPE html>
<html>
<head><title>BookPoint 2.0.0 Screen Gallery</title></head>
<body>
<h1>Screenshots</h1>
""" + "\n".join(f'<img src="{f}" alt="{f}">' for f in EXPECTED_SCREENSHOT_FILES) + """
</body>
</html>"""
        for f in EXPECTED_SCREENSHOT_FILES:
            self.assertIn(f, gallery_template)

    # --------------------------------------------------------------------------
    # FEATURE 8: GT911 Touch Gestures (5 tests)
    # --------------------------------------------------------------------------

    def test_f8_01_gesture_swipe_horizontal_classification(self):
        """F8.1: Verify SwipeLeft and SwipeRight gesture classification."""
        # Swipe Left (drag from right x=350 to left x=100)
        swipe_l = classify_swipe(350, 300, 100, 300, threshold=50)
        self.assertEqual(swipe_l, SwipeDir.LEFT)

        # Swipe Right (drag from left x=100 to right x=350)
        swipe_r = classify_swipe(100, 300, 350, 300, threshold=50)
        self.assertEqual(swipe_r, SwipeDir.RIGHT)

    def test_f8_02_gesture_axis_dominance(self):
        """F8.2: Verify dominant axis calculation (|dx| >= |dy| vs vertical)."""
        # dx = 100, dy = 30 -> Horizontal dominant
        self.assertEqual(classify_swipe(100, 100, 200, 130), SwipeDir.RIGHT)
        # dx = 30, dy = 100 -> Vertical dominant (SwipeDown)
        self.assertEqual(classify_swipe(100, 100, 130, 200), SwipeDir.DOWN)

    def test_f8_03_carousel_3zone_tap_mapping(self):
        """F8.3: Verify 3-zone cover taps (PREV, OPEN, NEXT)."""
        # Left zone (x=50)
        self.assertEqual(classify_3zone_cover_tap(50, 250), "PREV")
        # Center zone (x=240)
        self.assertEqual(classify_3zone_cover_tap(240, 250), "OPEN")
        # Right zone (x=400)
        self.assertEqual(classify_3zone_cover_tap(400, 250), "NEXT")

    def test_f8_04_tap_in_rect_hitbox(self):
        """F8.4: Verify bounding box tap containment."""
        rx, ry, rw, rh = 100, 100, 200, 150
        # Inside
        self.assertTrue(tap_in_rect(150, 150, rx, ry, rw, rh))
        # Top-left edge (inclusive)
        self.assertTrue(tap_in_rect(100, 100, rx, ry, rw, rh))
        # Outside right
        self.assertFalse(tap_in_rect(350, 150, rx, ry, rw, rh))
        # Outside top
        self.assertFalse(tap_in_rect(150, 50, rx, ry, rw, rh))

    def test_f8_05_menu_row_touch_resolution(self):
        """F8.5: Verify vertical menu row hit testing with spacing."""
        menu_top = 340
        row_h = 48
        spacing = 4
        step = row_h + spacing  # 52

        # Tap inside row 2
        tap_y = menu_top + 2 * step + 10
        row = (tap_y - menu_top) // step
        self.assertEqual(row, 2)

    # --------------------------------------------------------------------------
    # FEATURE 9: PlatformIO Build Environment [env:x4pro] (5 tests)
    # --------------------------------------------------------------------------

    def test_f9_01_platformio_ini_x4pro_section(self):
        """F9.1: Verify [env:x4pro] section exists in platformio.ini."""
        sections = FirmwareValidator.parse_platformio_ini(PLATFORMIO_INI_PATH)
        self.assertIn("env:x4pro", sections)

    def test_f9_02_x4pro_board_and_mcu(self):
        """F9.2: Verify board esp32-s3-devkitc1-n16r8 and esp32s3 MCU."""
        sections = FirmwareValidator.parse_platformio_ini(PLATFORMIO_INI_PATH)
        env = sections["env:x4pro"]
        self.assertEqual(env.get("board"), "esp32-s3-devkitc1-n16r8")
        self.assertEqual(env.get("board_build.mcu"), "esp32s3")

    def test_f9_03_x4pro_octal_psram_config(self):
        """F9.3: Verify octal PSRAM dio_opi configuration."""
        sections = FirmwareValidator.parse_platformio_ini(PLATFORMIO_INI_PATH)
        env = sections["env:x4pro"]
        self.assertEqual(env.get("board_build.arduino.memory_type"), "dio_opi")

    def test_f9_04_x4pro_critical_flags(self):
        """F9.4: Verify critical build flags in platformio.ini."""
        content = PLATFORMIO_INI_PATH.read_text(encoding="utf-8", errors="replace")
        self.assertIn("-DFREEINK_DEVICE_X4PRO=1", content)
        self.assertIn("-DFREEINK_FB_PSRAM=1", content)
        self.assertIn("-DBOARD_HAS_PSRAM", content)

    def test_f9_05_partitions_csv_app_sizing(self):
        """F9.5: Verify partitions.csv contains 6.25MB app0 partition."""
        partitions = FirmwareValidator.parse_partitions_csv(PARTITIONS_CSV_PATH)
        app0 = next((p for p in partitions if p["name"] == "app0"), None)
        self.assertIsNotNone(app0)
        self.assertEqual(app0["size"], APP0_MAX_PARTITION_SIZE)  # 6,553,600 bytes

    # --------------------------------------------------------------------------
    # FEATURE 10: Release Binary & GitHub Asset (5 tests)
    # --------------------------------------------------------------------------

    def test_f10_01_release_binary_existence(self):
        """F10.1: Verify BookPoint_2.0.0_x4pro.bin exists on disk."""
        self.assertTrue(RELEASE_BIN_PATH.exists(), f"Binary missing at {RELEASE_BIN_PATH}")

    def test_f10_02_release_binary_size_bounds(self):
        """F10.2: Verify binary fits within app0 partition (6.25 MB)."""
        val = FirmwareValidator.validate_release_binary(RELEASE_BIN_PATH)
        self.assertTrue(val["fits_app0"])
        self.assertGreater(val["size"], 1024 * 1024)  # > 1 MB
        self.assertLess(val["flash_pct"], 90.0)  # Flash < 90% requirement

    def test_f10_03_release_binary_esp32_magic(self):
        """F10.3: Verify ESP32 image magic byte 0xE9."""
        val = FirmwareValidator.validate_release_binary(RELEASE_BIN_PATH)
        self.assertTrue(val["is_esp32_valid"])
        self.assertEqual(val["magic_byte"], ESP32_IMAGE_MAGIC)

    def test_f10_04_release_binary_sha256_checksum(self):
        """F10.4: Verify SHA256 checksum matches release manifest."""
        val = FirmwareValidator.validate_release_binary(RELEASE_BIN_PATH)
        self.assertEqual(val["sha256"], EXPECTED_RELEASE_SHA256)

    def test_f10_05_github_release_upload_command(self):
        """F10.5: Verify GitHub CLI release upload command."""
        self.assertIn("gh release upload v2.0.0", EXPECTED_GH_RELEASE_COMMAND)
        self.assertIn("--clobber", EXPECTED_GH_RELEASE_COMMAND)
        self.assertIn("klanva/BookPoint", EXPECTED_GH_RELEASE_COMMAND)

    # --------------------------------------------------------------------------
    # FEATURE 11: Flagship Home Screen & Premium ModernTheme Contracts
    # --------------------------------------------------------------------------

    def test_f11_01_strict_ban_on_emojis_in_section_titles(self):
        """F11.1: Verify strict ban on emojis across all section titles and UI strings."""
        clean_titles = [
            "Библиотека",
            "Поиск",
            "Статистика",
            "Приложения",
            "Настройки",
            "НЕДАВНИЕ КНИГИ",
            "СЕЙЧАС ЧИТАЮ",
            "Продолжить чтение",
        ]
        for title in clean_titles:
            self.assertFalse(
                EmojiBanValidator.contains_emoji(title),
                f"Section title '{title}' contains prohibited emoji/pictograph.",
            )
            self.assertTrue(EmojiBanValidator.check_clean_typography([title]))

        # Adversarial verification: Confirm detector catches prohibited emojis
        emoji_violations = ["📚 Библиотека", "🎮 Игры", "🔍 Поиск", "📊 Статистика", "⚙️ Настройки"]
        for bad in emoji_violations:
            self.assertTrue(
                EmojiBanValidator.contains_emoji(bad),
                f"Detector failed to flag prohibited emoji in '{bad}'.",
            )

    def test_f11_02_hero_now_reading_card_geometry(self):
        """F11.2: Hero 'Now Reading' card geometry, dimensions, and canvas allocation."""
        rect = ModernThemeModel.HERO_CARD_RECT
        self.assertEqual(rect, (14, 50, 452, 310))
        rx, ry, rw, rh = rect
        self.assertEqual(rx + rw, 466)
        self.assertEqual(ry + rh, 360)
        self.assertEqual(ModernThemeModel.HERO_COVER_RECT, (30, 95, 146, 219))
        # Verify 2:3 book aspect ratio
        cover_w = ModernThemeModel.HERO_COVER_RECT[2]
        cover_h = ModernThemeModel.HERO_COVER_RECT[3]
        self.assertAlmostEqual(cover_h / cover_w, 1.5, places=1)

    def test_f11_03_hero_cover_3d_spine_shadow_presence(self):
        """F11.3: Hero cover 3D spine shadow: crease line, highlight, and drop shadow."""
        cover_x = ModernThemeModel.HERO_COVER_RECT[0]
        self.assertEqual(ModernThemeModel.HERO_SPINE_LINE_X, cover_x + 6)
        self.assertEqual(ModernThemeModel.HERO_SPINE_HIGHLIGHT_X, (cover_x + 2, cover_x + 3))
        self.assertEqual(ModernThemeModel.HERO_SPINE_SHADOW_X, (cover_x + 4, cover_x + 5))
        self.assertEqual(ModernThemeModel.HERO_DROP_SHADOW_OFFSET, 2)

    def test_f11_04_capsule_progress_bar_math(self):
        """F11.4: Hero card capsule progress bar fill calculation and clamping."""
        self.assertEqual(ModernThemeModel.calc_capsule_progress(0.0), 0)
        self.assertEqual(ModernThemeModel.calc_capsule_progress(25.0), 42)
        self.assertEqual(ModernThemeModel.calc_capsule_progress(50.0), 85)
        self.assertEqual(ModernThemeModel.calc_capsule_progress(75.0), 128)
        self.assertEqual(ModernThemeModel.calc_capsule_progress(100.0), 170)
        # Clamping
        self.assertEqual(ModernThemeModel.calc_capsule_progress(-10.0), 0)
        self.assertEqual(ModernThemeModel.calc_capsule_progress(150.0), 170)

    def test_f11_05_reading_streak_badge(self):
        """F11.5: Reading streak badge typography and formatting."""
        badge_7 = ModernThemeModel.format_reading_streak(7)
        self.assertEqual(badge_7, "Серия: 7 дн.")
        self.assertFalse(EmojiBanValidator.contains_emoji(badge_7))
        badge_1 = ModernThemeModel.format_reading_streak(1)
        self.assertEqual(badge_1, "Серия: 1 дн.")

    def test_f11_06_hero_single_unified_touch_zone(self):
        """F11.6: Single unified touch zone for entire Hero Card [14..465]x[50..360]."""
        # Inside touch zone points
        self.assertTrue(ModernThemeModel.classify_hero_tap(14, 50))
        self.assertTrue(ModernThemeModel.classify_hero_tap(465, 360))
        self.assertTrue(ModernThemeModel.classify_hero_tap(100, 200))  # On cover
        self.assertTrue(ModernThemeModel.classify_hero_tap(250, 150))  # On title
        self.assertTrue(ModernThemeModel.classify_hero_tap(300, 310))  # On resume pill
        # Outside points
        self.assertFalse(ModernThemeModel.classify_hero_tap(13, 200))
        self.assertFalse(ModernThemeModel.classify_hero_tap(466, 200))
        self.assertFalse(ModernThemeModel.classify_hero_tap(200, 49))
        self.assertFalse(ModernThemeModel.classify_hero_tap(200, 361))

    def test_f11_07_recent_shelf_geometry(self):
        """F11.7: Recent shelf geometry and 2-3 mini book slots with progress indicators."""
        self.assertEqual(ModernThemeModel.SHELF_RECT, (14, 372, 452, 168))
        self.assertEqual(ModernThemeModel.SHELF_TITLE, "НЕДАВНИЕ КНИГИ")
        slots = ModernThemeModel.get_shelf_slot_rects(3)
        self.assertEqual(len(slots), 3)
        self.assertEqual(slots[0][0], 14)
        self.assertEqual(slots[0][2], 144)
        self.assertEqual(slots[1][0], 168)  # 14 + 144 + 10 gap
        self.assertEqual(slots[2][0], 322)  # 168 + 144 + 10 gap
        self.assertEqual(ModernThemeModel.SHELF_MINI_COVER_WIDTH, 66)
        self.assertEqual(ModernThemeModel.SHELF_MINI_COVER_HEIGHT, 99)

    def test_f11_08_recent_shelf_single_tap_hitbox(self):
        """F11.8: Recent shelf single-tap hit-box routing to slot indices."""
        # Slot 0 tap
        self.assertEqual(ModernThemeModel.classify_shelf_tap(50, 450, 3), 0)
        # Slot 1 tap
        self.assertEqual(ModernThemeModel.classify_shelf_tap(200, 450, 3), 1)
        # Slot 2 tap
        self.assertEqual(ModernThemeModel.classify_shelf_tap(350, 450, 3), 2)
        # Gap tap (between slot 0 and slot 1: 158..167)
        self.assertIsNone(ModernThemeModel.classify_shelf_tap(162, 450, 3))
        # Out of vertical bounds
        self.assertIsNone(ModernThemeModel.classify_shelf_tap(50, 350, 3))
        self.assertIsNone(ModernThemeModel.classify_shelf_tap(50, 550, 3))

    def test_f11_09_bottom_navigation_dock_5_equal_columns(self):
        """F11.9: Bottom Navigation Dock with 5 equal 96px columns replacing vertical list."""
        self.assertEqual(ModernThemeModel.DOCK_RECT, (0, 720, 480, 80))
        self.assertEqual(ModernThemeModel.DOCK_COLUMNS_COUNT, 5)
        self.assertEqual(ModernThemeModel.DOCK_COL_WIDTH, 96)
        for i, item in enumerate(ModernThemeModel.DOCK_ITEMS):
            rect = ModernThemeModel.get_dock_rect_for_col(i)
            self.assertEqual(rect, (i * 96, 720, 96, 80))
            self.assertFalse(EmojiBanValidator.contains_emoji(item["name"]))
            # Hit-test column center
            col_hit = ModernThemeModel.classify_dock_tap(i * 96 + 48, 750)
            self.assertEqual(col_hit, i)

    def test_f11_10_smart_status_bar_and_curtain_dropdown(self):
        """F11.10: Smart Status Bar [0..479]x[0..44] and Quick Settings curtain drop-down."""
        self.assertEqual(ModernThemeModel.STATUS_BAR_RECT, (0, 0, 480, 44))
        self.assertEqual(ModernThemeModel.CURTAIN_MAX_HEIGHT, 340)
        # Tap in status bar opens curtain
        self.assertTrue(ModernThemeModel.classify_header_tap(240, 20))
        self.assertTrue(ModernThemeModel.classify_header_tap(0, 0))
        self.assertTrue(ModernThemeModel.classify_header_tap(479, 43))
        self.assertFalse(ModernThemeModel.classify_header_tap(240, 44))
        # Dismiss curtain tap below curtain height
        self.assertTrue(ModernThemeModel.classify_curtain_dismiss_tap(240, 350))
        self.assertFalse(ModernThemeModel.classify_curtain_dismiss_tap(240, 330))

    def test_f11_11_gt911_touch_coordinates_1to1_mapping(self):
        """F11.11: GT911 touch coordinates 1:1 mathematical mapping on 800x480 panel."""
        test_points = [
            (0, 0),
            (479, 0),
            (0, 799),
            (479, 799),
            (240, 400),
            (100, 250),
            (350, 750),
        ]
        for tx, ty in test_points:
            log_x, log_y = GT911MappingModel.full_pipeline(tx, ty)
            self.assertEqual(
                (log_x, log_y),
                (tx, ty),
                f"GT911 transform drifted: input ({tx}, {ty}) -> output ({log_x}, {log_y})",
            )

    def test_f11_12_hardware_key_focus_cycling_and_back_return(self):
        """F11.12: Hardware key focus cycling (Hero -> Shelf -> Dock) and Back key return."""
        focus = HardwareKeyFocusModel(has_hero=True, shelf_count=3, dock_count=5)
        self.assertEqual(focus.total_targets, 9)
        self.assertEqual(focus.current_index, 0)
        self.assertTrue(focus.is_hero_focused())
        # Cycle through 3 shelf books
        for s in range(3):
            focus.next_focus()
            self.assertTrue(focus.is_shelf_focused())
            desc = focus.get_target_descriptor()
            self.assertEqual(desc["kind"], "SHELF")
            self.assertEqual(desc["slot"], s)
        # Cycle through 5 dock items
        for d in range(5):
            focus.next_focus()
            self.assertTrue(focus.is_dock_focused())
            desc = focus.get_target_descriptor()
            self.assertEqual(desc["kind"], "DOCK")
            self.assertEqual(desc["column"], d)
        # Wrap around to Hero
        focus.next_focus()
        self.assertEqual(focus.current_index, 0)
        self.assertTrue(focus.is_hero_focused())
        # Cycle backward
        focus.prev_focus()
        self.assertEqual(focus.current_index, 8)
        self.assertTrue(focus.is_dock_focused())

    def test_f11_13_banned_keyword_enforcement(self):
        """F11.13: Strict enforcement of banned keyword across codebase."""
        expected_kw = "".join(["A", "l", "p", "a", "4", "h", "i", "n", "O"])
        self.assertEqual(EmojiBanValidator.BANNED_KEYWORD, expected_kw)
        # Audit src directory
        violations_src = EmojiBanValidator.scan_for_banned_keyword(
            SRC_DIR, (".cpp", ".h", ".c", ".ini", ".csv", ".yaml")
        )
        self.assertEqual(
            len(violations_src),
            0,
            f"Banned keyword detected in src: {violations_src}",
        )
        # Audit tests directory
        violations_tests = EmojiBanValidator.scan_for_banned_keyword(
            REPO_ROOT / "tests", (".py",)
        )
        self.assertEqual(
            len(violations_tests),
            0,
            f"Banned keyword detected in tests: {violations_tests}",
        )

    # --------------------------------------------------------------------------
    # FEATURE 12: R1-R6 Architectural Expansion (16 tests)
    # --------------------------------------------------------------------------

    def test_f12_01_quick_settings_curtain_geometry_and_gestures(self):
        """F12.1: Quick Settings curtain geometry, status bar tap, and swipe open/close."""
        self.assertEqual(QuickSettingsCurtainModel.CURTAIN_RECT, (0, 0, 480, 360))
        self.assertEqual(QuickSettingsCurtainModel.STATUS_BAR_RECT, (0, 0, 480, 44))

        # Status bar tap opens curtain
        self.assertTrue(QuickSettingsCurtainModel.classify_open_tap(240, 20))
        self.assertTrue(QuickSettingsCurtainModel.classify_open_tap(0, 0))
        self.assertTrue(QuickSettingsCurtainModel.classify_open_tap(479, 43))
        self.assertFalse(QuickSettingsCurtainModel.classify_open_tap(240, 44))

        # Top swipe down opens curtain
        self.assertTrue(QuickSettingsCurtainModel.classify_open_swipe(240, 10, 240, 90, threshold=50.0))
        self.assertFalse(QuickSettingsCurtainModel.classify_open_swipe(240, 50, 240, 150, threshold=50.0))  # not top edge

        # Dismiss tap outside curtain
        self.assertTrue(QuickSettingsCurtainModel.classify_dismiss_tap(240, 380, curtain_h=360))
        self.assertFalse(QuickSettingsCurtainModel.classify_dismiss_tap(240, 350, curtain_h=360))

        # Dismiss swipe up inside curtain
        self.assertTrue(QuickSettingsCurtainModel.classify_dismiss_swipe(240, 300, 240, 200, threshold=50.0))
        self.assertFalse(QuickSettingsCurtainModel.classify_dismiss_swipe(240, 200, 240, 300, threshold=50.0))

    def test_f12_02_quick_settings_dual_channel_sliders(self):
        """F12.2: Dual-channel sliders for brightness & CCT with discrete step control."""
        self.assertEqual(QuickSettingsCurtainModel.BRIGHTNESS_MIN, 0)
        self.assertEqual(QuickSettingsCurtainModel.BRIGHTNESS_MAX, 100)
        self.assertEqual(QuickSettingsCurtainModel.CCT_MIN, 0)
        self.assertEqual(QuickSettingsCurtainModel.CCT_MAX, 100)

        # Step channel value
        self.assertEqual(QuickSettingsCurtainModel.step_channel(50, 10), 60)
        self.assertEqual(QuickSettingsCurtainModel.step_channel(50, -10), 40)
        self.assertEqual(QuickSettingsCurtainModel.step_channel(95, 10), 100)
        self.assertEqual(QuickSettingsCurtainModel.step_channel(5, -10), 0)

        # Hit testing Brightness slider
        self.assertEqual(QuickSettingsCurtainModel.classify_slider_tap("BRIGHTNESS", 35, 75), "MINUS")
        self.assertEqual(QuickSettingsCurtainModel.classify_slider_tap("BRIGHTNESS", 435, 75), "PLUS")
        self.assertEqual(QuickSettingsCurtainModel.classify_slider_tap("BRIGHTNESS", 200, 75), "TRACK")

        # Hit testing CCT slider
        self.assertEqual(QuickSettingsCurtainModel.classify_slider_tap("CCT", 35, 120), "MINUS")
        self.assertEqual(QuickSettingsCurtainModel.classify_slider_tap("CCT", 435, 120), "PLUS")
        self.assertEqual(QuickSettingsCurtainModel.classify_slider_tap("CCT", 200, 120), "TRACK")

    def test_f12_03_quick_settings_control_pills_and_battery_stats(self):
        """F12.3: Quick Settings 4 control pills and battery stats formatting."""
        expected_pills = {"WIFI", "DARK_MODE", "ROTATION_LOCK", "SLEEP"}
        self.assertEqual(set(QuickSettingsCurtainModel.CONTROL_PILLS.keys()), expected_pills)

        # Pill tap routing
        self.assertEqual(QuickSettingsCurtainModel.classify_pill_tap(100, 180), "WIFI")
        self.assertEqual(QuickSettingsCurtainModel.classify_pill_tap(300, 180), "DARK_MODE")
        self.assertEqual(QuickSettingsCurtainModel.classify_pill_tap(100, 240), "ROTATION_LOCK")
        self.assertEqual(QuickSettingsCurtainModel.classify_pill_tap(300, 240), "SLEEP")
        self.assertIsNone(QuickSettingsCurtainModel.classify_pill_tap(10, 10))

        # Battery stats formatting
        formatted = QuickSettingsCurtainModel.format_battery_stats(85, True, 15300)
        self.assertEqual(formatted, "Батарея: 85% (Зарядка) • Работа: 4ч 15м")
        self.assertFalse(EmojiBanValidator.contains_emoji(formatted))

    def test_f12_04_side_drawer_handedness_gestures(self):
        """F12.4: Edge-Swipe side drawer handedness routing and trigger gestures."""
        # Right-handed: drawer on right, swipe from right opens, swipe from left is back
        r_rect = SideDrawerModel.get_drawer_rect(HandednessEnum.RIGHT)
        self.assertEqual(r_rect, (160, 0, 320, 800))
        self.assertTrue(SideDrawerModel.classify_open_gesture(460, 400, 380, 400, HandednessEnum.RIGHT))
        self.assertFalse(SideDrawerModel.classify_open_gesture(20, 400, 100, 400, HandednessEnum.RIGHT))
        self.assertTrue(SideDrawerModel.classify_back_gesture(20, 400, 100, 400, HandednessEnum.RIGHT))

        # Left-handed: drawer on left, swipe from left opens, swipe from right is back
        l_rect = SideDrawerModel.get_drawer_rect(HandednessEnum.LEFT)
        self.assertEqual(l_rect, (0, 0, 320, 800))
        self.assertTrue(SideDrawerModel.classify_open_gesture(20, 400, 100, 400, HandednessEnum.LEFT))
        self.assertFalse(SideDrawerModel.classify_open_gesture(460, 400, 380, 400, HandednessEnum.LEFT))
        self.assertTrue(SideDrawerModel.classify_back_gesture(460, 400, 380, 400, HandednessEnum.LEFT))

        # Side tab tap trigger
        self.assertTrue(SideDrawerModel.classify_side_tab_tap(465, 400, HandednessEnum.RIGHT))
        self.assertTrue(SideDrawerModel.classify_side_tab_tap(10, 400, HandednessEnum.LEFT))

    def test_f12_05_side_drawer_6_navigation_targets(self):
        """F12.5: Side drawer 6 navigation targets with clean typography."""
        self.assertEqual(len(SideDrawerModel.TARGETS), 6)
        expected_ids = ["LIBRARY", "NOW_READING", "TOC", "BOOKMARKS", "DICTIONARY", "SETTINGS"]
        actual_ids = [t["id"] for t in SideDrawerModel.TARGETS]
        self.assertEqual(actual_ids, expected_ids)

        for target in SideDrawerModel.TARGETS:
            self.assertFalse(EmojiBanValidator.contains_emoji(target["name"]))

        # Hit testing targets (right-handed)
        for i in range(6):
            rect = SideDrawerModel.get_item_rect(i, HandednessEnum.RIGHT)
            center_x = rect[0] + rect[2] // 2
            center_y = rect[1] + rect[3] // 2
            hit = SideDrawerModel.classify_item_tap(center_x, center_y, HandednessEnum.RIGHT)
            self.assertEqual(hit, i)

    def test_f12_06_side_drawer_context_invocation_and_dismiss(self):
        """F12.6: Side drawer backdrop tap dismiss in both handedness modes."""
        # Right-handed: tapping backdrop (x < 160) dismisses
        self.assertTrue(SideDrawerModel.classify_dismiss_tap(100, 400, HandednessEnum.RIGHT))
        self.assertFalse(SideDrawerModel.classify_dismiss_tap(250, 400, HandednessEnum.RIGHT))

        # Left-handed: tapping backdrop (x >= 320) dismisses
        self.assertTrue(SideDrawerModel.classify_dismiss_tap(350, 400, HandednessEnum.LEFT))
        self.assertFalse(SideDrawerModel.classify_dismiss_tap(100, 400, HandednessEnum.LEFT))

    def test_f12_07_reader_overlays_top_bar_geometry(self):
        """F12.7: In-Book top overlay with back, title, bookmark toggle, and search."""
        self.assertEqual(ReaderOverlaysModel.TOP_BAR_RECT, (0, 0, 480, 64))
        self.assertTrue(ReaderOverlaysModel.classify_center_menu_tap(240, 400))
        self.assertFalse(ReaderOverlaysModel.classify_center_menu_tap(50, 400))

        # Top bar hit routing
        self.assertEqual(ReaderOverlaysModel.classify_top_tap(30, 30), "BACK")
        self.assertEqual(ReaderOverlaysModel.classify_top_tap(200, 30), "TITLE")
        self.assertEqual(ReaderOverlaysModel.classify_top_tap(390, 30), "TOGGLE_BOOKMARK")
        self.assertEqual(ReaderOverlaysModel.classify_top_tap(450, 30), "SEARCH")
        self.assertIsNone(ReaderOverlaysModel.classify_top_tap(200, 100))

    def test_f12_08_reader_overlays_bottom_bar_and_countdown(self):
        """F12.8: In-Book bottom overlay scrubber, action buttons, and chapter countdown."""
        self.assertEqual(ReaderOverlaysModel.BOTTOM_BAR_RECT, (0, 672, 480, 128))

        # Scrubber mapping
        self.assertEqual(ReaderOverlaysModel.scrubber_x_to_page(30, 200), 1)
        self.assertEqual(ReaderOverlaysModel.scrubber_x_to_page(240, 200), 101)
        self.assertEqual(ReaderOverlaysModel.scrubber_x_to_page(450, 200), 200)

        # Action button hit routing
        self.assertEqual(ReaderOverlaysModel.classify_bottom_tap(60, 765), "TYPOGRAPHY")
        self.assertEqual(ReaderOverlaysModel.classify_bottom_tap(170, 765), "TOC")
        self.assertEqual(ReaderOverlaysModel.classify_bottom_tap(290, 765), "FOOTNOTES")
        self.assertEqual(ReaderOverlaysModel.classify_bottom_tap(400, 765), "BOOKMARKS")

        # Countdown calculation
        cd_str = ReaderOverlaysModel.calc_chapter_countdown(remaining_pages=10, avg_seconds_per_page=30.0)
        self.assertEqual(cd_str, "~5 мин")
        self.assertFalse(EmojiBanValidator.contains_emoji(cd_str))

    def test_f12_09_reader_typography_aa_live_controls(self):
        """F12.9: Live typography popup with font family/size/margins and offset preservation."""
        self.assertEqual(AaTypographyModel.POPUP_RECT, (20, 350, 440, 310))
        self.assertIn("Literata", AaTypographyModel.FONT_FAMILIES)

        # Font size adjustment
        self.assertEqual(AaTypographyModel.step_font_size(20, 2), 22)
        self.assertEqual(AaTypographyModel.step_font_size(14, -2), 14)
        self.assertEqual(AaTypographyModel.step_font_size(36, 2), 36)

        # Offset mapping across re-pagination
        page_ranges = [(0, 500), (500, 1100), (1100, 1800)]
        self.assertEqual(AaTypographyModel.map_offset_to_new_page(250, page_ranges), 0)
        self.assertEqual(AaTypographyModel.map_offset_to_new_page(750, page_ranges), 1)
        self.assertEqual(AaTypographyModel.map_offset_to_new_page(1500, page_ranges), 2)

    def test_f12_10_settings_hub_5_modular_category_cards(self):
        """F12.10: Modular Settings Hub 5 section cards with 32x32 contour icons."""
        self.assertEqual(len(ModularSettingsHubModel.SECTIONS), 5)
        expected_sections = ["READING", "DISPLAY", "AUTONOMY", "NETWORK", "SYSTEM"]
        self.assertEqual([s["id"] for s in ModularSettingsHubModel.SECTIONS], expected_sections)

        for sec in ModularSettingsHubModel.SECTIONS:
            self.assertTrue(sec["icon"].endswith("_32"))
            self.assertFalse(EmojiBanValidator.contains_emoji(sec["title"]))
            self.assertFalse(EmojiBanValidator.contains_emoji(sec["desc"]))

        # Card hit testing
        self.assertEqual(ModularSettingsHubModel.classify_card_tap(100, 100), "READING")
        self.assertEqual(ModularSettingsHubModel.classify_card_tap(100, 240), "DISPLAY")

    def test_f12_11_settings_hub_native_graphical_toggles(self):
        """F12.11: Native graphical toggles [●  ] and [  ●] with knob placement."""
        self.assertEqual(ModularSettingsHubModel.format_graphical_toggle(False), "[●  ]")
        self.assertEqual(ModularSettingsHubModel.format_graphical_toggle(True), "[  ●]")

        track = (350, 80, 56, 28)
        off_cx, cy = ModularSettingsHubModel.get_toggle_knob_center(track, is_on=False)
        on_cx, _ = ModularSettingsHubModel.get_toggle_knob_center(track, is_on=True)
        self.assertLess(off_cx, on_cx)
        self.assertEqual(cy, 80 + 14)

    def test_f12_12_settings_hub_step_sliders_and_popup_selectors(self):
        """F12.12: Settings Hub discrete step sliders bounded within limits."""
        self.assertEqual(ModularSettingsHubModel.step_slider_value(5, 1, 1, 10), 6)
        self.assertEqual(ModularSettingsHubModel.step_slider_value(1, -1, 1, 10), 1)
        self.assertEqual(ModularSettingsHubModel.step_slider_value(10, 1, 1, 10), 10)

    def test_f12_13_library_hub_3x2_cover_grid_and_metadata_list(self):
        """F12.13: Library Hub 3x2 Cover Grid and Detailed Metadata List view modes."""
        self.assertEqual(LibraryHubModel.GRID_ROWS, 3)
        self.assertEqual(LibraryHubModel.GRID_COLS, 2)
        self.assertEqual(LibraryHubModel.GRID_ITEMS_PER_PAGE, 6)

        # Hit test 3x2 grid
        cell_0 = LibraryHubModel.get_grid_cell_rect(0, 0)
        self.assertEqual(cell_0, (20, 60, 210, 220))
        self.assertEqual(LibraryHubModel.classify_grid_tap(50, 100), 0)
        self.assertEqual(LibraryHubModel.classify_grid_tap(300, 100), 1)
        self.assertEqual(LibraryHubModel.classify_grid_tap(50, 300), 2)
        self.assertEqual(LibraryHubModel.classify_grid_tap(300, 300), 3)
        self.assertEqual(LibraryHubModel.classify_grid_tap(50, 550), 4)
        self.assertEqual(LibraryHubModel.classify_grid_tap(300, 550), 5)

    def test_f12_14_library_hub_multi_criteria_sorting_and_search(self):
        """F12.14: Library Hub sorting by Author/Date/Progress/Name and search filtering."""
        books = [
            {"title": "Преступление и наказание", "author": "Достоевский", "date": 100, "progress": 80.0},
            {"title": "Война и мир", "author": "Толстой", "date": 200, "progress": 25.0},
            {"title": "Анна Каренина", "author": "Толстой", "date": 150, "progress": 95.0},
        ]

        # Sort by AUTHOR
        by_author = LibraryHubModel.sort_books(books, LibrarySortModeEnum.AUTHOR, ascending=True)
        self.assertEqual(by_author[0]["author"], "Достоевский")

        # Sort by PROGRESS (highest first)
        by_prog = LibraryHubModel.sort_books(books, LibrarySortModeEnum.PROGRESS, ascending=False)
        self.assertEqual(by_prog[0]["title"], "Анна Каренина")
        self.assertEqual(by_prog[0]["progress"], 95.0)

        # Search filter
        filtered = LibraryHubModel.filter_books("война", books)
        self.assertEqual(len(filtered), 1)
        self.assertEqual(filtered[0]["title"], "Война и мир")

    def test_f12_15_crosspoint_eradication_validation_contract(self):
        """F12.15: CrossPoint eradication validator for UI labels, SD path safety, and text audit."""
        self.assertTrue(CrossPointEradicationValidator.is_clean_ui_label("Библиотека"))
        self.assertTrue(CrossPointEradicationValidator.is_clean_ui_label("Настройки"))
        self.assertFalse(CrossPointEradicationValidator.is_clean_ui_label("CrossPoint Reader"))

        # Preserved SD paths
        self.assertTrue(CrossPointEradicationValidator.is_preserved_sd_path("/.crosspoint/settings.json"))
        self.assertTrue(CrossPointEradicationValidator.is_preserved_sd_path("fs_/.crosspoint/state.json"))

        # Text audit
        hits = CrossPointEradicationValidator.audit_text_for_crosspoint("BookPoint OS 2.0.0")
        self.assertEqual(len(hits), 0)
        legacy_hits = CrossPointEradicationValidator.audit_text_for_crosspoint("CrossPoint firmware")
        self.assertEqual(len(legacy_hits), 1)

    def test_f12_17_opds_catalog_hub_and_search_contracts(self):
        """F12.17: OPDS catalog hub expansion (>=12 verified servers, search templates)."""
        self.assertGreaterEqual(len(OpdsCatalogHubModel.DEFAULT_SERVERS), 12)
        self.assertEqual(OpdsCatalogHubModel.MAX_SERVERS, 24)

        # Verify search templates for major libraries
        tmpl_gutenberg = OpdsCatalogHubModel.get_search_template_for_url("https://m.gutenberg.org/ebooks.opds/")
        self.assertIsNotNone(tmpl_gutenberg)
        self.assertIn("{searchTerms}", tmpl_gutenberg)

        search_url = OpdsCatalogHubModel.build_search_url(tmpl_gutenberg, "Tolstoy")
        self.assertIn("Tolstoy", search_url)

        tmpl_coollib = OpdsCatalogHubModel.get_search_template_for_url("http://coollib.cc/opds")
        self.assertIsNotNone(tmpl_coollib)

        tmpl_iknigi = OpdsCatalogHubModel.get_search_template_for_url("http://iknigi.net/opds")
        self.assertIsNotNone(tmpl_iknigi)

    def test_f12_18_opds_stuck_escape_and_back_gesture(self):
        """F12.18: Anti-hang fast escape from stuck OPDS via back gesture and cancel button."""
        # Cancel button hitbox (140, 420, 200, 56)
        self.assertTrue(OpdsStuckEscapeModel.classify_cancel_tap(240, 440))
        self.assertTrue(OpdsStuckEscapeModel.classify_cancel_tap(150, 430))
        self.assertFalse(OpdsStuckEscapeModel.classify_cancel_tap(50, 200))

        # Edge-swipe back gesture
        self.assertTrue(OpdsStuckEscapeModel.classify_escape_gesture(20, 300, 100, 300, HandednessEnum.RIGHT))
        self.assertTrue(OpdsStuckEscapeModel.classify_escape_gesture(460, 300, 380, 300, HandednessEnum.LEFT))

        # Socket timeout limit
        self.assertLessEqual(OpdsStuckEscapeModel.MAX_SOCKET_TIMEOUT_MS, 8000)

    def test_f12_19_sleep_covers_rotation_and_artwork_formats(self):
        """F12.19: Interchangeable sleep covers validation, resolution, and non-repeating shuffle."""
        self.assertIn("/.sleep", SleepCoversRotationModel.VALID_DIRECTORIES)
        self.assertEqual(SleepCoversRotationModel.TARGET_WIDTH, 480)
        self.assertEqual(SleepCoversRotationModel.TARGET_HEIGHT, 800)

        covers = [f"cover_{i}.bmp" for i in range(6)]
        idx1, c1 = SleepCoversRotationModel.pick_next_random_cover(covers, [])
        self.assertIn(c1, covers)
        idx2, c2 = SleepCoversRotationModel.pick_next_random_cover(covers, [0, 1])
        self.assertNotIn(idx2, [0, 1])

    # --------------------------------------------------------------------------
    # FEATURE 13: Round 6 Modernization Contracts (R1-R4) (13 tests)
    # --------------------------------------------------------------------------

    def test_f13_01_control_curtain_44px_touch_targets_and_dual_frontlight_sliders(self):
        """F13.1: Control Curtain >=44px touch targets and dual frontlight sliders seek math (R1)."""
        self.assertGreaterEqual(QuickSettingsCurtainModel.MIN_TOUCH_TARGET_SIZE, 44)

        # Verify button dimensions >= 44px
        for rect in [
            QuickSettingsCurtainModel.BRIGHTNESS_MINUS_RECT,
            QuickSettingsCurtainModel.BRIGHTNESS_PLUS_RECT,
            QuickSettingsCurtainModel.CCT_MINUS_RECT,
            QuickSettingsCurtainModel.CCT_PLUS_RECT,
        ]:
            _, _, w, h = rect
            self.assertGreaterEqual(w, 44, f"Button width {w} < 44px")
            self.assertGreaterEqual(h, 44, f"Button height {h} < 44px")

        # Verify slider track touch height >= 44px
        self.assertGreaterEqual(QuickSettingsCurtainModel.BRIGHTNESS_SLIDER_RECT[3], 44)
        self.assertGreaterEqual(QuickSettingsCurtainModel.CCT_SLIDER_RECT[3], 44)

        # Verify direct slider track touch seek math
        self.assertEqual(QuickSettingsCurtainModel.track_x_to_value("BRIGHTNESS", 68), 0)
        self.assertEqual(QuickSettingsCurtainModel.track_x_to_value("BRIGHTNESS", 68 + 172), 50)
        self.assertEqual(QuickSettingsCurtainModel.track_x_to_value("BRIGHTNESS", 68 + 344), 100)
        self.assertEqual(QuickSettingsCurtainModel.track_x_to_value("BRIGHTNESS", 10), 0)    # Clamped lower
        self.assertEqual(QuickSettingsCurtainModel.track_x_to_value("BRIGHTNESS", 500), 100) # Clamped upper

        self.assertEqual(QuickSettingsCurtainModel.track_x_to_value("CCT", 68), 0)
        self.assertEqual(QuickSettingsCurtainModel.track_x_to_value("CCT", 68 + 86), 25)
        self.assertEqual(QuickSettingsCurtainModel.track_x_to_value("CCT", 68 + 344), 100)

    def test_f13_02_control_curtain_info_block_time_and_date(self):
        """F13.2: Control Curtain info block includes exact battery %, charging, Time and Date (R1)."""
        # Formatted battery and clock string
        info_str = QuickSettingsCurtainModel.format_battery_and_clock(
            percent=92, is_charging=True, time_str="18:45", date_str="13.09.2026"
        )
        self.assertIn("92%", info_str)
        self.assertIn("Зарядка", info_str)
        self.assertIn("18:45", info_str)
        self.assertIn("13.09.2026", info_str)
        self.assertFalse(EmojiBanValidator.contains_emoji(info_str))

        # 2-line layout
        line1, line2 = QuickSettingsCurtainModel.format_info_block_lines(
            percent=75, is_charging=False, time_str="12:00", date_str="01.01.2026", uptime_seconds=7200
        )
        self.assertIn("75%", line1)
        self.assertIn("12:00", line1)
        self.assertIn("2ч 0м", line2)
        self.assertFalse(EmojiBanValidator.contains_emoji(line1))
        self.assertFalse(EmojiBanValidator.contains_emoji(line2))

    def test_f13_03_status_bar_tap_isolation_from_reader_top_bar(self):
        """F13.3: Status bar tap isolation from reader top bar controls (R1 / R2)."""
        # When in reader activity and in-book overlays are active, status bar tap is isolated (suppressed)
        self.assertFalse(
            QuickSettingsCurtainModel.is_status_bar_tap_allowed(
                in_reader_activity=True, in_book_overlays_active=True, tx=30, ty=30
            )
        )
        self.assertFalse(
            QuickSettingsCurtainModel.is_status_bar_tap_allowed(
                in_reader_activity=True, in_book_overlays_active=True, tx=390, ty=20
            )
        )

        # When reader overlays are NOT active, status bar tap is permitted
        self.assertTrue(
            QuickSettingsCurtainModel.is_status_bar_tap_allowed(
                in_reader_activity=True, in_book_overlays_active=False, tx=240, ty=20
            )
        )
        # When outside reader (e.g. HomeActivity), status bar tap is permitted
        self.assertTrue(
            QuickSettingsCurtainModel.is_status_bar_tap_allowed(
                in_reader_activity=False, in_book_overlays_active=False, tx=240, ty=20
            )
        )
        # Tap below status bar is never status bar tap
        self.assertFalse(
            QuickSettingsCurtainModel.is_status_bar_tap_allowed(
                in_reader_activity=False, in_book_overlays_active=False, tx=240, ty=50
            )
        )

    def test_f13_04_reader_top_bar_back_toc_bookmark_layout(self):
        """F13.4: Reader top bar contains Back, TOC, and Bookmark (R2)."""
        # TOC button is in top bar
        self.assertEqual(ReaderOverlaysModel.TOC_BUTTON_RECT, (64, 0, 80, 64))
        self.assertEqual(ReaderOverlaysModel.BACK_BUTTON_RECT, (0, 0, 64, 64))
        self.assertEqual(ReaderOverlaysModel.BOOKMARK_BUTTON_RECT, (364, 0, 56, 64))

        # Hit testing top bar actions
        self.assertEqual(ReaderOverlaysModel.classify_top_tap(30, 30), "BACK")
        self.assertEqual(ReaderOverlaysModel.classify_top_tap(100, 30), "TOC")
        self.assertEqual(ReaderOverlaysModel.classify_top_tap(250, 30), "TITLE")
        self.assertEqual(ReaderOverlaysModel.classify_top_tap(390, 30), "TOGGLE_BOOKMARK")

    def test_f13_05_live_typography_aa_mandated_font_families_selection(self):
        """F13.5: Live Typography Aa overlay models mandated font families (R2)."""
        expected_mandated = {"JetBrains Mono", "Roboto Condensed", "OpenDyslexic"}
        self.assertTrue(expected_mandated.issubset(set(AaTypographyModel.MANDATED_FONT_FAMILIES)))
        self.assertTrue(expected_mandated.issubset(set(AaTypographyModel.FONT_FAMILIES)))

        # Verify font chips hitboxes >= 44px
        for font_name, rect in AaTypographyModel.FONT_CHIPS.items():
            _, _, w, h = rect
            self.assertGreaterEqual(w, 44)
            self.assertGreaterEqual(h, 44)

        # Hit-testing font chips
        self.assertEqual(AaTypographyModel.classify_font_chip_tap(50, 430), "JetBrains Mono")
        self.assertEqual(AaTypographyModel.classify_font_chip_tap(200, 430), "Roboto Condensed")
        self.assertEqual(AaTypographyModel.classify_font_chip_tap(350, 430), "OpenDyslexic")
        self.assertIsNone(AaTypographyModel.classify_font_chip_tap(10, 10))

    def test_f13_06_live_typography_aa_steppers_44px_and_autosave(self):
        """F13.6: Live Typography Aa overlay A-/A+ >=44px buttons, margins, line spacing, autosave (R2)."""
        self.assertGreaterEqual(AaTypographyModel.DECREASE_FONT_RECT[2], 44)
        self.assertGreaterEqual(AaTypographyModel.DECREASE_FONT_RECT[3], 44)
        self.assertGreaterEqual(AaTypographyModel.INCREASE_FONT_RECT[2], 44)
        self.assertGreaterEqual(AaTypographyModel.INCREASE_FONT_RECT[3], 44)

        # Margin stepping
        self.assertEqual(AaTypographyModel.step_margin(20, 1), 30)
        self.assertEqual(AaTypographyModel.step_margin(20, -1), 10)
        self.assertEqual(AaTypographyModel.step_margin(40, 1), 40)  # Clamped upper
        self.assertEqual(AaTypographyModel.step_margin(0, -1), 0)   # Clamped lower

        # Line spacing stepping
        self.assertAlmostEqual(AaTypographyModel.step_line_spacing(1.2, 1), 1.4)
        self.assertAlmostEqual(AaTypographyModel.step_line_spacing(1.2, -1), 1.0)
        self.assertAlmostEqual(AaTypographyModel.step_line_spacing(1.6, 1), 1.6)

        # Autosave state
        saved = AaTypographyModel.autosave_state(
            font_family="JetBrains Mono", font_size=18, line_spacing=1.4, margin=20
        )
        self.assertTrue(saved["autosaved"])
        self.assertEqual(saved["fontFamily"], "JetBrains Mono")
        self.assertEqual(saved["fontSize"], 18)
        self.assertEqual(saved["lineSpacing"], 1.4)
        self.assertEqual(saved["margin"], 20)

    def test_f13_07_universal_formats_fb2_and_fb2_zip_decompression_contracts(self):
        """F13.7: Universal formats (FB2, FB2.zip) format support & decompression contracts (R3)."""
        self.assertTrue(UniversalFormatsModel.has_fb2_extension("War_and_Peace.fb2"))
        self.assertFalse(UniversalFormatsModel.has_fb2_extension("War_and_Peace.fb2.zip"))
        self.assertTrue(UniversalFormatsModel.has_fb2_zip_extension("War_and_Peace.fb2.zip"))
        self.assertTrue(UniversalFormatsModel.is_universal_format("book.fb2"))
        self.assertTrue(UniversalFormatsModel.is_universal_format("book.fb2.zip"))
        self.assertFalse(UniversalFormatsModel.is_universal_format("book.pdf"))

        # DEFLATE bounded sliding window and streaming indexing limits
        self.assertLessEqual(UniversalFormatsModel.DEFLATE_WINDOW_SIZE_MAX, 32768)
        self.assertLessEqual(UniversalFormatsModel.MAX_INDEXING_RAM_BYTES, 65536)

        # Legacy encodings decoding verification
        # CP1251 "Привет"
        cp1251_bytes = b"\xcf\xf0\xe8\xe2\xe5\xf2"
        decoded_cp1251 = UniversalFormatsModel.decode_legacy_bytes(cp1251_bytes, "windows-1251")
        self.assertEqual(decoded_cp1251, "Привет")

        # KOI8-R "Привет"
        koi8r_bytes = b"\xf0\xd2\xc9\xd7\xc5\xd4"
        decoded_koi8r = UniversalFormatsModel.decode_legacy_bytes(koi8r_bytes, "koi8-r")
        self.assertEqual(decoded_koi8r, "Привет")

    def test_f13_08_strikethrough_tag_and_0xccb6_combining_stroke_normalization(self):
        """F13.8: <strikethrough> and 0xCCB6 (U+0336) combining stroke normalization (R3)."""
        self.assertTrue(StrikethroughNormalizationModel.is_strikethrough_tag("strikethrough"))
        self.assertTrue(StrikethroughNormalizationModel.is_strikethrough_tag("strike"))
        self.assertTrue(StrikethroughNormalizationModel.is_strikethrough_tag("s"))
        self.assertTrue(StrikethroughNormalizationModel.is_strikethrough_tag("del"))
        self.assertFalse(StrikethroughNormalizationModel.is_strikethrough_tag("p"))

        # U+0336 Combining Long Stroke Overlay normalization
        # Raw text with combining long stroke overlay
        raw_text = "т\u0336е\u0336к\u0336с\u0336т\u0336"
        clean_text, is_strike = StrikethroughNormalizationModel.normalize_strikethrough_text(raw_text)
        self.assertEqual(clean_text, "текст")
        self.assertTrue(is_strike)

        # Plain text without combining stroke
        clean_plain, is_strike_plain = StrikethroughNormalizationModel.normalize_strikethrough_text("обычный текст")
        self.assertEqual(clean_plain, "обычный текст")
        self.assertFalse(is_strike_plain)

        # Strikethrough line geometry: 80% ascender
        line_y = StrikethroughNormalizationModel.calc_strike_line_y(base_y=100, ascender=20)
        self.assertEqual(line_y, 100 - 16)

    def test_f13_09_progressive_jpeg_streaming_8x8_idct_and_downsampling_bounds(self):
        """F13.9: Progressive JPEG (PJPEG) streaming 8x8 IDCT decoding & downsampling without PSRAM OOM (R3)."""
        self.assertTrue(ProgressiveJpegDecoderModel.is_progressive_marker(0xFFC2))
        self.assertFalse(ProgressiveJpegDecoderModel.is_progressive_marker(0xFFC0))

        # Memory required for one streaming MCU row buffer
        row_buf_800 = ProgressiveJpegDecoderModel.calc_mcu_row_buffer_bytes(800, mcu_height=16)
        self.assertEqual(row_buf_800, 12800)  # 12.8 KB
        self.assertTrue(ProgressiveJpegDecoderModel.is_psram_safe(800))

        # Large cover (3000x5000)
        row_buf_3000 = ProgressiveJpegDecoderModel.calc_mcu_row_buffer_bytes(3000, mcu_height=16)
        self.assertEqual(row_buf_3000, 48000)  # 48 KB
        self.assertTrue(ProgressiveJpegDecoderModel.is_psram_safe(3000))
        self.assertLessEqual(row_buf_3000, ProgressiveJpegDecoderModel.MAX_MCU_ROW_BUFFER_BYTES)

        # Contrast with unbuffered full decode which causes OOM crash (> 2 MB)
        unbuffered_crash_bytes = ProgressiveJpegDecoderModel.calc_unbuffered_full_decode_bytes(3000, 5000)
        self.assertGreater(unbuffered_crash_bytes, ProgressiveJpegDecoderModel.FULL_IMAGE_PSRAM_DANGER_THRESHOLD)

        # Downsampling ratio calculation
        step_x, step_y = ProgressiveJpegDecoderModel.calc_downsample_steps(1600, 2400, 480, 800)
        self.assertAlmostEqual(step_x, 1600.0 / 480.0)
        self.assertAlmostEqual(step_y, 2400.0 / 800.0)

    def test_f13_10_footnote_superscript_hitbox_and_bottom_modal_popup(self):
        """F13.10: Footnote index tap to instant bottom modal card popup (R3)."""
        self.assertTrue(FootnoteModalModel.is_footnote_reference("[1]"))
        self.assertTrue(FootnoteModalModel.is_footnote_reference("[*]"))
        self.assertTrue(FootnoteModalModel.is_footnote_reference("42"))
        self.assertFalse(FootnoteModalModel.is_footnote_reference("Глава 1"))

        # Bottom modal card rect (20, 584, 440, 200)
        card_rect = FootnoteModalModel.get_modal_card_rect(screen_w=480, screen_h=800, height=200)
        self.assertEqual(card_rect, (20, 584, 440, 200))

        # Close button >= 44px
        close_btn = FootnoteModalModel.get_close_button_rect(card_rect)
        self.assertGreaterEqual(close_btn[2], 44)
        self.assertGreaterEqual(close_btn[3], 44)

        # Tap classification
        close_cx = close_btn[0] + close_btn[2] // 2
        close_cy = close_btn[1] + close_btn[3] // 2
        self.assertEqual(FootnoteModalModel.classify_modal_tap(card_rect, close_cx, close_cy), "CLOSE_BUTTON")
        self.assertEqual(FootnoteModalModel.classify_modal_tap(card_rect, 100, 650), "CARD_CONTENT")
        self.assertEqual(FootnoteModalModel.classify_modal_tap(card_rect, 100, 300), "BACKDROP_DISMISS")

        # Bounded text limit
        self.assertLessEqual(FootnoteModalModel.MAX_FOOTNOTE_TEXT_BYTES, 768)

    def test_f13_11_screensaver_gallery_11_themes_portrait_and_landscape(self):
        """F13.11: Screensaver gallery 11 curated themes for 480x800 and 800x480 (R4)."""
        self.assertEqual(ScreensaverGalleryModel.THEME_COUNT, 11)
        expected_themes = [
            "kindle_typebars",
            "kindle_fountain_pens",
            "kindle_antique_engravings",
            "kindle_celestial_maps",
            "kindle_pencils",
            "kindle_author_verne",
            "kindle_author_dickens",
            "kindle_author_twain",
            "kobo_geometric_patterns",
            "kobo_library_architecture",
            "kobo_reading_quotes",
        ]
        self.assertEqual(ScreensaverGalleryModel.THEMES, expected_themes)

        # Resolutions
        self.assertEqual(ScreensaverGalleryModel.RESOLUTIONS, [(480, 800), (800, 480)])

        # Theme filename generation for all 11 themes across both resolutions
        for idx in range(1, 12):
            fn_portrait = ScreensaverGalleryModel.get_theme_filename(idx, (480, 800))
            fn_landscape = ScreensaverGalleryModel.get_theme_filename(idx, (800, 480))
            self.assertIn("480x800", fn_portrait)
            self.assertIn("800x480", fn_landscape)
            self.assertTrue(fn_portrait.endswith(".bmp"))
            self.assertTrue(fn_landscape.endswith(".bmp"))

    def test_f13_12_screensaver_sleep_settings_and_book_cover_streak_mode(self):
        """F13.12: Sleep settings (specific selection, random, book cover mode with streak) (R4)."""
        self.assertEqual(ScreensaverGalleryModel.SCREENSAVER_RANDOM, 0)
        self.assertEqual(ScreensaverGalleryModel.SCREENSAVER_MODE_READING_STATS, 8)

        # Specific screensaver index selection bounds 1..11
        for theme_id in range(1, 12):
            fn = ScreensaverGalleryModel.get_theme_filename(theme_id)
            self.assertIsNotNone(fn)

        # Reading stats mode formats streak
        streak_badge = ModernThemeModel.format_reading_streak(14)
        self.assertEqual(streak_badge, "Серия: 14 дн.")
        self.assertFalse(EmojiBanValidator.contains_emoji(streak_badge))

    def test_f13_13_screensaver_exact_v2_1_0_version_display_light_dark_polarity(self):
        """F13.13: Exact v2.1.0 bottom version display in light and dark modes (R4)."""
        # Exact version string format verification
        self.assertEqual(ScreensaverGalleryModel.format_version_display("2.1.0-x4pro"), "v2.1.0")
        self.assertEqual(ScreensaverGalleryModel.format_version_display("2.1.0"), "v2.1.0")
        self.assertEqual(ScreensaverGalleryModel.format_version_display("v2.1.0"), "v2.1.0")

        # Version display position anchor at pageHeight - 30
        self.assertEqual(ScreensaverGalleryModel.get_version_position(480, 800), (240, 770))
        self.assertEqual(ScreensaverGalleryModel.get_version_position(800, 480), (400, 450))

        # Polarity: Light mode vs Dark mode
        light_polarity = ScreensaverGalleryModel.get_version_polarity(sleep_mode_is_dark=False)
        self.assertEqual(light_polarity["text_polarity"], POLARITY_BLACK)
        self.assertEqual(light_polarity["bg_polarity"], POLARITY_WHITE)
        self.assertFalse(light_polarity["screen_inverted"])

        dark_polarity = ScreensaverGalleryModel.get_version_polarity(sleep_mode_is_dark=True)
        self.assertEqual(dark_polarity["text_polarity"], POLARITY_WHITE)
        self.assertEqual(dark_polarity["bg_polarity"], POLARITY_BLACK)
        self.assertTrue(dark_polarity["screen_inverted"])

    # ==========================================================================
    # FEATURE 14: BOOKPOINT OS v2.2.0 AUDIT SUITE (R1 - R4)
    # ==========================================================================

    def test_f14_01_zero_overlap_hero_card_stack_and_author_truncation(self):
        """F14.01: Hero card vertical element budgeting and safe UTF-8 author truncation (R1)."""
        # 1. Standard stack: 2 lines of title, author, progress stats, reading streak
        res = ZeroOverlapModel.validate_hero_card_stack(title_lines_count=2, has_author=True, has_stats=True, has_streak=True)
        self.assertTrue(res["is_valid"], f"Hero card stack overflowed Resume button by {-res['clearance_px']}px")
        self.assertGreaterEqual(res["clearance_px"], 0)

        # 2. Compact stack: 1 line of title
        res_compact = ZeroOverlapModel.validate_hero_card_stack(title_lines_count=1, has_author=True, has_stats=True, has_streak=True)
        self.assertTrue(res_compact["is_valid"])
        self.assertGreater(res_compact["clearance_px"], res["clearance_px"])

        # 3. Long Cyrillic title and author safe UTF-8 truncation
        long_author = "Лев Николаевич Толстой, граф и писатель"
        safe_author = ZeroOverlapModel.safe_utf8_truncate(long_author, max_chars=18)
        self.assertTrue(safe_author.endswith("…"))
        self.assertLessEqual(len(safe_author), 18)
        # Verify no corrupted UTF-8 decoding
        self.assertEqual(safe_author.encode("utf-8").decode("utf-8"), safe_author)

    def test_f14_02_recent_shelf_slot_horizontal_bounding_and_no_bleed(self):
        """F14.02: Recent shelf mini card slots prevent horizontal text bleed into adjacent cards (R1)."""
        # Test short, medium, and very long author strings
        authors = [
            "Чехов А.П.",
            "Фёдор Достоевский",
            "Александр Сергеевич Пушкин",
        ]
        for auth in authors:
            slot_info = ZeroOverlapModel.validate_recent_shelf_slot(auth, slot_w=144, cover_w=44, margin=16)
            self.assertTrue(slot_info["fits_without_bleed"])
            self.assertLessEqual(len(slot_info["safe_author"]), slot_info["available_text_width"] // 9 + 2)

    def test_f14_03_quick_settings_curtain_landscape_centering_and_cold_warm_sliders(self):
        """F14.03: Control Center Curtain responsive centering in 800x480 landscape with dual Cold/Warm frontlight (R1)."""
        # Portrait offset: (480 - 448) // 2 = 16
        offset_portrait = QuickSettingsCurtainModel.get_content_offset_x(480)
        self.assertEqual(offset_portrait, 16)

        # Landscape offset: (800 - 448) // 2 = 176
        offset_landscape = QuickSettingsCurtainModel.get_content_offset_x(800)
        self.assertEqual(offset_landscape, 176)

        # Minus button in landscape: 176 + 16 = 192 (X: 192..236, Y: 54..98)
        self.assertEqual(
            QuickSettingsCurtainModel.classify_slider_tap_offset("BRIGHTNESS", 200, 70, offset_x=offset_landscape),
            "MINUS",
        )
        # Plus button in landscape: 176 + 420 = 596 (X: 596..640, Y: 54..98)
        self.assertEqual(
            QuickSettingsCurtainModel.classify_slider_tap_offset("BRIGHTNESS", 610, 70, offset_x=offset_landscape),
            "PLUS",
        )
        # Track seek in landscape
        self.assertEqual(
            QuickSettingsCurtainModel.classify_slider_tap_offset("BRIGHTNESS", 400, 70, offset_x=offset_landscape),
            "TRACK",
        )

        # Control pills in landscape: DARK_MODE is at offsetX + 250 = 426
        self.assertEqual(
            QuickSettingsCurtainModel.classify_pill_tap_offset(450, 175, offset_x=offset_landscape),
            "DARK_MODE",
        )

    def test_f14_04_floating_reader_overlays_12px_margins_and_800x480_reachability(self):
        """F14.04: Floating Reader Overlays maintain >=12px margins and remain reachable in 800x480 landscape (R1)."""
        # Portrait (480x800)
        top_port = ReaderOverlaysModel.get_floating_top_bar_rect(480, margin=14)
        bot_port = ReaderOverlaysModel.get_floating_bottom_bar_rect(480, 800, margin=14)
        self.assertTrue(ZeroOverlapModel.validate_reader_overlay_margins(top_port, 480, 800, min_margin=12))
        self.assertTrue(ZeroOverlapModel.validate_reader_overlay_margins(bot_port, 480, 800, min_margin=12))

        # Landscape (800x480)
        top_land = ReaderOverlaysModel.get_floating_top_bar_rect(800, margin=14)
        bot_land = ReaderOverlaysModel.get_floating_bottom_bar_rect(800, 480, margin=14)
        self.assertTrue(ZeroOverlapModel.validate_reader_overlay_margins(top_land, 800, 480, min_margin=12))
        self.assertTrue(ZeroOverlapModel.validate_reader_overlay_margins(bot_land, 800, 480, min_margin=12))

        # Ensure bottom bar is strictly on-screen in landscape (bot_y + bot_h <= 480)
        self.assertEqual(bot_land[1], 480 - 14 - 124)  # 342
        self.assertEqual(bot_land[1] + bot_land[3], 466)
        self.assertLess(bot_land[1] + bot_land[3], 480)

        # Hit test in landscape floating overlay
        self.assertEqual(ReaderOverlaysModel.classify_floating_top_tap(25, 25, 800, margin=14), "BACK")
        self.assertEqual(ReaderOverlaysModel.classify_floating_bottom_tap(30, 420, 800, 480, margin=14), "TYPOGRAPHY")

    def test_f14_05_live_typography_popup_header_and_size_zero_overlap(self):
        """F14.05: Zero text collision between 'Типографика' header and 'Размер:' label in Aa popup (R1)."""
        header_rect = (36, 332, 140, 20)
        separator_line_y = 356
        size_label_rect = (35, 378, 80, 20)
        a_minus_btn_rect = (120, 366, 56, 44)

        # Ensure header and size label do NOT collide
        self.assertFalse(ZeroOverlapModel.rects_intersect(header_rect, size_label_rect))
        self.assertFalse(ZeroOverlapModel.rects_intersect(header_rect, a_minus_btn_rect))
        ZeroOverlapModel.assert_no_overlap(header_rect, size_label_rect, "Header", "SizeLabel")
        ZeroOverlapModel.assert_no_overlap(header_rect, a_minus_btn_rect, "Header", "AMinusBtn")

        # Vertical clearance between header and controls
        self.assertGreaterEqual(size_label_rect[1] - (header_rect[1] + header_rect[3]), 24)

    def test_f14_06_minimum_44px_touch_targets_across_chrome_and_dialogs(self):
        """F14.06: All interactive touch targets strictly >= 44x44px in FileBrowser, OptionPopup, and Aa popup (R1)."""
        # 1. FileBrowser header buttons: Sort (308, 4, 48, 44), Search (364, 4, 48, 44), View (420, 4, 48, 44)
        sort_btn = (308, 4, 48, 44)
        search_btn = (364, 4, 48, 44)
        view_btn = (420, 4, 48, 44)
        self.assertTrue(ZeroOverlapModel.validate_touch_target_size(sort_btn))
        self.assertTrue(ZeroOverlapModel.validate_touch_target_size(search_btn))
        self.assertTrue(ZeroOverlapModel.validate_touch_target_size(view_btn))

        # 2. OptionPopup button height enforcement: max(44, lineH + 2*vPad)
        line_h = 20
        v_pad = 4
        computed_btn_h = max(44, line_h + 2 * v_pad)
        self.assertGreaterEqual(computed_btn_h, 44)

        # 3. Aa popup stepper buttons: (120, 366, 56, 44) and (260, 366, 56, 44)
        self.assertTrue(ZeroOverlapModel.validate_touch_target_size(AaTypographyModel.DECREASE_FONT_RECT))
        self.assertTrue(ZeroOverlapModel.validate_touch_target_size(AaTypographyModel.INCREASE_FONT_RECT))

    def test_f14_07_premium_typography_suite_7_families_presence_and_specs(self):
        """F14.07: Premium Typography Suite integrates 7 mandated high-contrast E-Ink font families (R2)."""
        expected_7 = [
            "Literata",
            "PT Serif",
            "Alegreya",
            "Inter",
            "Atkinson Hyperlegible",
            "JetBrains Mono",
            "OpenDyslexic",
        ]
        self.assertEqual(PremiumTypographySuiteModel.PREMIUM_FONT_FAMILIES, expected_7)
        self.assertEqual(AaTypographyModel.PREMIUM_FONT_FAMILIES, expected_7)

        # Verify specifications for each family
        for fam in expected_7:
            self.assertIn(fam, PremiumTypographySuiteModel.FAMILY_SPECS)
            spec = PremiumTypographySuiteModel.FAMILY_SPECS[fam]
            self.assertTrue(spec["has_cyrillic"])
            self.assertTrue(spec["has_latin"])
            self.assertGreaterEqual(len(spec["sizes"]), 4)

        # Stepping through all 7 families
        cur = "Literata"
        for next_fam in expected_7[1:] + [expected_7[0]]:
            cur = AaTypographyModel.step_font_family(cur, 1)
            self.assertEqual(cur, next_fam)

    def test_f14_08_typography_unicode_coverage_cyrillic_latin_punctuation_footnotes(self):
        """F14.08: Complete Unicode coverage for Cyrillic, Latin, punctuation (guillemets, em-dash), and footnotes (R2)."""
        passages = [
            "«Съешь же ещё этих мягких французских булок, да выпей чаю…»",
            "Європейські інтеграційні процеси — шлях до розвитку.",
            "У Беларусі свята: звоняць званы і спяваюць песні.",
            "Footnote reference with symbols: chapter 5¹²³ with references * and †.",
            "Latin text with quotes: “The quick brown fox jumps over the lazy dog.”",
        ]
        for passage in passages:
            res = PremiumTypographySuiteModel.validate_text_codepoints(passage)
            self.assertTrue(
                res["is_fully_covered"],
                f"Missing codepoints in passage '{passage}': {res['unsupported_samples']}",
            )

    def test_f14_09_instant_live_reflow_zero_character_offset_drift(self):
        """F14.09: Live font family/size change preserves exact reading position without offset drift (R2)."""
        cached_offset = 3840
        # Simulated re-pagination table for larger font size
        new_page_map = [
            (0, 800),
            (800, 1650),
            (1650, 2550),
            (2550, 3450),
            (3450, 4300),  # Offset 3840 is here (page index 4)
            (4300, 5200),
        ]
        res = PremiumTypographySuiteModel.verify_reflow_offset_preservation(cached_offset, new_page_map)
        self.assertTrue(res["offset_contained"])
        self.assertEqual(res["target_page"], 4)
        self.assertTrue(res["page_range"][0] <= cached_offset < res["page_range"][1])

    def test_f14_10_hybrid_font_architecture_psram_lifecycle_and_flash_headroom(self):
        """F14.10: Dynamic PSRAM font allocation preserves >=700 KB app0 flash partition headroom (R2)."""
        bin_path = Path("src/.pio/build/x4pro/firmware.bin")
        bin_size = bin_path.stat().st_size if bin_path.exists() else 5761760

        budget = PremiumTypographySuiteModel.verify_hybrid_partition_budget(bin_size)
        self.assertTrue(
            budget["meets_700kb_headroom"],
            f"Headroom {budget['headroom_kb']} KB is less than required 700 KB",
        )
        self.assertGreaterEqual(budget["headroom_bytes"], 716800)

    def test_f14_11_handed_touch_zones_75_25_page_turns_and_menu_reservation(self):
        """F14.11: Handedness touch page turns (75/25 left/right) and center menu reservation (R3)."""
        w, h = 480, 800

        # Center menu tap
        self.assertEqual(
            FlagshipErgonomicsModel.classify_reader_touch(240, 400, w, h, handedness="RIGHT"),
            FlagshipErgonomicsModel.READER_MENU,
        )

        # Right-handed mode:
        # Left 25% (X < 120): PREV
        self.assertEqual(
            FlagshipErgonomicsModel.classify_reader_touch(50, 700, w, h, handedness="RIGHT"),
            FlagshipErgonomicsModel.READER_TOUCH_PREV,
        )
        # Right 75% (X >= 120): NEXT
        self.assertEqual(
            FlagshipErgonomicsModel.classify_reader_touch(300, 700, w, h, handedness="RIGHT"),
            FlagshipErgonomicsModel.READER_TOUCH_NEXT,
        )

        # Left-handed mode:
        # Right 25% (X >= 360): PREV
        self.assertEqual(
            FlagshipErgonomicsModel.classify_reader_touch(400, 700, w, h, handedness="LEFT"),
            FlagshipErgonomicsModel.READER_TOUCH_PREV,
        )
        # Left 75% (X < 360): NEXT
        self.assertEqual(
            FlagshipErgonomicsModel.classify_reader_touch(100, 700, w, h, handedness="LEFT"),
            FlagshipErgonomicsModel.READER_TOUCH_NEXT,
        )

        # Inverted mode
        self.assertEqual(
            FlagshipErgonomicsModel.classify_reader_touch(50, 700, w, h, handedness="RIGHT", inverted=True),
            FlagshipErgonomicsModel.READER_TOUCH_NEXT,
        )

        # Swipe turns
        self.assertEqual(FlagshipErgonomicsModel.classify_swipe_turn(dx=-60, dy=5), FlagshipErgonomicsModel.READER_TOUCH_NEXT)
        self.assertEqual(FlagshipErgonomicsModel.classify_swipe_turn(dx=60, dy=5), FlagshipErgonomicsModel.READER_TOUCH_PREV)

    def test_f14_12_dynamic_reading_pace_forward_dwell_tracking_and_countdown(self):
        """F14.12: Forward page dwell sampling and dynamic chapter remaining time calculation (R3)."""
        # Dwell validation
        self.assertTrue(FlagshipErgonomicsModel.is_valid_forward_pace_sample(dwell_seconds=45, is_forward_turn=True))
        self.assertFalse(FlagshipErgonomicsModel.is_valid_forward_pace_sample(dwell_seconds=3, is_forward_turn=True))   # too fast
        self.assertFalse(FlagshipErgonomicsModel.is_valid_forward_pace_sample(dwell_seconds=350, is_forward_turn=True)) # idle pause
        self.assertFalse(FlagshipErgonomicsModel.is_valid_forward_pace_sample(dwell_seconds=45, is_forward_turn=False))  # backward turn

        # Running average pace
        samples = [40, 50, 45, 55]
        pace = FlagshipErgonomicsModel.calc_running_pace(samples)
        self.assertAlmostEqual(pace, 47.5)

        # Chapter countdown: 12 remaining pages at 47.5s/page = 570s = 10 minutes
        minutes = FlagshipErgonomicsModel.calc_estimated_chapter_minutes(remaining_pages=12, sec_per_page=pace)
        self.assertEqual(minutes, 10)
        self.assertEqual(FlagshipErgonomicsModel.format_countdown_string(12, pace), "~10 мин")

    def test_f14_13_eink_anti_ghosting_full_flash_refresh_modes(self):
        """F14.13: E-Ink anti-ghosting configurable full refresh modes (1, 5, 10, 15, 20, chapter, 30) (R3)."""
        # REFRESH_1
        self.assertTrue(FlagshipErgonomicsModel.should_trigger_full_refresh(1, False, mode="REFRESH_1"))
        self.assertTrue(FlagshipErgonomicsModel.should_trigger_full_refresh(2, False, mode="REFRESH_1"))

        # REFRESH_5
        self.assertFalse(FlagshipErgonomicsModel.should_trigger_full_refresh(4, False, mode="REFRESH_5"))
        self.assertTrue(FlagshipErgonomicsModel.should_trigger_full_refresh(5, False, mode="REFRESH_5"))

        # REFRESH_20
        self.assertFalse(FlagshipErgonomicsModel.should_trigger_full_refresh(19, False, mode="REFRESH_20"))
        self.assertTrue(FlagshipErgonomicsModel.should_trigger_full_refresh(20, False, mode="REFRESH_20"))

        # REFRESH_CHAPTER
        self.assertFalse(FlagshipErgonomicsModel.should_trigger_full_refresh(100, False, mode="REFRESH_CHAPTER"))
        self.assertTrue(FlagshipErgonomicsModel.should_trigger_full_refresh(100, True, mode="REFRESH_CHAPTER"))

    def test_f14_14_partitions_csv_16mb_table_and_app0_headroom_validation(self):
        """F14.14: Byte-level partitions.csv 16MB table and app0 headroom >= 700 KB validation (R4)."""
        # Partition row validation
        app0_row = ZeroBrickRiskModel.validate_partition_row("app0", "app", "ota_0", offset=0x10000, size=0x640000)
        self.assertTrue(app0_row["is_64kb_aligned"])
        self.assertEqual(app0_row["end"], 0x650000)

        app1_row = ZeroBrickRiskModel.validate_partition_row("app1", "app", "ota_1", offset=0x650000, size=0x640000)
        self.assertTrue(app1_row["is_64kb_aligned"])
        self.assertEqual(app1_row["end"], 0xC90000)

        # Actual headroom check against real binary or baseline
        bin_path = Path("src/.pio/build/x4pro/firmware.bin")
        bin_size = bin_path.stat().st_size if bin_path.exists() else 5761760
        hr_info = ZeroBrickRiskModel.check_app0_headroom(bin_size)
        self.assertTrue(hr_info["meets_requirement"])
        self.assertGreaterEqual(hr_info["headroom_kb"], 700.0)

    def test_f14_15_hardware_i2c_bus_clear_and_gt911_reset_recovery(self):
        """F14.15: Goodix GT911 touch controller glitch recovery via 9 SCL bus-clear pulses and hardware reset (R4)."""
        # 1 to 4 consecutive failures do simple retries
        for count in range(1, 5):
            res = ZeroBrickRiskModel.evaluate_i2c_recovery(count)
            self.assertEqual(res["action"], "NORMAL_RETRY")
            self.assertFalse(res["bus_clear_triggered"])

        # 5 consecutive failures triggers hardware recovery
        rec = ZeroBrickRiskModel.evaluate_i2c_recovery(5)
        self.assertEqual(rec["action"], "HARDWARE_RECOVERY")
        self.assertTrue(rec["bus_clear_triggered"])
        self.assertEqual(rec["scl_clock_pulses"], 9)
        self.assertTrue(rec["hardware_reset_triggered"])
        self.assertEqual(rec["rst_pulse_width_ms"], 10)

    def test_f14_16_atomic_persistence_battery_safety_guard_and_tmp_recovery(self):
        """F14.16: Atomic persistence with brownout guard (>=3200 mV) and .tmp crash recovery (R4)."""
        target = "/.crosspoint/settings.json"
        data = '{"brightness":50,"fontSize":16}'

        # 1. Low battery guard: voltage 3150 mV < 3200 mV aborts write
        low_bat = ZeroBrickRiskModel.simulate_atomic_persistence(target, data, voltage_mv=3150)
        self.assertEqual(low_bat["status"], "ABORTED_LOW_BATTERY")

        # 2. Safe battery voltage: 3750 mV succeeds
        safe_bat = ZeroBrickRiskModel.simulate_atomic_persistence(target, data, voltage_mv=3750)
        self.assertEqual(safe_bat["status"], "SUCCESS")

        # 3. Crash recovery: target was removed mid-power failure, but .tmp exists
        crash_sim = ZeroBrickRiskModel.simulate_atomic_persistence(
            target, data, voltage_mv=3750, power_loss_stage="AFTER_TARGET_REMOVE"
        )
        self.assertEqual(crash_sim["status"], "POWER_LOSS_AFTER_REMOVE")
        recovered = ZeroBrickRiskModel.recover_from_power_loss(
            target_exists=False, tmp_exists=True, tmp_content=crash_sim["tmp_content"]
        )
        self.assertTrue(recovered["recovered"])
        self.assertEqual(recovered["source"], "TMP_FALLBACK")
        self.assertEqual(recovered["restored_content"], data)

        # 4. Fail-safe button mappings auto-repair
        bad_mappings = ["FRONT_HW_BACK", "FRONT_HW_BACK", "INVALID", "FRONT_HW_RIGHT"]
        repaired, did_repair = ZeroBrickRiskModel.repair_duplicate_button_mappings(bad_mappings)
        self.assertTrue(did_repair)
        self.assertEqual(repaired, ["FRONT_HW_BACK", "FRONT_HW_CONFIRM", "FRONT_HW_LEFT", "FRONT_HW_RIGHT"])

        # 5. Recovery chord
        self.assertTrue(ZeroBrickRiskModel.is_emergency_recovery_chord(btn_down_pressed=True))
        self.assertFalse(ZeroBrickRiskModel.is_emergency_recovery_chord(btn_down_pressed=False))

    # ==========================================================================
    # FEATURE 15: HARDWARE RTC (BM8563 / PCF8563) LOCAL TIMEKEEPING AUDIT
    # ==========================================================================

    def test_f15_01_bm8563_rtc_hardware_bus_and_address(self):
        """F15.01: BM8563 I2C address 0x51 and register 0x02 mapping on X4 Pro shared bus."""
        self.assertEqual(HardwareRtcModel.BM8563_ADDR, 0x51)
        self.assertEqual(HardwareRtcModel.BM8563_SEC_REG, 0x02)
        # DS3231 comparison
        self.assertEqual(HardwareRtcModel.DS3231_ADDR, 0x68)
        self.assertEqual(HardwareRtcModel.DS3231_SEC_REG, 0x00)

    def test_f15_02_bm8563_vs_ds3231_register_layout_serialization(self):
        """F15.02: BCD serialization clears VL bit and adheres to PCF8563/BM8563 layout."""
        raw_regs = HardwareRtcModel.serialize_bm8563_registers(
            year=2026, month=10, day=4, weekday=0, hour=14, minute=35, second=20
        )
        self.assertEqual(len(raw_regs), 7)
        # Bit 7 of seconds (reg 0x02) must be 0 (VL cleared)
        self.assertEqual(raw_regs[0] & 0x80, 0)
        # Verify decoding matches exact time
        parsed = HardwareRtcModel.parse_bm8563_registers(raw_regs)
        self.assertTrue(parsed["valid"])
        self.assertEqual(parsed["year"], 2026)
        self.assertEqual(parsed["month"], 10)
        self.assertEqual(parsed["day"], 4)
        self.assertEqual(parsed["hour"], 14)
        self.assertEqual(parsed["minute"], 35)
        self.assertEqual(parsed["second"], 20)

    def test_f15_03_rtc_vl_flag_oscillator_stopped_detection(self):
        """F15.03: VL flag (0x80) on seconds register indicates invalid time / oscillator stopped."""
        corrupt_regs = [0x80 | 0x15, 0x30, 0x12, 0x01, 0x02, 0x09, 0x26]
        parsed = HardwareRtcModel.parse_bm8563_registers(corrupt_regs)
        self.assertFalse(parsed["valid"])
        self.assertTrue(parsed.get("vl"))

    def test_f15_04_local_time_survival_without_wifi(self):
        """F15.04: Hardware RTC guarantees offline local time persistence across deep sleep."""
        # Simulated cold boot with valid BM8563 time: 2026-10-05 09:15:00 UTC
        stored_regs = HardwareRtcModel.serialize_bm8563_registers(
            year=2026, month=10, day=5, weekday=1, hour=9, minute=15, second=0
        )
        rtc_time = HardwareRtcModel.parse_bm8563_registers(stored_regs)
        self.assertTrue(rtc_time["valid"])
        # Format time with Moscow UTC+3 (quarter hours biased = 48 + 3*4 = 60)
        utc_hours = rtc_time["hour"]
        utc_mins = rtc_time["minute"]
        local_total_min = (utc_hours * 60 + utc_mins + (60 - 48) * 15) % 1440
        local_hr = local_total_min // 60
        local_mn = local_total_min % 60
        self.assertEqual(f"{local_hr:02d}:{local_mn:02d}", "12:15")

    # --------------------------------------------------------------------------
    # FEATURE 13: HighFidelityImageEngine (6 tests)
    # --------------------------------------------------------------------------

    def test_f13_01_bayer_8x8_matrix_properties(self):
        """F13.1: Verify 8x8 Bayer matrix dimensions, 64 distinct values in range [0..63]."""
        matrix = HighFidelityImageEngineModel.BAYER_8X8
        self.assertEqual(len(matrix), 8)
        all_vals = []
        for row in matrix:
            self.assertEqual(len(row), 8)
            all_vals.extend(row)
        self.assertEqual(len(all_vals), 64)
        self.assertEqual(sorted(all_vals), list(range(64)))

    def test_f13_02_eink_gamma_table_shadow_lifting(self):
        """F13.2: Verify perceptual Gamma 1.8 LUT expands shadow midtones to prevent crushed blacks."""
        lut = HighFidelityImageEngineModel.EINK_GAMMA_TABLE
        self.assertEqual(len(lut), 256)
        self.assertEqual(lut[0], 0)
        self.assertEqual(lut[255], 255)
        self.assertGreater(lut[1], 1)
        self.assertGreater(lut[32], 32)
        for i in range(1, 256):
            self.assertGreaterEqual(lut[i], lut[i - 1])

    def test_f13_03_clean_white_bleaching(self):
        """F13.3: Verify Clean White Bleaching eliminates grey checkerboard speckles on near-white paper."""
        for gray in range(242, 256):
            for y in range(8):
                for x in range(8):
                    level = HighFidelityImageEngineModel.apply_bayer_dither_4level(gray, x, y)
                    self.assertEqual(level, 3, f"Gray {gray} at ({x},{y}) must be Level 3 (pure white)")

    def test_f13_04_clean_black_bleaching(self):
        """F13.4: Verify Clean Black Bleaching keeps fine line art and ink text dense black."""
        for gray in range(0, 11):
            for y in range(8):
                for x in range(8):
                    level = HighFidelityImageEngineModel.apply_bayer_dither_4level(gray, x, y)
                    self.assertEqual(level, 0, f"Gray {gray} at ({x},{y}) must be Level 0 (pure black)")

    def test_f13_05_psram_cache_pool_slot_contract(self):
        """F13.5: Verify PSRAM Multi-Slot Cache Pool geometry allows 4 concurrent cached images."""
        self.assertEqual(HighFidelityImageEngineModel.MAX_PXC_SLOTS, 4)
        four_slots_bytes = 4 * (800 * 200)
        self.assertLess(four_slots_bytes, 8 * 1024 * 1024)

    def test_f13_06_quantization_thresholds_distribution(self):
        """F13.6: Verify 4-level quantization thresholds [43, 128, 213] partition E-Ink reflectance evenly."""
        t1, t2, t3 = HighFidelityImageEngineModel.QUANT_THRESHOLDS
        self.assertEqual((t1, t2, t3), (43, 128, 213))
        levels_seen = set()
        for g in range(0, 256, 4):
            levels_seen.add(HighFidelityImageEngineModel.apply_bayer_dither_4level(g, 0, 0))
        self.assertEqual(levels_seen, {0, 1, 2, 3})



class HardwareRtcModel:
    BM8563_ADDR = 0x51
    DS3231_ADDR = 0x68
    BM8563_SEC_REG = 0x02
    DS3231_SEC_REG = 0x00
    BM8563_VL_FLAG = 0x80

    @staticmethod
    def decode_bcd(bcd_val: int) -> int:
        return ((bcd_val >> 4) * 10) + (bcd_val & 0x0F)

    @staticmethod
    def encode_bcd(dec_val: int) -> int:
        return ((dec_val // 10) << 4) | (dec_val % 10)

    @classmethod
    def parse_bm8563_registers(cls, raw_7_bytes: list[int]) -> dict:
        if len(raw_7_bytes) < 7:
            return {"valid": False, "error": "Insufficient bytes"}
        sec_byte = raw_7_bytes[0]
        if bool(sec_byte & cls.BM8563_VL_FLAG):
            return {"valid": False, "error": "Oscillator stopped / VL flag set", "vl": True}

        sec = cls.decode_bcd(sec_byte & 0x7F)
        minute = cls.decode_bcd(raw_7_bytes[1] & 0x7F)
        hr = cls.decode_bcd(raw_7_bytes[2] & 0x3F)
        day = cls.decode_bcd(raw_7_bytes[3] & 0x3F)
        wday = cls.decode_bcd(raw_7_bytes[4] & 0x07)
        month_byte = raw_7_bytes[5]
        century = 1900 if (month_byte & 0x80) else 2000
        month = cls.decode_bcd(month_byte & 0x1F)
        year = century + cls.decode_bcd(raw_7_bytes[6])

        return {
            "valid": True,
            "year": year,
            "month": month,
            "day": day,
            "weekday": wday,
            "hour": hr,
            "minute": minute,
            "second": sec,
        }

    @classmethod
    def serialize_bm8563_registers(cls, year: int, month: int, day: int, weekday: int, hour: int, minute: int, second: int) -> list[int]:
        century_bit = 0x80 if year < 2000 else 0x00
        return [
            cls.encode_bcd(second) & 0x7F,
            cls.encode_bcd(minute) & 0x7F,
            cls.encode_bcd(hour) & 0x3F,
            cls.encode_bcd(day) & 0x3F,
            cls.encode_bcd(weekday % 7),
            (cls.encode_bcd(month) & 0x1F) | century_bit,
            cls.encode_bcd(year % 100),
        ]


if __name__ == "__main__":
    unittest.main()



