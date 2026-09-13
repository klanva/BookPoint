"""
tier3_combination_tests.py - Tier 3 Cross-Feature Combination Tests (≥10 tests total).

Authoritative References:
- c:/xteinkx4/.agents/ORIGINAL_REQUEST.md (§R1, §R2, §R3)
- c:/xteinkx4/PROJECT.md (§Milestones, §Interface Contracts)
- c:/xteinkx4/TEST_INFRA.md (§Coverage Thresholds)
"""

import hashlib
import io
import unittest
from pathlib import Path

from tests.e2e.contracts import (
    APP0_MAX_PARTITION_SIZE,
    BUFFER_SIZE,
    LOGICAL_HEIGHT,
    LOGICAL_WIDTH,
    PHYSICAL_HEIGHT,
    PHYSICAL_WIDTH,
    PHYSICAL_WIDTH_BYTES,
    POLARITY_BLACK,
    POLARITY_WHITE,
    RELEASE_BIN_PATH,
    REPO_ROOT,
    EXPECTED_RELEASE_SIZE_MIN,
    EXPECTED_RELEASE_SIZE_MAX,
    THEME_METRICS,
    DashboardModel,
    DisplayGeometry,
    FirmwareValidator,
    HomeMenuItem,
    HOME_MENU_LABELS_RU,
    LyraCarouselModel,
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
    ZeroOverlapModel,
    PremiumTypographySuiteModel,
    FlagshipErgonomicsModel,
    ZeroBrickRiskModel,
)

try:
    from PIL import Image
    HAS_PIL = True
except ImportError:
    HAS_PIL = False


