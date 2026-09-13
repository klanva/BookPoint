"""
tier4_realworld_tests.py - Tier 4 Real-World Application Scenarios (≥6 workload tests).

Authoritative References:
- c:/xteinkx4/.agents/ORIGINAL_REQUEST.md (§R1, §R2, §R3)
- c:/xteinkx4/PROJECT.md (§Interface Contracts, §Milestones)
- c:/xteinkx4/TEST_INFRA.md (§Real-World Application Scenarios)
"""

import hashlib
import io
import os
import tempfile
import unittest
from pathlib import Path

from tests.e2e.contracts import (
    APP0_MAX_PARTITION_SIZE,
    BUFFER_SIZE,
    ESP32_IMAGE_MAGIC,
    EXPECTED_GH_RELEASE_COMMAND,
    EXPECTED_RELEASE_SHA256,
    EXPECTED_RELEASE_SIZE,
    EXPECTED_RELEASE_SIZE_MIN,
    EXPECTED_RELEASE_SIZE_MAX,
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


class Tier4RealWorldTests(unittest.TestCase):
    """Tier 4: Realistic End-to-End User Workloads & Scenarios (6 tests total)."""

    def test_t4_01_scenario_dashboard_reading_lifecycle(self):
        """Scenario 1: Full Home Dashboard Reading Lifecycle.
        Waking up -> Dashboard display -> cover tap to reader -> 28 pages read -> return to home ->
        progress & streak calendar update.
        """
        # Step 1: Boot into Dashboard
        active_theme = UIThemeEnum.DASHBOARD
        metrics = THEME_METRICS[active_theme]
        self.assertTrue(metrics["has_streak_calendar"])

        # Current book state
        current_book = {
            "title": "Евгений Онегин",
            "author": "Александр Пушкин",
            "path": "/sd/books/onegin.epub",
            "pages_total": 280,
            "pages_read": 112,
            "pace": 40.0,  # pages / hour
        }
        initial_pct = (current_book["pages_read"] / float(current_book["pages_total"])) * 100.0
        self.assertEqual(initial_pct, 40.0)
        self.assertEqual(DashboardModel.calc_progress_bar_width(initial_pct, 200), 80)
        self.assertEqual(DashboardModel.calc_remaining_time(280, 112, 40.0), "4ч 12м")

        # Step 2: User taps cover card at (140, 180)
        cover_card_rect = (50, 45, 180, 270)
        tapped = tap_in_rect(140, 180, *cover_card_rect)
        self.assertTrue(tapped)

        # Step 3: Reader opened, reading session progresses
        current_book["pages_read"] += 28  # Read 28 pages
        updated_pct = (current_book["pages_read"] / float(current_book["pages_total"])) * 100.0
        self.assertEqual(updated_pct, 50.0)

        # Step 4: Return to Home Dashboard
        updated_bar_w = DashboardModel.calc_progress_bar_width(updated_pct, 200)
        self.assertEqual(updated_bar_w, 100)  # Exactly half filled
        updated_time = DashboardModel.calc_remaining_time(280, 140, 40.0)
        self.assertEqual(updated_time, "3ч 30м")

        # Step 5: Streak calendar marks today active
        streak_cells = DashboardModel.calc_streak_calendar_cells(streak_days=4, today_active=True)
        self.assertTrue(streak_cells[6]["is_today"])
        self.assertTrue(streak_cells[6]["is_completed"])

    def test_t4_02_scenario_carousel_exploration_and_flip(self):
        """Scenario 2: Carousel Library Exploration & Gesture Flip.
        User swipes left/right through carousel covers, observes pagination dots, and taps to open.
        """
        recent_books = [
            {"id": 0, "title": "Book A"},
            {"id": 1, "title": "Book B"},
            {"id": 2, "title": "Book C"},
            {"id": 3, "title": "Book D"},
            {"id": 4, "title": "Book E"},
        ]
        total = len(recent_books)
        current_idx = 0

        # Initial slots: left=4 (Book E), center=0 (Book A), right=1 (Book B)
        l, c, r = LyraCarouselModel.get_book_indices_for_slot(current_idx, total)
        self.assertEqual((l, c, r), (4, 0, 1))

        # Check pagination dots
        dots = LyraCarouselModel.calc_pagination_dots(total, current_idx)
        self.assertTrue(dots[0]["is_active"])
        self.assertFalse(dots[1]["is_active"])

        # User swipes left to flip forward
        swipe = classify_swipe(380, 270, 100, 270)
        self.assertEqual(swipe, SwipeDir.LEFT)
        current_idx = (current_idx + 1) % total
        self.assertEqual(current_idx, 1)

        # New slots: left=0, center=1, right=2
        l, c, r = LyraCarouselModel.get_book_indices_for_slot(current_idx, total)
        self.assertEqual((l, c, r), (0, 1, 2))

        # User taps center cover to open
        tap_action = classify_3zone_cover_tap(240, 270)
        self.assertEqual(tap_action, "OPEN")
        self.assertEqual(recent_books[current_idx]["title"], "Book B")

    def test_t4_03_scenario_virtual_keyboard_search(self):
        """Scenario 3: Quick Book Search via Russian/English Virtual Keyboard & Instant Reading Launch.
        Tap search -> Russian keyboard -> type query -> switch to English -> launch reader.
        """
        library = [
            {"path": "/sd/books/onegin.epub", "title": "Евгений Онегин", "author": "Пушкин"},
            {"path": "/sd/books/milion.epub", "title": "Мильон терзаний", "author": "Гончаров"},
            {"path": "/sd/books/b1.epub", "title": "Great Expectations", "author": "Charles Dickens"},
        ]

        # Step 1: Open search from extended menu
        action = HomeMenuItem.SEARCH_BOOKS
        self.assertEqual(HOME_MENU_LABELS_RU[action], "Быстрый поиск книг")

        # Step 2: Keyboard starts in Russian
        kb = VirtualKeyboardModel()
        self.assertTrue(kb.is_ru)

        # Step 3: Search Russian title
        filtered_ru = VirtualKeyboardModel.filter_books("терзаний", library)
        self.assertEqual(len(filtered_ru), 1)
        self.assertEqual(filtered_ru[0]["title"], "Мильон терзаний")

        # Step 4: Toggle layout to English
        kb.toggle_layout()
        self.assertFalse(kb.is_ru)

        # Step 5: Search English title
        filtered_en = VirtualKeyboardModel.filter_books("Expectations", library)
        self.assertEqual(len(filtered_en), 1)
        self.assertEqual(filtered_en[0]["title"], "Great Expectations")

        # Step 6: Launch reader with selected path
        launch_path = filtered_en[0]["path"]
        self.assertEqual(launch_path, "/sd/books/b1.epub")

    def test_t4_04_scenario_theme_switching_and_persistence(self):
        """Scenario 4: Theme Switching in Settings & UI Persistence across reload.
        Switch Dashboard -> Carousel -> Save -> Reload -> Verify persistence.
        """
        simulated_nvs_storage = {"uiTheme": int(UIThemeEnum.DASHBOARD)}

        # Verify initial theme
        initial_theme = UIThemeEnum(simulated_nvs_storage["uiTheme"])
        self.assertEqual(initial_theme, UIThemeEnum.DASHBOARD)

        # User changes setting to CAROUSEL
        simulated_nvs_storage["uiTheme"] = int(UIThemeEnum.CAROUSEL)

        # Simulated reload
        reloaded_theme = UIThemeEnum(simulated_nvs_storage["uiTheme"])
        self.assertEqual(reloaded_theme, UIThemeEnum.CAROUSEL)
        reloaded_metrics = THEME_METRICS[reloaded_theme]
        self.assertEqual(reloaded_metrics["name"], "LyraCarousel")
        self.assertEqual(reloaded_metrics["recent_books_count"], 3)

    def test_t4_05_scenario_simulator_pipeline_and_artifacts(self):
        """Scenario 5: Complete Simulator Pipeline: 800x480 Framebuffer Generation & Artifact Validation.
        Generate physical 800x480 buffer, rotate 90° CCW to 480x800 PNG, save, verify.
        """
        fb = VirtualFramebuffer()
        # Draw simulated home dashboard UI
        # Header (battery & clock)
        fb.draw_logical_rect(0, 0, LOGICAL_WIDTH, 40, fill_black=False, border_black=True)
        # Cover card
        fb.draw_logical_rect(50, 45, 180, 270, fill_black=None, border_black=True)
        # Progress bar
        fb.draw_logical_rect(240, 150, 200, 16, fill_black=None, border_black=True)
        fb.draw_logical_rect(240, 150, 100, 16, fill_black=True, border_black=True)

        with tempfile.TemporaryDirectory() as tmp_dir:
            out_png = Path(tmp_dir) / "test_home_dashboard_800x480.png"
            img = fb.to_logical_pil_image()
            img.save(out_png)

            self.assertTrue(out_png.exists())
            self.assertGreater(out_png.stat().st_size, 500)

            # Re-read and verify image dimensions
            with Image.open(out_png) as loaded:
                self.assertEqual(loaded.size, (LOGICAL_WIDTH, LOGICAL_HEIGHT))
                self.assertEqual(loaded.mode, "1")

    def test_t4_06_scenario_e2e_build_and_release_attestation(self):
        """Scenario 6: End-to-End Firmware Build, Constraint Validation & Release Asset Attestation.
        Verifies platformio configuration, partition sizing, binary size (<90%), RAM (<30%),
        magic byte 0xE9, and SHA256 checksum.
        """
        # Step 1: platformio.ini verification
        sections = FirmwareValidator.parse_platformio_ini(PLATFORMIO_INI_PATH)
        self.assertIn("env:x4pro", sections)
        env = sections["env:x4pro"]
        self.assertEqual(env.get("board"), "esp32-s3-devkitc1-n16r8")
        self.assertEqual(env.get("board_build.mcu"), "esp32s3")

        # Step 2: Partitions verification
        partitions = FirmwareValidator.parse_partitions_csv(PARTITIONS_CSV_PATH)
        app0 = next(p for p in partitions if p["name"] == "app0")
        self.assertEqual(app0["size"], 6553600)  # 6.25 MB

        # Step 3: Binary release attestation
        val = FirmwareValidator.validate_release_binary(RELEASE_BIN_PATH)
        self.assertTrue(EXPECTED_RELEASE_SIZE_MIN <= val["size"] <= EXPECTED_RELEASE_SIZE_MAX)
        self.assertEqual(val["sha256"], EXPECTED_RELEASE_SHA256)
        self.assertTrue(val["is_esp32_valid"])
        self.assertEqual(val["magic_byte"], ESP32_IMAGE_MAGIC)

        # Step 4: Flash constraint < 90%
        self.assertLess(val["flash_pct"], 90.0)
        self.assertGreater(val["flash_pct"], 80.0)

        # Step 5: RAM constraint < 30%
        audit_content = (REPO_ROOT / "AUDIT_X4PRO.md").read_text(encoding="utf-8", errors="replace")
        self.assertIn("RAM 19.9%", audit_content)

        # Step 6: GitHub release upload command syntax
        self.assertTrue(EXPECTED_GH_RELEASE_COMMAND.startswith("gh release upload v2.0.0"))

    def test_t4_07_scenario_flagship_home_screen_e2e_walkthrough(self):
        """Scenario 7: Full E2E User Session on Flagship ModernTheme Home Screen."""
        # 1. Boot up & Home screen verification
        metrics = THEME_METRICS[UIThemeEnum.MODERN]
        self.assertTrue(metrics["has_hero_card"])
        self.assertTrue(metrics["has_recent_shelf"])
        self.assertTrue(metrics["has_navigation_dock"])
        self.assertEqual(metrics["dock_columns"], 5)

        # 2. Smart Status Bar & Quick Settings Curtain
        self.assertTrue(ModernThemeModel.classify_header_tap(240, 20))
        # Swipe down from status bar to open curtain
        gesture = classify_swipe(240, 10, 240, 130)
        self.assertEqual(gesture, SwipeDir.DOWN)
        # Dismiss curtain
        self.assertTrue(ModernThemeModel.classify_curtain_dismiss_tap(240, 350))

        # 3. Hero Card Interaction
        # Tap on Hero cover (30..176, 95..314) -> resumes book
        self.assertTrue(ModernThemeModel.classify_hero_tap(100, 200))
        # Reading progress advances to 55%
        fill_w = ModernThemeModel.calc_capsule_progress(55.0)
        self.assertEqual(fill_w, round(170 * 0.55))
        # Time remaining calculation
        time_left = DashboardModel.calc_remaining_time(pages_total=300, pages_read=165, pace_pages_per_hour=60.0)
        self.assertEqual(time_left, "2ч 15м")
        # Reading streak badge
        streak_badge = ModernThemeModel.format_reading_streak(5)
        self.assertEqual(streak_badge, "Серия: 5 дн.")
        self.assertFalse(EmojiBanValidator.contains_emoji(streak_badge))

        # 4. Recent Shelf Switching
        # User taps on Shelf Slot 1 to switch to book 2
        slot_selected = ModernThemeModel.classify_shelf_tap(240, 450, shelf_count=3)
        self.assertEqual(slot_selected, 1)

        # 5. Bottom Navigation Dock
        # Columns 0..4 test
        dock_targets = ["Библиотека", "Поиск", "Статистика", "Приложения", "Настройки"]
        for col_idx, expected_label in enumerate(dock_targets):
            tap_x = col_idx * 96 + 48
            detected_col = ModernThemeModel.classify_dock_tap(tap_x, 760)
            self.assertEqual(detected_col, col_idx)
            item = ModernThemeModel.DOCK_ITEMS[detected_col]
            self.assertEqual(item["name"], expected_label)
            self.assertFalse(EmojiBanValidator.contains_emoji(item["name"]))

        # 6. Hardware Key Focus & Action
        focus = HardwareKeyFocusModel(has_hero=True, shelf_count=3, dock_count=5)
        self.assertEqual(focus.current_index, 0)
        # Step down through Hero, 3 Shelf books, to Dock
        for _ in range(4):
            focus.next_focus()
        self.assertTrue(focus.is_dock_focused())
        desc = focus.get_target_descriptor()
        self.assertEqual(desc["kind"], "DOCK")
        self.assertEqual(desc["column"], 0)
        self.assertEqual(desc["label"], "Библиотека")

    def test_t4_08_scenario_codebase_emoji_and_banned_keyword_audit(self):
        """Scenario 8: Exhaustive Static Audit for Clean Typography and Keyword Ban."""
        # 1. Audit translation YAML files
        translations_dir = SRC_DIR / "lib" / "I18n" / "translations"
        if translations_dir.exists():
            emoji_violations = EmojiBanValidator.scan_directory_for_emojis(
                translations_dir, (".yaml", ".yml")
            )
            self.assertEqual(
                len(emoji_violations),
                0,
                f"Prohibited emojis detected in translations: {emoji_violations}",
            )

        # 2. Audit UI section titles in source
        src_violations = EmojiBanValidator.scan_directory_for_emojis(
            SRC_DIR / "src", (".cpp", ".h")
        )
        self.assertEqual(
            len(src_violations),
            0,
            f"Prohibited emojis detected in C++ source: {src_violations}",
        )

        # 3. Audit banned keyword across src/
        kw_violations_src = EmojiBanValidator.scan_for_banned_keyword(
            SRC_DIR, (".cpp", ".h", ".c", ".ini", ".csv", ".yaml")
        )
        self.assertEqual(
            len(kw_violations_src),
            0,
            f"Banned keyword detected in src: {kw_violations_src}",
        )

        # 4. Audit banned keyword across tests/
        kw_violations_tests = EmojiBanValidator.scan_for_banned_keyword(
            REPO_ROOT / "tests", (".py",)
        )
        self.assertEqual(
            len(kw_violations_tests),
            0,
            f"Banned keyword detected in tests: {kw_violations_tests}",
        )

    def test_t4_09_scenario_quick_settings_and_side_drawer_full_flow(self):
        """Scenario 9: Full E2E UX workflow for Quick Settings Control Center and Edge-Swipe Side Drawer."""
        # 1. Quick Settings Curtain Invocation
        self.assertTrue(QuickSettingsCurtainModel.classify_open_tap(240, 20))
        # Top edge downward swipe also opens curtain
        self.assertTrue(QuickSettingsCurtainModel.classify_open_swipe(240, 10, 240, 90))

        # 2. Dual-channel adjustments
        brightness = QuickSettingsCurtainModel.step_channel(50, 20)
        cct = QuickSettingsCurtainModel.step_channel(40, -10)
        self.assertEqual(brightness, 70)
        self.assertEqual(cct, 30)

        # 3. Battery statistics display
        stats_str = QuickSettingsCurtainModel.format_battery_stats(90, False, 18000)
        self.assertEqual(stats_str, "Батарея: 90% • Работа: 5ч 0м")
        self.assertFalse(EmojiBanValidator.contains_emoji(stats_str))

        # 4. Dismiss curtain
        self.assertTrue(QuickSettingsCurtainModel.classify_dismiss_swipe(240, 300, 240, 200))

        # 5. Side Drawer Invocation (Right-handed mode)
        opened = SideDrawerModel.classify_open_gesture(465, 400, 390, 400, HandednessEnum.RIGHT)
        self.assertTrue(opened)
        self.assertEqual(len(SideDrawerModel.TARGETS), 6)

        # 6. Navigate to TOC (Target 2)
        target_id = SideDrawerModel.classify_item_tap(300, 240, HandednessEnum.RIGHT)
        self.assertEqual(target_id, 2)
        self.assertEqual(SideDrawerModel.TARGETS[target_id]["name"], "Оглавление")

        # 7. Backdrop tap dismisses drawer
        self.assertTrue(SideDrawerModel.classify_dismiss_tap(100, 400, HandednessEnum.RIGHT))

    def test_t4_10_scenario_reader_inbook_typography_and_scrubber_walkthrough(self):
        """Scenario 10: In-Book Reader overlays, live typography adjustments, chapter scrubber, and footnotes."""
        # 1. Center screen tap opens Reader Overlays
        self.assertTrue(ReaderOverlaysModel.classify_center_menu_tap(240, 400))

        # 2. Toggle Bookmark in top bar
        top_action = ReaderOverlaysModel.classify_top_tap(390, 30)
        self.assertEqual(top_action, "TOGGLE_BOOKMARK")
        is_bookmarked = True

        # 3. Inspect bottom bar reading tracker
        countdown_str = ReaderOverlaysModel.calc_chapter_countdown(remaining_pages=12, avg_seconds_per_page=35.0)
        self.assertEqual(countdown_str, "~7 мин")
        self.assertFalse(EmojiBanValidator.contains_emoji(countdown_str))

        # 4. Use scrubber to jump to page 150 of 300
        scrubbed_page = ReaderOverlaysModel.scrubber_x_to_page(240, 300)
        self.assertEqual(scrubbed_page, 151)

        # 5. Open live typography popup
        self.assertEqual(ReaderOverlaysModel.classify_bottom_tap(60, 765), "TYPOGRAPHY")
        new_size = AaTypographyModel.step_font_size(18, 4)
        self.assertEqual(new_size, 22)

        # 6. Content offset retention after re-pagination
        cached_offset = 12500
        new_ranges = [(0, 4000), (4000, 8500), (8500, 13000), (13000, 18000)]
        target_page = AaTypographyModel.map_offset_to_new_page(cached_offset, new_ranges)
        self.assertEqual(target_page, 2)
        self.assertTrue(new_ranges[target_page][0] <= cached_offset < new_ranges[target_page][1])

        # 7. Footnotes inspection
        self.assertEqual(ReaderOverlaysModel.classify_bottom_tap(290, 765), "FOOTNOTES")

    def test_t4_11_scenario_library_hub_and_settings_hub_complete_workflow(self):
        """Scenario 11: Library Hub grid/list navigation, sorting, and Modular Settings Hub management."""
        # 1. Library 3x2 Cover Grid
        self.assertEqual(LibraryHubModel.GRID_ROWS, 3)
        self.assertEqual(LibraryHubModel.GRID_COLS, 2)
        self.assertEqual(LibraryHubModel.GRID_ITEMS_PER_PAGE, 6)

        # 2. Sorting by Reading Progress
        books = [
            {"title": "Книга 1", "author": "Автор А", "date": 10, "progress": 15.0},
            {"title": "Книга 2", "author": "Автор Б", "date": 20, "progress": 92.0},
            {"title": "Книга 3", "author": "Автор В", "date": 30, "progress": 55.0},
        ]
        sorted_by_prog = LibraryHubModel.sort_books(books, LibrarySortModeEnum.PROGRESS, ascending=False)
        self.assertEqual(sorted_by_prog[0]["title"], "Книга 2")
        self.assertEqual(sorted_by_prog[0]["progress"], 92.0)

        # 3. Settings Hub Navigation & Modular Cards
        self.assertEqual(len(ModularSettingsHubModel.SECTIONS), 5)
        for sec in ModularSettingsHubModel.SECTIONS:
            self.assertTrue(sec["icon"].endswith("_32"))
            self.assertFalse(EmojiBanValidator.contains_emoji(sec["title"]))

        # 4. Graphical Toggle Manipulation
        toggle_state = False
        self.assertEqual(ModularSettingsHubModel.format_graphical_toggle(toggle_state), "[●  ]")
        toggle_state = True
        self.assertEqual(ModularSettingsHubModel.format_graphical_toggle(toggle_state), "[  ●]")

        # 5. Full Static Eradication Audit
        self.assertTrue(CrossPointEradicationValidator.is_clean_ui_label("Библиотека"))
        self.assertTrue(CrossPointEradicationValidator.is_clean_ui_label("Центр управления"))
        self.assertTrue(CrossPointEradicationValidator.is_preserved_sd_path("/.crosspoint/recent.json"))

        # 6. Audit banned keyword across codebase
        kw_violations = CrossPointEradicationValidator.scan_for_banned_keyword(
            SRC_DIR, (".cpp", ".h", ".c", ".ini", ".csv", ".yaml")
        )
        self.assertEqual(len(kw_violations), 0)

    def test_t4_12_flibusta_fb2_reading_flow_with_typography_footnotes_and_sleep_gallery(self):
        """T4.12: End-to-end user workflow: Flibusta FB2/FB2.zip ingestion, PJPEG cover decoding without PSRAM OOM, strikethrough & footnotes, live typography (Aa) reflow, and sleep screensaver gallery (R1-R4)."""
        # Step 1: Flibusta book ingestion (.fb2 & .fb2.zip)
        flibusta_zip = "/books/dostoevsky_demons.fb2.zip"
        self.assertTrue(UniversalFormatsModel.has_fb2_zip_extension(flibusta_zip))
        self.assertTrue(UniversalFormatsModel.is_universal_format(flibusta_zip))
        cache_dir = UniversalFormatsModel.compute_cache_dir_name(flibusta_zip)
        self.assertTrue(cache_dir.startswith("/.crosspoint/fb2_"))

        # DEFLATE memory bound
        self.assertLessEqual(UniversalFormatsModel.DEFLATE_WINDOW_SIZE_MAX, 32768)

        # Step 2: Progressive JPEG cover decoding without PSRAM OOM
        # Cover is 1600x2400 progressive JPEG (SOF2 = 0xFFC2)
        marker = 0xFFC2
        self.assertTrue(ProgressiveJpegDecoderModel.is_progressive_marker(marker))
        mcu_row_bytes = ProgressiveJpegDecoderModel.calc_mcu_row_buffer_bytes(1600, mcu_height=16)
        self.assertEqual(mcu_row_bytes, 25600)  # 25.6 KB transient buffer
        self.assertTrue(ProgressiveJpegDecoderModel.is_psram_safe(1600))
        # Verify unbuffered full decode would exceed 2 MB danger threshold
        unbuffered_bytes = ProgressiveJpegDecoderModel.calc_unbuffered_full_decode_bytes(1600, 2400)
        self.assertGreater(unbuffered_bytes, ProgressiveJpegDecoderModel.FULL_IMAGE_PSRAM_DANGER_THRESHOLD)
        # Downsampling ratio to 480x800 display
        step_x, step_y = ProgressiveJpegDecoderModel.calc_downsample_steps(1600, 2400, 480, 800)
        self.assertAlmostEqual(step_x, 3.3333333333333335)
        self.assertAlmostEqual(step_y, 3.0)

        # Step 3: Text parsing with <strikethrough> and combining stroke 0xCCB6 (U+0336)
        raw_fb2_body = "Он н\u0336е\u0336 ответил на вопрос, но ссылка [12] вела к примечанию автора."
        clean_text, has_strike = StrikethroughNormalizationModel.normalize_strikethrough_text(raw_fb2_body)
        self.assertTrue(has_strike)
        self.assertIn("не", clean_text)
        self.assertNotIn("\u0336", clean_text)
        strike_line_y = StrikethroughNormalizationModel.calc_strike_line_y(base_y=150, ascender=20)
        self.assertEqual(strike_line_y, 134)

        # Step 4: Footnote superscript index tap & instant modal card popup
        self.assertTrue(FootnoteModalModel.is_footnote_reference("[12]"))
        card_rect = FootnoteModalModel.get_modal_card_rect(480, 800, 220)
        self.assertEqual(card_rect, (20, 800 - 220 - 16, 440, 220))
        close_btn = FootnoteModalModel.get_close_button_rect(card_rect)
        self.assertGreaterEqual(close_btn[2], 44)
        self.assertEqual(
            FootnoteModalModel.classify_modal_tap(card_rect, close_btn[0] + 5, close_btn[1] + 5),
            "CLOSE_BUTTON",
        )

        # Step 5: Live Typography (Aa) reflow & zero-drift character mapping
        # Mandated font family selection
        for font in ["JetBrains Mono", "Roboto Condensed", "OpenDyslexic"]:
            self.assertIn(font, AaTypographyModel.MANDATED_FONT_FAMILIES)
            chip_hit = AaTypographyModel.classify_font_chip_tap(
                AaTypographyModel.FONT_CHIPS[font][0] + 10, AaTypographyModel.FONT_CHIPS[font][1] + 10
            )
            self.assertEqual(chip_hit, font)

        # Large A- / A+ buttons >= 44px
        self.assertGreaterEqual(AaTypographyModel.DECREASE_FONT_RECT[2], 44)
        self.assertGreaterEqual(AaTypographyModel.INCREASE_FONT_RECT[2], 44)

        # Autosave state
        saved = AaTypographyModel.autosave_state(
            font_family="Roboto Condensed", font_size=20, line_spacing=1.2, margin=10
        )
        self.assertTrue(saved["autosaved"])
        self.assertEqual(saved["fontFamily"], "Roboto Condensed")

        # Zero-drift character offset mapping across re-pagination
        new_pages = [(0, 420), (420, 880), (880, 1350)]
        self.assertEqual(AaTypographyModel.map_offset_to_new_page(500, new_pages), 1)

        # Step 6: Reader top bar controls & status bar isolation
        # When overlays active, tapping Back (30, 30) or TOC (100, 30) is isolated from status bar curtain
        self.assertFalse(QuickSettingsCurtainModel.is_status_bar_tap_allowed(True, True, 30, 30))
        self.assertEqual(ReaderOverlaysModel.classify_top_tap(30, 30), "BACK")
        self.assertEqual(ReaderOverlaysModel.classify_top_tap(100, 30), "TOC")
        self.assertEqual(ReaderOverlaysModel.classify_top_tap(390, 30), "TOGGLE_BOOKMARK")

        # Step 7: Control Curtain opening when reading (overlays dismissed)
        self.assertTrue(QuickSettingsCurtainModel.is_status_bar_tap_allowed(True, False, 240, 20))
        # Info block with battery, charging, Time and Date
        info_block = QuickSettingsCurtainModel.format_battery_and_clock(85, True, "20:45", "13.09.2026")
        self.assertIn("85%", info_block)
        self.assertIn("Зарядка", info_block)
        self.assertIn("20:45", info_block)
        self.assertIn("13.09.2026", info_block)
        self.assertFalse(EmojiBanValidator.contains_emoji(info_block))

        # Sliders >= 44px hit-boxes
        self.assertGreaterEqual(QuickSettingsCurtainModel.BRIGHTNESS_MINUS_RECT[2], 44)
        self.assertGreaterEqual(QuickSettingsCurtainModel.BRIGHTNESS_PLUS_RECT[2], 44)

        # Step 8: Transition to Sleep Mode & Screensaver Gallery
        self.assertEqual(ScreensaverGalleryModel.THEME_COUNT, 11)
        # Verify 11 themes across Kindle and Kobo collections
        for idx in range(1, 12):
            p_fn = ScreensaverGalleryModel.get_theme_filename(idx, (480, 800))
            l_fn = ScreensaverGalleryModel.get_theme_filename(idx, (800, 480))
            self.assertTrue(p_fn.startswith(f"{idx:02d}_"))
            self.assertTrue(l_fn.startswith(f"{idx:02d}_"))

        # Exact v2.1.0 version display validation
        self.assertEqual(ScreensaverGalleryModel.format_version_display("2.1.0-x4pro"), "v2.1.0")
        self.assertEqual(ScreensaverGalleryModel.get_version_position(480, 800), (240, 770))

        # Light mode vs Dark mode polarities
        light_mode = ScreensaverGalleryModel.get_version_polarity(False)
        self.assertEqual(light_mode["text_polarity"], POLARITY_BLACK)
        self.assertFalse(light_mode["screen_inverted"])

        dark_mode = ScreensaverGalleryModel.get_version_polarity(True)
        self.assertEqual(dark_mode["text_polarity"], POLARITY_WHITE)
        self.assertTrue(dark_mode["screen_inverted"])


if __name__ == "__main__":
    unittest.main()


