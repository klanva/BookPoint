#!/usr/bin/env python3
"""
E-Ink 800x480 Screen Simulator & Visual Artifact Generator for Xteink X4 Pro
Firmware: BookPoint 2.0.0
Author: Google Deepmind Antigravity Agent
"""

import os
import sys
import argparse
import math
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

# Screen dimensions (Portrait mode: 480x800)
SCREEN_WIDTH = 480
SCREEN_HEIGHT = 800

# E-Ink Color Palette
INK_BLACK = (18, 18, 18)
INK_DARK_GRAY = (85, 85, 85)
INK_LIGHT_GRAY = (185, 185, 185)
INK_WHITE = (248, 248, 246) # Warm paper tint
INK_PURE_WHITE = (255, 255, 255)

# Hardware bezel dimensions for device mockup
BEZEL_X = 24
BEZEL_TOP = 36
BEZEL_BOTTOM = 64
DEVICE_WIDTH = SCREEN_WIDTH + BEZEL_X * 2
DEVICE_HEIGHT = SCREEN_HEIGHT + BEZEL_TOP + BEZEL_BOTTOM

# Font helper
def get_fonts():
    font_paths = [
        "C:/xteinkx4/src/lib/EpdFont/builtinFonts/source/NotoSans/NotoSans-Regular.ttf",
        "C:/xteinkx4/src/lib/EpdFont/builtinFonts/source/NotoSans/NotoSans-Bold.ttf",
        "C:/xteinkx4/src/lib/EpdFont/builtinFonts/source/NotoSerif/NotoSerif-Regular.ttf",
        "C:/xteinkx4/src/lib/EpdFont/builtinFonts/source/NotoSerif/NotoSerif-Bold.ttf",
        "C:/Windows/Fonts/arial.ttf",
        "C:/Windows/Fonts/arialbd.ttf"
    ]
    
    regular_path = font_paths[0] if os.path.exists(font_paths[0]) else "C:/Windows/Fonts/arial.ttf"
    bold_path = font_paths[1] if os.path.exists(font_paths[1]) else "C:/Windows/Fonts/arialbd.ttf"
    serif_path = font_paths[2] if os.path.exists(font_paths[2]) else regular_path
    serif_bold_path = font_paths[3] if os.path.exists(font_paths[3]) else bold_path
    
    return {
        "title_large": ImageFont.truetype(bold_path, 26),
        "title": ImageFont.truetype(bold_path, 20),
        "header": ImageFont.truetype(bold_path, 16),
        "body_bold": ImageFont.truetype(bold_path, 14),
        "body": ImageFont.truetype(regular_path, 14),
        "small_bold": ImageFont.truetype(bold_path, 12),
        "small": ImageFont.truetype(regular_path, 12),
        "tiny": ImageFont.truetype(regular_path, 10),
        "serif_header": ImageFont.truetype(serif_bold_path, 18),
        "serif_body": ImageFont.truetype(serif_path, 15),
        "key_label": ImageFont.truetype(bold_path, 15)
    }

def draw_dither_rect(draw, x0, y0, x1, y1, step=2):
    """Draws 1-bit E-ink 50% dither pattern."""
    for y in range(int(y0), int(y1)):
        for x in range(int(x0), int(x1)):
            if (x + y) % step == 0:
                draw.point((x, y), fill=INK_BLACK)