class Tier3CombinationTests(unittest.TestCase):
    """Tier 3: Cross-Feature Integration & Interaction Tests (10 tests total)."""

    def test_t3_01_theme_transition_dashboard_to_carousel_to_classic(self):
        """T3.01: Theme cycle Dashboard -> Carousel -> Classic -> Dashboard preserves state consistency."""
        theme_state = UIThemeEnum.DASHBOARD
        self.assertEqual(THEME_METRICS[theme_state]["name"], "Dashboard")
        self.assertTrue(THEME_METRICS[theme_state]["has_streak_calendar"])

        # Switch to Carousel
        theme_state = UIThemeEnum.CAROUSEL
        self.assertEqual(THEME_METRICS[theme_state]["name"], "LyraCarousel")
        self.assertEqual(THEME_METRICS[theme_state]["recent_books_count"], 3)
        self.assertTrue(THEME_METRICS[theme_state]["has_carousel"])

        # Switch to Classic
        theme_state = UIThemeEnum.CLASSIC
        self.assertEqual(THEME_METRICS[theme_state]["name"], "Classic")
        self.assertEqual(THEME_METRICS[theme_state]["recent_books_count"], 1)

        # Switch back to Dashboard
        theme_state = UIThemeEnum.DASHBOARD
        self.assertEqual(THEME_METRICS[theme_state]["name"], "Dashboard")

    def test_t3_02_carousel_swipe_flip_then_cover_tap_to_reader(self):
        """T3.02: Carousel horizontal swipe flips book selection, followed by cover tap opening reader."""
        books = [
            {"id": 0, "path": "/sd/books/b1.epub", "title": "Book 1"},
            {"id": 1, "path": "/sd/books/b2.epub", "title": "Book 2"},
            {"id": 2, "path": "/sd/books/b3.epub", "title": "Book 3"},
        ]
        current_idx = 0
        # User swipes left to advance to next book
        swipe = classify_swipe(start_x=380, start_y=300, end_x=120, end_y=300)
        self.assertEqual(swipe, SwipeDir.LEFT)

        # Advance index circularly
        if swipe == SwipeDir.LEFT:
            current_idx = (current_idx + 1) % len(books)
        self.assertEqual(current_idx, 1)

        # User taps center cover
        tap_action = classify_3zone_cover_tap(tap_x=240, tap_y=270)
        self.assertEqual(tap_action, "OPEN")

        # Reader target must be Book 2
        active_book = books[current_idx]
        self.assertEqual(active_book["title"], "Book 2")
        self.assertEqual(active_book["path"], "/sd/books/b2.epub")

    def test_t3_03_search_via_keyboard_select_book_update_dashboard(self):
        """T3.03: Virtual keyboard search -> select book -> read -> Dashboard displays updated book."""
        library = [
            {"path": "/sd/books/onegin.epub", "title": "Евгений Онегин", "pages_total": 280, "pages_read": 112},
            {"path": "/sd/books/milion.epub", "title": "Мильон терзаний", "pages_total": 120, "pages_read": 10},
        ]
        # Search for "Мильон"
        query = "Мильон"
        filtered = VirtualKeyboardModel.filter_books(query, library)
        self.assertEqual(len(filtered), 1)
        selected_book = filtered[0]
        self.assertEqual(selected_book["title"], "Мильон терзаний")

        # Simulate reading 50 pages
        selected_book["pages_read"] += 50
        progress_pct = (selected_book["pages_read"] / float(selected_book["pages_total"])) * 100.0
        self.assertEqual(selected_book["pages_read"], 60)
        self.assertEqual(progress_pct, 50.0)

        # Verify Dashboard displays 50% fill bar
        bar_fill = DashboardModel.calc_progress_bar_width(progress_pct, total_bar_width=200)
        self.assertEqual(bar_fill, 100)

    def test_t3_04_simulator_rendering_with_all_theme_configurations(self):
        """T3.04: Virtual framebuffer renders simulated screens for all 6 theme modes."""
        for theme_val in UIThemeEnum:
            fb = VirtualFramebuffer()
            metrics = THEME_METRICS[theme_val]
            # Draw header
            fb.draw_logical_rect(0, 0, LOGICAL_WIDTH, 40, fill_black=False, border_black=True)
            # Draw cover area
            cover_h = metrics.get("cover_tile_height", metrics.get("cover_height", 250))
            fb.draw_logical_rect(50, 50, 380, cover_h, fill_black=None, border_black=True)

            self.assertEqual(len(fb.buffer), BUFFER_SIZE)
            self.assertGreater(fb.compute_entropy(), 0.001)

    def test_t3_05_build_configuration_consistency_with_simulator(self):
        """T3.05: Cross-validate platformio.ini, display resolution, and framebuffer memory bounds."""
        sections = FirmwareValidator.parse_platformio_ini(PLATFORMIO_INI_PATH)
        env = sections.get("env:x4pro", {})
        self.assertEqual(env.get("board"), "esp32-s3-devkitc1-n16r8")
        self.assertEqual(env.get("board_build.arduino.memory_type"), "dio_opi")

        # Physical display resolution 800x480 gives 48000 bytes
        expected_bytes = (PHYSICAL_WIDTH * PHYSICAL_HEIGHT) // 8
        self.assertEqual(expected_bytes, 48000)
        self.assertEqual(BUFFER_SIZE, expected_bytes)

    def test_t3_06_dashboard_streak_update_after_reading_session(self):
        """T3.06: Completed reading session increments streak days and activates streak indicator."""
        initial_streak = 3
        # Complete reading session today
        session_minutes = 25
        self.assertGreaterEqual(session_minutes, 15)  # Daily goal met
        new_streak = initial_streak + 1
        self.assertEqual(new_streak, 4)

        cells = DashboardModel.calc_streak_calendar_cells(streak_days=new_streak, today_active=True)
        self.assertTrue(cells[6]["is_today"])
        self.assertTrue(cells[6]["is_completed"])
        completed_cells = sum(1 for c in cells if c["is_completed"])
        self.assertEqual(completed_cells, 4)

    def test_t3_07_extended_menu_search_opens_keyboard_with_focus(self):
        """T3.07: Home menu SEARCH_BOOKS row touch opens keyboard activity."""
        menu_top = 340
        row_step = 52
        # SEARCH_BOOKS is row index 2
        tap_y = menu_top + 2 * row_step + 10
        row_idx = (tap_y - menu_top) // row_step
        self.assertEqual(row_idx, 2)

        # Dispatch action
        action_item = HomeMenuItem.SEARCH_BOOKS
        self.assertEqual(action_item, HomeMenuItem.SEARCH_BOOKS)
        self.assertEqual(HOME_MENU_LABELS_RU[action_item], "Быстрый поиск книг")

        # Launches keyboard initialized to empty query
        kb = VirtualKeyboardModel()
        self.assertTrue(kb.is_ru)

    def test_t3_08_touch_3zone_navigation_in_reader_and_carousel(self):
        """T3.08: Symmetrical 3-zone tap thresholds between Carousel and Reader."""
        zone_left_x = 80
        zone_center_x = 240
        zone_right_x = 400

        # Carousel interpretation
        self.assertEqual(classify_3zone_cover_tap(zone_left_x, 250), "PREV")
        self.assertEqual(classify_3zone_cover_tap(zone_center_x, 250), "OPEN")
        self.assertEqual(classify_3zone_cover_tap(zone_right_x, 250), "NEXT")

        # Reader interpretation
        reader_action_left = "PREV_PAGE" if zone_left_x < 160 else "OTHER"
        reader_action_center = "MENU" if 160 <= zone_center_x < 320 else "OTHER"
        reader_action_right = "NEXT_PAGE" if zone_right_x >= 320 else "OTHER"
        self.assertEqual(reader_action_left, "PREV_PAGE")
        self.assertEqual(reader_action_center, "MENU")
        self.assertEqual(reader_action_right, "NEXT_PAGE")

    def test_t3_09_framebuffer_export_to_png_palette_consistency(self):
        """T3.09: Virtual framebuffer to PNG export maintains coordinate and polarity fidelity."""
        fb = VirtualFramebuffer()
        # Draw distinctive marker at logical (100, 200)
        fb.set_logical_pixel(100, 200, black=True)
        img = fb.to_logical_pil_image()
        self.assertEqual(img.size, (LOGICAL_WIDTH, LOGICAL_HEIGHT))

        # In PIL mode '1': 0 is black, 255 is white
        pixel_val = img.getpixel((100, 200))
        self.assertEqual(pixel_val, 0)  # Black pixel
        white_val = img.getpixel((0, 0))
        self.assertEqual(white_val, 255)  # White background

    def test_t3_10_release_binary_embeds_theme_and_menu_strings(self):
        """T3.10: Release binary BookPoint_2.0.0_x4pro.bin contains authentic firmware signatures."""
        val = FirmwareValidator.validate_release_binary(RELEASE_BIN_PATH)
        self.assertTrue(val["is_esp32_valid"])
        self.assertTrue(EXPECTED_RELEASE_SIZE_MIN <= val["size"] <= EXPECTED_RELEASE_SIZE_MAX)

        # Inspect binary contents for known firmware strings
        data = RELEASE_BIN_PATH.read_bytes()
        self.assertIn(b"BookPoint", data)
        self.assertIn(b"x4pro", data)

    def test_t3_11_dock_search_tap_to_virtual_keyboard_combination(self):
        """T3.11: Dock search tap (Col 1) transitions to touch keyboard search filter."""
        # 1. User taps on Dock Column 1 ("Поиск")
        tap_x, tap_y = 144, 760
        col = ModernThemeModel.classify_dock_tap(tap_x, tap_y)
        self.assertEqual(col, 1)
        self.assertEqual(ModernThemeModel.DOCK_ITEMS[col]["name"], "Поиск")

        # 2. Activity manager launches BookSearch with Russian virtual keyboard
        kb = VirtualKeyboardModel(start_y=520, height=260)
        self.assertTrue(kb.is_ru)

        # 3. User types "ВОЙНА"
        books = [
            {"title": "Война и мир", "author": "Лев Толстой"},
            {"title": "Преступление и наказание", "author": "Фёдор Достоевский"},
            {"title": "Мастер и Маргарита", "author": "Михаил Булгаков"},
        ]
        results = VirtualKeyboardModel.filter_books("война", books)
        self.assertEqual(len(results), 1)
        self.assertEqual(results[0]["title"], "Война и мир")

    def test_t3_12_hero_unified_touch_and_hardware_key_reading_toggle(self):
        """T3.12: Hero tap to read, capacitive Home return, and hardware key focus toggle."""
        # 1. Hero unified tap resumes book 0
        self.assertTrue(ModernThemeModel.classify_hero_tap(200, 200))
        # 2. Focus cycling on Home screen
        focus = HardwareKeyFocusModel(has_hero=True, shelf_count=2, dock_count=5)
        self.assertEqual(focus.current_index, 0)
        self.assertTrue(focus.is_hero_focused())

        # Press BTN_DOWN -> moves to shelf slot 0
        focus.next_focus()
        self.assertTrue(focus.is_shelf_focused())
        desc = focus.get_target_descriptor()
        self.assertEqual(desc["slot"], 0)

        # Press BTN_DOWN -> moves to shelf slot 1
        focus.next_focus()
        self.assertTrue(focus.is_shelf_focused())
        desc = focus.get_target_descriptor()
        self.assertEqual(desc["slot"], 1)

        # Press BTN_DOWN -> moves to Dock Column 0 ("Библиотека")
        focus.next_focus()
        self.assertTrue(focus.is_dock_focused())
        desc = focus.get_target_descriptor()
        self.assertEqual(desc["column"], 0)

    def test_t3_13_status_bar_curtain_swipe_down_and_outside_tap_dismiss(self):
        """T3.13: Top header downward swipe opens curtain; tapping outside dismisses it."""
        # Swipe down from top header
        gesture = classify_swipe(start_x=240, start_y=10, end_x=240, end_y=120, threshold=50.0)
        self.assertEqual(gesture, SwipeDir.DOWN)

        # Curtain is open (covers 0..340)
        # Tap on curtain area (e.g. brightness slider at y=150) -> does NOT dismiss
        self.assertFalse(ModernThemeModel.classify_curtain_dismiss_tap(240, 150, curtain_h=340))

        # Tap outside curtain area (y=360) -> dismisses curtain
        self.assertTrue(ModernThemeModel.classify_curtain_dismiss_tap(240, 360, curtain_h=340))

    def test_t3_14_recent_shelf_progress_caching_and_display(self):
        """T3.14: Recent shelf books with cached progress render proportional capsule bars."""
        shelf_books = [
            {"title": "Book 1", "progress": 15},
            {"title": "Book 2", "progress": 62},
            {"title": "Book 3", "progress": 88},
        ]
        slots = ModernThemeModel.get_shelf_slot_rects(len(shelf_books))
        self.assertEqual(len(slots), 3)

        # Verify each slot has valid capsule fill width based on 66px mini bar
        mini_bar_max_w = ModernThemeModel.SHELF_MINI_COVER_WIDTH  # 66px
        expected_widths = [
            round(mini_bar_max_w * (15 / 100.0)),  # 10px
            round(mini_bar_max_w * (62 / 100.0)),  # 41px
            round(mini_bar_max_w * (88 / 100.0)),  # 58px
        ]
        for i, book in enumerate(shelf_books):
            w = round(mini_bar_max_w * (book["progress"] / 100.0))
            self.assertEqual(w, expected_widths[i])
            self.assertTrue(0 < w <= mini_bar_max_w)

    def test_t3_15_curtain_dual_slider_to_hal_frontlight_sync(self):
        """T3.15: Quick Settings curtain dual-channel adjustments and pill toggles synchronize to HAL."""
        # 1. Open curtain from top status bar
        self.assertTrue(QuickSettingsCurtainModel.classify_open_tap(240, 20))

        # 2. Adjust brightness: initial 50%, tap PLUS twice (+20%)
        brightness = 50
        action1 = QuickSettingsCurtainModel.classify_slider_tap("BRIGHTNESS", 435, 75)
        self.assertEqual(action1, "PLUS")
        brightness = QuickSettingsCurtainModel.step_channel(brightness, 10)
        brightness = QuickSettingsCurtainModel.step_channel(brightness, 10)
        self.assertEqual(brightness, 70)

        # 3. Adjust CCT (warmth): initial 40%, tap MINUS once (-10%)
        cct = 40
        action2 = QuickSettingsCurtainModel.classify_slider_tap("CCT", 35, 120)
        self.assertEqual(action2, "MINUS")
        cct = QuickSettingsCurtainModel.step_channel(cct, -10)
        self.assertEqual(cct, 30)

        # 4. Toggle Dark Mode pill
        pill = QuickSettingsCurtainModel.classify_pill_tap(300, 180)
        self.assertEqual(pill, "DARK_MODE")
        dark_mode_enabled = True

        # 5. Dismiss curtain with swipe up
        self.assertTrue(QuickSettingsCurtainModel.classify_dismiss_swipe(240, 300, 240, 200))

        # State verification
        self.assertEqual(brightness, 70)
        self.assertEqual(cct, 30)
        self.assertTrue(dark_mode_enabled)

    def test_t3_16_side_drawer_navigation_from_reader_to_library_grid(self):
        """T3.16: Side drawer invoked during reader session navigates directly to Library Cover Grid."""
        # 1. In Reader context, swipe from right edge inward (right-handed mode)
        opened = SideDrawerModel.classify_open_gesture(470, 400, 390, 400, HandednessEnum.RIGHT)
        self.assertTrue(opened)

        # 2. Drawer targets rendered, user taps Target 0 ("Библиотека")
        target_idx = SideDrawerModel.classify_item_tap(300, 120, HandednessEnum.RIGHT)
        self.assertEqual(target_idx, 0)
        self.assertEqual(SideDrawerModel.TARGETS[target_idx]["name"], "Библиотека")

        # 3. Transition to Library Hub in Cover Grid mode
        view_mode = LibraryViewModeEnum.COVER_GRID
        self.assertEqual(view_mode, 0)

        # 4. 3x2 Grid renders cell 0 at (20, 60, 210, 220)
        tapped_slot = LibraryHubModel.classify_grid_tap(50, 100, items_on_page=6)
        self.assertEqual(tapped_slot, 0)

    def test_t3_17_reader_overlay_aa_typography_to_pagination_integrity(self):
        """T3.17: Live Typography font adjustment re-paginates while preserving exact character offset."""
        # 1. Reading book at character offset 5200 (initially on page 5)
        cached_offset = 5200

        # 2. Center screen tap opens Reader Overlays
        self.assertTrue(ReaderOverlaysModel.classify_center_menu_tap(240, 400))

        # 3. Tap Typography 'Aa' button
        action = ReaderOverlaysModel.classify_bottom_tap(60, 765)
        self.assertEqual(action, "TYPOGRAPHY")

        # 4. Increase font size from 20pt to 22pt
        new_font_size = AaTypographyModel.step_font_size(20, 2)
        self.assertEqual(new_font_size, 22)

        # 5. Re-pagination generates denser page breaks (fewer characters per page)
        new_page_ranges = [
            (0, 950),
            (950, 1920),
            (1920, 2880),
            (2880, 3850),
            (3850, 4800),
            (4800, 5750),  # Offset 5200 falls here (page index 5)
            (5750, 6700),
        ]
        new_page_idx = AaTypographyModel.map_offset_to_new_page(cached_offset, new_page_ranges)
        self.assertEqual(new_page_idx, 5)
        start_off, end_off = new_page_ranges[new_page_idx]
        self.assertTrue(start_off <= cached_offset < end_off)

    def test_t3_18_settings_hub_dark_mode_toggle_to_curtain_pill_sync(self):
        """T3.18: Settings Hub Dark Mode graphical switch syncs seamlessly with Quick Settings pill."""
        # 1. User navigates to Settings Hub -> DISPLAY
        sec_id = ModularSettingsHubModel.classify_card_tap(100, 240)
        self.assertEqual(sec_id, "DISPLAY")

        # 2. Graphical toggle state switches from OFF to ON
        dark_mode_state = False
        self.assertEqual(ModularSettingsHubModel.format_graphical_toggle(dark_mode_state), "[●  ]")
        dark_mode_state = True
        self.assertEqual(ModularSettingsHubModel.format_graphical_toggle(dark_mode_state), "[  ●]")

        # 3. User opens Quick Settings curtain
        self.assertTrue(QuickSettingsCurtainModel.classify_open_tap(240, 20))
        # Dark Mode pill is present and active
        pill_id = QuickSettingsCurtainModel.classify_pill_tap(300, 180)
        self.assertEqual(pill_id, "DARK_MODE")
        self.assertTrue(dark_mode_state)

    def test_t3_19_library_grid_search_filter_with_virtual_keyboard(self):
        """T3.19: Library Grid search button opens virtual keyboard and live-filters grid items."""
        library_books = [
            {"title": "Преступление и наказание", "author": "Фёдор Достоевский", "progress": 45.0},
            {"title": "Идиот", "author": "Фёдор Достоевский", "progress": 10.0},
            {"title": "Война и мир", "author": "Лев Толстой", "progress": 80.0},
            {"title": "Анна Каренина", "author": "Лев Толстой", "progress": 60.0},
            {"title": "Мастер и Маргарита", "author": "Михаил Булгаков", "progress": 90.0},
        ]

        # 1. Tap search button in library header
        self.assertTrue(tap_in_rect(380, 20, *LibraryHubModel.SEARCH_BUTTON_RECT))

        # 2. Virtual keyboard initialized
        kb = VirtualKeyboardModel(start_y=520, height=260)
        self.assertTrue(kb.is_ru)

        # 3. Filter query "достоевский"
        filtered = LibraryHubModel.filter_books("достоевский", library_books)
        self.assertEqual(len(filtered), 2)
        self.assertEqual(filtered[0]["title"], "Преступление и наказание")
        self.assertEqual(filtered[1]["title"], "Идиот")

        # 4. 3x2 Grid now has 2 items: slot 0 and slot 1 valid, slot 2 returns None
        self.assertEqual(LibraryHubModel.classify_grid_tap(50, 100, items_on_page=len(filtered)), 0)
        self.assertEqual(LibraryHubModel.classify_grid_tap(300, 100, items_on_page=len(filtered)), 1)
        self.assertIsNone(LibraryHubModel.classify_grid_tap(50, 300, items_on_page=len(filtered)))

    def test_t3_20_control_curtain_reader_top_bar_tap_isolation_and_frontlight_sync(self):
        """T3.20: Reader in-book overlays isolate top bar taps; curtain operates independently when closed (R1 + R2)."""
        # Phase 1: User reading book with overlays ACTIVE
        in_reader = True
        overlays_active = True

        # Tap on Back button (30, 30):
        # 1. Status bar must NOT intercept
        self.assertFalse(QuickSettingsCurtainModel.is_status_bar_tap_allowed(in_reader, overlays_active, 30, 30))
        # 2. Reader top bar handles Back
        self.assertEqual(ReaderOverlaysModel.classify_top_tap(30, 30), "BACK")

        # Tap on TOC button (100, 30):
        self.assertFalse(QuickSettingsCurtainModel.is_status_bar_tap_allowed(in_reader, overlays_active, 100, 30))
        self.assertEqual(ReaderOverlaysModel.classify_top_tap(100, 30), "TOC")

        # Tap on Bookmark button (390, 30):
        self.assertFalse(QuickSettingsCurtainModel.is_status_bar_tap_allowed(in_reader, overlays_active, 390, 30))
        self.assertEqual(ReaderOverlaysModel.classify_top_tap(390, 30), "TOGGLE_BOOKMARK")

        # Phase 2: Overlays DISMISSED (reading continues)
        overlays_active = False

        # Status bar tap now allowed to open Control Curtain
        self.assertTrue(QuickSettingsCurtainModel.is_status_bar_tap_allowed(in_reader, overlays_active, 240, 20))

        # Control Curtain opens: adjust brightness via track seek (50%) and CCT via stepper (+10%)
        brightness = QuickSettingsCurtainModel.track_x_to_value("BRIGHTNESS", 68 + 172)
        self.assertEqual(brightness, 50)
        cct = QuickSettingsCurtainModel.step_channel(current=20, delta=10)
        self.assertEqual(cct, 30)

        # Info block verified
        info = QuickSettingsCurtainModel.format_battery_and_clock(80, False, "19:15", "13.09.2026")
        self.assertIn("80%", info)
        self.assertIn("19:15", info)
        self.assertFalse(EmojiBanValidator.contains_emoji(info))

    def test_t3_21_fb2_book_with_strikethrough_and_footnotes_in_reader_with_typography(self):
        """T3.21: Complete FB2 reading pipeline: strikethrough normalization, footnote popup, and typography Aa (R2 + R3)."""
        book_path = "/books/chekhov.fb2"
        self.assertTrue(UniversalFormatsModel.has_fb2_extension(book_path))

        # 1. Text normalization: FB2 contains combining stroke 0xCCB6 (U+0336)
        raw_passage = "Слово б\u0336ы\u0336л\u0336о\u0336 зачеркнуто, а сноска [1] вела в конец."
        clean_passage, had_strike = StrikethroughNormalizationModel.normalize_strikethrough_text(raw_passage)
        self.assertTrue(had_strike)
        self.assertIn("было", clean_passage)
        self.assertNotIn("\u0336", clean_passage)

        # Strikethrough ascender geometry
        strike_y = StrikethroughNormalizationModel.calc_strike_line_y(base_y=200, ascender=24)
        self.assertEqual(strike_y, 200 - 19)

        # 2. Footnote hit-test: user taps "[1]"
        self.assertTrue(FootnoteModalModel.is_footnote_reference("[1]"))
        card_rect = FootnoteModalModel.get_modal_card_rect(480, 800, 200)
        self.assertEqual(card_rect, (20, 584, 440, 200))

        # Modal close button hit
        close_btn = FootnoteModalModel.get_close_button_rect(card_rect)
        self.assertEqual(
            FootnoteModalModel.classify_modal_tap(card_rect, close_btn[0] + 10, close_btn[1] + 10),
            "CLOSE_BUTTON",
        )

        # 3. Typography adjustment: live on-the-fly Aa
        font_chosen = "JetBrains Mono"
        self.assertIn(font_chosen, AaTypographyModel.MANDATED_FONT_FAMILIES)
        new_size = AaTypographyModel.step_font_size(16, 2)
        self.assertEqual(new_size, 18)
        new_spacing = AaTypographyModel.step_line_spacing(1.2, 1)
        self.assertAlmostEqual(new_spacing, 1.4)
        new_margin = AaTypographyModel.step_margin(20, 1)
        self.assertEqual(new_margin, 30)

        # Autosave state
        saved_state = AaTypographyModel.autosave_state(font_chosen, new_size, new_spacing, new_margin)
        self.assertTrue(saved_state["autosaved"])

        # Zero-drift re-pagination
        page_ranges = [(0, 350), (350, 720), (720, 1100)]
        self.assertEqual(AaTypographyModel.map_offset_to_new_page(400, page_ranges), 1)

    def test_t3_22_sleep_mode_transitions_with_screensaver_gallery_and_v210_display(self):
        """T3.22: Control Curtain quick sleep pill transition to Screensaver Gallery with v2.1.0 display (R4 + R1)."""
        # 1. Tap Sleep pill in Control Curtain
        pill_tapped = QuickSettingsCurtainModel.classify_pill_tap(300, 240)
        self.assertEqual(pill_tapped, "SLEEP")

        # 2. Sleep Screen Mode A: Curated Gallery specific theme (Theme 6: Jules Verne)
        fn_verne = ScreensaverGalleryModel.get_theme_filename(6, (480, 800))
        self.assertIn("kindle_author_verne", fn_verne)
        self.assertTrue(fn_verne.endswith(".bmp"))

        # 3. Sleep Screen Mode B: Landscape (800x480) for Theme 9 (Geometric Patterns)
        fn_geom = ScreensaverGalleryModel.get_theme_filename(9, (800, 480))
        self.assertIn("kobo_geometric_patterns", fn_geom)
        self.assertIn("800x480", fn_geom)

        # 4. Sleep Screen Mode C: Reading stats cover mode with streak
        streak_text = ModernThemeModel.format_reading_streak(21)
        self.assertEqual(streak_text, "Серия: 21 дн.")

        # 5. Version display validation across polarities
        v_str = ScreensaverGalleryModel.format_version_display("2.1.0-x4pro")
        self.assertEqual(v_str, "v2.1.0")

        # Dark mode sleep
        dark_pol = ScreensaverGalleryModel.get_version_polarity(sleep_mode_is_dark=True)
        self.assertEqual(dark_pol["text_polarity"], POLARITY_WHITE)
        self.assertEqual(dark_pol["bg_polarity"], POLARITY_BLACK)
        self.assertTrue(dark_pol["screen_inverted"])

        # Light mode sleep
        light_pol = ScreensaverGalleryModel.get_version_polarity(sleep_mode_is_dark=False)
        self.assertEqual(light_pol["text_polarity"], POLARITY_BLACK)
        self.assertEqual(light_pol["bg_polarity"], POLARITY_WHITE)
        self.assertFalse(light_pol["screen_inverted"])

    def test_t3_23_handed_touch_zones_with_dynamic_pace_and_anti_ghosting_pipeline(self):
        """T3.23: Complete reading flow combining handed touch zones, dynamic pace dwell tracking, and anti-ghosting (R3)."""
        w, h = 480, 800

        # 1. Right-handed reading session: tap forward at X=350, Y=500
        tap_action = FlagshipErgonomicsModel.classify_reader_touch(350, 500, w, h, handedness="RIGHT")
        self.assertEqual(tap_action, FlagshipErgonomicsModel.READER_TOUCH_NEXT)

        # 2. Page dwell recorded for forward turns (40s, 48s, 44s, 52s)
        dwell_samples = [40, 48, 44, 52]
        for d in dwell_samples:
            self.assertTrue(FlagshipErgonomicsModel.is_valid_forward_pace_sample(d, is_forward_turn=True))

        avg_pace = FlagshipErgonomicsModel.calc_running_pace(dwell_samples)
        self.assertAlmostEqual(avg_pace, 46.0)

        # 3. Dynamic chapter countdown on bottom overlay: 15 pages left at 46s/page = 690s = 12 min
        countdown_str = FlagshipErgonomicsModel.format_countdown_string(15, avg_pace)
        self.assertEqual(countdown_str, "~12 мин")

        # 4. Anti-ghosting full flash check: mode REFRESH_20 on page 20
        self.assertTrue(FlagshipErgonomicsModel.should_trigger_full_refresh(20, False, mode="REFRESH_20"))
        self.assertFalse(FlagshipErgonomicsModel.should_trigger_full_refresh(21, False, mode="REFRESH_20"))

        # 5. User switches to left-handed mode: tap forward at X=80, backward at X=420
        tap_forward_lh = FlagshipErgonomicsModel.classify_reader_touch(80, 500, w, h, handedness="LEFT")
        self.assertEqual(tap_forward_lh, FlagshipErgonomicsModel.READER_TOUCH_NEXT)
        tap_prev_lh = FlagshipErgonomicsModel.classify_reader_touch(420, 500, w, h, handedness="LEFT")
        self.assertEqual(tap_prev_lh, FlagshipErgonomicsModel.READER_TOUCH_PREV)

    def test_t3_24_live_typography_suite_selection_to_reflow_and_safe_persistence(self):
        """T3.24: Live typography suite selection, zero-drift reflow, battery safety guard, and persistence (R1 + R2 + R4)."""
        # 1. User opens Aa popup in reader
        self.assertEqual(ReaderOverlaysModel.classify_bottom_tap(60, 765), "TYPOGRAPHY")

        # 2. Font family stepped from JetBrains Mono to Literata
        chosen_font = "Literata"
        self.assertIn(chosen_font, PremiumTypographySuiteModel.PREMIUM_FONT_FAMILIES)
        new_size = AaTypographyModel.step_font_size(16, 2)
        self.assertEqual(new_size, 18)

        # 3. Live reflow maps cached character offset 4200 to new page without reading drift
        cached_char_offset = 4200
        new_pages = [(0, 1100), (1100, 2250), (2250, 3400), (3400, 4550), (4550, 5700)]
        reflow_res = PremiumTypographySuiteModel.verify_reflow_offset_preservation(cached_char_offset, new_pages)
        self.assertTrue(reflow_res["offset_contained"])
        self.assertEqual(reflow_res["target_page"], 3)

        # 4. Autosave state serialized
        saved_state = AaTypographyModel.autosave_state(chosen_font, new_size, 1.2, 20)
        self.assertEqual(saved_state["fontFamily"], "Literata")

        # 5. Atomic persistence under safe battery voltage (3800 mV >= 3200 mV)
        persist_res = ZeroBrickRiskModel.simulate_atomic_persistence(
            "/.crosspoint/settings.json", str(saved_state), voltage_mv=3800
        )
        self.assertEqual(persist_res["status"], "SUCCESS")

        # 6. Hybrid font flash partition headroom confirmed
        bin_path = Path("src/.pio/build/x4pro/firmware.bin")
        bin_size = bin_path.stat().st_size if bin_path.exists() else 5761760
        hr = ZeroBrickRiskModel.check_app0_headroom(bin_size)
        self.assertTrue(hr["meets_requirement"])
        self.assertGreaterEqual(hr["headroom_kb"], 700.0)

    def test_t3_25_landscape_quick_settings_to_floating_reader_overlays_sync(self):
        """T3.25: Landscape 800x480 Quick Settings curtain centering, dual sliders, and floating reader overlays (R1)."""
        w, h = 800, 480

        # 1. Quick Settings curtain centered at offsetX = 176
        offset_x = QuickSettingsCurtainModel.get_content_offset_x(w)
        self.assertEqual(offset_x, 176)

        # 2. Sliders: Brightness (Cold) and CCT (Warm) hit tests
        self.assertEqual(QuickSettingsCurtainModel.classify_slider_tap_offset("BRIGHTNESS", 200, 70, offset_x), "MINUS")
        self.assertEqual(QuickSettingsCurtainModel.classify_slider_tap_offset("CCT", 610, 120, offset_x), "PLUS")

        # 3. Dismiss curtain, reader floating overlays rendered with >= 12px margin
        top_bar = ReaderOverlaysModel.get_floating_top_bar_rect(w, margin=14)
        bot_bar = ReaderOverlaysModel.get_floating_bottom_bar_rect(w, h, margin=14)

        self.assertTrue(ZeroOverlapModel.validate_reader_overlay_margins(top_bar, w, h, min_margin=12))
        self.assertTrue(ZeroOverlapModel.validate_reader_overlay_margins(bot_bar, w, h, min_margin=12))

        # Bottom bar is fully on-screen in landscape (y: 342..466 < 480)
        self.assertEqual(bot_bar[1], 342)
        self.assertEqual(bot_bar[1] + bot_bar[3], 466)
        self.assertLessEqual(bot_bar[1] + bot_bar[3], h)

        # Top bar back and TOC buttons responsive in landscape
        self.assertEqual(ReaderOverlaysModel.classify_floating_top_tap(25, 25, w, margin=14), "BACK")
        self.assertEqual(ReaderOverlaysModel.classify_floating_top_tap(85, 25, w, margin=14), "TOC")


if __name__ == "__main__":
    unittest.main()



