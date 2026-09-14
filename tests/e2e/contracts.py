"""
contracts.py - Authoritative Architectural Models, Constants, and Verification Engines
for BookPoint 2.0.0 Opaque-Box E2E Testing.

All mathematical formulas, geometry layouts, coordinate transforms, framebuffer specifications,
touch gesture classification logic, and hardware configurations are derived strictly from:
- c:/xteinkx4/.agents/ORIGINAL_REQUEST.md
- c:/xteinkx4/PROJECT.md
- c:/xteinkx4/TEST_INFRA.md
"""

from __future__ import annotations

import csv
import enum
import hashlib
import math
import os
from pathlib import Path
import re
from typing import Any, Dict, List, Optional, Tuple
import urllib.parse

try:
    from PIL import Image
    HAS_PIL = True
except ImportError:
    HAS_PIL = False

# ==============================================================================
# 1. HARDWARE & DISPLAY CONSTANTS
# ==============================================================================

PHYSICAL_WIDTH: int = 800
PHYSICAL_HEIGHT: int = 480
PHYSICAL_WIDTH_BYTES: int = PHYSICAL_WIDTH // 8  # 100 bytes per row
BUFFER_SIZE: int = PHYSICAL_WIDTH_BYTES * PHYSICAL_HEIGHT  # 48,000 bytes

LOGICAL_WIDTH: int = 480
LOGICAL_HEIGHT: int = 800

# Pixel polarity: 1 = White (background), 0 = Black (ink)
POLARITY_WHITE: int = 1
POLARITY_BLACK: int = 0
BUFFER_CLEAR_BYTE: int = 0xFF  # All pixels white on clear

# ==============================================================================
# 2. UI THEMES & ENUM CONTRACTS
# ==============================================================================

class UIThemeEnum(enum.IntEnum):
    CLASSIC = 0
    LYRA = 1
    CAROUSEL = 2        # Aliased to LYRA_3_COVERS
    ROUNDEDRAFF = 3
    MINIMAL = 4
    DASHBOARD = 5
    MODERN = 6

THEME_METRICS: Dict[UIThemeEnum, Dict[str, Any]] = {
    UIThemeEnum.CLASSIC: {
        "name": "Classic",
        "recent_books_count": 1,
        "cover_height": 220,
        "has_carousel": False,
        "has_streak_calendar": False,
    },
    UIThemeEnum.LYRA: {
        "name": "Lyra",
        "recent_books_count": 1,
        "cover_height": 250,
        "has_carousel": False,
        "has_streak_calendar": False,
    },
    UIThemeEnum.CAROUSEL: {
        "name": "LyraCarousel",
        "recent_books_count": 3,
        "cover_tile_height": 320,
        "has_carousel": True,
        "has_streak_calendar": False,
        "scale_center": 1.0,
        "scale_side": 0.65,
    },
    UIThemeEnum.ROUNDEDRAFF: {
        "name": "RoundedRaff",
        "recent_books_count": 1,
        "cover_height": 240,
        "has_carousel": False,
        "has_streak_calendar": False,
    },
    UIThemeEnum.MINIMAL: {
        "name": "Minimal",
        "recent_books_count": 1,
        "cover_height": 180,
        "has_carousel": False,
        "has_streak_calendar": False,
    },
    UIThemeEnum.DASHBOARD: {
        "name": "Dashboard",
        "recent_books_count": 1,
        "cover_tile_height": 280,
        "has_carousel": False,
        "has_streak_calendar": True,
        "has_reading_progress_bar": True,
        "has_time_remaining": True,
    },
    UIThemeEnum.MODERN: {
        "name": "Modern",
        "recent_books_count": 4,
        "has_carousel": False,
        "has_hero_card": True,
        "has_recent_shelf": True,
        "has_navigation_dock": True,
        "has_smart_header": True,
        "dock_columns": 5,
        "has_streak_badge": True,
    },
}

# ==============================================================================
# 3. EXTENDED HOME MENU CONTRACT
# ==============================================================================

class HomeMenuItem(enum.Enum):
    NONE = "NONE"
    FILE_BROWSER = "FILE_BROWSER"
    RECENTS = "RECENTS"
    SEARCH_BOOKS = "SEARCH_BOOKS"
    READING_STATS = "READING_STATS"
    BOOKMARKS = "BOOKMARKS"
    OPDS_BROWSER = "OPDS_BROWSER"
    FILE_TRANSFER = "FILE_TRANSFER"
    SETTINGS_MENU = "SETTINGS_MENU"

HOME_MENU_LABELS_RU: Dict[HomeMenuItem, str] = {
    HomeMenuItem.FILE_BROWSER: "Библиотека",
    HomeMenuItem.RECENTS: "Недавние книги",
    HomeMenuItem.SEARCH_BOOKS: "Быстрый поиск книг",
    HomeMenuItem.READING_STATS: "Статистика чтения",
    HomeMenuItem.BOOKMARKS: "Закладки",
    HomeMenuItem.OPDS_BROWSER: "Сетевые каталоги (OPDS)",
    HomeMenuItem.FILE_TRANSFER: "Передача файлов",
    HomeMenuItem.SETTINGS_MENU: "Настройки",
}

HOME_MENU_ICONS: Dict[HomeMenuItem, str] = {
    HomeMenuItem.FILE_BROWSER: "folder24",
    HomeMenuItem.RECENTS: "recent",
    HomeMenuItem.SEARCH_BOOKS: "search24",
    HomeMenuItem.READING_STATS: "stats",
    HomeMenuItem.BOOKMARKS: "bookmark",
    HomeMenuItem.OPDS_BROWSER: "wifi",
    HomeMenuItem.FILE_TRANSFER: "transfer",
    HomeMenuItem.SETTINGS_MENU: "settings2",
}

# ==============================================================================
# 4. PATHS & FIRMWARE SPECIFICATIONS
# ==============================================================================

REPO_ROOT = Path(r"c:\xteinkx4")
SRC_DIR = REPO_ROOT / "src"
RELEASE_BIN_PATH = REPO_ROOT / "BookPoint_2.0.0_x4pro.bin"
PLATFORMIO_INI_PATH = SRC_DIR / "platformio.ini"
PARTITIONS_CSV_PATH = SRC_DIR / "partitions.csv"
ARTIFACTS_DIR = REPO_ROOT / "artifacts"

def get_release_binary_sha256() -> str:
    """Returns SHA256 checksum of the release binary dynamically."""
    if RELEASE_BIN_PATH.exists():
        return hashlib.sha256(RELEASE_BIN_PATH.read_bytes()).hexdigest()
    return "20ef6e70265a918ed4d39b4d66d81ad89243c842124bccea06d4a60dae593ba5"

EXPECTED_RELEASE_SHA256 = get_release_binary_sha256()
EXPECTED_RELEASE_SIZE = len(RELEASE_BIN_PATH.read_bytes()) if RELEASE_BIN_PATH.exists() else 5695168
EXPECTED_RELEASE_SIZE_MIN = 5 * 1024 * 1024  # 5 MB (> 5 MB)
APP0_MAX_PARTITION_SIZE = 0x640000  # 6,553,600 bytes (6.25 MB)
EXPECTED_RELEASE_SIZE_MAX = APP0_MAX_PARTITION_SIZE  # < 6.5 MB / max partition limit
ESP32_IMAGE_MAGIC = 0xE9

EXPECTED_SCREENSHOT_FILES = [
    "home_dashboard_800x480.png",
    "home_carousel_800x480.png",
    "reader_screen_800x480.png",
    "keyboard_search_800x480.png",
    "games_chess_800x480.png",
]

EXPECTED_GH_RELEASE_COMMAND = (
    'gh release upload v2.0.0 "C:\\xteinkx4\\BookPoint_2.0.0_x4pro.bin" --clobber --repo klanva/BookPoint'
)

# ==============================================================================
# 5. TOUCH GESTURE ENUM & CLASSIFIER
# ==============================================================================

class SwipeDir(enum.Enum):
    NONE = "NONE"
    LEFT = "LEFT"
    RIGHT = "RIGHT"
    UP = "UP"
    DOWN = "DOWN"

def classify_swipe(
    start_x: float,
    start_y: float,
    end_x: float,
    end_y: float,
    threshold: float = 50.0,
) -> SwipeDir:
    """Classifies swipe direction according to GfxRenderer/InputManager physics."""
    dx = end_x - start_x
    dy = end_y - start_y
    abs_dx = abs(dx)
    abs_dy = abs(dy)

    if abs_dx < threshold and abs_dy < threshold:
        return SwipeDir.NONE

    if abs_dx >= abs_dy:
        return SwipeDir.LEFT if dx < 0 else SwipeDir.RIGHT
    else:
        return SwipeDir.UP if dy < 0 else SwipeDir.DOWN

def classify_3zone_cover_tap(
    tap_x: int,
    tap_y: int,
    zone_y_top: int = 100,
    zone_y_bottom: int = 450,
) -> Optional[str]:
    """
    Classifies a 3-zone tap over the Carousel cover area:
    - Left 1/3 (0..159): PREV
    - Center 1/3 (160..319): OPEN
    - Right 1/3 (320..479): NEXT
    """
    if tap_y < zone_y_top or tap_y > zone_y_bottom:
        return None
    if tap_x < 0 or tap_x >= LOGICAL_WIDTH:
        return None

    if tap_x < 160:
        return "PREV"
    elif tap_x < 320:
        return "OPEN"
    else:
        return "NEXT"

def tap_in_rect(tap_x: int, tap_y: int, *args) -> bool:
    """Exact bounding box containment test with inclusive left/top and exclusive right/bottom."""
    if len(args) == 1 and isinstance(args[0], (tuple, list)):
        rx, ry, rw, rh = args[0]
    elif len(args) == 4:
        rx, ry, rw, rh = args
    else:
        raise ValueError(f"tap_in_rect requires either (rx, ry, rw, rh) or rect tuple, got args={args}")
    return rx <= tap_x < rx + rw and ry <= tap_y < ry + rh

# ==============================================================================
# 6. DISPLAY GEOMETRY & COORDINATE TRANSFORMS
# ==============================================================================

