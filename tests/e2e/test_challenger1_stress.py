"""
test_challenger1_stress.py - Empirical Challenger 1 Stress Harness
Validating:
1. Touch coordinates & gesture detection: boundary taps, edge swipes, handedness flipping (right vs left handed), curtain opening/dismissing hitboxes.
2. Backlight dual-channel sliders: boundary values (0%, 100%, negative/overflow steps), CCT cold/warm balancing.
3. Physical GT911 coordinate mapping parity with GfxRenderer.
4. Multi-turn state transitions & handedness flipping lifecycle.
"""

import math
import random
import unittest
from typing import Tuple, List, Dict, Any, Optional

from tests.e2e.contracts import (
    LOGICAL_WIDTH,
    LOGICAL_HEIGHT,
    PHYSICAL_WIDTH,
    PHYSICAL_HEIGHT,
    DisplayGeometry,
    QuickSettingsCurtainModel,
    SideDrawerModel,
    HandednessEnum,
    classify_swipe,
    SwipeDir,
    tap_in_rect,
)


class TestChallenger1TouchAndGestures(unittest.TestCase):
    """
    Stress-testing Touch coordinates & gesture detection:
    - Boundary taps
    - Edge swipes
    - Handedness flipping (Right vs Left)
    - Curtain opening and dismissing hitboxes
    """

    def test_01_status_bar_curtain_open_boundary_taps(self):
        """Verify status bar curtain open tap hitbox [0..479] x [0..43]."""
        # Inside status bar corners and edges
        self.assertTrue(QuickSettingsCurtainModel.classify_open_tap(0, 0))
        self.assertTrue(QuickSettingsCurtainModel.classify_open_tap(LOGICAL_WIDTH - 1, 0))
        self.assertTrue(QuickSettingsCurtainModel.classify_open_tap(0, 43))
        self.assertTrue(QuickSettingsCurtainModel.classify_open_tap(LOGICAL_WIDTH - 1, 43))
        self.assertTrue(QuickSettingsCurtainModel.classify_open_tap(240, 20))

        # Critical boundaries at y = 44 (first row below status bar)
        for x in [0, 100, 240, 360, LOGICAL_WIDTH - 1]:
            self.assertFalse(QuickSettingsCurtainModel.classify_open_tap(x, 44))
            self.assertFalse(QuickSettingsCurtainModel.classify_open_tap(x, 45))
            self.assertFalse(QuickSettingsCurtainModel.classify_open_tap(x, 100))

        # Out of screen boundaries
        self.assertFalse(QuickSettingsCurtainModel.classify_open_tap(-1, 20))
        self.assertFalse(QuickSettingsCurtainModel.classify_open_tap(LOGICAL_WIDTH, 20))
        self.assertFalse(QuickSettingsCurtainModel.classify_open_tap(240, -1))

    def test_02_curtain_dismiss_tap_boundary_hitbox(self):
        """Verify curtain dismiss tap hitbox outside curtain (y >= 360)."""
        curtain_h = 360
        # Inside curtain (y < 360) -> MUST NOT dismiss
        for y in [0, 44, 150, 250, 359]:
            for x in [0, 100, 240, 479]:
                self.assertFalse(
                    QuickSettingsCurtainModel.classify_dismiss_tap(x, y, curtain_h=curtain_h),
                    f"Point ({x}, {y}) should NOT dismiss curtain",
                )

        # Outside curtain (y >= 360) -> MUST dismiss
        for y in [360, 361, 400, 600, 799]:
            for x in [0, 100, 240, 479]:
                self.assertTrue(
                    QuickSettingsCurtainModel.classify_dismiss_tap(x, y, curtain_h=curtain_h),
                    f"Point ({x}, {y}) SHOULD dismiss curtain",
                )

        # Out of screen boundaries
        self.assertFalse(QuickSettingsCurtainModel.classify_dismiss_tap(-1, 400, curtain_h))
        self.assertFalse(QuickSettingsCurtainModel.classify_dismiss_tap(LOGICAL_WIDTH, 400, curtain_h))
        self.assertFalse(QuickSettingsCurtainModel.classify_dismiss_tap(240, LOGICAL_HEIGHT, curtain_h))

    def test_03_curtain_swipe_gestures_stress(self):
        """Stress-test open swipe (downward from top) and dismiss swipe (upward inside curtain)."""
        # Open swipe: start_y <= 44, dy >= 50, |dx| < dy
        # Exactly at threshold dy = 50
        self.assertTrue(QuickSettingsCurtainModel.classify_open_swipe(240, 44, 240, 94, threshold=50.0))
        # Just below threshold dy = 49
        self.assertFalse(QuickSettingsCurtainModel.classify_open_swipe(240, 44, 240, 93, threshold=50.0))
        # Starting below status bar (start_y = 45) -> Rejected
        self.assertFalse(QuickSettingsCurtainModel.classify_open_swipe(240, 45, 240, 150, threshold=50.0))
        # Equal dx and dy (strictly diagonal) -> Rejected
        self.assertFalse(QuickSettingsCurtainModel.classify_open_swipe(200, 20, 260, 80, threshold=50.0))
        # Upward swipe from top -> Rejected
        self.assertFalse(QuickSettingsCurtainModel.classify_open_swipe(240, 40, 240, 10, threshold=50.0))

        # Dismiss swipe: start_y <= 360, dy >= 50 (upward: start_y - end_y >= 50), |dx| < dy
        self.assertTrue(QuickSettingsCurtainModel.classify_dismiss_swipe(240, 360, 240, 310, threshold=50.0))
        self.assertFalse(QuickSettingsCurtainModel.classify_dismiss_swipe(240, 360, 240, 311, threshold=50.0))
        # Starting outside curtain (start_y = 361) -> Rejected
        self.assertFalse(QuickSettingsCurtainModel.classify_dismiss_swipe(240, 361, 240, 200, threshold=50.0))
        # Downward swipe -> Rejected
        self.assertFalse(QuickSettingsCurtainModel.classify_dismiss_swipe(240, 200, 240, 300, threshold=50.0))

    def test_04_curtain_pills_exhaustive_hitbox_and_dead_zones(self):
        """Test all 4 control pills corners, centers, and dead zones."""
        pills = QuickSettingsCurtainModel.CONTROL_PILLS
        for pill_id, info in pills.items():
            rx, ry, rw, rh = info["rect"]
            # Inside corners
            self.assertEqual(QuickSettingsCurtainModel.classify_pill_tap(rx, ry), pill_id)
            self.assertEqual(QuickSettingsCurtainModel.classify_pill_tap(rx + rw - 1, ry), pill_id)
            self.assertEqual(QuickSettingsCurtainModel.classify_pill_tap(rx, ry + rh - 1), pill_id)
            self.assertEqual(QuickSettingsCurtainModel.classify_pill_tap(rx + rw - 1, ry + rh - 1), pill_id)
            self.assertEqual(QuickSettingsCurtainModel.classify_pill_tap(rx + rw // 2, ry + rh // 2), pill_id)

            # 1px outside edges
            self.assertNotEqual(QuickSettingsCurtainModel.classify_pill_tap(rx - 1, ry), pill_id)
            self.assertNotEqual(QuickSettingsCurtainModel.classify_pill_tap(rx + rw, ry), pill_id)
            self.assertNotEqual(QuickSettingsCurtainModel.classify_pill_tap(rx, ry - 1), pill_id)
            self.assertNotEqual(QuickSettingsCurtainModel.classify_pill_tap(rx, ry + rh), pill_id)

        # Dead zones between pills:
        # Horizontal gap between column 1 (rx=20, w=210 -> 230) and column 2 (rx=250): x in [230..249]
        for x in range(230, 250):
            self.assertIsNone(QuickSettingsCurtainModel.classify_pill_tap(x, 175))
            self.assertIsNone(QuickSettingsCurtainModel.classify_pill_tap(x, 235))

        # Vertical gap between row 1 (ry=155, h=46 -> 201) and row 2 (ry=215): y in [201..214]
        for y in range(201, 215):
            self.assertIsNone(QuickSettingsCurtainModel.classify_pill_tap(100, y))
            self.assertIsNone(QuickSettingsCurtainModel.classify_pill_tap(350, y))

    def test_05_side_drawer_handedness_inversion_symmetry(self):
        """
        Validate perfect geometric symmetry between Right-handed and Left-handed modes.
        Drawer width is 320px on 480px wide screen.
        """
        # Right handed drawer: x in [160..479], backdrop in [0..159]
        r_rect = SideDrawerModel.get_drawer_rect(HandednessEnum.RIGHT)
        self.assertEqual(r_rect, (160, 0, 320, 800))
        # Left handed drawer: x in [0..319], backdrop in [320..479]
        l_rect = SideDrawerModel.get_drawer_rect(HandednessEnum.LEFT)
        self.assertEqual(l_rect, (0, 0, 320, 800))

        # Backdrop dismissal boundary tests
        # Right-handed:
        self.assertTrue(SideDrawerModel.classify_dismiss_tap(0, 400, HandednessEnum.RIGHT))
        self.assertTrue(SideDrawerModel.classify_dismiss_tap(159, 400, HandednessEnum.RIGHT))
        self.assertFalse(SideDrawerModel.classify_dismiss_tap(160, 400, HandednessEnum.RIGHT))
        self.assertFalse(SideDrawerModel.classify_dismiss_tap(479, 400, HandednessEnum.RIGHT))

        # Left-handed:
        self.assertFalse(SideDrawerModel.classify_dismiss_tap(0, 400, HandednessEnum.LEFT))
        self.assertFalse(SideDrawerModel.classify_dismiss_tap(319, 400, HandednessEnum.LEFT))
        self.assertTrue(SideDrawerModel.classify_dismiss_tap(320, 400, HandednessEnum.LEFT))
        self.assertTrue(SideDrawerModel.classify_dismiss_tap(479, 400, HandednessEnum.LEFT))

        # Side tab pull trigger:
        # Right-handed tab: y in [340..460], x >= 460
        self.assertTrue(SideDrawerModel.classify_side_tab_tap(460, 340, HandednessEnum.RIGHT))
        self.assertTrue(SideDrawerModel.classify_side_tab_tap(479, 460, HandednessEnum.RIGHT))
        self.assertFalse(SideDrawerModel.classify_side_tab_tap(459, 400, HandednessEnum.RIGHT))
        self.assertFalse(SideDrawerModel.classify_side_tab_tap(470, 339, HandednessEnum.RIGHT))
        self.assertFalse(SideDrawerModel.classify_side_tab_tap(470, 461, HandednessEnum.RIGHT))

        # Left-handed tab: y in [340..460], x <= 20
        self.assertTrue(SideDrawerModel.classify_side_tab_tap(0, 340, HandednessEnum.LEFT))
        self.assertTrue(SideDrawerModel.classify_side_tab_tap(20, 460, HandednessEnum.LEFT))
        self.assertFalse(SideDrawerModel.classify_side_tab_tap(21, 400, HandednessEnum.LEFT))
        self.assertFalse(SideDrawerModel.classify_side_tab_tap(10, 339, HandednessEnum.LEFT))
        self.assertFalse(SideDrawerModel.classify_side_tab_tap(10, 461, HandednessEnum.LEFT))

    def test_06_side_drawer_edge_swipe_mutual_exclusivity(self):
        """
        Verify mutual exclusivity between side drawer open swipe and back gesture
        under both right-handed and left-handed modes.
        """
        # Right-handed mode:
        # Right edge swipe leftward: start_x >= 450, dx <= -50
        r_open = SideDrawerModel.classify_open_gesture(460, 400, 390, 400, HandednessEnum.RIGHT)
        r_back = SideDrawerModel.classify_back_gesture(460, 400, 390, 400, HandednessEnum.RIGHT)
        self.assertTrue(r_open)
        self.assertFalse(r_back)

        # Left edge swipe rightward: start_x <= 30, dx >= 50
        l_open_in_r = SideDrawerModel.classify_open_gesture(20, 400, 90, 400, HandednessEnum.RIGHT)
        l_back_in_r = SideDrawerModel.classify_back_gesture(20, 400, 90, 400, HandednessEnum.RIGHT)
        self.assertFalse(l_open_in_r)
        self.assertTrue(l_back_in_r)

        # Left-handed mode:
        # Left edge swipe rightward: start_x <= 30, dx >= 50
        l_open = SideDrawerModel.classify_open_gesture(20, 400, 90, 400, HandednessEnum.LEFT)
        l_back = SideDrawerModel.classify_back_gesture(20, 400, 90, 400, HandednessEnum.LEFT)
        self.assertTrue(l_open)
        self.assertFalse(l_back)

        # Right edge swipe leftward: start_x >= 450, dx <= -50
        r_open_in_l = SideDrawerModel.classify_open_gesture(460, 400, 390, 400, HandednessEnum.LEFT)
        r_back_in_l = SideDrawerModel.classify_back_gesture(460, 400, 390, 400, HandednessEnum.LEFT)
        self.assertFalse(r_open_in_l)
        self.assertTrue(r_back_in_l)

        # Swipe toward edge (moving off-screen) should be rejected
        self.assertFalse(SideDrawerModel.classify_open_gesture(450, 400, 470, 400, HandednessEnum.RIGHT))
        self.assertFalse(SideDrawerModel.classify_open_gesture(20, 400, 5, 400, HandednessEnum.LEFT))

    def test_07_side_drawer_all_targets_and_inter_item_gaps(self):
        """Verify all 6 drawer item rects and that gaps between items return None."""
        for handedness in [HandednessEnum.RIGHT, HandednessEnum.LEFT]:
            base_x = 160 if handedness == HandednessEnum.RIGHT else 0
            for i in range(len(SideDrawerModel.TARGETS)):
                rect = SideDrawerModel.get_item_rect(i, handedness)
                rx, ry, rw, rh = rect
                self.assertEqual(rw, 288)
                self.assertEqual(rh, 56)
                self.assertEqual(rx, base_x + 16)
                self.assertEqual(ry, 100 + i * 70)

                # Tap corners and center
                self.assertEqual(SideDrawerModel.classify_item_tap(rx, ry, handedness), i)
                self.assertEqual(SideDrawerModel.classify_item_tap(rx + rw - 1, ry, handedness), i)
                self.assertEqual(SideDrawerModel.classify_item_tap(rx, ry + rh - 1, handedness), i)
                self.assertEqual(SideDrawerModel.classify_item_tap(rx + rw - 1, ry + rh - 1, handedness), i)
                self.assertEqual(SideDrawerModel.classify_item_tap(rx + rw // 2, ry + rh // 2, handedness), i)

                # Gap below item: ry + 56 to ry + 69 (14px gap)
                for gap_y in range(ry + 56, ry + 70):
                    self.assertIsNone(
                        SideDrawerModel.classify_item_tap(rx + rw // 2, gap_y, handedness),
                        f"Gap Y {gap_y} should return None",
                    )


class TestChallenger1BacklightDualSlidersAndCCT(unittest.TestCase):
    """
    Stress-testing Backlight Dual-Channel Sliders & CCT balancing:
    - Step arithmetic boundaries [0..100%]
    - Negative and large overflow steps
    - CCT cold/warm duty calculation invariants
    - Monotonicity and duty conservation
    """

    # Reproduction of C++ FrontlightManager GAMMA_TABLE
    GAMMA_TABLE = [
        0,     32,    101,   197,   318,   460,   622,   803,   1001,  1217,  1449,  1697,  1960,  2237,  2529,
        2835,  3155,  3488,  3834,  4193,  4565,  4949,  5345,  5753,  6173,  6604,  7047,  7502,  7967,  8444,
        8931,  9429,  9938,  10457, 10987, 11527, 12077, 12638, 13208, 13789, 14379, 14979, 15588, 16208, 16836,
        17474, 18122, 18779, 19445, 20120, 20804, 21497, 22200, 22911, 23631, 24360, 25097, 25843, 26598, 27362,
        28134, 28914, 29703, 30500, 31306, 32120, 32942, 33772, 34611, 35457, 36312, 37175, 38045, 38924, 39811,
        40705, 41608, 42518, 43436, 44361, 45295, 46236, 47185, 48141, 49105, 50076, 51055, 52042, 53036, 54037,
        55046, 56062, 57086, 58117, 59155, 60200, 61253, 62313, 63380, 64454, 65535
    ]

    @classmethod
    def perceptual_duty_cpp(cls, pct: int, full: int = 1023) -> int:
        """Exact reproduction of C++ perceptualDuty(pct, full) in BookPoint v2.2.4."""
        if pct <= 0:
            return 0
        if pct > 100:
            pct = 100
        duty = (full * cls.GAMMA_TABLE[pct] + 32767) // 65535
        if full >= 1023 and duty < 2:
            return 2
        return duty if duty > 0 else 1

    @classmethod
    def calculate_cct_channels(cls, brightness_pct: int, warmth_pct: int, full: int = 1023) -> Tuple[int, int, int]:
        """
        Exact reproduction of C++ FrontlightManager::apply() duty split logic in BookPoint v2.2.4:
        totalDuty = perceptualDuty(brightness, full)
        Mixed color balance clamping: warmDuty clamped to [1, totalDuty-1] when totalDuty >= 2.
        """
        total_duty = cls.perceptual_duty_cpp(brightness_pct, full)
        if total_duty == 0:
            return 0, 0, 0
        if warmth_pct <= 0:
            return total_duty, total_duty, 0
        if warmth_pct >= 100:
            return total_duty, 0, total_duty

        warm_duty = (total_duty * warmth_pct + 50) // 100
        if total_duty >= 2:
            if warm_duty < 1:
                warm_duty = 1
            if warm_duty >= total_duty:
                warm_duty = total_duty - 1
        cool_duty = total_duty - warm_duty
        return total_duty, cool_duty, warm_duty

    def test_08_slider_step_boundary_and_extreme_deltas(self):
        """Test step_channel with extreme positive, negative, and boundary values."""
        # 0% lower boundary
        self.assertEqual(QuickSettingsCurtainModel.step_channel(0, -1), 0)
        self.assertEqual(QuickSettingsCurtainModel.step_channel(0, -10), 0)
        self.assertEqual(QuickSettingsCurtainModel.step_channel(0, -999999), 0)

        # 100% upper boundary
        self.assertEqual(QuickSettingsCurtainModel.step_channel(100, 1), 100)
        self.assertEqual(QuickSettingsCurtainModel.step_channel(100, 10), 100)
        self.assertEqual(QuickSettingsCurtainModel.step_channel(100, 999999), 100)

        # Zero delta
        self.assertEqual(QuickSettingsCurtainModel.step_channel(45, 0), 45)

        # Stepping up from 0 to 100 by 10
        val = 0
        for expected in [10, 20, 30, 40, 50, 60, 70, 80, 90, 100]:
            val = QuickSettingsCurtainModel.step_channel(val, 10)
            self.assertEqual(val, expected)
        # Extra step at 100 stays 100
        self.assertEqual(QuickSettingsCurtainModel.step_channel(val, 10), 100)

        # Stepping down from 100 to 0 by 10
        for expected in [90, 80, 70, 60, 50, 40, 30, 20, 10, 0]:
            val = QuickSettingsCurtainModel.step_channel(val, -10)
            self.assertEqual(val, expected)
        # Extra step at 0 stays 0
        self.assertEqual(QuickSettingsCurtainModel.step_channel(val, -10), 0)

    def test_09_gamma_table_integrity(self):
        """Verify the 101-element perceptual gamma lookup table properties."""
        self.assertEqual(len(self.GAMMA_TABLE), 101)
        self.assertEqual(self.GAMMA_TABLE[0], 0)
        self.assertEqual(self.GAMMA_TABLE[100], 65535)

        # Strict monotonicity for pct >= 1
        for i in range(1, 101):
            self.assertGreater(
                self.GAMMA_TABLE[i],
                self.GAMMA_TABLE[i - 1],
                f"Gamma table failed strict monotonicity at index {i}",
            )
            self.assertGreaterEqual(self.GAMMA_TABLE[i], 0)
            self.assertLessEqual(self.GAMMA_TABLE[i], 65535)

    def test_10_cct_duty_conservation_exhaustive_10201_pairs(self):
        """
        Exhaustively verify duty conservation and bounds for all 10,201 pairs:
        (brightness in [0..100], warmth in [0..100]).
        Invariants:
        1. totalDuty == coolDuty + warmDuty (exact conservation)
        2. 0 <= coolDuty <= totalDuty <= 1023
        3. 0 <= warmDuty <= totalDuty <= 1023
        """
        full_pwm = 1023  # 10-bit PWM for ESP32-S3 / X4 Pro

        for b in range(101):
            for w in range(101):
                total, cool, warm = self.calculate_cct_channels(b, w, full_pwm)

                # Invariant 1: Conservation
                self.assertEqual(
                    cool + warm,
                    total,
                    f"Duty not conserved for b={b}, w={w}: cool={cool}, warm={warm}, total={total}",
                )

                # Invariant 2: Bounds
                self.assertTrue(
                    0 <= cool <= total <= full_pwm,
                    f"Cool duty out of bounds for b={b}, w={w}: cool={cool}, total={total}",
                )
                self.assertTrue(
                    0 <= warm <= total <= full_pwm,
                    f"Warm duty out of bounds for b={b}, w={w}: warm={warm}, total={total}",
                )

                # Invariant 3: Boundary extremes
                if b == 0:
                    self.assertEqual(total, 0)
                    self.assertEqual(cool, 0)
                    self.assertEqual(warm, 0)

                if w == 0:
                    self.assertEqual(warm, 0)
                    self.assertEqual(cool, total)

                if w == 100:
                    self.assertEqual(warm, total)
                    self.assertEqual(cool, 0)

    def test_11_cct_warmth_monotonicity_for_every_brightness(self):
        """
        Verify that for every non-zero brightness, increasing warmth
        monotonically increases warmDuty and monotonically decreases coolDuty.
        """
        full_pwm = 1023
        for b in range(1, 101):
            prev_warm = -1
            prev_cool = 999999
            for w in range(101):
                _, cool, warm = self.calculate_cct_channels(b, w, full_pwm)
                self.assertGreaterEqual(
                    warm,
                    prev_warm,
                    f"Warm duty decreased when warmth increased: b={b}, w={w}, warm={warm} < {prev_warm}",
                )
                self.assertLessEqual(
                    cool,
                    prev_cool,
                    f"Cool duty increased when warmth increased: b={b}, w={w}, cool={cool} > {prev_cool}",
                )
                prev_warm = warm
                prev_cool = cool

    def test_12_fuzz_random_adversarial_touch_coordinates(self):
        """Fuzz test gesture classifier with 5000 random adversarial coordinates."""
        rng = random.Random(42)

        for _ in range(5000):
            sx = rng.randint(-200, 1000)
            sy = rng.randint(-200, 1200)
            ex = rng.randint(-200, 1000)
            ey = rng.randint(-200, 1200)
            thresh = rng.uniform(10.0, 100.0)

            # Classify swipe
            s_dir = classify_swipe(sx, sy, ex, ey, thresh)
            self.assertIn(s_dir, [SwipeDir.NONE, SwipeDir.LEFT, SwipeDir.RIGHT, SwipeDir.UP, SwipeDir.DOWN])

            # Classify curtain open/dismiss
            open_s = QuickSettingsCurtainModel.classify_open_swipe(sx, sy, ex, ey, thresh)
            dismiss_s = QuickSettingsCurtainModel.classify_dismiss_swipe(sx, sy, ex, ey, thresh)
            self.assertIsInstance(open_s, bool)
            self.assertIsInstance(dismiss_s, bool)

            # Classify side drawer open / back
            for h in [HandednessEnum.RIGHT, HandednessEnum.LEFT]:
                d_open = SideDrawerModel.classify_open_gesture(sx, sy, ex, ey, h, thresh)
                d_back = SideDrawerModel.classify_back_gesture(sx, sy, ex, ey, h, thresh)
                self.assertIsInstance(d_open, bool)
                self.assertIsInstance(d_back, bool)
                # Mutual exclusivity check
                self.assertFalse(
                    d_open and d_back,
                    f"Adversarial swipe ({sx},{sy})->({ex},{ey}) simultaneously triggered open AND back for {h}!",
                )


class TestChallenger1HardwareAndLifecycle(unittest.TestCase):
    """
    Validates physical GT911 hardware coordinate transforms and multi-turn state transitions.
    """

    def test_13_physical_gt911_coordinates_to_logical_gestures(self):
        """
        Verify that raw physical GT911 touch coordinates (800x480 landscape)
        map cleanly to logical (480x800 portrait) gestures.
        """
        # Status bar tap in physical space:
        # Logical: y < 44 -> physical x < 44
        for phy_x in [0, 20, 43]:
            for phy_y in [0, 240, 479]:
                log_x, log_y = DisplayGeometry.physical_to_logical(phy_x, phy_y)
                self.assertTrue(QuickSettingsCurtainModel.classify_open_tap(log_x, log_y))

        # First row below status bar in physical space:
        # Logical: y = 44 -> physical x = 44
        for phy_y in [0, 240, 479]:
            log_x, log_y = DisplayGeometry.physical_to_logical(44, phy_y)
            self.assertFalse(QuickSettingsCurtainModel.classify_open_tap(log_x, log_y))

        # Side tab pull tab in physical space:
        # Right-handed logical: x >= 460, y in [340..460]
        # Physical: phy_y = 479 - x <= 19, phy_x = y in [340..460]
        for phy_x in [340, 400, 460]:
            for phy_y in [0, 10, 19]:
                log_x, log_y = DisplayGeometry.physical_to_logical(phy_x, phy_y)
                self.assertTrue(SideDrawerModel.classify_side_tab_tap(log_x, log_y, HandednessEnum.RIGHT))

        # Left-handed logical: x <= 20, y in [340..460]
        # Physical: phy_y = 479 - x >= 459, phy_x = y in [340..460]
        for phy_x in [340, 400, 460]:
            for phy_y in [459, 470, 479]:
                log_x, log_y = DisplayGeometry.physical_to_logical(phy_x, phy_y)
                self.assertTrue(SideDrawerModel.classify_side_tab_tap(log_x, log_y, HandednessEnum.LEFT))

    def test_14_complex_multiturn_quick_settings_workflow(self):
        """
        Simulate a full user interaction sequence in Quick Settings Control Center:
        1. Open curtain via downward swipe from top edge
        2. Decrease brightness from 60% to 0% (6 taps on MINUS)
        3. Try decreasing below 0% (verify clamping)
        4. Increase warmth to 100% (10 taps on PLUS)
        5. Toggle Dark Mode and Rotation Lock
        6. Dismiss curtain via backdrop tap
        7. Verify final persisted state
        """
        # Step 1: Open curtain
        open_swipe = QuickSettingsCurtainModel.classify_open_swipe(240, 10, 240, 80)
        self.assertTrue(open_swipe)

        # Initial state
        brightness = 60
        warmth = 50
        dark_mode = False
        rotation_lock = False

        # Step 2: 6 minus taps on brightness
        for _ in range(6):
            action = QuickSettingsCurtainModel.classify_slider_tap("BRIGHTNESS", 35, 75)
            self.assertEqual(action, "MINUS")
            brightness = QuickSettingsCurtainModel.step_channel(brightness, -10)
        self.assertEqual(brightness, 0)

        # Step 3: Extra minus tap at 0%
        brightness = QuickSettingsCurtainModel.step_channel(brightness, -10)
        self.assertEqual(brightness, 0)

        # Step 4: 5 plus taps on warmth to reach 100%
        for _ in range(5):
            action = QuickSettingsCurtainModel.classify_slider_tap("CCT", 435, 120)
            self.assertEqual(action, "PLUS")
            warmth = QuickSettingsCurtainModel.step_channel(warmth, 10)
        self.assertEqual(warmth, 100)

        # Step 5: Toggle pills
        pill_dark = QuickSettingsCurtainModel.classify_pill_tap(300, 180)
        self.assertEqual(pill_dark, "DARK_MODE")
        dark_mode = not dark_mode

        pill_rot = QuickSettingsCurtainModel.classify_pill_tap(100, 240)
        self.assertEqual(pill_rot, "ROTATION_LOCK")
        rotation_lock = not rotation_lock

        # Step 6: Dismiss curtain by tapping outside curtain (y = 400 >= 360)
        dismiss_tap = QuickSettingsCurtainModel.classify_dismiss_tap(240, 400)
        self.assertTrue(dismiss_tap)

        # Step 7: Verify final state
        self.assertEqual(brightness, 0)
        self.assertEqual(warmth, 100)
        self.assertTrue(dark_mode)
        self.assertTrue(rotation_lock)

    def test_15_handedness_runtime_flip_and_navigation(self):
        """
        Simulate runtime flip of handedness:
        1. User in Right-handed mode opens Side Drawer via swipe from right edge
        2. Taps target 5 (Settings)
        3. Changes handedness setting from RIGHT to LEFT
        4. Drawer dismisses, user returns
        5. User tries right-edge swipe -> now triggers BACK gesture, NOT drawer
        6. User does left-edge swipe -> now successfully OPENS drawer
        """
        handedness = HandednessEnum.RIGHT

        # 1. Right edge swipe opens drawer
        self.assertTrue(SideDrawerModel.classify_open_gesture(465, 400, 395, 400, handedness))
        self.assertFalse(SideDrawerModel.classify_back_gesture(465, 400, 395, 400, handedness))

        # 2. Select target 5 (Settings)
        item_idx = SideDrawerModel.classify_item_tap(300, 470, handedness)
        self.assertEqual(item_idx, 5)
        self.assertEqual(SideDrawerModel.TARGETS[item_idx]["id"], "SETTINGS")

        # 3. Flip handedness to LEFT
        handedness = HandednessEnum.LEFT

        # 5. Right edge swipe is now BACK, not drawer
        self.assertFalse(SideDrawerModel.classify_open_gesture(465, 400, 395, 400, handedness))
        self.assertTrue(SideDrawerModel.classify_back_gesture(465, 400, 395, 400, handedness))

        # 6. Left edge swipe now OPENS drawer
        self.assertTrue(SideDrawerModel.classify_open_gesture(15, 400, 85, 400, handedness))
        self.assertFalse(SideDrawerModel.classify_back_gesture(15, 400, 85, 400, handedness))

        # Pull tab is now on left side
        self.assertTrue(SideDrawerModel.classify_side_tab_tap(10, 400, handedness))
        self.assertFalse(SideDrawerModel.classify_side_tab_tap(470, 400, handedness))


if __name__ == "__main__":
    unittest.main()