def draw_header_bar(draw, fonts, title="BookPoint", battery_pct=84, time_str="18:45"):
    """Draws top status bar with clock, battery, and title."""
    # Top bar background
    draw.rectangle([0, 0, SCREEN_WIDTH, 34], fill=INK_WHITE)
    draw.line([0, 34, SCREEN_WIDTH, 34], fill=INK_LIGHT_GRAY, width=1)
    
    # Title / Mode
    draw.text((16, 8), title, font=fonts["small_bold"], fill=INK_BLACK)
    
    # Clock (Centered)
    w_clock = draw.textlength(time_str, font=fonts["small_bold"])
    draw.text(((SCREEN_WIDTH - w_clock) // 2, 8), time_str, font=fonts["small_bold"], fill=INK_BLACK)
    
    # Battery & WiFi (Right-aligned)
    bat_x = SCREEN_WIDTH - 64
    bat_y = 11
    # Battery icon outline
    draw.rounded_rectangle([bat_x, bat_y, bat_x + 24, bat_y + 12], radius=2, outline=INK_BLACK, width=1)
    draw.rectangle([bat_x + 24, bat_y + 3, bat_x + 26, bat_y + 9], fill=INK_BLACK)
    # Battery fill
    fill_w = int(20 * (battery_pct / 100.0))
    if fill_w > 0:
        draw.rectangle([bat_x + 2, bat_y + 2, bat_x + 2 + fill_w, bat_y + 10], fill=INK_BLACK)
    
    # Percentage text
    pct_str = f"{battery_pct}%"
    draw.text((bat_x - 30, 9), pct_str, font=fonts["tiny"], fill=INK_BLACK)

def render_dashboard_screen(fonts):
    """Renders Home Dashboard Theme (DashboardTheme) with rich metadata & quick menu."""
    img = Image.new("RGB", (SCREEN_WIDTH, SCREEN_HEIGHT), INK_WHITE)
    draw = ImageDraw.Draw(img)
    
    draw_header_bar(draw, fonts, title="BookPoint 2.0.0", battery_pct=88, time_str="19:00")
    
    # --- HERO COVER CARD (Top section) ---
    hero_y = 44
    card_h = 290
    draw.rounded_rectangle([16, hero_y, SCREEN_WIDTH - 16, hero_y + card_h], radius=12, outline=INK_BLACK, width=2)
    # Card soft drop shadow / inner line
    draw.rounded_rectangle([18, hero_y + 2, SCREEN_WIDTH - 18, hero_y + card_h - 2], radius=10, outline=INK_LIGHT_GRAY, width=1)
    
    # Book Cover Art (Left side of hero card)
    cover_x = 30
    cover_y = hero_y + 16
    cover_w = 140
    cover_h = 200
    draw.rounded_rectangle([cover_x, cover_y, cover_x + cover_w, cover_y + cover_h], radius=6, outline=INK_BLACK, width=2)
    draw.rectangle([cover_x + 2, cover_y + 2, cover_x + cover_w - 2, cover_y + cover_h - 2], fill=INK_WHITE)
    # Stylized book cover graphics
    draw.rounded_rectangle([cover_x + 8, cover_y + 8, cover_x + cover_w - 8, cover_y + cover_h - 8], radius=4, outline=INK_DARK_GRAY, width=1)
    draw_dither_rect(draw, cover_x + 12, cover_y + 12, cover_x + cover_w - 12, cover_y + 70, step=4)
    draw.text((cover_x + 16, cover_y + 78), "МАСТЕР И", font=fonts["header"], fill=INK_BLACK)
    draw.text((cover_x + 16, cover_y + 98), "МАРГАРИТА", font=fonts["header"], fill=INK_BLACK)
    draw.line([cover_x + 16, cover_y + 122, cover_x + cover_w - 16, cover_y + 122], fill=INK_BLACK, width=1)
    draw.text((cover_x + 16, cover_y + 130), "М. Булгаков", font=fonts["small"], fill=INK_DARK_GRAY)
    draw.text((cover_x + 16, cover_y + 175), "EPUB • 1.4 MB", font=fonts["tiny"], fill=INK_DARK_GRAY)
    
    # Metadata Details (Right side of hero card)
    meta_x = cover_x + cover_w + 16
    draw.text((meta_x, hero_y + 18), "ТЕКУЩАЯ КНИГА", font=fonts["tiny"], fill=INK_DARK_GRAY)
    draw.text((meta_x, hero_y + 34), "Мастер и", font=fonts["title"], fill=INK_BLACK)
    draw.text((meta_x, hero_y + 58), "Маргарита", font=fonts["title"], fill=INK_BLACK)
    draw.text((meta_x, hero_y + 84), "Михаил Булгаков", font=fonts["body"], fill=INK_DARK_GRAY)
    
    # Progress Bar
    draw.text((meta_x, hero_y + 118), "Прогресс чтения:", font=fonts["small"], fill=INK_BLACK)
    draw.text((meta_x + 170, hero_y + 116), "42%", font=fonts["header"], fill=INK_BLACK)
    
    pb_y = hero_y + 138
    pb_w = 210
    pb_h = 10
    draw.rounded_rectangle([meta_x, pb_y, meta_x + pb_w, pb_y + pb_h], radius=5, outline=INK_BLACK, width=1)
    fill_pb = int((pb_w - 2) * 0.42)
    draw.rounded_rectangle([meta_x + 1, pb_y + 1, meta_x + 1 + fill_pb, pb_y + pb_h - 1], radius=4, fill=INK_BLACK)
    
    # Stats details
    draw.text((meta_x, hero_y + 158), "📖 160 из 380 стр.", font=fonts["small"], fill=INK_BLACK)
    draw.text((meta_x, hero_y + 178), "⏱️ Осталось: ~1 ч 45 мин", font=fonts["small"], fill=INK_BLACK)
    draw.text((meta_x, hero_y + 198), "⚡ Скорость: 215 сл/мин", font=fonts["small"], fill=INK_DARK_GRAY)
    
    # Bottom card streak pill
    streak_y = hero_y + card_h - 48
    draw.rounded_rectangle([30, streak_y, SCREEN_WIDTH - 30, streak_y + 36], radius=8, fill=INK_DARK_GRAY)
    draw.text((44, streak_y + 8), "🔥 Серия чтения: 5 дней подряд! (+45 мин сегодня)", font=fonts["small_bold"], fill=INK_WHITE)
    
    # --- QUICK ACTION TOUCH MENU (Bottom section) ---
    menu_y = hero_y + card_h + 14
    menu_items = [
        ("📁", "Обзор файлов", "Все папки и книги на MicroSD"),
        ("🕒", "Последние книги", "История чтения и прогресс"),
        ("🔍", "Поиск книг", "Мгновенный поиск по библиотеке"),
        ("📊", "Статистика чтения", "Графики активности и аналитика"),
        ("🔖", "Закладки и цитаты", "Сохраненные заметки и закладки"),
        ("📶", "Передача файлов", "WiFi / WebDAV загрузка по воздуху"),
        ("⚙️", "Настройки", "Экран, шрифты, сенсор, темы")
    ]
    
    row_h = 54
    for i, (icon, title, desc) in enumerate(menu_items):
        item_y = menu_y + i * (row_h + 4)
        if item_y + row_h > SCREEN_HEIGHT - 38:
            break
        
        # Row card with touch-friendly rounded box
        draw.rounded_rectangle([16, item_y, SCREEN_WIDTH - 16, item_y + row_h], radius=8, outline=INK_LIGHT_GRAY, width=1)
        
        # Highlight on search (simulating focus or touch)
        if i == 2:
            draw.rounded_rectangle([16, item_y, SCREEN_WIDTH - 16, item_y + row_h], radius=8, outline=INK_BLACK, width=2)
            draw_dither_rect(draw, 18, item_y + 2, 24, item_y + row_h - 2, step=2)
        
        # Icon
        draw.text((32, item_y + 14), icon, font=fonts["header"], fill=INK_BLACK)
        # Title
        draw.text((68, item_y + 10), title, font=fonts["body_bold"], fill=INK_BLACK)
        # Description
        draw.text((68, item_y + 30), desc, font=fonts["tiny"], fill=INK_DARK_GRAY)
        # Arrow
        draw.text((SCREEN_WIDTH - 40, item_y + 18), "›", font=fonts["header"], fill=INK_DARK_GRAY)
        
    # Bottom navigation hint bar
    draw.rectangle([0, SCREEN_HEIGHT - 30, SCREEN_WIDTH, SCREEN_HEIGHT], fill=INK_WHITE)
    draw.line([0, SCREEN_HEIGHT - 30, SCREEN_WIDTH, SCREEN_HEIGHT - 30], fill=INK_LIGHT_GRAY, width=1)
    draw.text((20, SCREEN_HEIGHT - 22), "Тап: Открыть  •  Свайп вверх/вниз: Меню  •  Кнопка Home: Домой", font=fonts["tiny"], fill=INK_DARK_GRAY)
    
    return img

def render_carousel_screen(fonts):
    """Renders Lyra Carousel Theme (LyraCarouselTheme) with 3 covers & bottom plaque."""
    img = Image.new("RGB", (SCREEN_WIDTH, SCREEN_HEIGHT), INK_WHITE)
    draw = ImageDraw.Draw(img)
    
    draw_header_bar(draw, fonts, title="BookPoint • Карусель", battery_pct=88, time_str="19:02")
    
    carousel_y = 52
    
    # 1. LEFT NEIGHBOR COVER (Dimmed / Dithered preview)
    left_w = 90
    left_h = 135
    left_x = 16
    left_y = carousel_y + 40
    draw.rounded_rectangle([left_x, left_y, left_x + left_w, left_y + left_h], radius=6, outline=INK_LIGHT_GRAY, width=1)
    draw_dither_rect(draw, left_x, left_y, left_x + left_w, left_y + left_h, step=3)
    draw.text((left_x + 10, left_y + 20), "1984", font=fonts["small_bold"], fill=INK_BLACK)
    draw.text((left_x + 10, left_y + 40), "Дж. Оруэлл", font=fonts["tiny"], fill=INK_BLACK)
    
    # 2. RIGHT NEIGHBOR COVER (Dimmed / Dithered preview)
    right_w = 90
    right_h = 135
    right_x = SCREEN_WIDTH - 16 - right_w
    right_y = carousel_y + 40
    draw.rounded_rectangle([right_x, right_y, right_x + right_w, right_y + right_h], radius=6, outline=INK_LIGHT_GRAY, width=1)
    draw_dither_rect(draw, right_x, right_y, right_x + right_w, right_y + right_h, step=3)
    draw.text((right_x + 10, right_y + 20), "Дюна", font=fonts["small_bold"], fill=INK_BLACK)
    draw.text((right_x + 10, right_y + 40), "Ф. Герберт", font=fonts["tiny"], fill=INK_BLACK)
    
    # 3. CENTER FOCUSED HERO COVER (Magnified 3D-like depth)
    center_w = 190
    center_h = 260
    center_x = (SCREEN_WIDTH - center_w) // 2
    center_y = carousel_y + 10
    
    # Drop shadow
    draw.rounded_rectangle([center_x + 4, center_y + 4, center_x + center_w + 4, center_y + center_h + 4], radius=10, fill=INK_LIGHT_GRAY)
    # White card frame
    draw.rounded_rectangle([center_x, center_y, center_x + center_w, center_y + center_h], radius=8, fill=INK_WHITE, outline=INK_BLACK, width=2)
    # Inner border
    draw.rounded_rectangle([center_x + 4, center_y + 4, center_x + center_w - 4, center_y + center_h - 4], radius=6, outline=INK_BLACK, width=1)
    
    # Artwork in center
    draw_dither_rect(draw, center_x + 10, center_y + 10, center_x + center_w - 10, center_y + 80, step=4)
    draw.text((center_x + 16, center_y + 95), "ПИКНИК НА", font=fonts["header"], fill=INK_BLACK)
    draw.text((center_x + 16, center_y + 118), "ОБОЧИНЕ", font=fonts["header"], fill=INK_BLACK)
    draw.line([center_x + 16, center_y + 144, center_x + center_w - 16, center_y + 144], fill=INK_BLACK, width=1)
    draw.text((center_x + 16, center_y + 152), "А. и Б. Стругацкие", font=fonts["small_bold"], fill=INK_DARK_GRAY)
    draw.text((center_x + 16, center_y + 172), "Научная фантастика", font=fonts["tiny"], fill=INK_DARK_GRAY)
    draw.text((center_x + 16, center_y + 225), "EPUB • 720 KB", font=fonts["tiny"], fill=INK_DARK_GRAY)
    
    # --- BOTTOM METADATA PLAQUE ---
    plaque_y = center_y + center_h + 14
    # Title
    t = "Пикник на обочине"
    w_t = draw.textlength(t, font=fonts["title"])
    draw.text(((SCREEN_WIDTH - w_t) // 2, plaque_y), t, font=fonts["title"], fill=INK_BLACK)
    
    # Author
    a = "Аркадий и Борис Стругацкие"
    w_a = draw.textlength(a, font=fonts["body"])
    draw.text(((SCREEN_WIDTH - w_a) // 2, plaque_y + 26), a, font=fonts["body"], fill=INK_DARK_GRAY)
    
    # Progress bar + Percentage
    bar_w = 220
    bar_h = 7
    bar_x = (SCREEN_WIDTH - bar_w) // 2
    bar_y = plaque_y + 54
    draw.rounded_rectangle([bar_x, bar_y, bar_x + bar_w, bar_y + bar_h], radius=3, outline=INK_BLACK, width=1)
    fill_w = int((bar_w - 2) * 0.68)
    draw.rectangle([bar_x + 1, bar_y + 1, bar_x + 1 + fill_w, bar_y + bar_h - 1], fill=INK_BLACK)
    draw.text((bar_x + bar_w + 10, bar_y - 4), "68%", font=fonts["small_bold"], fill=INK_BLACK)
    
    # Carousel pagination dots
    dot_y = plaque_y + 76
    dots_count = 5
    dot_spacing = 18
    dots_total_w = (dots_count - 1) * dot_spacing
    dots_start_x = (SCREEN_WIDTH - dots_total_w) // 2
    for i in range(dots_count):
        dx = dots_start_x + i * dot_spacing
        if i == 2:  # Active center dot
            draw.ellipse([dx - 5, dot_y - 5, dx + 5, dot_y + 5], fill=INK_BLACK)
        else:
            draw.ellipse([dx - 3, dot_y - 3, dx + 3, dot_y + 3], outline=INK_DARK_GRAY, width=1)
            
    # --- BOTTOM TILES (Touch Action Grid) ---
    tile_y = dot_y + 24
    tiles = [
        ("📁", "Обзор файлов"),
        ("🔍", "Поиск книг"),
        ("📊", "Статистика"),
        ("⚙️", "Настройки")
    ]
    
    tile_h = 56
    tile_w = (SCREEN_WIDTH - 32 - 12) // 2
    for i, (icon, label) in enumerate(tiles):
        row = i // 2
        col = i % 2
        tx = 16 + col * (tile_w + 12)
        ty = tile_y + row * (tile_h + 8)
        
        draw.rounded_rectangle([tx, ty, tx + tile_w, ty + tile_h], radius=8, outline=INK_BLACK, width=1)
        draw.text((tx + 16, ty + 16), icon, font=fonts["header"], fill=INK_BLACK)
        draw.text((tx + 46, ty + 18), label, font=fonts["body_bold"], fill=INK_BLACK)
        
    # Hint footer
    draw.line([0, SCREEN_HEIGHT - 32, SCREEN_WIDTH, SCREEN_HEIGHT - 32], fill=INK_LIGHT_GRAY, width=1)
    draw.text((20, SCREEN_HEIGHT - 24), "Свайп влево/вправо: Карусель  •  Тап по центру: Читать  •  Home: Выход", font=fonts["tiny"], fill=INK_DARK_GRAY)
    
    return img

def render_search_screen(fonts):
    """Renders Book Search with Cyrillic on-screen virtual touch keyboard (BookSearchActivity)."""
    img = Image.new("RGB", (SCREEN_WIDTH, SCREEN_HEIGHT), INK_WHITE)
    draw = ImageDraw.Draw(img)
    
    draw_header_bar(draw, fonts, title="Поиск книг", battery_pct=88, time_str="19:05")
    
    # Search input field
    input_y = 44
    draw.rounded_rectangle([16, input_y, SCREEN_WIDTH - 16, input_y + 44], radius=8, outline=INK_BLACK, width=2)
    draw.text((28, input_y + 12), "🔍", font=fonts["header"], fill=INK_BLACK)
    query_text = "Булга"
    draw.text((56, input_y + 10), query_text, font=fonts["title"], fill=INK_BLACK)
    # Flashing cursor
    cursor_x = 56 + int(draw.textlength(query_text, font=fonts["title"])) + 2
    draw.line([cursor_x, input_y + 10, cursor_x, input_y + 34], fill=INK_BLACK, width=2)
    # Clear button [✕]
    draw.text((SCREEN_WIDTH - 44, input_y + 12), "✕", font=fonts["header"], fill=INK_DARK_GRAY)
    
    # Results Count
    draw.text((20, input_y + 54), "Найдено книг: 4", font=fonts["small_bold"], fill=INK_DARK_GRAY)
    
    # Search Results List
    results = [
        ("Мастер и Маргарита", "Михаил Булгаков", "EPUB • 1.4 МБ", "Прочитано 42%"),
        ("Собачье сердце", "Михаил Булгаков", "EPUB • 420 КБ", "Не начато"),
        ("Белая гвардия", "Михаил Булгаков", "XTC • 890 КБ", "Прочитано 100%"),
        ("Записки юного врача", "Михаил Булгаков", "EPUB • 310 КБ", "Не начато")
    ]
    
    res_y = input_y + 76
    row_h = 58
    for i, (title, author, meta, prog) in enumerate(results):
        ry = res_y + i * row_h
        # Row selection highlight on item 0
        if i == 0:
            draw.rounded_rectangle([16, ry, SCREEN_WIDTH - 16, ry + row_h - 4], radius=6, outline=INK_BLACK, width=2)
            draw_dither_rect(draw, 18, ry + 2, 24, ry + row_h - 6, step=2)
        else:
            draw.line([20, ry + row_h - 4, SCREEN_WIDTH - 20, ry + row_h - 4], fill=INK_LIGHT_GRAY, width=1)
            
        draw.text((32, ry + 6), title, font=fonts["body_bold"], fill=INK_BLACK)
        draw.text((32, ry + 28), f"{author} • {meta}", font=fonts["small"], fill=INK_DARK_GRAY)
        draw.text((SCREEN_WIDTH - 140, ry + 16), prog, font=fonts["small_bold"], fill=INK_BLACK)
        
    # --- ON-SCREEN TOUCH KEYBOARD (ЙЦУКЕН Layout) ---
    kb_y = SCREEN_HEIGHT - 270
    draw.rectangle([0, kb_y, SCREEN_WIDTH, SCREEN_HEIGHT], fill=INK_WHITE)
    draw.line([0, kb_y, SCREEN_WIDTH, kb_y], fill=INK_BLACK, width=2)
    
    rows = [
        ["Й", "Ц", "У", "К", "Е", "Н", "Г", "Ш", "Щ", "З", "Х"],
        ["Ф", "Ы", "В", "А", "П", "Р", "О", "Л", "Д", "Ж", "Э"],
        ["⇧", "Я", "Ч", "С", "М", "И", "Т", "Ь", "Б", "Ю", "⌫"],
        ["?123", "🌐 RU", "       ПРОБЕЛ       ", "ПОИСК 🔍"]
    ]
    
    cur_y = kb_y + 10
    key_h = 54
    key_margin = 4
    
    for row_idx, row in enumerate(rows):
        total_keys = len(row)
        if row_idx == 3:
            # Special bottom row with custom widths
            widths = [70, 75, 195, 100]
            cur_x = 8
            for key, kw in zip(row, widths):
                draw.rounded_rectangle([cur_x, cur_y, cur_x + kw, cur_y + key_h], radius=6, outline=INK_BLACK, width=1)
                if "ПОИСК" in key:
                    draw.rounded_rectangle([cur_x, cur_y, cur_x + kw, cur_y + key_h], radius=6, fill=INK_BLACK)
                    w_k = draw.textlength(key, font=fonts["small_bold"])
                    draw.text((cur_x + (kw - w_k) // 2, cur_y + 18), key, font=fonts["small_bold"], fill=INK_WHITE)
                else:
                    w_k = draw.textlength(key, font=fonts["small_bold"])
                    draw.text((cur_x + (kw - w_k) // 2, cur_y + 18), key, font=fonts["small_bold"], fill=INK_BLACK)
                cur_x += kw + key_margin
        else:
            kw = (SCREEN_WIDTH - 16 - (total_keys - 1) * key_margin) // total_keys
            cur_x = 8
            for key in row:
                draw.rounded_rectangle([cur_x, cur_y, cur_x + kw, cur_y + key_h], radius=5, outline=INK_BLACK, width=1)
                w_k = draw.textlength(key, font=fonts["key_label"])
                draw.text((cur_x + (kw - w_k) // 2, cur_y + 16), key, font=fonts["key_label"], fill=INK_BLACK)
                cur_x += kw + key_margin
                
        cur_y += key_h + key_margin
        
    return img

def render_reader_screen(fonts):
    """Renders Reader View with dual status bar, clean typography and reading progress."""
    img = Image.new("RGB", (SCREEN_WIDTH, SCREEN_HEIGHT), INK_WHITE)
    draw = ImageDraw.Draw(img)
    
    # TOP STATUS BAR (Book Title & Time)
    draw.rectangle([0, 0, SCREEN_WIDTH, 30], fill=INK_WHITE)
    draw.line([0, 30, SCREEN_WIDTH, 30], fill=INK_LIGHT_GRAY, width=1)
    draw.text((20, 7), "Мастер и Маргарита • Глава 3", font=fonts["tiny"], fill=INK_DARK_GRAY)
    draw.text((SCREEN_WIDTH - 60, 7), "19:10", font=fonts["tiny"], fill=INK_DARK_GRAY)
    
    # READER CONTENT (Crisp book text in Serif)
    content_y = 54
    draw.text((28, content_y), "ГЛАВА 3. СЕДЬМОЕ ДОКАЗАТЕЛЬСТВО", font=fonts["serif_header"], fill=INK_BLACK)
    draw.line([28, content_y + 28, 180, content_y + 28], fill=INK_BLACK, width=2)
    
    paragraphs = [
        "— Да, пора сказать главное, — заговорил Воланд, наклонившись к ошеломленному поэту. — Никакое это не совпадение, и не досужий вымысел.",
        "Берлиоз тихо перевел дух и взглянул на профессора с растущим беспокойством. Тот же, словно не замечая произведенного эффекта, продолжал неторопливо чертить концом трости таинственные знаки на влажном песке аллеи.",
        "— Человек смертен, но это было бы еще полбеды. Плохо то, что он иногда внезапно смертен, вот в чем фокус! И вообще не может сказать, что он будет делать в сегодняшний вечер.",
        "— Ну, здесь уж вы преувеличиваете! — возразил Берлиоз, нервно поправляя очки. — Сегодняшний вечер мне известен вполне точно. Сейчас я зайду в МАССОЛИТ, а в десять часов вечера у меня заседание...",
        "— Этого не может быть, — твердо прервал иностранец. — Аннушка уже купила подсолнечное масло, и не только купила, но даже и разлила. Так что заседание не состоится."
    ]
    
    cur_y = content_y + 44
    line_h = 24
    for p in paragraphs:
        # Wrap paragraph text to screen width with margins
        words = p.split()
        line = ""
        for word in words:
            test_line = f"{line} {word}".strip()
            if draw.textlength(test_line, font=fonts["serif_body"]) < (SCREEN_WIDTH - 56):
                line = test_line
            else:
                draw.text((28, cur_y), line, font=fonts["serif_body"], fill=INK_BLACK)
                cur_y += line_h
                line = word
        if line:
            draw.text((28, cur_y), line, font=fonts["serif_body"], fill=INK_BLACK)
            cur_y += line_h
        cur_y += 12 # Paragraph gap
        
    # BOTTOM STATUS BAR (Progress, Pages, Battery, Time remaining)
    sb_y = SCREEN_HEIGHT - 36
    draw.rectangle([0, sb_y, SCREEN_WIDTH, SCREEN_HEIGHT], fill=INK_WHITE)
    draw.line([0, sb_y, SCREEN_WIDTH, sb_y], fill=INK_LIGHT_GRAY, width=1)
    
    # Mini Progress Bar
    draw.rectangle([20, sb_y + 12, SCREEN_WIDTH - 20, sb_y + 15], fill=INK_LIGHT_GRAY)
    draw.rectangle([20, sb_y + 12, int(20 + (SCREEN_WIDTH - 40) * 0.42), sb_y + 15], fill=INK_BLACK)
    
    # Progress readouts
    draw.text((20, sb_y + 18), "Стр. 160 из 380 (42%)", font=fonts["tiny"], fill=INK_BLACK)
    draw.text((SCREEN_WIDTH // 2 - 40, sb_y + 18), "Осталось: ~1 ч 45 мин", font=fonts["tiny"], fill=INK_BLACK)
    draw.text((SCREEN_WIDTH - 85, sb_y + 18), "🔋 88% • 19:10", font=fonts["tiny"], fill=INK_BLACK)
    
    return img

def render_stats_screen(fonts):
    """Renders Reading Statistics Screen (ReadingStatsActivity) with weekly bar chart."""
    img = Image.new("RGB", (SCREEN_WIDTH, SCREEN_HEIGHT), INK_WHITE)
    draw = ImageDraw.Draw(img)
    
    draw_header_bar(draw, fonts, title="Статистика чтения", battery_pct=88, time_str="19:12")
    
    # Screen Header
    draw.text((20, 46), "АКТИВНОСТЬ ЗА НЕДЕЛЮ", font=fonts["small_bold"], fill=INK_DARK_GRAY)
    
    # Bar Chart Area
    chart_y = 70
    chart_h = 160
    chart_w = SCREEN_WIDTH - 40
    draw.rounded_rectangle([20, chart_y, 20 + chart_w, chart_y + chart_h], radius=8, outline=INK_LIGHT_GRAY, width=1)
    
    # Y-axis guidelines
    draw.line([30, chart_y + 35, 20 + chart_w - 10, chart_y + 35], fill=INK_LIGHT_GRAY, width=1)
    draw.text((26, chart_y + 22), "120 мин", font=fonts["tiny"], fill=INK_DARK_GRAY)
    draw.line([30, chart_y + 85, 20 + chart_w - 10, chart_y + 85], fill=INK_LIGHT_GRAY, width=1)
    draw.text((26, chart_y + 72), "60 мин", font=fonts["tiny"], fill=INK_DARK_GRAY)
    draw.line([30, chart_y + 135, 20 + chart_w - 10, chart_y + 135], fill=INK_BLACK, width=1)
    
    # Days data: (Day, Minutes)
    days_data = [
        ("Пн", 45),
        ("Вт", 65),
        ("Ср", 30),
        ("Чт", 90),
        ("Пт", 75),
        ("Сб", 115),
        ("Вс", 85)
    ]
    
    bar_width = 34
    spacing = (chart_w - 60 - len(days_data) * bar_width) // (len(days_data) - 1)
    
    for i, (day, mins) in enumerate(days_data):
        bx = 55 + i * (bar_width + spacing)
        # Scale: 120 mins = 100px
        bh = int((mins / 120.0) * 100)
        by = chart_y + 135 - bh
        
        # Draw bar
        if day == "Сб": # Highlight peak
            draw.rounded_rectangle([bx, by, bx + bar_width, chart_y + 135], radius=4, fill=INK_BLACK)
        else:
            draw.rounded_rectangle([bx, by, bx + bar_width, chart_y + 135], radius=4, outline=INK_BLACK, width=2)
            draw_dither_rect(draw, bx + 2, by + 2, bx + bar_width - 2, chart_y + 135 - 2, step=3)
            
        # Minutes on top
        draw.text((bx + 4, by - 16), f"{mins}м", font=fonts["tiny"], fill=INK_BLACK)
        # Day label below
        draw.text((bx + 8, chart_y + 140), day, font=fonts["small_bold"], fill=INK_BLACK)
        
    # --- SUMMARY METRICS CARDS ---
    cards_y = chart_y + chart_h + 20
    metrics = [
        ("⏱️ Общее время чтения", "48 ч 25 мин", "+3.5 ч на этой неделе"),
        ("📖 Прочитано страниц", "2,140 стр.", "В среднем 45 стр. в день"),
        ("📚 Завершено книг", "9 книг", "Цель на год: 25 книг"),
        ("🔥 Текущая серия", "7 дней подряд", "Лучшая серия: 21 день"),
        ("⚡ Средняя скорость", "224 слова / мин", "Выше среднего на 15%")
    ]
    
    c_h = 62
    for i, (m_title, m_val, m_sub) in enumerate(metrics):
        cy = cards_y + i * (c_h + 8)
        draw.rounded_rectangle([20, cy, SCREEN_WIDTH - 20, cy + c_h], radius=8, outline=INK_BLACK, width=1)
        draw.text((36, cy + 10), m_title, font=fonts["small_bold"], fill=INK_DARK_GRAY)
        draw.text((36, cy + 30), m_val, font=fonts["header"], fill=INK_BLACK)
        draw.text((SCREEN_WIDTH - 210, cy + 34), m_sub, font=fonts["tiny"], fill=INK_DARK_GRAY)
        
    # Footer
    draw.line([0, SCREEN_HEIGHT - 32, SCREEN_WIDTH, SCREEN_HEIGHT - 32], fill=INK_LIGHT_GRAY, width=1)
    draw.text((20, SCREEN_HEIGHT - 24), "Кнопка Home: Главное меню  •  Тап по карточке: Детали", font=fonts["tiny"], fill=INK_DARK_GRAY)
    
    return img

def render_device_frame(content_img):
    """Embeds 480x800 screen into Xteink X4 Pro hardware bezel with buttons."""
    device_img = Image.new("RGB", (DEVICE_WIDTH, DEVICE_HEIGHT), (35, 36, 40)) # Matte dark chassis
    draw = ImageDraw.Draw(device_img)
    
    # Outer device chassis with rounded corners
    draw.rounded_rectangle([0, 0, DEVICE_WIDTH, DEVICE_HEIGHT], radius=24, outline=(70, 72, 78), width=2)
    
    # Power Button (Top right side indicator)
    draw.rectangle([DEVICE_WIDTH - 48, 6, DEVICE_WIDTH - 20, 10], fill=(120, 122, 128))
    
    # Volume / Page turn buttons on left bezel
    draw.rounded_rectangle([2, 180, 8, 260], radius=3, fill=(100, 102, 108))
    draw.rounded_rectangle([2, 280, 8, 360], radius=3, fill=(100, 102, 108))
    
    # Screen inner bezel recess
    draw.rounded_rectangle([BEZEL_X - 2, BEZEL_TOP - 2, BEZEL_X + SCREEN_WIDTH + 2, BEZEL_TOP + SCREEN_HEIGHT + 2], radius=4, outline=(15, 15, 18), width=2)
    
    # Paste E-Ink display buffer
    device_img.paste(content_img, (BEZEL_X, BEZEL_TOP))
    
    # X4 Pro Capacitive Touch Home Button (Circle at bottom center)
    home_cx = DEVICE_WIDTH // 2
    home_cy = BEZEL_TOP + SCREEN_HEIGHT + (BEZEL_BOTTOM // 2)
    draw.ellipse([home_cx - 15, home_cy - 15, home_cx + 15, home_cy + 15], outline=(90, 92, 98), width=2)
    draw.ellipse([home_cx - 6, home_cy - 6, home_cx + 6, home_cy + 6], outline=(120, 122, 128), width=1)
    
    # Brand subtle logo
    draw.text((36, home_cy - 8), "XTEINK X4 PRO", font=ImageFont.truetype("C:/Windows/Fonts/arialbd.ttf", 11), fill=(100, 102, 108))
    draw.text((DEVICE_WIDTH - 110, home_cy - 8), "BookPoint 2.0", font=ImageFont.truetype("C:/Windows/Fonts/arial.ttf", 11), fill=(90, 92, 98))
    
    return device_img

def run_gui(screens, fonts, out_path):
    try:
        import tkinter as tk
        from PIL import ImageTk
    except ImportError as e:
        print(f"[!] Tkinter or ImageTk not available: {e}")
        return

    root = tk.Tk()
    root.title("Xteink X4 Pro (ESP32-S3) E-Ink Simulator - BookPoint 2.0.0")
    root.configure(bg="#222326")

    screen_keys = list(screens.keys())
    state = {"current_idx": 0, "tk_img": None}

    # Frame for controls
    ctrl_frame = tk.Frame(root, bg="#2b2c30", padx=10, pady=8)
    ctrl_frame.pack(side=tk.TOP, fill=tk.X)

    canvas_w = DEVICE_WIDTH
    canvas_h = DEVICE_HEIGHT
    canvas = tk.Canvas(root, width=canvas_w, height=canvas_h, bg="#1a1b1e", highlightthickness=0)
    canvas.pack(side=tk.TOP, padx=16, pady=12)

    status_label = tk.Label(ctrl_frame, text="", fg="#d0d0d0", bg="#2b2c30", font=("Arial", 11, "bold"))
    status_label.pack(side=tk.LEFT, padx=8)

    def update_screen():
        key = screen_keys[state["current_idx"]]
        label, render_func = screens[key]
        status_label.config(text=f"Экран: {label}")
        img = render_func(fonts)
        framed = render_device_frame(img)
        state["tk_img"] = ImageTk.PhotoImage(framed)
        canvas.delete("all")
        canvas.create_image(0, 0, anchor=tk.NW, image=state["tk_img"])

    def set_screen(idx):
        state["current_idx"] = idx % len(screen_keys)
        update_screen()

    def on_save():
        key = screen_keys[state["current_idx"]]
        label, render_func = screens[key]
        img = render_func(fonts)
        framed = render_device_frame(img)
        save_path = out_path / f"screenshot_{key}.png"
        framed.save(save_path)
        print(f"[+] Снимок сохранен: {save_path}")

    btn_style = {"bg": "#3e4046", "fg": "#ffffff", "activebackground": "#52545c", "activeforeground": "#ffffff", "font": ("Arial", 9), "relief": tk.FLAT, "padx": 6, "pady": 3}

    b_prev = tk.Button(ctrl_frame, text="◀ Пред.", command=lambda: set_screen(state["current_idx"] - 1), **btn_style)
    b_prev.pack(side=tk.LEFT, padx=4)
    b_next = tk.Button(ctrl_frame, text="След. ▶", command=lambda: set_screen(state["current_idx"] + 1), **btn_style)
    b_next.pack(side=tk.LEFT, padx=4)

    for i, (k, (lbl, _)) in enumerate(screens.items()):
        short_title = lbl.split("(")[0].strip().replace("Home ", "")
        b = tk.Button(ctrl_frame, text=short_title, command=lambda idx=i: set_screen(idx), **btn_style)
        b.pack(side=tk.LEFT, padx=3)

    b_save = tk.Button(ctrl_frame, text="📸 Сохранить", command=on_save, bg="#0d6efd", fg="#ffffff", activebackground="#0b5ed7", font=("Arial", 9, "bold"), relief=tk.FLAT, padx=8, pady=3)
    b_save.pack(side=tk.RIGHT, padx=4)

    def on_canvas_click(event):
        home_cx = DEVICE_WIDTH // 2
        home_cy = BEZEL_TOP + SCREEN_HEIGHT + (BEZEL_BOTTOM // 2)
        dist_sq = (event.x - home_cx) ** 2 + (event.y - home_cy) ** 2
        if dist_sq <= 25 ** 2:
            set_screen(0)
            return

        if BEZEL_X <= event.x < BEZEL_X + SCREEN_WIDTH and BEZEL_TOP <= event.y < BEZEL_TOP + SCREEN_HEIGHT:
            tx = event.x - BEZEL_X
            ty = event.y - BEZEL_TOP
            if state["current_idx"] == 0:
                if 450 < ty < 510:
                    set_screen(2)
                elif 510 <= ty < 570:
                    set_screen(4)
                elif ty < 334:
                    set_screen(3)
            elif state["current_idx"] == 1:
                if ty > 420:
                    set_screen(2)
                elif 140 <= tx <= 340 and ty < 320:
                    set_screen(3)
                elif tx < 140:
                    set_screen(0)
                elif tx > 340:
                    set_screen(2)

    canvas.bind("<Button-1>", on_canvas_click)
    root.bind("<Left>", lambda e: set_screen(state["current_idx"] - 1))
    root.bind("<Right>", lambda e: set_screen(state["current_idx"] + 1))
    root.bind("<Escape>", lambda e: set_screen(0))
    root.bind("<h>", lambda e: set_screen(0))
    root.bind("<H>", lambda e: set_screen(0))

    update_screen()
    root.mainloop()

def main():
    parser = argparse.ArgumentParser(description="Xteink X4 Pro E-Ink 800x480 Screen Simulator")
    parser.add_argument("--render-all", action="store_true", help="Render all screen shots to output dir")
    parser.add_argument("--out-dir", type=str, default="screenshots", help="Directory to save screenshots")
    parser.add_argument("--with-frame", action="store_true", help="Include physical device frame in output")
    parser.add_argument("--gui", action="store_true", help="Launch interactive Tkinter desktop simulator")
    args = parser.parse_args()

    fonts = get_fonts()
    out_path = Path(args.out_dir)
    out_path.mkdir(parents=True, exist_ok=True)

    screens = {
        "01_home_dashboard": ("Home Dashboard (DashboardTheme)", render_dashboard_screen),
        "02_home_carousel": ("Home Lyra Carousel (LyraCarouselTheme)", render_carousel_screen),
        "03_book_search": ("Book Search with Touch Keyboard", render_search_screen),
        "04_reader_epub": ("EPUB Reader with Dual Status Bar", render_reader_screen),
        "05_reading_stats": ("Reading Statistics & Weekly Chart", render_stats_screen),
    }

    if args.gui:
        run_gui(screens, fonts, out_path)
    else:
        print(f"[*] Rendering {len(screens)} screens to: {out_path.resolve()}")
        for key, (label, render_func) in screens.items():
            screen_img = render_func(fonts)
            raw_file = out_path / f"{key}_eink.png"
            screen_img.save(raw_file, "PNG")
            print(f"  [+] Saved: {raw_file.name} ({screen_img.size[0]}x{screen_img.size[1]})")

            framed_img = render_device_frame(screen_img)
            frame_file = out_path / f"{key}_device.png"
            framed_img.save(frame_file, "PNG")
            print(f"  [+] Saved: {frame_file.name} ({framed_img.size[0]}x{framed_img.size[1]})")

        print("[*] All screenshots successfully generated!")

if __name__ == "__main__":
    main()