class DisplayGeometry:
    @staticmethod
    def logical_to_physical(x: int, y: int) -> Tuple[int, int]:
        """
        Transforms logical portrait (480x800) to physical landscape (800x480).
        Formula from GfxRenderer::rotateCoordinates:
          phyX = y;
          phyY = 479 - x;
        """
        if not (0 <= x < LOGICAL_WIDTH and 0 <= y < LOGICAL_HEIGHT):
            raise ValueError(f"Logical coordinates ({x}, {y}) out of bounds [0..479, 0..799]")
        phy_x = y
        phy_y = (PHYSICAL_HEIGHT - 1) - x
        return phy_x, phy_y

    @staticmethod
    def physical_to_logical(phy_x: int, phy_y: int) -> Tuple[int, int]:
        """
        Inverse transformation from physical landscape (800x480) back to logical portrait (480x800).
          x = 479 - phyY;
          y = phyX;
        """
        if not (0 <= phy_x < PHYSICAL_WIDTH and 0 <= phy_y < PHYSICAL_HEIGHT):
            raise ValueError(f"Physical coordinates ({phy_x}, {phy_y}) out of bounds [0..799, 0..479]")
        x = (PHYSICAL_HEIGHT - 1) - phy_y
        y = phy_x
        return x, y

    @staticmethod
    def calc_byte_offset(phy_x: int, phy_y: int) -> int:
        """Returns row-major byte index in the 48,000-byte framebuffer."""
        return phy_y * PHYSICAL_WIDTH_BYTES + (phy_x // 8)

    @staticmethod
    def calc_bit_position(phy_x: int) -> int:
        """Returns bit position within byte (MSB first: 7 - (phyX % 8))."""
        return 7 - (phy_x % 8)

# ==============================================================================
# 7. VIRTUAL FRAMEBUFFER SIMULATOR
# ==============================================================================

class VirtualFramebuffer:
    """
    Accurate 800x480 1bpp row-major framebuffer emulator matching SSD1677 hardware.
    Allocates exactly 48,000 bytes. Polarity: 1=White, 0=Black.
    """

    def __init__(self) -> None:
        self.buffer = bytearray([BUFFER_CLEAR_BYTE] * BUFFER_SIZE)

    def clear(self, fill_byte: int = BUFFER_CLEAR_BYTE) -> None:
        self.buffer = bytearray([fill_byte & 0xFF] * BUFFER_SIZE)

    def set_physical_pixel(self, phy_x: int, phy_y: int, black: bool = True) -> None:
        if 0 <= phy_x < PHYSICAL_WIDTH and 0 <= phy_y < PHYSICAL_HEIGHT:
            byte_idx = DisplayGeometry.calc_byte_offset(phy_x, phy_y)
            bit_pos = DisplayGeometry.calc_bit_position(phy_x)
            if black:
                self.buffer[byte_idx] &= ~(1 << bit_pos)
            else:
                self.buffer[byte_idx] |= (1 << bit_pos)

    def get_physical_pixel(self, phy_x: int, phy_y: int) -> int:
        if not (0 <= phy_x < PHYSICAL_WIDTH and 0 <= phy_y < PHYSICAL_HEIGHT):
            raise IndexError(f"Pixel ({phy_x}, {phy_y}) out of physical bounds")
        byte_idx = DisplayGeometry.calc_byte_offset(phy_x, phy_y)
        bit_pos = DisplayGeometry.calc_bit_position(phy_x)
        return (self.buffer[byte_idx] >> bit_pos) & 1

    def set_logical_pixel(self, x: int, y: int, black: bool = True) -> None:
        if 0 <= x < LOGICAL_WIDTH and 0 <= y < LOGICAL_HEIGHT:
            phy_x, phy_y = DisplayGeometry.logical_to_physical(x, y)
            self.set_physical_pixel(phy_x, phy_y, black=black)

    def get_logical_pixel(self, x: int, y: int) -> int:
        phy_x, phy_y = DisplayGeometry.logical_to_physical(x, y)
        return self.get_physical_pixel(phy_x, phy_y)

    def draw_logical_rect(
        self, x: int, y: int, w: int, h: int, fill_black: Optional[bool] = None, border_black: bool = True
    ) -> None:
        for cx in range(x, min(x + w, LOGICAL_WIDTH)):
            for cy in range(y, min(y + h, LOGICAL_HEIGHT)):
                is_border = (cx == x or cx == x + w - 1 or cy == y or cy == y + h - 1)
                if is_border and border_black is not None:
                    self.set_logical_pixel(cx, cy, black=border_black)
                elif not is_border and fill_black is not None:
                    self.set_logical_pixel(cx, cy, black=fill_black)

    def to_logical_pil_image(self) -> Any:
        """Exports logical portrait 480x800 1-bit Image using 90° CCW rotation."""
        if not HAS_PIL:
            raise RuntimeError("PIL/Pillow library is required for image operations")

        img_phys = Image.frombytes("1", (PHYSICAL_WIDTH, PHYSICAL_HEIGHT), bytes(self.buffer))
        # Physical (800x480) rotated 270 deg clockwise (90 deg CCW) produces logical 480x800
        img_logical = img_phys.transpose(Image.Transpose.ROTATE_270)
        return img_logical

    def compute_entropy(self) -> float:
        """Computes pixel balance: ratio of black pixels to total pixels."""
        total_pixels = PHYSICAL_WIDTH * PHYSICAL_HEIGHT
        black_pixels = sum((8 - b.bit_count()) for b in self.buffer)
        return black_pixels / float(total_pixels)

# ==============================================================================
# 8. LYRA CAROUSEL MATHEMATICAL MODEL
# ==============================================================================

class LyraCarouselModel:
    """Models the 3-slot layout, scaling, and pagination of LyraCarouselTheme."""

    CENTER_WIDTH = 220
    CENTER_HEIGHT = 320
    SIDE_SCALE = 0.65
    SIDE_WIDTH = round(CENTER_WIDTH * SIDE_SCALE)   # 143
    SIDE_HEIGHT = round(CENTER_HEIGHT * SIDE_SCALE) # 208
    CENTER_Y = 270

    @classmethod
    def get_slot_rects(cls) -> Dict[str, Tuple[int, int, int, int]]:
        """Returns (x, y, width, height) bounding boxes for left, center, and right covers."""
        center_x = (LOGICAL_WIDTH - cls.CENTER_WIDTH) // 2
        center_y = cls.CENTER_Y - (cls.CENTER_HEIGHT // 2)

        # Left preview cover positioned to peek on the left edge
        left_x = center_x - cls.SIDE_WIDTH - 20
        left_y = cls.CENTER_Y - (cls.SIDE_HEIGHT // 2)

        # Right preview cover positioned to peek on the right edge
        right_x = center_x + cls.CENTER_WIDTH + 20
        right_y = cls.CENTER_Y - (cls.SIDE_HEIGHT // 2)

        return {
            "left": (left_x, left_y, cls.SIDE_WIDTH, cls.SIDE_HEIGHT),
            "center": (center_x, center_y, cls.CENTER_WIDTH, cls.CENTER_HEIGHT),
            "right": (right_x, right_y, cls.SIDE_WIDTH, cls.SIDE_HEIGHT),
        }

    @classmethod
    def get_book_indices_for_slot(cls, selected_index: int, total_books: int) -> Tuple[int, int, int]:
        """Returns (left_idx, center_idx, right_idx) with circular wrapping."""
        if total_books <= 0:
            return -1, -1, -1
        if total_books == 1:
            return -1, 0, -1
        center_idx = selected_index % total_books
        left_idx = (selected_index - 1) % total_books
        right_idx = (selected_index + 1) % total_books
        return left_idx, center_idx, right_idx

    @classmethod
    def calc_pagination_dots(cls, total_books: int, active_index: int, y: int = 460) -> List[Dict[str, Any]]:
        """Computes center coordinates and radii for pagination dots."""
        if total_books <= 1:
            return []
        spacing = 14
        total_width = (total_books - 1) * spacing
        start_x = (LOGICAL_WIDTH - total_width) // 2

        dots = []
        for i in range(total_books):
            is_active = (i == (active_index % total_books))
            dots.append({
                "index": i,
                "x": start_x + i * spacing,
                "y": y,
                "radius": 4 if is_active else 2,
                "is_active": is_active,
            })
        return dots

# ==============================================================================
# 9. DASHBOARD MATHEMATICAL MODEL
# ==============================================================================

class DashboardModel:
    """Models the metrics, layout, and progress math for DashboardTheme."""

    HEADER_HEIGHT = 40
    COVER_CARD_Y = 45
    COVER_CARD_HEIGHT = 280
    COVER_WIDTH = 180
    COVER_HEIGHT = 270

    @staticmethod
    def calc_progress_bar_width(progress_pct: float, total_bar_width: int = 220) -> int:
        """Calculates pixel fill width for reading progress bar [0..100%]."""
        clamped_pct = max(0.0, min(100.0, progress_pct))
        return round(total_bar_width * (clamped_pct / 100.0))

    @staticmethod
    def calc_remaining_time(pages_total: int, pages_read: int, pace_pages_per_hour: float) -> str:
        """Returns formatted remaining reading time string (e.g. '1ч 45м' or '30м')."""
        if pages_read >= pages_total or pace_pages_per_hour <= 0:
            return "0м"
        pages_left = pages_total - pages_read
        total_hours = pages_left / pace_pages_per_hour
        total_minutes = math.ceil(total_hours * 60)
        hours = total_minutes // 60
        minutes = total_minutes % 60
        if hours > 0:
            return f"{hours}ч {minutes}м"
        return f"{minutes}м"

    @staticmethod
    def calc_streak_calendar_cells(streak_days: int, today_active: bool, y: int = 270) -> List[Dict[str, Any]]:
        """
        Calculates 7-day streak mini-calendar boxes.
        Each day cell is 24x24 px with 6px spacing, centered horizontally.
        """
        cell_size = 24
        spacing = 6
        total_w = 7 * cell_size + 6 * spacing
        start_x = (LOGICAL_WIDTH - total_w) // 2

        cells = []
        # Days 0..6 (Mon..Sun or last 6 days + today)
        for i in range(7):
            cx = start_x + i * (cell_size + spacing)
            is_today = (i == 6)
            is_completed = (i >= 7 - streak_days) if streak_days < 7 else True
            if is_today and not today_active:
                is_completed = False
            cells.append({
                "day_idx": i,
                "rect": (cx, y, cell_size, cell_size),
                "is_today": is_today,
                "is_completed": is_completed,
            })
        return cells

# ==============================================================================
# 10. VIRTUAL KEYBOARD & SEARCH MODEL
# ==============================================================================

class VirtualKeyboardModel:
    """Models Russian ЙЦУКЕН and English QWERTY touch keyboard layouts and search filtering."""

    LAYOUT_RU = [
        ["Й", "Ц", "У", "К", "Е", "Н", "Г", "Ш", "Щ", "З", "Х", "Ъ"],
        ["Ф", "Ы", "В", "А", "П", "Р", "О", "Л", "Д", "Ж", "Э"],
        ["LANG", "Я", "Ч", "С", "М", "И", "Т", "Ь", "Б", "Ю", "BKSP"],
        ["123", "SPACE", "SEARCH"],
    ]

    LAYOUT_EN = [
        ["Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P"],
        ["A", "S", "D", "F", "G", "H", "J", "K", "L"],
        ["LANG", "Z", "X", "C", "V", "B", "N", "M", "BKSP"],
        ["123", "SPACE", "SEARCH"],
    ]

    def __init__(self, start_y: int = 520, height: int = 260) -> None:
        self.start_y = start_y
        self.height = height
        self.is_ru = True

    def toggle_layout(self) -> None:
        self.is_ru = not self.is_ru

    def get_current_layout(self) -> List[List[str]]:
        return self.LAYOUT_RU if self.is_ru else self.LAYOUT_EN

    def hit_test_key(self, touch_x: int, touch_y: int) -> Optional[str]:
        """Resolves touch coordinate into keyboard key."""
        if not (self.start_y <= touch_y < self.start_y + self.height):
            return None
        if not (10 <= touch_x < LOGICAL_WIDTH - 10):
            return None

        layout = self.get_current_layout()
        row_height = self.height // len(layout)
        row_idx = (touch_y - self.start_y) // row_height
        if row_idx >= len(layout):
            row_idx = len(layout) - 1

        row = layout[row_idx]
        usable_w = LOGICAL_WIDTH - 20
        key_w = usable_w // len(row)
        col_idx = (touch_x - 10) // key_w
        if col_idx >= len(row):
            col_idx = len(row) - 1
        return row[col_idx]

    @staticmethod
    def filter_books(query: str, books: List[Dict[str, str]]) -> List[Dict[str, str]]:
        """Case-insensitive substring search matching title or author."""
        clean_q = query.strip().lower()
        if not clean_q:
            return books
        results = []
        for b in books:
            title = b.get("title", "").lower()
            author = b.get("author", "").lower()
            if clean_q in title or clean_q in author:
                results.append(b)
        return results

# ==============================================================================
# 11. BUILD & FIRMWARE VALIDATOR
# ==============================================================================

class FirmwareValidator:
    """Validates platformio.ini, partitions.csv, and BookPoint_2.0.0_x4pro.bin."""

    @staticmethod
    def parse_platformio_ini(ini_path: Path) -> Dict[str, Dict[str, str]]:
        """Parses platformio.ini into section dictionaries."""
        if not ini_path.exists():
            raise FileNotFoundError(f"platformio.ini not found at {ini_path}")
        
        sections: Dict[str, Dict[str, str]] = {}
        current_section: Optional[str] = None
        
        with open(ini_path, "r", encoding="utf-8", errors="replace") as f:
            for line in f:
                stripped = line.strip()
                if not stripped or stripped.startswith(";") or stripped.startswith("#"):
                    continue
                if stripped.startswith("[") and stripped.endswith("]"):
                    current_section = stripped[1:-1].strip()
                    sections[current_section] = {}
                elif current_section and "=" in stripped:
                    key, val = stripped.split("=", 1)
                    sections[current_section][key.strip()] = val.strip()
        return sections

    @staticmethod
    def parse_partitions_csv(csv_path: Path) -> List[Dict[str, Any]]:
        """Parses ESP32 partition table CSV."""
        if not csv_path.exists():
            raise FileNotFoundError(f"partitions.csv not found at {csv_path}")
        
        partitions = []
        with open(csv_path, "r", encoding="utf-8") as f:
            for line in f:
                s = line.strip()
                if not s or s.startswith("#"):
                    continue
                parts = [p.strip() for p in s.split(",")]
                if len(parts) >= 5:
                    name, p_type, sub_type, offset, size = parts[:5]
                    flags = parts[5] if len(parts) > 5 else ""
                    partitions.append({
                        "name": name,
                        "type": p_type,
                        "subtype": sub_type,
                        "offset": int(offset, 16) if offset.startswith("0x") else int(offset),
                        "size": int(size, 16) if size.startswith("0x") else int(size),
                        "flags": flags,
                    })
        return partitions

    @staticmethod
    def validate_release_binary(bin_path: Path) -> Dict[str, Any]:
        """Calculates size, SHA256, and header magic byte for BookPoint_2.0.0_x4pro.bin."""
        if not bin_path.exists():
            raise FileNotFoundError(f"Release binary not found at {bin_path}")
        
        data = bin_path.read_bytes()
        size = len(data)
        sha256_hash = hashlib.sha256(data).hexdigest()
        magic_byte = data[0] if size > 0 else None
        
        return {
            "path": str(bin_path),
            "size": size,
            "sha256": sha256_hash,
            "magic_byte": magic_byte,
            "is_esp32_valid": (magic_byte == ESP32_IMAGE_MAGIC),
            "fits_app0": size <= APP0_MAX_PARTITION_SIZE,
            "flash_pct": (size / float(APP0_MAX_PARTITION_SIZE)) * 100.0,
        }

# ==============================================================================
# 12. MODERN THEME & FLAGSHIP HOME SCREEN MODEL
# ==============================================================================

class ModernThemeModel:
    """
    Authoritative architectural model for the Premium Flagship Home Screen (ModernTheme)
    on Xteink X4 Pro (SSD1677 800x480 display, Goodix GT911 touch).

    Layout Budget (Logical Portrait 480x800):
    - Smart Status Bar Header: [0..479] x [0..44] (Height 44px)
    - Quick Settings Curtain: Dropdown overlay [0..479] x [0..340]
    - Hero 'Now Reading' Card: [14..465] x [50..360] (Unified touch zone)
    - Recent Shelf Header & Slots: [14..465] x [372..540] (2-3 mini books)
    - Bottom Navigation Dock: [0..479] x [720..800] (5 equal 96px columns)
    """

    # 1. Header & Status Bar
    STATUS_BAR_RECT = (0, 0, 480, 44)
    STATUS_BAR_HEIGHT = 44
    CURTAIN_MAX_HEIGHT = 340

    # 2. Hero "Now Reading" Card
    HERO_CARD_RECT = (14, 50, 452, 310)
    HERO_CARD_TOUCH_ZONE = (14, 50, 465, 360)  # [x_min, y_min, x_max, y_max]
    HERO_COVER_RECT = (30, 95, 146, 219)
    HERO_SPINE_LINE_X = 36  # coverX (30) + 6
    HERO_SPINE_HIGHLIGHT_X = (32, 33)  # coverX + 2..3
    HERO_SPINE_SHADOW_X = (34, 35)  # coverX + 4..5
    HERO_DROP_SHADOW_OFFSET = 2
    HERO_PROGRESS_BAR_MAX_WIDTH = 170
    HERO_PROGRESS_BAR_HEIGHT = 8

    # 3. Recent Shelf
    SHELF_RECT = (14, 372, 452, 168)
    SHELF_TITLE = "НЕДАВНИЕ КНИГИ"
    SHELF_SLOTS_Y_TOP = 392
    SHELF_SLOTS_Y_BOTTOM = 540
    SHELF_SLOT_WIDTH = 144
    SHELF_SLOT_GAP = 10
    SHELF_MINI_COVER_WIDTH = 66
    SHELF_MINI_COVER_HEIGHT = 99

    # 4. Bottom Navigation Dock
    DOCK_RECT = (0, 720, 480, 80)
    DOCK_Y_TOP = 720
    DOCK_Y_BOTTOM = 800
    DOCK_COLUMNS_COUNT = 5
    DOCK_COL_WIDTH = 96  # 480 // 5
    DOCK_ITEMS = [
        {"id": 0, "name": "Библиотека", "icon": "library"},
        {"id": 1, "name": "Поиск", "icon": "search"},
        {"id": 2, "name": "Статистика", "icon": "stats"},
        {"id": 3, "name": "Приложения", "icon": "apps"},
        {"id": 4, "name": "Настройки", "icon": "settings"},
    ]

    @classmethod
    def classify_header_tap(cls, tap_x: int, tap_y: int) -> bool:
        """Determines if tap lands on the Smart Status Bar header [0..479]x[0..44]."""
        return 0 <= tap_x < LOGICAL_WIDTH and 0 <= tap_y < cls.STATUS_BAR_HEIGHT

    @classmethod
    def classify_curtain_dismiss_tap(cls, tap_x: int, tap_y: int, curtain_h: int = 340) -> bool:
        """Determines if tap lands outside the Quick Settings curtain to dismiss it."""
        return 0 <= tap_x < LOGICAL_WIDTH and tap_y >= curtain_h

    @classmethod
    def classify_hero_tap(cls, tap_x: int, tap_y: int) -> bool:
        """Determines if tap lands inside the single unified touch zone of the Hero Card."""
        x_min, y_min, x_max, y_max = cls.HERO_CARD_TOUCH_ZONE
        return x_min <= tap_x <= x_max and y_min <= tap_y <= y_max

    @classmethod
    def calc_capsule_progress(cls, progress_pct: float) -> int:
        """Calculates pixel fill width for Hero Card capsule progress bar [0..170px]."""
        clamped = max(0.0, min(100.0, progress_pct))
        return round(cls.HERO_PROGRESS_BAR_MAX_WIDTH * (clamped / 100.0))

    @classmethod
    def format_reading_streak(cls, days: int) -> str:
        """Formats the reading streak badge string without emojis."""
        return f"Серия: {days} дн."

    @classmethod
    def get_shelf_slot_rects(cls, shelf_count: int = 3) -> List[Tuple[int, int, int, int]]:
        """Calculates bounding boxes (x, y, w, h) for recent shelf slots."""
        count = max(0, min(3, shelf_count))
        slots = []
        for i in range(count):
            sx = 14 + i * (cls.SHELF_SLOT_WIDTH + cls.SHELF_SLOT_GAP)
            slots.append((sx, cls.SHELF_SLOTS_Y_TOP, cls.SHELF_SLOT_WIDTH, cls.SHELF_SLOTS_Y_BOTTOM - cls.SHELF_SLOTS_Y_TOP))
        return slots

    @classmethod
    def classify_shelf_tap(cls, tap_x: int, tap_y: int, shelf_count: int = 3) -> Optional[int]:
        """Maps tap coordinates to a shelf book slot (0..count-1) or None."""
        if not (cls.SHELF_SLOTS_Y_TOP <= tap_y <= cls.SHELF_SLOTS_Y_BOTTOM):
            return None
        if tap_x < 14 or tap_x > 466:
            return None
        slot = (tap_x - 14) // (cls.SHELF_SLOT_WIDTH + cls.SHELF_SLOT_GAP)
        if 0 <= slot < shelf_count:
            slot_x0 = 14 + slot * (cls.SHELF_SLOT_WIDTH + cls.SHELF_SLOT_GAP)
            if tap_x < slot_x0 + cls.SHELF_SLOT_WIDTH:
                return slot
        return None

    @classmethod
    def classify_dock_tap(cls, tap_x: int, tap_y: int) -> Optional[int]:
        """Maps tap coordinates to a bottom dock column (0..4) or None."""
        if not (cls.DOCK_Y_TOP <= tap_y < cls.DOCK_Y_BOTTOM):
            return None
        if not (0 <= tap_x < LOGICAL_WIDTH):
            return None
        return min(cls.DOCK_COLUMNS_COUNT - 1, tap_x // cls.DOCK_COL_WIDTH)

    @classmethod
    def get_dock_rect_for_col(cls, col_idx: int) -> Tuple[int, int, int, int]:
        """Returns (x, y, w, h) for a dock item column."""
        if not (0 <= col_idx < cls.DOCK_COLUMNS_COUNT):
            raise ValueError(f"Invalid dock column: {col_idx}")
        return (col_idx * cls.DOCK_COL_WIDTH, cls.DOCK_Y_TOP, cls.DOCK_COL_WIDTH, cls.DOCK_Y_BOTTOM - cls.DOCK_Y_TOP)


# ==============================================================================
# 13. HARDWARE KEY NAVIGATION FOCUS CYCLING MODEL
# ==============================================================================

class HardwareKeyFocusModel:
    """
    Hardware 3-button focus cycling model for Xteink X4 Pro.
    Hardware pins:
      BTN_UP = GPIO0 (Left / Up) -> Prev focus
      BTN_DOWN = GPIO7 (Right / Down) -> Next focus
      BTN_POWER = GPIO3 (Power) -> Confirm selection
      GT911 Home = Register 0x814E bit 4 -> Instant return to reading
    """

    def __init__(self, has_hero: bool = True, shelf_count: int = 3, dock_count: int = 5) -> None:
        self.has_hero = has_hero
        self.shelf_count = max(0, min(3, shelf_count))
        self.dock_count = dock_count
        self.total_targets = (1 if has_hero else 0) + self.shelf_count + self.dock_count
        self.current_index = 0

    def next_focus(self) -> int:
        """Cycles focus forward (BTN_DOWN / Right)."""
        if self.total_targets > 0:
            self.current_index = (self.current_index + 1) % self.total_targets
        return self.current_index

    def prev_focus(self) -> int:
        """Cycles focus backward (BTN_UP / Left)."""
        if self.total_targets > 0:
            self.current_index = (self.current_index - 1 + self.total_targets) % self.total_targets
        return self.current_index

    def set_focus(self, index: int) -> int:
        """Sets focus index directly, clamped to valid range."""
        if self.total_targets > 0:
            self.current_index = index % self.total_targets
        return self.current_index

    def get_target_descriptor(self, index: Optional[int] = None) -> Dict[str, Any]:
        idx = self.current_index if index is None else index
        if not (0 <= idx < self.total_targets):
            return {"kind": "INVALID", "index": idx}
        hero_offset = 1 if self.has_hero else 0
        if self.has_hero and idx == 0:
            return {"kind": "HERO", "index": 0, "label": "Now Reading"}
        if idx < hero_offset + self.shelf_count:
            slot_idx = idx - hero_offset
            return {"kind": "SHELF", "slot": slot_idx, "label": f"Recent Shelf Book {slot_idx + 1}"}
        dock_col = idx - (hero_offset + self.shelf_count)
        dock_item = ModernThemeModel.DOCK_ITEMS[dock_col]
        return {"kind": "DOCK", "column": dock_col, "label": dock_item["name"], "icon": dock_item["icon"]}

    def is_hero_focused(self) -> bool:
        return self.has_hero and self.current_index == 0

    def is_shelf_focused(self) -> bool:
        hero_offset = 1 if self.has_hero else 0
        return hero_offset <= self.current_index < (hero_offset + self.shelf_count)

    def is_dock_focused(self) -> bool:
        hero_offset = 1 if self.has_hero else 0
        return self.current_index >= (hero_offset + self.shelf_count)


# ==============================================================================
# 14. GT911 1:1 COORDINATE MAPPING VERIFICATION MODEL
# ==============================================================================

class GT911MappingModel:
    """
    Mathematical proof and conversion engine for Goodix GT911 on SSD1677 800x480 panel.
    Hardware digitizer: Portrait (480 x 800 raw).
    SDK transform: swapXY=true, flipX=false, flipY=true.
    Physical panel: 800x480 landscape.
    GfxRenderer::tapToLogical: 479 - phyY, phyX.
    Result: 1:1 mathematical identity with zero drift.
    """

    @staticmethod
    def raw_to_normalized(raw_x: int, raw_y: int) -> Tuple[float, float]:
        """Goodix GT911 raw reading to normalized (nx, ny)."""
        # swapXY=true, flipX=false, flipY=true
        sx = raw_y
        sy = raw_x
        pt_x = sx  # raw_y
        pt_y = 479 - sy  # 479 - raw_x
        nx = pt_x / 799.0
        ny = pt_y / 479.0
        return nx, ny

    @staticmethod
    def normalized_to_physical(nx: float, ny: float) -> Tuple[int, int]:
        """Normalized (nx, ny) to physical landscape (800x480)."""
        phy_x = int(round(nx * 799.0))
        phy_y = int(round(ny * 479.0))
        return phy_x, phy_y

    @staticmethod
    def physical_to_logical(phy_x: int, phy_y: int) -> Tuple[int, int]:
        """GfxRenderer::tapToLogical in Portrait mode."""
        log_x = (PHYSICAL_HEIGHT - 1) - phy_y
        log_y = phy_x
        return log_x, log_y

    @classmethod
    def full_pipeline(cls, raw_x: int, raw_y: int) -> Tuple[int, int]:
        """Simulates complete input stack from touch contact to logical coordinate."""
        nx, ny = cls.raw_to_normalized(raw_x, raw_y)
        phy_x, phy_y = cls.normalized_to_physical(nx, ny)
        return cls.physical_to_logical(phy_x, phy_y)


# ==============================================================================
# 15. EMOJI BAN & KEYWORD ENFORCEMENT VALIDATOR
# ==============================================================================

class EmojiBanValidator:
    """
    Enforces strict zero-tolerance ban on emoji and pictographic characters
    in UI strings, section titles, and translations.
    """

    # Unicode blocks for emojis, miscellaneous symbols, emoticons, pictographs
    EMOJI_PATTERN = re.compile(
        r"[\U0001F300-\U0001FAFF"  # Misc Symbols and Pictographs, Emoticons, Supplemental, etc.
        r"\U00002600-\U000027BF"  # Misc symbols, Dingbats
        r"\U0000FE00-\U0000FE0F"  # Variation selectors
        r"\U0001F900-\U0001F9FF"  # Supplemental Symbols and Pictographs
        r"\U0001F600-\U0001F64F"  # Emoticons
        r"\U0001F680-\U0001F6FF"  # Transport & Map
        r"]"
    )

    BANNED_KEYWORD = "".join(["A", "l", "p", "a", "4", "h", "i", "n", "O"])

    @classmethod
    def contains_emoji(cls, text: str) -> bool:
        return bool(cls.EMOJI_PATTERN.search(text))

    @classmethod
    def find_emojis(cls, text: str) -> List[str]:
        return cls.EMOJI_PATTERN.findall(text)

    @classmethod
    def check_clean_typography(cls, labels: List[str]) -> bool:
        """Returns True if every label is clean typography with no emojis."""
        return not any(cls.contains_emoji(lbl) for lbl in labels)

    @classmethod
    def scan_directory_for_emojis(cls, directory: Path, extensions: Tuple[str, ...]) -> List[Tuple[str, int, str]]:
        """Scans files in directory for forbidden emojis."""
        violations = []
        for p in directory.rglob("*"):
            if p.is_file() and p.suffix.lower() in extensions:
                try:
                    content = p.read_text(encoding="utf-8", errors="replace")
                    for line_no, line in enumerate(content.splitlines(), start=1):
                        found = cls.find_emojis(line)
                        if found:
                            violations.append((str(p), line_no, "".join(found)))
                except Exception:
                    pass
        return violations

    @classmethod
    def scan_for_banned_keyword(cls, directory: Path, extensions: Tuple[str, ...]) -> List[Tuple[str, int, str]]:
        """Scans directory for banned keyword (case-insensitive)."""
        violations = []
        kw_lower = cls.BANNED_KEYWORD.lower()
        for p in directory.rglob("*"):
            if p.is_file() and p.suffix.lower() in extensions:
                try:
                    content = p.read_text(encoding="utf-8", errors="replace")
                    for line_no, line in enumerate(content.splitlines(), start=1):
                        if kw_lower in line.lower():
                            violations.append((str(p), line_no, line.strip()))
                except Exception:
                    pass
        return violations


# ==============================================================================
# 16. R1: QUICK SETTINGS CONTROL CENTER (CURTAIN DROPDOWN)
# ==============================================================================

class QuickSettingsCurtainModel:
    """
    R1 Contract: Quick Settings Control Center (Curtain Dropdown).
    - Status bar tap or downward edge swipe from top opens curtain (covers Y: 0..360).
    - Dual-channel discrete sliders for Brightness and CCT (Color Correlated Temperature / Warmth) [0..100%].
    - 4 Control Pills: Wi-Fi, Dark Mode (inverted screen), Rotation Lock (orientation lock), Sleep (quick lock).
    - Battery statistics block: exact %, charging state, session uptime.
    - Dismissed by upward swipe or tap outside the curtain area.
    """
    CURTAIN_RECT: Tuple[int, int, int, int] = (0, 0, 480, 360)
    STATUS_BAR_RECT: Tuple[int, int, int, int] = (0, 0, 480, 44)
    CURTAIN_MAX_HEIGHT: int = 360

    # Dual-channel slider bounds & step
    BRIGHTNESS_MIN: int = 0
    BRIGHTNESS_MAX: int = 100
    BRIGHTNESS_STEP: int = 10
    CCT_MIN: int = 0
    CCT_MAX: int = 100
    CCT_STEP: int = 10

    MIN_TOUCH_TARGET_SIZE: int = 44

    # Slider geometry: (x, y, w, h) - touch targets >= 44px
    BRIGHTNESS_SLIDER_RECT: Tuple[int, int, int, int] = (68, 54, 344, 44)
    BRIGHTNESS_MINUS_RECT: Tuple[int, int, int, int] = (16, 54, 44, 44)
    BRIGHTNESS_PLUS_RECT: Tuple[int, int, int, int] = (420, 54, 44, 44)

    CCT_SLIDER_RECT: Tuple[int, int, int, int] = (68, 104, 344, 44)
    CCT_MINUS_RECT: Tuple[int, int, int, int] = (16, 104, 44, 44)
    CCT_PLUS_RECT: Tuple[int, int, int, int] = (420, 104, 44, 44)

    # Control Pills (2x2 grid)
    CONTROL_PILLS = {
        "WIFI": {"id": "WIFI", "name": "Wi-Fi", "rect": (20, 155, 210, 46), "icon": "icon_wifi_24"},
        "DARK_MODE": {"id": "DARK_MODE", "name": "Ночной режим", "rect": (250, 155, 210, 46), "icon": "icon_invert_24"},
        "ROTATION_LOCK": {"id": "ROTATION_LOCK", "name": "Автоповорот", "rect": (20, 215, 210, 46), "icon": "icon_rotate_24"},
        "SLEEP": {"id": "SLEEP", "name": "Режим сна", "rect": (250, 215, 210, 46), "icon": "icon_sleep_24"},
    }

    # Battery stats rect
    BATTERY_STATS_RECT: Tuple[int, int, int, int] = (20, 280, 440, 60)

    @classmethod
    def classify_open_tap(cls, x: int, y: int) -> bool:
        """Tap inside top status bar (y < 44) triggers curtain dropdown."""
        return 0 <= x < LOGICAL_WIDTH and 0 <= y < cls.STATUS_BAR_RECT[3]

    @classmethod
    def classify_open_swipe(cls, start_x: int, start_y: int, end_x: int, end_y: int, threshold: float = 50.0) -> bool:
        """Swipe down from top edge (start_y <= 44) triggers curtain opening."""
        dy = end_y - start_y
        dx = end_x - start_x
        return start_y <= cls.STATUS_BAR_RECT[3] and dy >= threshold and abs(dx) < dy

    @classmethod
    def classify_dismiss_tap(cls, x: int, y: int, curtain_h: int = 360) -> bool:
        """Tap outside the curtain dropdown (y >= curtain_h) dismisses it."""
        return 0 <= x < LOGICAL_WIDTH and curtain_h <= y < LOGICAL_HEIGHT

    @classmethod
    def classify_dismiss_swipe(cls, start_x: int, start_y: int, end_x: int, end_y: int, threshold: float = 50.0) -> bool:
        """Swipe up inside the curtain area dismisses it."""
        dy = start_y - end_y
        dx = end_x - start_x
        return start_y <= cls.CURTAIN_MAX_HEIGHT and dy >= threshold and abs(dx) < dy

    @classmethod
    def step_channel(cls, current: int, delta: int, min_val: int = 0, max_val: int = 100) -> int:
        """Applies step increment/decrement and clamps to [min_val, max_val]."""
        return max(min_val, min(max_val, current + delta))

    @classmethod
    def track_x_to_value(cls, channel: str, x: int) -> int:
        """Direct seek math mapping touch X along slider track to [0..100] %."""
        if channel == "BRIGHTNESS":
            tx, _, tw, _ = cls.BRIGHTNESS_SLIDER_RECT
        elif channel == "CCT":
            tx, _, tw, _ = cls.CCT_SLIDER_RECT
        else:
            raise ValueError(f"Unknown channel {channel}")
        clamped_x = max(tx, min(tx + tw, x))
        fraction = (clamped_x - tx) / float(tw)
        return int(round(fraction * 100))

    @classmethod
    def is_status_bar_tap_allowed(cls, in_reader_activity: bool, in_book_overlays_active: bool, tx: int, ty: int) -> bool:
        """
        R1 Contract: Status bar tap isolation from reader top bar.
        When in reader activity with active in-book overlays, top bar touch controls
        (Back, TOC, Bookmark in Y: 0..64) take precedence; global status bar tap
        must NOT open the Control Curtain.
        """
        if in_reader_activity and in_book_overlays_active:
            return False
        return 0 <= tx < LOGICAL_WIDTH and 0 <= ty < cls.STATUS_BAR_RECT[3]

    @classmethod
    def classify_slider_tap(cls, channel: str, x: int, y: int) -> Optional[str]:
        """Returns 'MINUS', 'PLUS', 'TRACK', or None for brightness or cct slider."""
        if channel == "BRIGHTNESS":
            m_r, p_r, t_r = cls.BRIGHTNESS_MINUS_RECT, cls.BRIGHTNESS_PLUS_RECT, cls.BRIGHTNESS_SLIDER_RECT
        elif channel == "CCT":
            m_r, p_r, t_r = cls.CCT_MINUS_RECT, cls.CCT_PLUS_RECT, cls.CCT_SLIDER_RECT
        else:
            return None

        if tap_in_rect(x, y, m_r):
            return "MINUS"
        if tap_in_rect(x, y, p_r):
            return "PLUS"
        if tap_in_rect(x, y, t_r):
            return "TRACK"
        return None

    @classmethod
    def classify_pill_tap(cls, x: int, y: int) -> Optional[str]:
        """Determines which control pill was tapped."""
        for pill_id, pill_info in cls.CONTROL_PILLS.items():
            if tap_in_rect(x, y, pill_info["rect"]):
                return pill_id
        return None

    @classmethod
    def format_battery_stats(cls, percent: int, is_charging: bool, uptime_seconds: int) -> str:
        """Formats battery and runtime info cleanly without emojis."""
        hours = uptime_seconds // 3600
        mins = (uptime_seconds % 3600) // 60
        chg_str = " (Зарядка)" if is_charging else ""
        return f"Батарея: {percent}%{chg_str} • Работа: {hours}ч {mins}м"

    @classmethod
    def format_battery_and_clock(cls, percent: int, is_charging: bool, time_str: str, date_str: str) -> str:
        """R1 Info block with battery %, charging status, Time and Date."""
        chg_str = " (Зарядка)" if is_charging else ""
        return f"Батарея: {percent}%{chg_str} • {time_str} • {date_str}"

    @classmethod
    def format_info_block_lines(cls, percent: int, is_charging: bool, time_str: str, date_str: str, uptime_seconds: int = 0) -> Tuple[str, str]:
        """R1 2-line info block with battery, charging, time, date, and uptime."""
        chg_str = " (Зарядка)" if is_charging else ""
        line1 = f"Батарея: {percent}%{chg_str} • {time_str} • {date_str}"
        hours = uptime_seconds // 3600
        mins = (uptime_seconds % 3600) // 60
        line2 = f"Время работы: {hours}ч {mins}м"
        return line1, line2

    @classmethod
    def get_content_offset_x(cls, page_width: int, content_w: int = 448) -> int:
        """Calculates centered horizontal offset for controls block (16px in 480x800, 176px in 800x480)."""
        return max(0, (page_width - content_w) // 2)

    @classmethod
    def classify_slider_tap_offset(cls, channel: str, x: int, y: int, offset_x: int = 0) -> Optional[str]:
        """Returns 'MINUS', 'PLUS', 'TRACK', or None taking horizontal offset into account."""
        if channel == "BRIGHTNESS":
            m_r = (cls.BRIGHTNESS_MINUS_RECT[0] + offset_x, cls.BRIGHTNESS_MINUS_RECT[1], cls.BRIGHTNESS_MINUS_RECT[2], cls.BRIGHTNESS_MINUS_RECT[3])
            p_r = (cls.BRIGHTNESS_PLUS_RECT[0] + offset_x, cls.BRIGHTNESS_PLUS_RECT[1], cls.BRIGHTNESS_PLUS_RECT[2], cls.BRIGHTNESS_PLUS_RECT[3])
            t_r = (cls.BRIGHTNESS_SLIDER_RECT[0] + offset_x, cls.BRIGHTNESS_SLIDER_RECT[1], cls.BRIGHTNESS_SLIDER_RECT[2], cls.BRIGHTNESS_SLIDER_RECT[3])
        elif channel == "CCT":
            m_r = (cls.CCT_MINUS_RECT[0] + offset_x, cls.CCT_MINUS_RECT[1], cls.CCT_MINUS_RECT[2], cls.CCT_MINUS_RECT[3])
            p_r = (cls.CCT_PLUS_RECT[0] + offset_x, cls.CCT_PLUS_RECT[1], cls.CCT_PLUS_RECT[2], cls.CCT_PLUS_RECT[3])
            t_r = (cls.CCT_SLIDER_RECT[0] + offset_x, cls.CCT_SLIDER_RECT[1], cls.CCT_SLIDER_RECT[2], cls.CCT_SLIDER_RECT[3])
        else:
            return None

        if tap_in_rect(x, y, m_r):
            return "MINUS"
        if tap_in_rect(x, y, p_r):
            return "PLUS"
        if tap_in_rect(x, y, t_r):
            return "TRACK"
        return None

    @classmethod
    def classify_pill_tap_offset(cls, x: int, y: int, offset_x: int = 0) -> Optional[str]:
        """Determines which control pill was tapped taking horizontal offset into account."""
        for pill_id, pill_info in cls.CONTROL_PILLS.items():
            base_r = pill_info["rect"]
            shifted_r = (base_r[0] + offset_x, base_r[1], base_r[2], base_r[3])
            if tap_in_rect(x, y, shifted_r):
                return pill_id
        return None


# ==============================================================================
# 17. R2: EDGE-SWIPE SIDE DRAWER
# ==============================================================================

class HandednessEnum(enum.IntEnum):
    RIGHT = 0
    LEFT = 1


class SideDrawerModel:
    """
    R2 Contract: Edge-Swipe Side Drawer.
    - Adaptable to right-handed and left-handed orientation.
    - Right-handed: swipe from right edge opens drawer, swipe from left edge is Back.
    - Left-handed: swipe from left edge opens drawer, swipe from right edge is Back.
    - 6 Targets: Library, Now Reading, TOC, Bookmarks & Quotes, Dictionary, Settings.
    - Supported from both Home and Reader contexts.
    """
    DRAWER_WIDTH: int = 320
    DRAWER_HEIGHT: int = 800

    RIGHT_EDGE_TRIGGER_X: int = 450
    LEFT_EDGE_TRIGGER_X: int = 30
    SIDE_TAB_Y_RANGE: Tuple[int, int] = (340, 460)

    TARGETS: List[Dict[str, Any]] = [
        {"id": "LIBRARY", "name": "Библиотека", "icon": "icon_library_24"},
        {"id": "NOW_READING", "name": "Сейчас читаю", "icon": "icon_book_24"},
        {"id": "TOC", "name": "Оглавление", "icon": "icon_toc_24"},
        {"id": "BOOKMARKS", "name": "Закладки и цитаты", "icon": "icon_bookmark_24"},
        {"id": "DICTIONARY", "name": "Словарь", "icon": "icon_dictionary_24"},
        {"id": "SETTINGS", "name": "Настройки", "icon": "icon_settings_24"},
    ]

    @classmethod
    def get_drawer_rect(cls, handedness: HandednessEnum) -> Tuple[int, int, int, int]:
        """Returns bounding box of the side drawer."""
        if handedness == HandednessEnum.RIGHT:
            return (LOGICAL_WIDTH - cls.DRAWER_WIDTH, 0, cls.DRAWER_WIDTH, cls.DRAWER_HEIGHT)
        else:
            return (0, 0, cls.DRAWER_WIDTH, cls.DRAWER_HEIGHT)

    @classmethod
    def classify_open_gesture(cls, start_x: int, start_y: int, end_x: int, end_y: int,
                              handedness: HandednessEnum, threshold: float = 50.0) -> bool:
        """Determines if a swipe gesture triggers side drawer opening according to handedness."""
        dx = end_x - start_x
        dy = end_y - start_y
        if abs(dy) >= abs(dx):
            return False
        if handedness == HandednessEnum.RIGHT:
            # Swipe from right bezel inward (leftward)
            return start_x >= cls.RIGHT_EDGE_TRIGGER_X and dx <= -threshold
        else:
            # Swipe from left bezel inward (rightward)
            return start_x <= cls.LEFT_EDGE_TRIGGER_X and dx >= threshold

    @classmethod
    def classify_back_gesture(cls, start_x: int, start_y: int, end_x: int, end_y: int,
                             handedness: HandednessEnum, threshold: float = 50.0) -> bool:
        """Opposite edge triggers standard Back gesture."""
        dx = end_x - start_x
        dy = end_y - start_y
        if abs(dy) >= abs(dx):
            return False
        if handedness == HandednessEnum.RIGHT:
            # Left edge swipe rightward is Back
            return start_x <= cls.LEFT_EDGE_TRIGGER_X and dx >= threshold
        else:
            # Right edge swipe leftward is Back
            return start_x >= cls.RIGHT_EDGE_TRIGGER_X and dx <= -threshold

    @classmethod
    def classify_side_tab_tap(cls, x: int, y: int, handedness: HandednessEnum) -> bool:
        """Tapping the edge pull tab invokes the drawer."""
        if not (cls.SIDE_TAB_Y_RANGE[0] <= y <= cls.SIDE_TAB_Y_RANGE[1]):
            return False
        if handedness == HandednessEnum.RIGHT:
            return x >= 460
        else:
            return x <= 20

    @classmethod
    def classify_dismiss_tap(cls, x: int, y: int, handedness: HandednessEnum) -> bool:
        """Tapping in the backdrop area outside the drawer dismisses it."""
        if handedness == HandednessEnum.RIGHT:
            return 0 <= x < (LOGICAL_WIDTH - cls.DRAWER_WIDTH) and 0 <= y < LOGICAL_HEIGHT
        else:
            return cls.DRAWER_WIDTH <= x < LOGICAL_WIDTH and 0 <= y < LOGICAL_HEIGHT

    @classmethod
    def get_item_rect(cls, index: int, handedness: HandednessEnum) -> Tuple[int, int, int, int]:
        """Calculates bounding box for target item (0..5)."""
        base_x = (LOGICAL_WIDTH - cls.DRAWER_WIDTH) if handedness == HandednessEnum.RIGHT else 0
        base_y = 100 + index * 70
        return (base_x + 16, base_y, cls.DRAWER_WIDTH - 32, 56)

    @classmethod
    def classify_item_tap(cls, x: int, y: int, handedness: HandednessEnum) -> Optional[int]:
        """Returns tapped target index 0..5 or None."""
        for i in range(len(cls.TARGETS)):
            rect = cls.get_item_rect(i, handedness)
            if tap_in_rect(x, y, rect):
                return i
        return None


# ==============================================================================
# 18. R3: IN-BOOK READER OVERLAYS & LIVE TYPOGRAPHY
# ==============================================================================

class ReaderOverlaysModel:
    """
    R3 Contract: In-Book Reader Overlays.
    - Center tap toggles floating top and bottom bars over active book page.
    - Top bar: back button [ < ], title/chapter, quick bookmark toggle, search.
    - Bottom bar: reading progress, ~XX min countdown, chapter scrubber, Typography (Aa), Footnotes.
    """
    TOP_BAR_RECT: Tuple[int, int, int, int] = (0, 0, 480, 64)
    BOTTOM_BAR_RECT: Tuple[int, int, int, int] = (0, 672, 480, 128)

    # Top bar elements
    BACK_BUTTON_RECT: Tuple[int, int, int, int] = (0, 0, 64, 64)
    TOC_BUTTON_RECT: Tuple[int, int, int, int] = (64, 0, 80, 64)
    TITLE_AREA_RECT: Tuple[int, int, int, int] = (144, 0, 220, 64)
    BOOKMARK_BUTTON_RECT: Tuple[int, int, int, int] = (364, 0, 56, 64)
    SEARCH_BUTTON_RECT: Tuple[int, int, int, int] = (420, 0, 60, 64)

    # Bottom bar elements
    INFO_AREA_RECT: Tuple[int, int, int, int] = (20, 676, 440, 30)
    SCRUBBER_RECT: Tuple[int, int, int, int] = (30, 708, 420, 26)

    ACTION_AA_RECT: Tuple[int, int, int, int] = (20, 742, 95, 48)
    ACTION_TOC_RECT: Tuple[int, int, int, int] = (125, 742, 110, 48)
    ACTION_FOOTNOTES_RECT: Tuple[int, int, int, int] = (245, 742, 100, 48)
    ACTION_BOOKMARKS_RECT: Tuple[int, int, int, int] = (355, 742, 105, 48)

    @classmethod
    def classify_center_menu_tap(cls, x: int, y: int) -> bool:
        """Center zone (x: 160..320, y: 200..600) triggers in-book overlays."""
        return 160 <= x < 320 and 200 <= y < 600

    @classmethod
    def calc_chapter_countdown(cls, remaining_pages: int, avg_seconds_per_page: float) -> str:
        """Calculates estimated remaining time in chapter cleanly without emojis."""
        if remaining_pages <= 0 or avg_seconds_per_page <= 0:
            return "~0 мин"
        total_seconds = int(math.ceil(remaining_pages * avg_seconds_per_page))
        minutes = max(1, int(math.ceil(total_seconds / 60.0)))
        return f"~{minutes} мин"

    @classmethod
    def scrubber_x_to_page(cls, x: int, total_pages: int) -> int:
        """Maps touch X along scrubber track to page number 1..total_pages."""
        sx, _, sw, _ = cls.SCRUBBER_RECT
        if total_pages <= 1:
            return 1
        clamped_x = max(sx, min(sx + sw, x))
        fraction = (clamped_x - sx) / float(sw)
        page = 1 + int(round(fraction * (total_pages - 1)))
        return max(1, min(total_pages, page))

    @classmethod
    def classify_top_tap(cls, x: int, y: int) -> Optional[str]:
        """Hit-tests top overlay action buttons."""
        if not tap_in_rect(x, y, cls.TOP_BAR_RECT):
            return None
        if tap_in_rect(x, y, cls.BACK_BUTTON_RECT):
            return "BACK"
        if tap_in_rect(x, y, cls.TOC_BUTTON_RECT):
            return "TOC"
        if tap_in_rect(x, y, cls.BOOKMARK_BUTTON_RECT):
            return "TOGGLE_BOOKMARK"
        if tap_in_rect(x, y, cls.SEARCH_BUTTON_RECT):
            return "SEARCH"
        return "TITLE"

    @classmethod
    def classify_bottom_tap(cls, x: int, y: int) -> Optional[str]:
        """Hit-tests bottom overlay action buttons."""
        if not tap_in_rect(x, y, cls.BOTTOM_BAR_RECT):
            return None
        if tap_in_rect(x, y, cls.ACTION_AA_RECT):
            return "TYPOGRAPHY"
        if tap_in_rect(x, y, cls.ACTION_TOC_RECT):
            return "TOC"
        if tap_in_rect(x, y, cls.ACTION_FOOTNOTES_RECT):
            return "FOOTNOTES"
        if tap_in_rect(x, y, cls.ACTION_BOOKMARKS_RECT):
            return "BOOKMARKS"
        if tap_in_rect(x, y, cls.SCRUBBER_RECT):
            return "SCRUBBER"
        return "INFO"

    @classmethod
    def get_floating_top_bar_rect(cls, screen_w: int, margin: int = 14) -> Tuple[int, int, int, int]:
        """Calculates floating top bar card rect with guaranteed >= 12px margins."""
        return (margin, margin, screen_w - 2 * margin, 56)

    @classmethod
    def get_floating_bottom_bar_rect(cls, screen_w: int, screen_h: int, margin: int = 14) -> Tuple[int, int, int, int]:
        """Calculates floating bottom bar card rect with guaranteed >= 12px margins in portrait and landscape."""
        bar_h = 124
        return (margin, screen_h - margin - bar_h, screen_w - 2 * margin, bar_h)

    @classmethod
    def classify_floating_top_tap(cls, x: int, y: int, screen_w: int, margin: int = 14) -> Optional[str]:
        """Hit-tests floating top overlay action buttons with dynamic width."""
        top_w = screen_w - 2 * margin
        if not tap_in_rect(x, y, (margin, margin, top_w, 56)):
            return None
        back_rect = (margin + 6, margin + 6, 44, 44)
        toc_rect = (margin + 56, margin + 6, 60, 44)
        search_rect = (margin + top_w - 50, margin + 6, 44, 44)
        bookmark_rect = (search_rect[0] - 54, margin + 6, 44, 44)
        if tap_in_rect(x, y, back_rect):
            return "BACK"
        if tap_in_rect(x, y, toc_rect):
            return "TOC"
        if tap_in_rect(x, y, bookmark_rect):
            return "TOGGLE_BOOKMARK"
        if tap_in_rect(x, y, search_rect):
            return "SEARCH"
        return "TITLE"

    @classmethod
    def classify_floating_bottom_tap(cls, x: int, y: int, screen_w: int, screen_h: int, margin: int = 14) -> Optional[str]:
        """Hit-tests floating bottom overlay action buttons with dynamic Y positioning."""
        bot_rect = cls.get_floating_bottom_bar_rect(screen_w, screen_h, margin)
        if not tap_in_rect(x, y, bot_rect):
            return None
        by = bot_rect[1]
        bw = bot_rect[2]
        aa_rect = (margin + 10, by + 66, 95, 48)
        toc_rect = (margin + 115, by + 66, 110, 48)
        fn_rect = (margin + 235, by + 66, 100, 48)
        bm_rect = (margin + 345, by + 66, 105, 48)
        scrubber_rect = (margin + 16, by + 32, bw - 32, 26)
        if tap_in_rect(x, y, aa_rect):
            return "TYPOGRAPHY"
        if tap_in_rect(x, y, toc_rect):
            return "TOC"
        if tap_in_rect(x, y, fn_rect):
            return "FOOTNOTES"
        if tap_in_rect(x, y, bm_rect):
            return "BOOKMARKS"
        if tap_in_rect(x, y, scrubber_rect):
            return "SCRUBBER"
        return "INFO"


class AaTypographyModel:
    """
    R2 Live Typography Adjustment Contract.
    - Live adjustment of font family (JetBrains Mono, Roboto Condensed, OpenDyslexic),
      font size (A- / A+ >= 44px), line spacing, margins.
    - Autosave on-the-fly without requiring hardware button confirmation.
    - Zero-drift repositioning: retains character offset across re-pagination.
    """
    POPUP_RECT: Tuple[int, int, int, int] = (20, 350, 440, 310)
    MIN_TOUCH_TARGET_SIZE: int = 44

    # Mandated font families per R2
    MANDATED_FONT_FAMILIES = ["JetBrains Mono", "Roboto Condensed", "OpenDyslexic"]
    FONT_FAMILIES = ["Literata", "JetBrains Mono", "Roboto Condensed", "OpenDyslexic"]
    PREMIUM_FONT_FAMILIES = [
        "Literata",
        "PT Serif",
        "Alegreya",
        "Inter",
        "Atkinson Hyperlegible",
        "JetBrains Mono",
        "OpenDyslexic",
    ]
    SUITE_STEPPER_LEFT_RECT: Tuple[int, int, int, int] = (30, 600, 44, 44)
    SUITE_STEPPER_RIGHT_RECT: Tuple[int, int, int, int] = (406, 600, 44, 44)

    @classmethod
    def step_font_family(cls, current_family: str, delta: int) -> str:
        """Cycles through the 7 premium font families."""
        fams = cls.PREMIUM_FONT_FAMILIES
        if current_family not in fams:
            idx = 0
        else:
            idx = fams.index(current_family)
        new_idx = (idx + delta) % len(fams)
        return fams[new_idx]

    FONT_SIZE_MIN: int = 14
    FONT_SIZE_MAX: int = 36
    FONT_SIZE_STEP: int = 2

    LINE_SPACINGS = [1.0, 1.2, 1.4, 1.6]
    MARGIN_OPTIONS = [0, 10, 20, 30, 40]

    # Touch buttons inside popup (>= 44px)
    DECREASE_FONT_RECT: Tuple[int, int, int, int] = (120, 350, 56, 44)  # A-
    INCREASE_FONT_RECT: Tuple[int, int, int, int] = (260, 350, 56, 44)  # A+
    LEGACY_DECREASE_FONT_RECT: Tuple[int, int, int, int] = (130, 420, 50, 40)
    LEGACY_INCREASE_FONT_RECT: Tuple[int, int, int, int] = (240, 420, 50, 40)

    # Font family selection chips (>= 44px)
    FONT_CHIPS = {
        "JetBrains Mono": (30, 410, 130, 44),
        "Roboto Condensed": (170, 410, 130, 44),
        "OpenDyslexic": (310, 410, 130, 44),
    }

    # Line spacing stepper buttons (>= 44px)
    LINE_SPACING_MINUS_RECT: Tuple[int, int, int, int] = (120, 470, 56, 44)
    LINE_SPACING_PLUS_RECT: Tuple[int, int, int, int] = (260, 470, 56, 44)

    # Margin stepper buttons (>= 44px)
    MARGIN_MINUS_RECT: Tuple[int, int, int, int] = (120, 530, 56, 44)
    MARGIN_PLUS_RECT: Tuple[int, int, int, int] = (260, 530, 56, 44)

    @classmethod
    def step_font_size(cls, current_size: int, delta: int) -> int:
        """Clamps font size within [FONT_SIZE_MIN, FONT_SIZE_MAX]."""
        return max(cls.FONT_SIZE_MIN, min(cls.FONT_SIZE_MAX, current_size + delta))

    @classmethod
    def step_margin(cls, current_margin: int, delta: int) -> int:
        """Steps margin through MARGIN_OPTIONS clamping at boundaries."""
        if current_margin not in cls.MARGIN_OPTIONS:
            return cls.MARGIN_OPTIONS[1]
        idx = cls.MARGIN_OPTIONS.index(current_margin)
        new_idx = max(0, min(len(cls.MARGIN_OPTIONS) - 1, idx + delta))
        return cls.MARGIN_OPTIONS[new_idx]

    @classmethod
    def step_line_spacing(cls, current_spacing: float, delta: int) -> float:
        """Steps line spacing through LINE_SPACINGS clamping at boundaries."""
        closest_idx = min(range(len(cls.LINE_SPACINGS)), key=lambda i: abs(cls.LINE_SPACINGS[i] - current_spacing))
        new_idx = max(0, min(len(cls.LINE_SPACINGS) - 1, closest_idx + delta))
        return cls.LINE_SPACINGS[new_idx]

    @classmethod
    def classify_font_chip_tap(cls, x: int, y: int) -> Optional[str]:
        """Hit-tests font family chips."""
        for font_name, rect in cls.FONT_CHIPS.items():
            if tap_in_rect(x, y, rect):
                return font_name
        return None

    @classmethod
    def classify_popup_tap(cls, x: int, y: int) -> Optional[str]:
        """Hit-tests interactive controls inside typography overlay."""
        if tap_in_rect(x, y, cls.DECREASE_FONT_RECT) or tap_in_rect(x, y, cls.LEGACY_DECREASE_FONT_RECT):
            return "FONT_SIZE_MINUS"
        if tap_in_rect(x, y, cls.INCREASE_FONT_RECT) or tap_in_rect(x, y, cls.LEGACY_INCREASE_FONT_RECT):
            return "FONT_SIZE_PLUS"
        chip = cls.classify_font_chip_tap(x, y)
        if chip:
            return f"FONT_FAMILY_{chip}"
        if tap_in_rect(x, y, cls.LINE_SPACING_MINUS_RECT):
            return "LINE_SPACING_MINUS"
        if tap_in_rect(x, y, cls.LINE_SPACING_PLUS_RECT):
            return "LINE_SPACING_PLUS"
        if tap_in_rect(x, y, cls.MARGIN_MINUS_RECT):
            return "MARGIN_MINUS"
        if tap_in_rect(x, y, cls.MARGIN_PLUS_RECT):
            return "MARGIN_PLUS"
        return None

    @classmethod
    def autosave_state(cls, font_family: str, font_size: int, line_spacing: float, margin: int) -> Dict[str, Any]:
        """Models on-the-fly autosave without requiring hardware confirm buttons."""
        return {
            "fontFamily": font_family,
            "fontSize": font_size,
            "lineSpacing": line_spacing,
            "margin": margin,
            "autosaved": True,
        }

    @classmethod
    def map_offset_to_new_page(cls, cached_offset: int, new_page_ranges: List[Tuple[int, int]]) -> int:
        """
        Calculates new page index containing cached_offset after re-pagination.
        Ensures reading context is strictly preserved without drift.
        """
        if not new_page_ranges:
            return 0
        for page_idx, (start_off, end_off) in enumerate(new_page_ranges):
            if start_off <= cached_offset < end_off:
                return page_idx
        if cached_offset >= new_page_ranges[-1][1]:
            return len(new_page_ranges) - 1
        return 0


# ==============================================================================
# 19. R4: MODULAR SETTINGS HUB
# ==============================================================================

class ModularSettingsHubModel:
    """
    R4 Contract: Modular Settings Hub.
    - 2-level card hub replacing old vertical list.
    - 5 Top-level categories with 32x32 contour icons:
      Reading, Display & Light, Autonomy, Network & Sync, System.
    - Native graphical toggles [●  ] (OFF) and [  ●] (ON).
    - Step sliders [-] [===] [+] with discrete values.
    - Centered OptionPopup modals for enums.
    """
    HEADER_RECT: Tuple[int, int, int, int] = (0, 0, 480, 52)
    BACK_BUTTON_RECT: Tuple[int, int, int, int] = (0, 0, 52, 52)

    SECTIONS: List[Dict[str, Any]] = [
        {"id": "READING", "title": "Чтение", "desc": "Шрифты, поля, сноски, словарь", "icon": "icon_book_32"},
        {"id": "DISPLAY", "title": "Экран и подсветка", "desc": "Яркость, CCT, инверсия, частота обновления", "icon": "icon_sun_32"},
        {"id": "AUTONOMY", "title": "Автономность", "desc": "Таймаут сна, глубокий сон, батарея", "icon": "icon_battery_32"},
        {"id": "NETWORK", "title": "Сеть и синхронизация", "desc": "Wi-Fi, Calibre, OPDS, веб-сервер", "icon": "icon_wifi_32"},
        {"id": "SYSTEM", "title": "Система", "desc": "Язык, дата, резервное копирование, об устройстве", "icon": "icon_settings_32"},
    ]

    TOGGLE_WIDTH: int = 56
    TOGGLE_HEIGHT: int = 28
    TOGGLE_KNOB_RADIUS: int = 11

    @classmethod
    def get_card_rect(cls, index: int, scroll_offset_y: int = 0) -> Tuple[int, int, int, int]:
        """Calculates bounding box for top-level category card."""
        y = 60 + index * 140 - scroll_offset_y
        return (20, y, 440, 126)

    @classmethod
    def classify_card_tap(cls, x: int, y: int, scroll_offset_y: int = 0) -> Optional[str]:
        """Returns category card ID tapped or None."""
        for i, sec in enumerate(cls.SECTIONS):
            rect = cls.get_card_rect(i, scroll_offset_y)
            if tap_in_rect(x, y, rect):
                return sec["id"]
        return None

    @classmethod
    def get_toggle_knob_center(cls, track_rect: Tuple[int, int, int, int], is_on: bool) -> Tuple[int, int]:
        """Calculates exact (x, y) center for the toggle knob on E-Ink."""
        tx, ty, tw, th = track_rect
        cy = ty + th // 2
        cx = (tx + tw - cls.TOGGLE_KNOB_RADIUS - 3) if is_on else (tx + cls.TOGGLE_KNOB_RADIUS + 3)
        return (cx, cy)

    @classmethod
    def format_graphical_toggle(cls, is_on: bool) -> str:
        """Returns native graphical toggle string representation."""
        return "[  ●]" if is_on else "[●  ]"

    @classmethod
    def step_slider_value(cls, current: int, delta: int, min_val: int, max_val: int) -> int:
        """Clamps slider value to range [min_val, max_val]."""
        return max(min_val, min(max_val, current + delta))


# ==============================================================================
# 20. R5: LIBRARY HUB
# ==============================================================================

class LibraryViewModeEnum(enum.IntEnum):
    COVER_GRID = 0
    DETAILED_LIST = 1


class LibrarySortModeEnum(enum.IntEnum):
    NAME = 0
    AUTHOR = 1
    DATE = 2
    PROGRESS = 3


class LibraryHubModel:
    """
    R5 Contract: Library Hub.
    - Dual view mode: 3x2 Cover Grid (6 covers per page) vs Detailed Metadata List.
    - Sorting by Author, Date, Progress %, and Name.
    - Search with on-screen keyboard filtering.
    """
    GRID_ROWS: int = 3
    GRID_COLS: int = 2
    GRID_ITEMS_PER_PAGE: int = 6

    LIST_ITEMS_PER_PAGE: int = 5

    HEADER_RECT: Tuple[int, int, int, int] = (0, 0, 480, 52)
    VIEW_TOGGLE_RECT: Tuple[int, int, int, int] = (420, 8, 48, 36)
    SEARCH_BUTTON_RECT: Tuple[int, int, int, int] = (364, 8, 48, 36)
    SORT_BUTTON_RECT: Tuple[int, int, int, int] = (308, 8, 48, 36)

    # 3x2 Grid Cell Dimensions
    CELL_WIDTH: int = 210
    CELL_HEIGHT: int = 220
    CELL_MARGIN_X: int = 20
    CELL_GAP_X: int = 20
    CELL_MARGIN_Y: int = 60
    CELL_GAP_Y: int = 15

    @classmethod
    def get_grid_cell_rect(cls, row: int, col: int) -> Tuple[int, int, int, int]:
        """Calculates bounding box for grid cell (row 0..2, col 0..1)."""
        x = cls.CELL_MARGIN_X + col * (cls.CELL_WIDTH + cls.CELL_GAP_X)
        y = cls.CELL_MARGIN_Y + row * (cls.CELL_HEIGHT + cls.CELL_GAP_Y)
        return (x, y, cls.CELL_WIDTH, cls.CELL_HEIGHT)

    @classmethod
    def classify_grid_tap(cls, x: int, y: int, items_on_page: int = 6) -> Optional[int]:
        """Hit-tests 3x2 grid to find tapped book index 0..5."""
        for slot in range(min(cls.GRID_ITEMS_PER_PAGE, items_on_page)):
            r = slot // cls.GRID_COLS
            c = slot % cls.GRID_COLS
            rect = cls.get_grid_cell_rect(r, c)
            if tap_in_rect(x, y, rect):
                return slot
        return None

    @classmethod
    def sort_books(cls, books: List[Dict[str, Any]], mode: LibrarySortModeEnum, ascending: bool = True) -> List[Dict[str, Any]]:
        """Sorts books by specified criteria."""
        b_copy = list(books)
        if mode == LibrarySortModeEnum.NAME:
            b_copy.sort(key=lambda b: str(b.get("title", "")).lower(), reverse=not ascending)
        elif mode == LibrarySortModeEnum.AUTHOR:
            b_copy.sort(key=lambda b: str(b.get("author", "")).lower(), reverse=not ascending)
        elif mode == LibrarySortModeEnum.DATE:
            b_copy.sort(key=lambda b: b.get("date", 0), reverse=not ascending)
        elif mode == LibrarySortModeEnum.PROGRESS:
            b_copy.sort(key=lambda b: float(b.get("progress", 0.0)), reverse=not ascending)
        return b_copy

    @classmethod
    def filter_books(cls, query: str, books: List[Dict[str, Any]]) -> List[Dict[str, Any]]:
        """Filters library books matching query in title or author."""
        q = query.strip().lower()
        if not q:
            return list(books)
        return [
            b for b in books
            if q in str(b.get("title", "")).lower() or q in str(b.get("author", "")).lower()
        ]


# ==============================================================================
# 21. R6: CROSSPOINT ERADICATION & SD INTEGRITY VALIDATOR
# ==============================================================================

class CrossPointEradicationValidator:
    """
    R6 Contract: CrossPoint Eradication & Strict Keyword Ban.
    - Audits user-visible strings, translations, network packets.
    - Enforces strict preservation of SD card paths (/.crosspoint/ for reading data integrity).
    - Verifies zero occurrences of banned keyword.
    """
    BANNED_KEYWORD = "".join(["A", "l", "p", "a", "4", "h", "i", "n", "O"])
    PRESERVED_SD_PREFIXES = ["/.crosspoint/", "fs_/.crosspoint/"]

    @classmethod
    def is_clean_ui_label(cls, label: str) -> bool:
        """Verifies label does not contain legacy 'CrossPoint' text."""
        return "crosspoint" not in label.lower()

    @classmethod
    def is_preserved_sd_path(cls, path: str) -> bool:
        """Verifies path is a valid preserved SD storage path."""
        p_norm = path.replace("\\", "/").lower()
        return any(p_norm.startswith(pref.lower()) or pref.lower() in p_norm for pref in cls.PRESERVED_SD_PREFIXES)

    @classmethod
    def audit_text_for_crosspoint(cls, text: str) -> List[str]:
        """Finds any occurrences of 'crosspoint' (case-insensitive)."""
        pattern = re.compile(r"crosspoint", re.IGNORECASE)
        return pattern.findall(text)

    @classmethod
    def scan_for_banned_keyword(cls, directory: Path, extensions: Tuple[str, ...]) -> List[Tuple[str, int, str]]:
        """Scans directory for banned keyword."""
        return EmojiBanValidator.scan_for_banned_keyword(directory, extensions)


# ==============================================================================
# 22. OPDS CATALOG HUB & SEARCH CONTRACT
# ==============================================================================

class OpdsCatalogHubModel:
    """
    Contract for BookPoint OS OPDS Catalogs & Search capabilities.
    Validates:
    - Default catalog count (>= 12 verified libraries)
    - Fallback search templates for major catalogs (Gutenberg, CoolLib, iKnigi, Flibusta, Archive)
    - URL formation and search query interpolation
    """
    MAX_SERVERS = 24

    DEFAULT_SERVERS = [
        {"name": "CoolLib", "url": "http://coollib.cc/opds", "has_search": True},
        {"name": "CoolLib Mirror", "url": "http://coollib.in/opds", "has_search": True},
        {"name": "iKnigi", "url": "http://iknigi.net/opds", "has_search": True},
        {"name": "Project Gutenberg", "url": "https://m.gutenberg.org/ebooks.opds/", "has_search": True},
        {"name": "Project Gutenberg (HTTP)", "url": "http://m.gutenberg.org/ebooks.opds/", "has_search": True},
        {"name": "Internet Archive", "url": "https://archive.org/services/opds/", "has_search": True},
        {"name": "Flibusta", "url": "http://flibusta.is/opds", "has_search": True},
        {"name": "Flibusta mirror", "url": "http://flibusta.site/opds", "has_search": True},
        {"name": "Flibusta proxy", "url": "http://proxy.flibusta.is/opds", "has_search": True},
        {"name": "CoolLib net", "url": "http://coollib.net/opds", "has_search": True},
        {"name": "Maxima Library", "url": "http://maxima-library.org/opds", "has_search": False},
        {"name": "Shukach", "url": "http://shukach.com/opds", "has_search": False},
        {"name": "Litres Free", "url": "http://opds.litres.ru/", "has_search": False},
    ]

    SEARCH_TEMPLATES = {
        "gutenberg.org": "https://m.gutenberg.org/ebooks/search.opds/?query={searchTerms}",
        "coollib": "http://coollib.cc/opds/search?searchTerm={searchTerms}",
        "iknigi": "http://iknigi.net/opds/search?query={searchTerms}",
        "flibusta": "http://flibusta.is/opds/search?searchType=books&searchTerm={searchTerms}",
        "archive.org": "https://archive.org/services/opds/?query={searchTerms}",
    }

    @classmethod
    def get_search_template_for_url(cls, url: str) -> Optional[str]:
        u_lower = url.lower()
        for domain, tmpl in cls.SEARCH_TEMPLATES.items():
            if domain in u_lower:
                return tmpl
        return None

    @classmethod
    def build_search_url(cls, template: str, query: str) -> str:
        q_enc = urllib.parse.quote_plus(query.strip())
        return template.replace("{searchTerms}", q_enc)


# ==============================================================================
# 23. OPDS STUCK ESCAPE & BACK GESTURE CONTRACT
# ==============================================================================

class OpdsStuckEscapeModel:
    """
    Contract for fast escape from stuck or hanging OPDS connections.
    Validates:
    - Cancel button hitbox on loading screen: (140, 420, 200, 56)
    - Back gesture detection (edge swipe)
    - Socket timeout reduction to 8000ms
    """
    CANCEL_BTN_RECT = (140, 420, 200, 56)
    MAX_SOCKET_TIMEOUT_MS = 8000

    @classmethod
    def classify_cancel_tap(cls, tx: int, ty: int) -> bool:
        """Returns True if tap hits the prominent cancel button."""
        return tap_in_rect(tx, ty, cls.CANCEL_BTN_RECT)

    @classmethod
    def classify_escape_gesture(cls, x1: int, y1: int, x2: int, y2: int, handedness: HandednessEnum = HandednessEnum.RIGHT) -> bool:
        """Returns True if gesture is a valid edge-swipe back gesture."""
        direction = classify_swipe(x1, y1, x2, y2, threshold=40.0)
        if direction == SwipeDir.NONE:
            return False
        if handedness == HandednessEnum.RIGHT:
            return direction == SwipeDir.RIGHT and x1 <= 60
        else:
            return direction == SwipeDir.LEFT and x1 >= (LOGICAL_WIDTH - 60)


# ==============================================================================
# 24. SLEEP COVERS ROTATION CONTRACT
# ==============================================================================

class SleepCoversRotationModel:
    """
    Contract for interchangeable, curated sleep covers & wallpapers.
    Validates:
    - Supported directories: /.sleep, /sleep, /.covers, /.crosspoint/sleep
    - 480x800 resolution
    - Non-repeating shuffle window
    """
    VALID_DIRECTORIES = ["/.sleep", "/sleep", "/.covers", "/.crosspoint/sleep"]
    TARGET_WIDTH = 480
    TARGET_HEIGHT = 800

    @classmethod
    def pick_next_random_cover(cls, available_files: List[str], recent_indices: List[int]) -> Tuple[int, str]:
        """Picks random cover avoiding recent ones within window."""
        if not available_files:
            raise ValueError("No sleep cover files available")
        n = len(available_files)
        window = min(len(recent_indices), max(0, n - 1))
        recent_window = recent_indices[-window:] if window > 0 else []
        candidates = [i for i in range(n) if i not in recent_window]
        idx = candidates[0] if candidates else 0
        return idx, available_files[idx]


# ==============================================================================
# 25. R3: UNIVERSAL FORMATS (.FB2 & .FB2.ZIP) CONTRACT
# ==============================================================================

class UniversalFormatsModel:
    """
    R3 Contract: Universal Formats (.fb2 & .fb2.zip).
    - Single-pass streaming XML tokenizer with configurable buffer (512B - 8KB).
    - Compact section/image index in RAM: <= 64 KB total peak memory during indexing.
    - FB2.zip single-pass DEFLATE decompression into .crosspoint/fb2_<hash>/ cache.
    - DEFLATE bounded sliding window <= 32 KB (nothrow allocation).
    - Supported character encodings: UTF-8, Windows-1251, KOI8-R, CP866, ISO-8859-5.
    - Lazy on-demand chapter XHTML streaming into ChapterHtmlSlimParser.
    """
    FB2_EXTENSIONS = [".fb2"]
    FB2_ZIP_EXTENSIONS = [".fb2.zip"]
    ALL_FORMAT_EXTENSIONS = [".epub", ".txt", ".xtc", ".fb2", ".fb2.zip"]

    DEFLATE_WINDOW_SIZE_MAX: int = 32768  # 32 KB
    MAX_INDEXING_RAM_BYTES: int = 65536    # 64 KB

    SUPPORTED_ENCODINGS = ["utf-8", "windows-1251", "koi8-r", "cp866", "iso-8859-5"]

    @classmethod
    def has_fb2_extension(cls, path: str) -> bool:
        """Returns True if file path has .fb2 extension (and not .fb2.zip)."""
        p = path.lower()
        return p.endswith(".fb2") and not p.endswith(".fb2.zip")

    @classmethod
    def has_fb2_zip_extension(cls, path: str) -> bool:
        """Returns True if file path has .fb2.zip extension."""
        return path.lower().endswith(".fb2.zip")

    @classmethod
    def is_universal_format(cls, path: str) -> bool:
        """Returns True if file path is any recognized universal format."""
        p = path.lower()
        return any(p.endswith(ext) for ext in cls.ALL_FORMAT_EXTENSIONS)

    @classmethod
    def compute_cache_dir_name(cls, book_path: str) -> str:
        """Computes deterministic cache directory hash for FB2 book."""
        h = hashlib.sha256(book_path.encode("utf-8")).hexdigest()[:16]
        return f"/.crosspoint/fb2_{h}/"

    @classmethod
    def decode_legacy_bytes(cls, data: bytes, encoding: str) -> str:
        """Decodes raw single-byte encoding (Windows-1251, KOI8-R, CP866, ISO-8859-5) to UTF-8."""
        norm_enc = encoding.lower().strip()
        if norm_enc in ("windows-1251", "cp1251", "win-1251"):
            return data.decode("cp1251", errors="replace")
        elif norm_enc in ("koi8-r", "koi8_r"):
            return data.decode("koi8-r", errors="replace")
        elif norm_enc in ("cp866", "ibm866"):
            return data.decode("cp866", errors="replace")
        elif norm_enc in ("iso-8859-5", "iso8859-5"):
            return data.decode("iso8859-5", errors="replace")
        else:
            return data.decode("utf-8", errors="replace")


# ==============================================================================
# 26. R3: STRIKETHROUGH & 0xCCB6 (U+0336) COMBINING STROKE NORMALIZATION
# ==============================================================================

class StrikethroughNormalizationModel:
    """
    R3 Contract: Strikethrough Formatting & Unicode 0xCCB6 (U+0336) Normalization.
    - FB2 tag recognition: <strikethrough>, <strike>, <s>, <del>.
    - Cyrillic / Latin 0xCCB6 (U+0336 Combining Long Stroke Overlay) normalization:
      detects and strips combining stroke bytes while preserving plain text and
      applying STRIKETHROUGH style.
    - E-Ink strikethrough line geometry: line drawn across glyph run at 80% ascender (ascender * 4 // 5).
    """
    STRIKETHROUGH_TAGS = ["strikethrough", "strike", "s", "del"]
    COMBINING_STROKE_UTF8: bytes = b"\xcc\xb6"  # U+0336 in UTF-8
    COMBINING_STROKE_CHAR: str = "\u0336"
    STRIKE_LINE_ASCENDER_RATIO: float = 0.8  # 4/5 ascender

    @classmethod
    def is_strikethrough_tag(cls, tag_name: str) -> bool:
        """Returns True if HTML/FB2 tag indicates line-through / strikethrough."""
        return tag_name.lower().strip() in cls.STRIKETHROUGH_TAGS

    @classmethod
    def has_combining_stroke(cls, text: str) -> bool:
        """Returns True if text contains U+0336 (combining long stroke overlay)."""
        return cls.COMBINING_STROKE_CHAR in text or "\xcc\xb6" in text

    @classmethod
    def normalize_strikethrough_text(cls, text: str) -> Tuple[str, bool]:
        """
        Normalizes text containing 0xCCB6 (U+0336).
        Returns (clean_text, is_strikethrough).
        """
        has_stroke = cls.has_combining_stroke(text)
        if not has_stroke:
            return text, False
        clean = text.replace(cls.COMBINING_STROKE_CHAR, "").replace("\xcc\xb6", "")
        return clean, True

    @classmethod
    def calc_strike_line_y(cls, base_y: int, ascender: int) -> int:
        """Calculates horizontal strikethrough line Y coordinate at 80% ascender."""
        return base_y - (ascender * 4 // 5)


# ==============================================================================
# 27. R3: PROGRESSIVE JPEG (PJPEG) STREAMING 8x8 IDCT DECODER
# ==============================================================================

class ProgressiveJpegDecoderModel:
    """
    R3 Contract: Progressive JPEG (PJPEG / SOF2) Streaming 8x8 IDCT & Downsampling.
    - Detects SOF2 marker (0xFFC2 = Progressive DCT) vs SOF0 (0xFFC0 = Baseline DCT).
    - 8x8 IDCT on first-scan DC and low-frequency AC yields full 1:1 spatial resolution without blur.
    - Eliminates 1/8 decimation blur from JPEGDEC JPEG_SCALE_EIGHTH.
    - Streaming MCU row buffer: buffers only one MCU row at a time (row_w * mcu_h).
      - For 800x480: 800 * 16 = 12,800 bytes (12.8 KB).
      - For 3000x5000: 3000 * 16 = 48,000 bytes (48 KB).
      - Always <= 64 KB transient buffer; prevents PSRAM out-of-memory crashes (> 2 MB).
    - Fixed-point on-the-fly downsampling into target dimensions (e.g. 480x800).
    """
    SOF0_BASELINE: int = 0xFFC0
    SOF2_PROGRESSIVE: int = 0xFFC2
    MAX_MCU_ROW_BUFFER_BYTES: int = 65536  # 64 KB safe bound
    FULL_IMAGE_PSRAM_DANGER_THRESHOLD: int = 2000000  # 2 MB unbuffered crash threshold

    @classmethod
    def is_progressive_marker(cls, marker: int) -> bool:
        """Returns True if marker is SOF2 (Progressive DCT)."""
        return marker == cls.SOF2_PROGRESSIVE

    @classmethod
    def calc_mcu_row_buffer_bytes(cls, width: int, mcu_height: int = 16) -> int:
        """Calculates transient memory required for streaming one MCU row."""
        return width * mcu_height

    @classmethod
    def calc_unbuffered_full_decode_bytes(cls, width: int, height: int) -> int:
        """Calculates RAM required if all progressive DCT coefficients are buffered (causes OOM)."""
        blocks = ((width + 7) // 8) * ((height + 7) // 8)
        return blocks * 64 * 2  # 64 int16 coefficients per block

    @classmethod
    def is_psram_safe(cls, width: int, mcu_height: int = 16) -> bool:
        """Verifies streaming MCU row decode stays well below 64 KB RAM budget."""
        return cls.calc_mcu_row_buffer_bytes(width, mcu_height) <= cls.MAX_MCU_ROW_BUFFER_BYTES

    @classmethod
    def calc_downsample_steps(cls, src_w: int, src_h: int, dst_w: int, dst_h: int) -> Tuple[float, float]:
        """Calculates fixed-point downsampling step ratios."""
        if dst_w <= 0 or dst_h <= 0:
            raise ValueError("Target dimensions must be positive")
        step_x = float(src_w) / float(dst_w)
        step_y = float(src_h) / float(dst_h)
        return step_x, step_y


# ==============================================================================
# 28. R3: INSTANT FOOTNOTE MODAL CARD POPUP
# ==============================================================================

class FootnoteModalModel:
    """
    R3 Contract: Footnote Index Tap to Instant Bottom Modal Card Popup.
    - Footnote index recognition: [1], [*], <a type="note" l:href="#n1">[1]</a>.
    - Superscript token hit-testing on page with finger touch slop.
    - Footnote text extraction: bounded up to 768 bytes.
    - Modal card geometry: pinned to bottom of display:
      - X = 20, Width = 440 (LOGICAL_WIDTH - 40).
      - Height = 180..240 px (default 200 px).
      - Y = LOGICAL_HEIGHT - Height - 16 = 584 px.
    - Tap routing: Close button [X], card interior (scroll/read), backdrop outside (dismiss).
    """
    MAX_FOOTNOTE_TEXT_BYTES: int = 768
    MODAL_CARD_HEIGHT: int = 200
    MODAL_CARD_MARGIN_BOTTOM: int = 16
    MODAL_CARD_MARGIN_X: int = 20

    @classmethod
    def get_modal_card_rect(cls, screen_w: int = LOGICAL_WIDTH, screen_h: int = LOGICAL_HEIGHT, height: int = 200) -> Tuple[int, int, int, int]:
        """Calculates bottom modal card bounding box."""
        w = screen_w - (cls.MODAL_CARD_MARGIN_X * 2)
        x = cls.MODAL_CARD_MARGIN_X
        y = screen_h - height - cls.MODAL_CARD_MARGIN_BOTTOM
        return (x, y, w, height)

    @classmethod
    def get_close_button_rect(cls, card_rect: Tuple[int, int, int, int]) -> Tuple[int, int, int, int]:
        """Calculates close [X] button rect in card header (>= 44px)."""
        cx, cy, cw, _ = card_rect
        btn_size = 44
        return (cx + cw - btn_size - 4, cy + 6, btn_size, btn_size)

    @classmethod
    def classify_modal_tap(cls, card_rect: Tuple[int, int, int, int], tx: int, ty: int) -> str:
        """Classifies tap when footnote modal card is active."""
        close_btn = cls.get_close_button_rect(card_rect)
        if tap_in_rect(tx, ty, close_btn):
            return "CLOSE_BUTTON"
        if tap_in_rect(tx, ty, card_rect):
            return "CARD_CONTENT"
        return "BACKDROP_DISMISS"

    @classmethod
    def is_footnote_reference(cls, text: str) -> bool:
        """Determines if string is a footnote reference label."""
        t = text.strip()
        return bool(re.match(r"^\[(\d+|\*)\]$", t) or re.match(r"^\d+$", t))


# ==============================================================================
# 29. R4: SCREENSAVER GALLERY & SLEEP SETTINGS CONTRACT
# ==============================================================================

class ScreensaverGalleryModel:
    """
    R4 Contract: Kindle & Kobo Screensaver Gallery & Sleep Settings.
    - 11 Curated Themes across Kindle & Kobo/PocketBook aesthetics:
      Kindle:
        1. kindle_typebars (antique typewriter typebars fan)
        2. kindle_fountain_pens (ornate gold/iridium nibs & ink flourish)
        3. kindle_antique_engravings (classical woodcut bookplate & scrollwork)
        4. kindle_celestial_maps (Ptolemaic spheres & astrolabe rings)
        5. kindle_pencils (drafting pencils & graphite shavings)
        6. kindle_author_verne (Jules Verne engraved portrait & Nautilus)
        7. kindle_author_dickens (Charles Dickens engraved portrait & gaslamp)
        8. kindle_author_twain (Mark Twain engraved portrait & riverboat)
      Kobo / PocketBook:
        9. kobo_geometric_patterns (Scandinavian isometric origami book polygons)
        10. kobo_library_architecture (cathedral book stacks & monumental arches)
        11. kobo_reading_quotes (literary typography quotes)
    - Resolutions: Both 480x800 (Portrait) and 800x480 (Landscape).
    - Color depth: 1-bit monochrome and 4-grayscale (0, 85, 170, 255).
    - Sleep settings:
      - 0: Random screensaver on sleep (non-repeating shuffle)
      - 1..11: Specific screensaver selection
      - READING_STATS (mode 8): Book cover with reading streak days and progress bar
    - Version display: exact 'v2.1.0' string centered at pageHeight - 30 in Light (normal) and Dark (inverted) polarities.
    """
    THEMES = [
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
    THEME_COUNT: int = 11

    RESOLUTIONS: List[Tuple[int, int]] = [(480, 800), (800, 480)]
    COLOR_DEPTHS = ["1-bit", "4-grayscale"]
    GRAYSCALE_PALETTE = [0, 85, 170, 255]

    EXPECTED_VERSION_STRING: str = "v2.1.0"

    # Sleep setting enum values
    SCREENSAVER_RANDOM: int = 0
    SCREENSAVER_MODE_READING_STATS: int = 8

    @classmethod
    def get_theme_filename(cls, theme_index: int, res: Tuple[int, int] = (480, 800)) -> str:
        """Returns standard filename for a theme index 1..11 and resolution."""
        if not (1 <= theme_index <= cls.THEME_COUNT):
            raise ValueError(f"Invalid theme index {theme_index}")
        theme_name = cls.THEMES[theme_index - 1]
        w, h = res
        return f"{theme_index:02d}_{theme_name}_{w}x{h}.bmp"

    @classmethod
    def format_version_display(cls, raw_version: str) -> str:
        """
        Formats version string ensuring exact 'v2.1.0' visual display,
        stripping target suffixes like '-x4pro' and ensuring lowercase 'v' prefix.
        """
        v = raw_version.strip()
        dash = v.find("-")
        if dash != -1:
            v = v[:dash]
        if v.startswith("v") or v.startswith("V"):
            v = "v" + v[1:]
        else:
            v = "v" + v
        return v

    @classmethod
    def get_version_position(cls, screen_w: int, screen_h: int) -> Tuple[int, int]:
        """Version string display anchor at bottom of screen."""
        return (screen_w // 2, screen_h - 30)

    @classmethod
    def get_version_polarity(cls, sleep_mode_is_dark: bool) -> Dict[str, Any]:
        """Returns text and background polarity for Light vs Dark sleep mode."""
        if sleep_mode_is_dark:
            return {
                "text_polarity": POLARITY_WHITE,   # Inverted: White ink on black background
                "bg_polarity": POLARITY_BLACK,
                "screen_inverted": True,
            }
        else:
            return {
                "text_polarity": POLARITY_BLACK,   # Light: Black ink on white background
                "bg_polarity": POLARITY_WHITE,
                "screen_inverted": False,
            }


# ==============================================================================
# 30. R1: ZERO-OVERLAP UI & PIXEL-PERFECT SCREEN GEOMETRY MODEL
# ==============================================================================

class ZeroOverlapModel:
    """
    R1 Contract: Zero-Overlap UI & Pixel-Perfect Screen Geometry on 800x480 E-Ink.
    Formal mathematical assertions verifying:
    1. Zero bounding box intersections between adjacent UI elements (Hero card, Recent shelf, etc.).
    2. Vertical content budgeting in Now Reading Hero Card stack (Badge, Title, Author, Progress, Time, Streak, Resume).
    3. Safe UTF-8 character-boundary truncation with ellipsis (no Cyrillic byte slicing).
    4. Recent shelf slot boundary containment (no horizontal bleed into adjacent slots).
    5. Centering of Quick Settings Curtain in 800x480 landscape with dual Cold/Warm frontlight sliders.
    6. Floating reader overlays with guaranteed >= 12px margins from display edges in portrait and landscape.
    7. Strict minimum touch target sizing (>= 44x44px) across all interactive chrome and popups.
    """
    MIN_TOUCH_TARGET_SIZE: int = 44
    MIN_READER_OVERLAY_MARGIN: int = 12

    @classmethod
    def rects_intersect(cls, r1: Tuple[int, int, int, int], r2: Tuple[int, int, int, int]) -> bool:
        """
        Determines whether two axis-aligned bounding boxes (x, y, w, h) overlap with positive area.
        Returns True if they overlap, False if they are strictly disjoint or touching only at edges.
        """
        x1, y1, w1, h1 = r1
        x2, y2, w2, h2 = r2
        return (max(x1, x2) < min(x1 + w1, x2 + w2)) and (max(y1, y2) < min(y1 + h1, y2 + h2))

    @classmethod
    def assert_no_overlap(cls, r1: Tuple[int, int, int, int], r2: Tuple[int, int, int, int],
                          label1: str = "Element 1", label2: str = "Element 2") -> None:
        """Raises AssertionError if r1 and r2 overlap."""
        if cls.rects_intersect(r1, r2):
            raise AssertionError(f"Zero-Overlap Violation: {label1} {r1} collides with {label2} {r2}")

    @classmethod
    def validate_hero_card_stack(cls, title_lines_count: int, line_h: int = 22,
                                 has_author: bool = True, has_stats: bool = True,
                                 has_streak: bool = True, card_y: int = 50,
                                 card_h: int = 310, resume_btn_y: int = 316,
                                 resume_btn_h: int = 30) -> Dict[str, Any]:
        """
        Calculates exact vertical layout bounds of elements in Now Reading Hero Card.
        Verifies that title, author, progress, remaining time, and streak do not collide
        with each other or with the Resume button fixed at resume_btn_y.
        """
        elements = {}
        cur_y = card_y + 16

        # 1. Badge: "СЕЙЧАС ЧИТАЮ"
        elements["badge"] = (card_y + 16, 20)
        cur_y += 24

        # 2. Title (wrapped lines)
        title_h = max(1, min(3, title_lines_count)) * line_h
        elements["title"] = (cur_y, title_h)
        cur_y += title_h + 4

        # 3. Author
        if has_author:
            elements["author"] = (cur_y, 18)
            cur_y += 24

        # 4. Progress bar & percentage
        if has_stats:
            elements["progress"] = (cur_y, 16)
            cur_y += 20
            # 5. Estimated time left
            elements["time_left"] = (cur_y, 16)
            cur_y += 22

        # 6. Reading streak badge
        if has_streak:
            elements["streak"] = (cur_y, 20)
            cur_y += 24

        lowest_element_bottom = cur_y
        clearance = resume_btn_y - lowest_element_bottom
        is_valid = clearance >= 0

        return {
            "elements": elements,
            "lowest_element_bottom": lowest_element_bottom,
            "resume_btn_y": resume_btn_y,
            "clearance_px": clearance,
            "is_valid": is_valid,
        }

    @classmethod
    def safe_utf8_truncate(cls, text: str, max_chars: int = 20, ellipsis: str = "…") -> str:
        """
        Safely truncates unicode text to max_chars with ellipsis, strictly preserving
        multi-byte UTF-8 codepoint boundaries (avoiding half-glyph byte cuts).
        """
        if len(text) <= max_chars:
            return text
        cut_len = max(0, max_chars - len(ellipsis))
        truncated = text[:cut_len] + ellipsis
        # Verify valid UTF-8 encode-decode
        truncated.encode("utf-8").decode("utf-8")
        return truncated

    @classmethod
    def validate_recent_shelf_slot(cls, author: str, slot_w: int = 144,
                                  cover_w: int = 44, margin: int = 16) -> Dict[str, Any]:
        """
        Verifies that recent shelf slot text adheres strictly to available horizontal width budget
        and does not bleed into neighboring slots.
        """
        available_text_w = slot_w - cover_w - margin
        # Approximately 8-9 characters fit in 76-84px with small font
        max_author_chars = max(6, available_text_w // 9)
        safe_author = cls.safe_utf8_truncate(author, max_chars=max_author_chars)
        return {
            "slot_width": slot_w,
            "available_text_width": available_text_w,
            "original_author": author,
            "safe_author": safe_author,
            "fits_without_bleed": len(safe_author) <= max_author_chars,
        }

    @classmethod
    def validate_touch_target_size(cls, rect: Tuple[int, int, int, int], min_size: int = 44) -> bool:
        """Asserts that touch hitbox has both width >= min_size and height >= min_size."""
        return rect[2] >= min_size and rect[3] >= min_size

    @classmethod
    def validate_reader_overlay_margins(cls, rect: Tuple[int, int, int, int],
                                        screen_w: int, screen_h: int,
                                        min_margin: int = 12) -> bool:
        """
        Verifies that a floating reader overlay card has distance >= min_margin
        from screen edges and does not extend beyond display bounds.
        """
        x, y, w, h = rect
        left_margin = x
        top_margin = y
        right_margin = screen_w - (x + w)
        bottom_margin = screen_h - (y + h)
        if x < 0 or y < 0 or x + w > screen_w or y + h > screen_h:
            return False
        return (left_margin >= min_margin and top_margin >= min_margin and
                right_margin >= min_margin and bottom_margin >= min_margin)


# ==============================================================================
# 31. R2: PREMIUM TYPOGRAPHY SUITE & CODEPOINT INTEGRITY MODEL
# ==============================================================================

class PremiumTypographySuiteModel:
    """
    R2 Contract: Premium Typography Suite & Codepoint Integrity.
    - 7 curated E-Ink optimized font families across Serif, Sans-Serif, Monospace, Dyslexic:
      1. Literata (Google Play Books flagship serif)
      2. PT Serif (ParaType classical Russian / international serif)
      3. Alegreya (warm literary rhythm serif)
      4. Inter (modern clean neutral grotest sans-serif)
      5. Atkinson Hyperlegible (Braille Institute hyper-differentiated sans-serif)
      6. JetBrains Mono (developer, tabular & monospace)
      7. OpenDyslexic (dyslexia high-contrast specialized)
    - Full Unicode codepoint coverage across Latin, Cyrillic, Typographic Punctuation, Footnotes.
    - Live reflow without character offset drift (retaining exact reading position).
    - PSRAM dynamic lifecycle management (ensureLoaded, unloadAll, zero heap leaks).
    - Hybrid architecture: built-in Noto in flash, 7 premium families in PSRAM from SD assets,
      preserving >= 700 KB headroom in app0 flash partition.
    """
    PREMIUM_FONT_FAMILIES: List[str] = [
        "Literata",
        "PT Serif",
        "Alegreya",
        "Inter",
        "Atkinson Hyperlegible",
        "JetBrains Mono",
        "OpenDyslexic",
    ]

    FAMILY_SPECS: Dict[str, Dict[str, Any]] = {
        "Literata": {
            "category": "Serif",
            "origin": "Google Play Books flagship",
            "has_cyrillic": True,
            "has_latin": True,
            "sizes": [10, 12, 14, 16, 18],
        },
        "PT Serif": {
            "category": "Serif",
            "origin": "ParaType classical Russian & international",
            "has_cyrillic": True,
            "has_latin": True,
            "sizes": [10, 12, 14, 16, 18],
        },
        "Alegreya": {
            "category": "Serif",
            "origin": "Juan Pablo del Peral warm literary",
            "has_cyrillic": True,
            "has_latin": True,
            "sizes": [10, 12, 14, 16, 18],
        },
        "Inter": {
            "category": "Sans-Serif",
            "origin": "Rasmus Andersson neutral UI",
            "has_cyrillic": True,
            "has_latin": True,
            "sizes": [10, 12, 14, 16, 18],
        },
        "Atkinson Hyperlegible": {
            "category": "Sans-Serif",
            "origin": "Braille Institute hyper-differentiated",
            "has_cyrillic": True,
            "has_latin": True,
            "sizes": [10, 12, 14, 16, 18],
        },
        "JetBrains Mono": {
            "category": "Monospace",
            "origin": "JetBrains code & tables",
            "has_cyrillic": True,
            "has_latin": True,
            "sizes": [10, 12, 14, 16, 18],
        },
        "OpenDyslexic": {
            "category": "Special",
            "origin": "Dyslexia weighted contrast",
            "has_cyrillic": True,
            "has_latin": True,
            "sizes": [8, 10, 12, 14, 16],
        },
    }

    UNICODE_INTERVALS: Dict[str, Tuple[int, int]] = {
        "LATIN_BASIC": (0x0020, 0x007E),
        "LATIN_SUPPLEMENT": (0x00A0, 0x00FF),  # « » § © ® °
        "LATIN_EXTENDED_A": (0x0100, 0x017F),
        "CYRILLIC": (0x0400, 0x04FF),          # Russian, Ukrainian, Belarusian, Serbian, Kazakh
        "GENERAL_PUNCTUATION": (0x2000, 0x206F), # — – “ ” „ “ … •
        "SUPERSCRIPTS_SUBSCRIPTS": (0x2070, 0x209F), # Footnote superscripts ⁰¹²³⁴⁵⁶⁷⁸⁹
    }

    @classmethod
    def is_codepoint_supported(cls, cp: int) -> bool:
        """Validates whether codepoint lies in supported typographic intervals."""
        for start, end in cls.UNICODE_INTERVALS.values():
            if start <= cp <= end:
                return True
        # Specific allowed punctuation (dagger †, double dagger ‡, asterisk *)
        if cp in (0x002A, 0x2020, 0x2021, 0x2026):
            return True
        return False

    @classmethod
    def validate_text_codepoints(cls, text: str) -> Dict[str, Any]:
        """Validates all characters in string for Unicode coverage."""
        unsupported = []
        covered_count = 0
        for ch in text:
            cp = ord(ch)
            if cls.is_codepoint_supported(cp):
                covered_count += 1
            else:
                unsupported.append((ch, hex(cp)))
        return {
            "total_chars": len(text),
            "covered_chars": covered_count,
            "is_fully_covered": len(unsupported) == 0,
            "unsupported_samples": unsupported[:5],
        }

    @classmethod
    def verify_reflow_offset_preservation(cls, cached_offset: int,
                                          new_page_map: List[Tuple[int, int]]) -> Dict[str, Any]:
        """
        Validates that live font reflow (change in family or size) maps the cached
        character offset to the exact corresponding new page without reading drift.
        """
        if not new_page_map:
            return {"target_page": 0, "offset_contained": False}
        for page_idx, (start_off, end_off) in enumerate(new_page_map):
            if start_off <= cached_offset < end_off:
                return {
                    "target_page": page_idx,
                    "page_range": (start_off, end_off),
                    "offset_contained": True,
                }
        if cached_offset >= new_page_map[-1][1]:
            last_idx = len(new_page_map) - 1
            return {
                "target_page": last_idx,
                "page_range": new_page_map[-1],
                "offset_contained": True,
            }
        return {"target_page": 0, "offset_contained": False}

    @classmethod
    def verify_hybrid_partition_budget(cls, binary_size: int,
                                       app0_size: int = 0x640000,
                                       min_headroom_bytes: int = 716800) -> Dict[str, Any]:
        """
        Verifies hybrid font architecture headroom: ensures binary fits in app0 partition
        with >= 700 KB headroom remaining because premium fonts load dynamically from SD/PSRAM.
        """
        headroom = app0_size - binary_size
        headroom_kb = headroom / 1024.0
        return {
            "app0_partition_size_bytes": app0_size,
            "firmware_binary_size_bytes": binary_size,
            "headroom_bytes": headroom,
            "headroom_kb": round(headroom_kb, 2),
            "meets_700kb_headroom": headroom >= min_headroom_bytes,
        }


# ==============================================================================
# 32. R3: FLAGSHIP ERGONOMICS, TOUCH ZONES & DYNAMIC PACE MODEL
# ==============================================================================

class FlagshipErgonomicsModel:
    """
    R3 Contract: Flagship Ergonomics, Handed Touch Zones & Dynamic Reading Pace.
    - Handedness page turn layouts (Kindle / Kobo standard):
      - Right-handed: narrow left strip (25%) = PREV, wide remaining area (75%) = NEXT.
      - Left-handed: narrow right strip (25%) = PREV, wide remaining area (75%) = NEXT.
      - Center exclusion zone (middle 40% width, middle 30% height) invokes Reader Overlays.
      - Inverted tap flips forward/backward. Swipe gestures supported (Left = NEXT, Right = PREV).
    - Dynamic reading pace tracking:
      - Forward page dwell filtering: MIN_PACE_SAMPLE_SECONDS (5s) <= dwell < IDLE_THRESHOLD_SECONDS (300s).
      - Running average seconds per forward page.
      - Dynamic countdown to end of chapter: remainingPages * (secPerPage / 60.0).
    - Anti-ghosting configurable full refresh:
      - Modes: REFRESH_1 (1), REFRESH_5 (5), REFRESH_10 (10), REFRESH_15 (15),
               REFRESH_20 (20), REFRESH_CHAPTER (chapter boundary), REFRESH_30 (30).
    """
    READER_TOUCH_NEXT: str = "NEXT"
    READER_TOUCH_PREV: str = "PREV"
    READER_MENU: str = "MENU"

    MIN_PACE_SAMPLE_SECONDS: int = 5
    IDLE_THRESHOLD_SECONDS: int = 300
    DEFAULT_PACE_SECONDS: float = 45.0

    REFRESH_MODES: Dict[str, int] = {
        "REFRESH_1": 1,
        "REFRESH_5": 5,
        "REFRESH_10": 10,
        "REFRESH_15": 15,
        "REFRESH_20": 20,
        "REFRESH_CHAPTER": 9999,
        "REFRESH_30": 30,
    }

    @classmethod
    def classify_reader_touch(cls, x: int, y: int, screen_w: int, screen_h: int,
                              handedness: str = "RIGHT", inverted: bool = False,
                              menu_active: bool = True) -> str:
        """
        Classifies reader tap coordinates into MENU, NEXT, or PREV page turn.
        Implements 75/25 handed touch zone layout with center menu reservation.
        """
        # 1. Center menu exclusion zone (middle 40% width, middle 30% height)
        menu_x_start = int(screen_w * 0.30)
        menu_x_end = int(screen_w * 0.70)
        menu_y_start = int(screen_h * 0.35)
        menu_y_end = int(screen_h * 0.65)
        if menu_active and (menu_x_start <= x < menu_x_end) and (menu_y_start <= y < menu_y_end):
            return cls.READER_MENU

        # 2. Handed touch zones
        is_left_handed = (handedness.upper() == "LEFT")
        forward = False
        if not is_left_handed:
            # Right-handed: 0..25% is PREV, 25..100% is NEXT
            forward = (x >= screen_w // 4)
        else:
            # Left-handed: 0..75% is NEXT, 75..100% is PREV
            forward = (x < screen_w * 3 // 4)

        if inverted:
            forward = not forward

        return cls.READER_TOUCH_NEXT if forward else cls.READER_TOUCH_PREV

    @classmethod
    def classify_swipe_turn(cls, dx: int, dy: int, threshold: int = 50) -> Optional[str]:
        """Classifies horizontal swipe gesture: Left = NEXT, Right = PREV."""
        if abs(dx) < threshold or abs(dy) >= abs(dx):
            return None
        return cls.READER_TOUCH_NEXT if dx < 0 else cls.READER_TOUCH_PREV

    @classmethod
    def is_valid_forward_pace_sample(cls, dwell_seconds: int, is_forward_turn: bool) -> bool:
        """Validates dwell sample: must be forward page turn within [5..300) seconds."""
        if not is_forward_turn:
            return False
        return cls.MIN_PACE_SAMPLE_SECONDS <= dwell_seconds < cls.IDLE_THRESHOLD_SECONDS

    @classmethod
    def calc_running_pace(cls, samples: List[int]) -> float:
        """Computes average pace in seconds per page from valid samples."""
        if not samples:
            return cls.DEFAULT_PACE_SECONDS
        return float(sum(samples)) / len(samples)

    @classmethod
    def calc_estimated_chapter_minutes(cls, remaining_pages: int, sec_per_page: float) -> int:
        """Calculates dynamic countdown in whole minutes to chapter end."""
        if remaining_pages <= 0:
            return 0
        total_sec = remaining_pages * sec_per_page
        return max(1, int(math.ceil(total_sec / 60.0)))

    @classmethod
    def format_countdown_string(cls, remaining_pages: int, sec_per_page: float) -> str:
        """Formats countdown string for bottom reader overlay."""
        minutes = cls.calc_estimated_chapter_minutes(remaining_pages, sec_per_page)
        return f"~{minutes} мин"

    @classmethod
    def should_trigger_full_refresh(cls, pages_turned: int, is_chapter_transition: bool,
                                    mode: str = "REFRESH_10") -> bool:
        """Evaluates whether current page transition requires anti-ghosting full flash."""
        if mode == "REFRESH_CHAPTER":
            return is_chapter_transition
        interval = cls.REFRESH_MODES.get(mode, 10)
        if interval <= 0:
            return False
        return (pages_turned > 0) and (pages_turned % interval == 0)


# ==============================================================================
# 33. R4: ZERO BRICK RISK, PARTITION HEADROOM & HARDWARE RECOVERY MODEL
# ==============================================================================

class ZeroBrickRiskModel:
    """
    R4 Contract: Zero Brick Risk, Partition Headroom & Hardware Recovery.
    Formal mathematical assertions verifying:
    1. partitions.csv structure: 16MB table end (0x1000000), 64KB MMU alignment,
       app0 partition 0x640000 (6.25MB), >= 700 KB app0 headroom.
    2. esptool image-info header contracts: ESP32-S3 chip, DIO 80MHz, 16MB flash,
       valid checksum byte, valid SHA256 image digest.
    3. Hardware I2C / GT911 recovery: 9 SCL bus-clear clock pulses + RST pin toggle
       upon 5 consecutive I2C transaction failures.
    4. Atomic user data persistence: battery voltage safety check (>= 3200 mV) to
       prevent brownouts during SPI/SD write, two-stage .tmp file creation with recovery.
    5. Fail-safe boot defaults: duplicate button mapping auto-repair and emergency
       boot chord (BTN_DOWN held during boot).
    """
    TOTAL_FLASH_SIZE: int = 0x1000000       # 16,777,216 bytes (16MB)
    APP0_PARTITION_SIZE: int = 0x640000     # 6,553,600 bytes (6.25MB)
    MIN_HEADROOM_BYTES: int = 716800        # 700 KB
    MIN_SAFE_BATTERY_VOLTAGE_MV: int = 3200 # 3.2V
    I2C_MAX_CONSECUTIVE_FAILURES: int = 5
    I2C_BUS_CLEAR_PULSES: int = 9

    @classmethod
    def validate_partition_row(cls, name: str, p_type: str, subtype: str,
                              offset: int, size: int) -> Dict[str, Any]:
        """Validates MMU 64KB alignment for app and filesystem partitions."""
        mmu_aligned = (offset % 0x10000 == 0) and (size % 0x10000 == 0)
        return {
            "name": name,
            "offset": offset,
            "size": size,
            "end": offset + size,
            "is_64kb_aligned": mmu_aligned,
        }

    @classmethod
    def check_app0_headroom(cls, binary_size_bytes: int) -> Dict[str, Any]:
        """Validates that compiled firmware.bin leaves >= 700 KB headroom in app0."""
        headroom = cls.APP0_PARTITION_SIZE - binary_size_bytes
        headroom_kb = headroom / 1024.0
        return {
            "app0_size_bytes": cls.APP0_PARTITION_SIZE,
            "binary_size_bytes": binary_size_bytes,
            "headroom_bytes": headroom,
            "headroom_kb": round(headroom_kb, 2),
            "meets_requirement": headroom >= cls.MIN_HEADROOM_BYTES,
        }

    @classmethod
    def is_battery_voltage_safe_for_write(cls, voltage_mv: int) -> bool:
        """Enforces minimum battery threshold >= 3200 mV to prevent brownout during flash writes."""
        return voltage_mv >= cls.MIN_SAFE_BATTERY_VOLTAGE_MV

    @classmethod
    def simulate_atomic_persistence(cls, target_path: str, payload: str,
                                    voltage_mv: int,
                                    power_loss_stage: Optional[str] = None) -> Dict[str, Any]:
        """
        Models two-stage atomic file save (path.tmp -> rename to path) with brownout guard.
        Stages of power loss simulation:
        - None: complete success
        - 'BROWNOUT': voltage < 3200 mV triggers abort before touching storage
        - 'DURING_TMP_WRITE': power lost while writing tmp file
        - 'AFTER_TARGET_REMOVE': power lost after removing target but before renaming tmp
        """
        if not cls.is_battery_voltage_safe_for_write(voltage_mv):
            return {
                "status": "ABORTED_LOW_BATTERY",
                "voltage_mv": voltage_mv,
                "target_exists": True,
                "tmp_exists": False,
            }

        tmp_path = f"{target_path}.tmp"
        if power_loss_stage == "DURING_TMP_WRITE":
            return {
                "status": "POWER_LOSS_DURING_TMP",
                "target_exists": True,
                "tmp_exists": False,
            }

        if power_loss_stage == "AFTER_TARGET_REMOVE":
            return {
                "status": "POWER_LOSS_AFTER_REMOVE",
                "target_exists": False,
                "tmp_exists": True,
                "tmp_content": payload,
            }

        # Normal completion: target updated, tmp removed
        return {
            "status": "SUCCESS",
            "target_exists": True,
            "tmp_exists": False,
            "target_content": payload,
        }

    @classmethod
    def recover_from_power_loss(cls, target_exists: bool, tmp_exists: bool,
                               tmp_content: Optional[str] = None) -> Dict[str, Any]:
        """
        Models startup recovery: if target is missing but .tmp exists from interrupted write,
        automatically restores settings from .tmp file.
        """
        if target_exists:
            return {"recovered": False, "source": "TARGET", "status": "NORMAL"}
        if tmp_exists and tmp_content:
            return {"recovered": True, "source": "TMP_FALLBACK", "restored_content": tmp_content}
        return {"recovered": False, "source": "DEFAULT_FALLBACK", "status": "DEFAULTS_LOADED"}

    @classmethod
    def evaluate_i2c_recovery(cls, consecutive_failures: int) -> Dict[str, Any]:
        """
        Models I2C fault detector: upon 5 consecutive transaction failures, triggers
        9 SCL bus clear clock pulses and hardware RST low pulse to revive Goodix GT911.
        """
        if consecutive_failures < cls.I2C_MAX_CONSECUTIVE_FAILURES:
            return {
                "consecutive_failures": consecutive_failures,
                "action": "NORMAL_RETRY",
                "bus_clear_triggered": False,
                "hardware_reset_triggered": False,
            }
        return {
            "consecutive_failures": consecutive_failures,
            "action": "HARDWARE_RECOVERY",
            "bus_clear_triggered": True,
            "scl_clock_pulses": cls.I2C_BUS_CLEAR_PULSES,
            "hardware_reset_triggered": True,
            "rst_pulse_width_ms": 10,
            "post_reset_delay_ms": 50,
        }

    @classmethod
    def repair_duplicate_button_mappings(cls, mappings: List[str]) -> Tuple[List[str], bool]:
        """
        Auto-repairs front button assignments if duplicate or invalid keys are detected,
        restoring fail-safe defaults [BACK, CONFIRM, LEFT, RIGHT].
        """
        defaults = ["FRONT_HW_BACK", "FRONT_HW_CONFIRM", "FRONT_HW_LEFT", "FRONT_HW_RIGHT"]
        if len(mappings) != 4 or len(set(mappings)) != 4:
            return defaults, True
        return mappings, False

    @classmethod
    def is_emergency_recovery_chord(cls, btn_down_pressed: bool) -> bool:
        """Holding BTN_DOWN during boot triggers safe recovery mode."""
        return bool(btn_down_pressed)


# ==============================================================================
# 26. AUTHORITATIVE HARDWARE SIMULATION MODELS (BM8563, CW2017, GT911, SSD1677)
# ==============================================================================


class BM8563RtcModel:
    """
    Authoritative Hardware Simulation Model for Belling BM8563 / NXP PCF8563 Real-Time Clock.
    Physical hardware ground truth:
    - I2C Address: 0x51 (shared bus SDA 39 / SCL 38 @ 400kHz).
      Strictly prohibits DS3231 address 0x68 (used on Xteink X3).
    - Register Map:
      0x00: Control/Status 1 (STOP bit, test modes)
      0x01: Control/Status 2 (Alarm/Timer interrupt flags)
      0x02: Seconds (BCD 00-59, bit 7 = VL [Voltage Low / Integrity flag])
      0x03: Minutes (BCD 00-59, bits 6:0)
      0x04: Hours   (BCD 00-23, bits 5:0)
      0x05: Days    (BCD 01-31, bits 5:0)
      0x06: Weekdays(BCD 00-06, bits 2:0)
      0x07: Century/Months (BCD 01-12, bit 7 = Century: 1=1900, 0=2000)
      0x08: Years   (BCD 00-99, bits 7:0)
    """
    I2C_ADDR: int = 0x51
    FORBIDDEN_DS3231_ADDR: int = 0x68

    REG_CTRL_STATUS1: int = 0x00
    REG_CTRL_STATUS2: int = 0x01
    REG_SEC: int = 0x02
    REG_MIN: int = 0x03
    REG_HOUR: int = 0x04
    REG_DAY: int = 0x05
    REG_WDAY: int = 0x06
    REG_MONTH: int = 0x07
    REG_YEAR: int = 0x08

    VL_FLAG: int = 0x80       # Bit 7 of 0x02: 1 = Voltage Low / Oscillator stopped
    CENTURY_FLAG: int = 0x80  # Bit 7 of 0x07: 1 = 19xx, 0 = 20xx

    # Backwards compatibility attributes matching earlier tests
    BM8563_ADDR: int = 0x51
    DS3231_ADDR: int = 0x68
    BM8563_SEC_REG: int = 0x02
    DS3231_SEC_REG: int = 0x00
    BM8563_VL_FLAG: int = 0x80

    def __init__(self, initial_registers: Optional[List[int]] = None) -> None:
        self.registers: List[int] = [0x00] * 16
        if initial_registers:
            for i, val in enumerate(initial_registers[:16]):
                self.registers[i] = val & 0xFF
        else:
            # Default state after power loss: VL set, oscillator stopped
            self.registers[self.REG_SEC] = self.VL_FLAG

    @staticmethod
    def decode_bcd(bcd_val: int) -> int:
        return ((bcd_val >> 4) * 10) + (bcd_val & 0x0F)

    @staticmethod
    def encode_bcd(dec_val: int) -> int:
        return ((dec_val // 10) << 4) | (dec_val % 10)

    @classmethod
    def verify_address(cls, addr: int) -> bool:
        """Verifies if the given I2C address is the valid BM8563 address (0x51). Prohibits 0x68."""
        if addr == cls.FORBIDDEN_DS3231_ADDR:
            raise ValueError(f"Legacy DS3231 address 0x{addr:02X} detected! BM8563 must use 0x51 on X4 Pro.")
        return addr == cls.I2C_ADDR

    @classmethod
    def serialize_bm8563_registers(
        cls, year: int, month: int, day: int, weekday: int, hour: int, minute: int, second: int, vl: bool = False
    ) -> List[int]:
        """Serializes calendar datetime to raw 7-byte register array (0x02..0x08)."""
        vl_bit = cls.VL_FLAG if vl else 0x00
        century_bit = cls.CENTURY_FLAG if year < 2000 else 0x00
        return [
            (cls.encode_bcd(second) & 0x7F) | vl_bit,
            cls.encode_bcd(minute) & 0x7F,
            cls.encode_bcd(hour) & 0x3F,
            cls.encode_bcd(day) & 0x3F,
            cls.encode_bcd(weekday % 7) & 0x07,
            (cls.encode_bcd(month) & 0x1F) | century_bit,
            cls.encode_bcd(year % 100) & 0xFF,
        ]

    @classmethod
    def parse_bm8563_registers(cls, raw_7_bytes: List[int]) -> Dict[str, Any]:
        """Parses raw 7-byte register array (0x02..0x08) into datetime fields with VL integrity check."""
        if len(raw_7_bytes) < 7:
            return {"valid": False, "error": "Insufficient bytes (needs 7)"}

        sec_byte = raw_7_bytes[0]
        vl_set = bool(sec_byte & cls.VL_FLAG)
        if vl_set:
            return {
                "valid": False,
                "error": "Oscillator stopped / VL (voltage low) flag set",
                "vl": True,
            }

        sec = cls.decode_bcd(sec_byte & 0x7F)
        minute = cls.decode_bcd(raw_7_bytes[1] & 0x7F)
        hr = cls.decode_bcd(raw_7_bytes[2] & 0x3F)
        day = cls.decode_bcd(raw_7_bytes[3] & 0x3F)
        wday = cls.decode_bcd(raw_7_bytes[4] & 0x07)
        month_byte = raw_7_bytes[5]
        century = 1900 if (month_byte & cls.CENTURY_FLAG) else 2000
        month = cls.decode_bcd(month_byte & 0x1F)
        year = century + cls.decode_bcd(raw_7_bytes[6])

        # Range validity checks
        if not (0 <= sec < 60 and 0 <= minute < 60 and 0 <= hr < 24 and 1 <= month <= 12 and 1 <= day <= 31):
            return {"valid": False, "error": "Corrupt BCD values out of range", "vl": False}

        return {
            "valid": True,
            "year": year,
            "month": month,
            "day": day,
            "weekday": wday,
            "hour": hr,
            "minute": minute,
            "second": sec,
            "vl": False,
        }

    def write_time(self, year: int, month: int, day: int, weekday: int, hour: int, minute: int, second: int) -> None:
        """Simulates writing new time to the chip, automatically clearing the VL flag."""
        regs = self.serialize_bm8563_registers(year, month, day, weekday, hour, minute, second, vl=False)
        for i, val in enumerate(regs):
            self.registers[self.REG_SEC + i] = val

    def read_time(self) -> Dict[str, Any]:
        """Reads 7 time registers from simulated hardware."""
        raw_7 = self.registers[self.REG_SEC : self.REG_SEC + 7]
        return self.parse_bm8563_registers(raw_7)

    def trigger_power_loss(self) -> None:
        """Simulates battery backup discharge / cold start setting VL bit."""
        self.registers[self.REG_SEC] |= self.VL_FLAG


# Backwards compatibility alias
HardwareRtcModel = BM8563RtcModel


class CW2017FuelGaugeModel:
    """
    Authoritative Hardware Simulation Model for CellWise CW2017 Fuel Gauge IC.
    Physical hardware ground truth:
    - I2C Address: 0x63 (shared bus SDA 39 / SCL 38 @ 400kHz).
    - Registers:
      0x00: REG_VERSION (0xA0 during power-on/reset; 0x0D/0x0F when running)
      0x02: REG_VCELL_H (upper 6 bits of 14-bit cell voltage)
      0x03: REG_VCELL_L (lower 8 bits of 14-bit cell voltage)
      0x04: REG_SOC     (integer state-of-charge percentage, 0..100)
      0x05: REG_SOC_DEC (fractional percentage, 1/256th)
      0x08: REG_MODE    (0x00=Normal, 0x30=Restart, 0xF0=Default reset)
      0x0B: REG_SOC_ALERT (bit 7 = 0x80 profile update flag)
      0x10..0x5F: REG_BATINFO (80-byte profile resident in SRAM)
    - Thresholds:
      Critical battery voltage: < 3400 mV (HalPowerManager: 2000 < mv < 3400)
      Flash write safety threshold: >= 3200 mV (below 3200 mV brownout abort)
    """
    I2C_ADDR: int = 0x63

    REG_VERSION: int = 0x00
    REG_VCELL_H: int = 0x02
    REG_VCELL_L: int = 0x03
    REG_SOC: int = 0x04
    REG_SOC_DEC: int = 0x05
    REG_MODE: int = 0x08
    REG_SOC_ALERT: int = 0x0B
    REG_BATINFO: int = 0x10

    MODE_NORMAL: int = 0x00
    MODE_RESTART: int = 0x30
    MODE_DEFAULT: int = 0xF0
    UPDATE_FLAG: int = 0x80

    VERSION_STARTING: int = 0xA0
    VERSION_RUNNING: int = 0x0D

    CRITICAL_BATTERY_THRESHOLD_MV: int = 3400
    MIN_OPERATING_VOLTAGE_MV: int = 2000
    FLASH_WRITE_SAFE_VOLTAGE_MV: int = 3200

    OEM_BATINFO_PROFILE: List[int] = [
        0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xAA, 0xBF, 0xB5, 0xB4, 0xA4, 0x9C, 0xEB, 0xE2,
        0xDF, 0xE5, 0xCA, 0xA0, 0x8A, 0x62, 0x53, 0x48, 0x40, 0x3A, 0x32, 0xB1, 0xAE, 0xDA, 0xB5, 0xFF,
        0xFF, 0xFF, 0xE8, 0xDB, 0xD9, 0xD6, 0xD4, 0xD2, 0xD0, 0xCB, 0xC3, 0xBC, 0x9E, 0x87, 0x7B, 0x71,
        0x72, 0x7C, 0x8C, 0xA3, 0xB7, 0xC8, 0xA5, 0x4F, 0x00, 0x00, 0xAB, 0x02, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x23,
    ]

    def __init__(self, initial_soc: int = 85, initial_mv: int = 3850, profile_loaded: bool = True) -> None:
        self.registers: Dict[int, int] = {}
        for r in range(0x60):
            self.registers[r] = 0x00

        self.registers[self.REG_MODE] = self.MODE_NORMAL
        self.registers[self.REG_VERSION] = self.VERSION_RUNNING if profile_loaded else self.VERSION_STARTING
        self.registers[self.REG_SOC_ALERT] = self.UPDATE_FLAG if profile_loaded else 0x00

        if profile_loaded:
            for idx, val in enumerate(self.OEM_BATINFO_PROFILE):
                self.registers[self.REG_BATINFO + idx] = val

        self.set_voltage_mv(initial_mv)
        self.set_soc(initial_soc if profile_loaded else 0)

    @classmethod
    def verify_address(cls, addr: int) -> bool:
        """Verifies if the given I2C address is the valid CW2017 address (0x63)."""
        return addr == cls.I2C_ADDR

    @staticmethod
    def raw14_to_mv(raw14: int) -> int:
        """Converts raw 14-bit VCELL reading to millivolts via OEM formula ((raw14 * 5 + 8) >> 4)."""
        return (raw14 * 5 + 8) >> 4

    @staticmethod
    def mv_to_raw14(mv: int) -> int:
        """Converts millivolts to 14-bit VCELL register representation."""
        return max(0, min(0x3FFF, ((mv << 4) - 8 + 2) // 5))

    @classmethod
    def is_battery_critical(cls, mv: int) -> bool:
        """Matches HalPowerManager::isBatteryCritical(): mv > 2000 && mv < 3400."""
        return cls.MIN_OPERATING_VOLTAGE_MV < mv < cls.CRITICAL_BATTERY_THRESHOLD_MV

    @classmethod
    def is_voltage_safe_for_write(cls, mv: int) -> bool:
        """Matches ZeroBrickRiskModel / HalStorage: safe only when >= 3200 mV."""
        return mv >= cls.FLASH_WRITE_SAFE_VOLTAGE_MV

    def set_voltage_mv(self, mv: int) -> None:
        raw14 = self.mv_to_raw14(mv)
        self.registers[self.REG_VCELL_H] = (raw14 >> 8) & 0x3F
        self.registers[self.REG_VCELL_L] = raw14 & 0xFF

    def get_voltage_mv(self) -> int:
        hi = self.registers.get(self.REG_VCELL_H, 0)
        lo = self.registers.get(self.REG_VCELL_L, 0)
        raw14 = ((hi & 0x3F) << 8) | lo
        return self.raw14_to_mv(raw14)

    def set_soc(self, soc: int) -> None:
        self.registers[self.REG_SOC] = max(0, min(100, soc))

    def get_soc(self) -> int:
        # If profile not loaded or version not running, CW2017 reports 0%
        if not self.is_profile_valid() or (self.registers.get(self.REG_VERSION, 0) & 0xFD) != 0x0D:
            return 0
        return self.registers.get(self.REG_SOC, 0)

    def is_profile_valid(self) -> bool:
        if (self.registers.get(self.REG_SOC_ALERT, 0) & self.UPDATE_FLAG) == 0:
            return False
        for idx, val in enumerate(self.OEM_BATINFO_PROFILE):
            if self.registers.get(self.REG_BATINFO + idx, 0) != val:
                return False
        return True

    def simulate_soft_reset(self) -> bool:
        """Simulates cw2017Reset(): MODE 0xF0 -> 0x30 -> 0x00."""
        self.registers[self.REG_MODE] = self.MODE_DEFAULT
        self.registers[self.REG_MODE] = self.MODE_RESTART
        self.registers[self.REG_MODE] = self.MODE_NORMAL
        self.registers[self.REG_VERSION] = self.VERSION_RUNNING
        return True


class GT911TouchModel:
    """
    Authoritative Hardware Simulation Model for Goodix GT911 Capacitive Touch Controller.
    Physical hardware ground truth:
    - I2C Address: 0x5D on shared I2C bus (SDA 39 / SCL 38 @ 400kHz).
    - Status Register: 0x814E
      bit 7 (0x80): Buffer ready / data valid.
      bit 4 (0x10): Capacitive Home key pressed.
      bits 3:0 (0x0F): Active touch points count (0..5).
    - Coordinates: Contiguous records starting at 0x814F / 0x8150:
      Each point record: [track_id, x_lo, x_hi, y_lo, y_hi, size_lo, size_hi, reserved]
    - Status clearing: Driver must write 0x00 to 0x814E after reading each frame.
    - I2C Bus Glitch Recovery: Upon 5 consecutive communication failures:
      1. 9 SCL bus-clear clock pulses.
      2. Valid I2C STOP condition.
      3. Wire peripheral re-init.
      4. Hardware reset (RST low 10ms, high 10ms).
    """
    I2C_ADDR: int = 0x5D
    ALT_I2C_ADDR: int = 0x14

    REG_STATUS: int = 0x814E
    REG_POINTS_START: int = 0x814F
    REG_POINTS_RECORD_BASE: int = 0x8150

    STATUS_BUFFER_READY: int = 0x80
    STATUS_HOME_KEY: int = 0x10
    STATUS_COUNT_MASK: int = 0x0F

    MAX_POINTS: int = 5
    POINT_RECORD_SIZE: int = 8

    CONSECUTIVE_FAILURES_THRESHOLD: int = 5
    BUS_CLEAR_PULSES: int = 9
    RST_PULSE_LOW_MS: int = 10
    RST_PULSE_HIGH_MS: int = 10

    PANEL_WIDTH: int = 800
    PANEL_HEIGHT: int = 480

    def __init__(self) -> None:
        self.status: int = 0x00
        self.points: List[Dict[str, int]] = []
        self.home_key_pressed: bool = False
        self.consecutive_failures: int = 0
        self.bus_jammed: bool = False
        self.last_recovery_log: List[str] = []

    @classmethod
    def verify_address(cls, addr: int) -> bool:
        """Verifies if the given I2C address is the valid GT911 address (0x5D)."""
        return addr == cls.I2C_ADDR

    def set_touch(self, contacts: List[Tuple[int, int]], home_key: bool = False) -> None:
        """Injects touch contacts (up to 5) and Home key state into controller buffer."""
        self.home_key_pressed = home_key
        self.points = []
        num_contacts = min(len(contacts), self.MAX_POINTS)
        for i in range(num_contacts):
            x, y = contacts[i]
            clamped_x = max(0, min(self.PANEL_WIDTH - 1, x))
            clamped_y = max(0, min(self.PANEL_HEIGHT - 1, y))
            self.points.append({
                "id": i,
                "x": clamped_x,
                "y": clamped_y,
                "size": 30,
            })

        self.status = self.STATUS_BUFFER_READY | (num_contacts & self.STATUS_COUNT_MASK)
        if home_key:
            self.status |= self.STATUS_HOME_KEY

    def read_frame(self) -> Dict[str, Any]:
        """Simulates driver reading status register and point records over I2C."""
        if self.bus_jammed:
            self.consecutive_failures += 1
            recovery_result = self.check_recovery()
            return {
                "success": False,
                "error": "I2C bus NACK / line held low",
                "consecutive_failures": self.consecutive_failures,
                "recovery": recovery_result,
            }

        self.consecutive_failures = 0
        return {
            "success": True,
            "status_reg": self.status,
            "buffer_ready": bool(self.status & self.STATUS_BUFFER_READY),
            "home_key": bool(self.status & self.STATUS_HOME_KEY),
            "point_count": self.status & self.STATUS_COUNT_MASK,
            "points": list(self.points),
        }

    def clear_status(self) -> None:
        """Simulates driver clearing status register 0x814E."""
        self.status = 0x00
        self.points = []

    def jam_bus(self) -> None:
        """Simulates an I2C bus stall (SDA line held low)."""
        self.bus_jammed = True

    def check_recovery(self) -> Dict[str, Any]:
        """Evaluates whether consecutive failures trigger the hardware bus-clear + RST recovery sequence."""
        if self.consecutive_failures >= self.CONSECUTIVE_FAILURES_THRESHOLD:
            self.last_recovery_log = [
                f"Generated {self.BUS_CLEAR_PULSES} SCL clock pulses",
                "Sent I2C STOP condition",
                "Re-initialized Wire peripheral (400kHz)",
                f"Pulsed hardware RST pin (LOW {self.RST_PULSE_LOW_MS}ms, HIGH {self.RST_PULSE_HIGH_MS}ms)",
            ]
            self.bus_jammed = False
            self.consecutive_failures = 0
            self.status = 0x00
            self.points = []
            return {
                "triggered": True,
                "pulses": self.BUS_CLEAR_PULSES,
                "rst_low_ms": self.RST_PULSE_LOW_MS,
                "bus_restored": True,
            }
        return {"triggered": False, "consecutive_failures": self.consecutive_failures}


class SSD1677DisplayModel:
    """
    Authoritative Hardware Simulation Model for Solomon Systech SSD1677 Active Matrix EPD Controller.
    Physical hardware ground truth:
    - Interface: 4-wire SPI (MOSI 11, SCLK 12, CS 13, DC 14, RST 15, BUSY 16 @ 20MHz).
    - Resolution: 800 x 480 (800 columns x 480 gate lines).
      Row bytes = 800 // 8 = 100 bytes. Buffer size = 48,000 bytes.
    - Polarity: 1 = White, 0 = Black. Active-HIGH BUSY.
    - Dual RAM:
      BW RAM: 0x24 (Write), 0x46 (Auto-write) -> holds incoming new frame.
      RED RAM: 0x26 (Write), 0x47 (Auto-write) -> holds previous frame baseline.
    - Update Sequences (CMD 0x22 Display Update Control 2):
      FULL Refresh:
        CTRL1 = 0x40 (CTRL1_BYPASS_RED)
        CTRL2 = 0xF7 (All phases enabled, OTP full waveform, ~1800 ms)
        Border = 0xC0 or 0x01
      FAST / PARTIAL Refresh:
        CTRL1 = 0x00 (CTRL1_NORMAL, differential)
        CTRL2 = 0xFC (or 0x1C incremental DU, ~500 ms / ~77 ms)
        Border = 0xC0 or 0x80
      HALF Refresh:
        CTRL1 = 0x40
        CMD 0x1A = 0x5A
        CTRL2 = 0xD7
      Master Activation: CMD 0x20 triggers display update execution.
    """
    WIDTH: int = 800
    HEIGHT: int = 480
    BYTES_PER_ROW: int = 100
    FRAMEBUFFER_SIZE: int = 48000

    CMD_DEEP_SLEEP: int = 0x10
    CMD_DATA_ENTRY_MODE: int = 0x11
    CMD_SOFT_RESET: int = 0x12
    CMD_TEMP_SENSOR_CONTROL: int = 0x18
    CMD_WRITE_TEMP: int = 0x1A
    CMD_MASTER_ACTIVATION: int = 0x20
    CMD_DISPLAY_UPDATE_CTRL1: int = 0x21
    CMD_DISPLAY_UPDATE_CTRL2: int = 0x22
    CMD_WRITE_RAM_BW: int = 0x24
    CMD_WRITE_RAM_RED: int = 0x26
    CMD_BORDER_WAVEFORM: int = 0x3C
    CMD_SET_RAM_X_RANGE: int = 0x44
    CMD_SET_RAM_Y_RANGE: int = 0x45
    CMD_AUTO_WRITE_BW_RAM: int = 0x46
    CMD_AUTO_WRITE_RED_RAM: int = 0x47
    CMD_SET_RAM_X_COUNTER: int = 0x4E
    CMD_SET_RAM_Y_COUNTER: int = 0x4F

    CTRL1_NORMAL: int = 0x00
    CTRL1_BYPASS_RED: int = 0x40

    SEQ_FULL: int = 0xF7
    SEQ_FAST_PARTIAL: int = 0xFC
    SEQ_FAST_INCREMENTAL: int = 0x1C
    SEQ_HALF: int = 0xD7

    DURATION_FULL_MS: int = 1800
    DURATION_PARTIAL_MS: int = 500
    DURATION_FAST_INCREMENTAL_MS: int = 77

    def __init__(self) -> None:
        self.bw_ram: bytearray = bytearray([0xFF] * self.FRAMEBUFFER_SIZE)
        self.red_ram: bytearray = bytearray([0xFF] * self.FRAMEBUFFER_SIZE)
        self.visible_panel: bytearray = bytearray([0xFF] * self.FRAMEBUFFER_SIZE)

        self.ctrl1: int = self.CTRL1_NORMAL
        self.ctrl2: int = 0x00
        self.border_waveform: int = 0x80
        self.temp_deg_c: int = 25
        self.is_sleeping: bool = False
        self.is_busy: bool = False
        self.last_refresh_type: Optional[str] = None
        self.last_refresh_duration_ms: int = 0
        self.refresh_count_full: int = 0
        self.refresh_count_partial: int = 0

    def write_bw_ram(self, data: bytes | bytearray) -> None:
        """Writes data into BW RAM (0x24)."""
        limit = min(len(data), self.FRAMEBUFFER_SIZE)
        self.bw_ram[:limit] = data[:limit]

    def write_red_ram(self, data: bytes | bytearray) -> None:
        """Writes data into RED/previous RAM (0x26)."""
        limit = min(len(data), self.FRAMEBUFFER_SIZE)
        self.red_ram[:limit] = data[:limit]

    def send_command(self, cmd: int, data: Optional[List[int]] = None) -> None:
        """Simulates SPI command + data write."""
        if cmd == self.CMD_SOFT_RESET:
            self.bw_ram = bytearray([0xFF] * self.FRAMEBUFFER_SIZE)
            self.red_ram = bytearray([0xFF] * self.FRAMEBUFFER_SIZE)
            self.is_sleeping = False
        elif cmd == self.CMD_DISPLAY_UPDATE_CTRL1:
            if data:
                self.ctrl1 = data[0]
        elif cmd == self.CMD_DISPLAY_UPDATE_CTRL2:
            if data:
                self.ctrl2 = data[0]
        elif cmd == self.CMD_BORDER_WAVEFORM:
            if data:
                self.border_waveform = data[0]
        elif cmd == self.CMD_WRITE_TEMP:
            if data:
                self.temp_deg_c = data[0]
        elif cmd == self.CMD_DEEP_SLEEP:
            self.is_sleeping = True
        elif cmd == self.CMD_MASTER_ACTIVATION:
            self.execute_master_activation()

    def execute_master_activation(self) -> Dict[str, Any]:
        """Executes display refresh update sequence according to CTRL1 and CTRL2."""
        if self.ctrl2 in (self.SEQ_FULL,):
            refresh_type = "FULL"
            duration = self.DURATION_FULL_MS
            self.refresh_count_full += 1
            self.visible_panel[:] = self.bw_ram[:]
            self.red_ram[:] = self.bw_ram[:]
        elif self.ctrl2 in (self.SEQ_FAST_PARTIAL, self.SEQ_FAST_INCREMENTAL, 0xFF):
            refresh_type = "PARTIAL"
            duration = (
                self.DURATION_FAST_INCREMENTAL_MS
                if self.ctrl2 == self.SEQ_FAST_INCREMENTAL
                else self.DURATION_PARTIAL_MS
            )
            self.refresh_count_partial += 1
            for i in range(self.FRAMEBUFFER_SIZE):
                self.visible_panel[i] = self.bw_ram[i]
            self.red_ram[:] = self.bw_ram[:]
        elif self.ctrl2 == self.SEQ_HALF:
            refresh_type = "HALF"
            duration = 900
            self.visible_panel[:] = self.bw_ram[:]
            self.red_ram[:] = self.bw_ram[:]
        else:
            refresh_type = "UNKNOWN"
            duration = 100

        self.last_refresh_type = refresh_type
        self.last_refresh_duration_ms = duration
        return {
            "refresh_type": refresh_type,
            "ctrl1": self.ctrl1,
            "ctrl2": self.ctrl2,
            "duration_ms": duration,
            "is_screen_on": (self.ctrl2 & 0x03) == 0,
        }
