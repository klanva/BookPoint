# BookPoint — Research: свежие прошивки Xteink X3/X4 (сентябрь 2026)

Склонировано на диск: `C:\xteinkx4\firmwares\` (18 репозиториев, свежие ветки).

## 1. Инвентарь прошивок

| Прошивка | Репозиторий | Версия | Посл. коммит | FB2 | Фокус |
|---|---|---|---|---|---|
| CrossPoint (апстрим) | crosspoint-reader/crosspoint-reader | 1.5.0 | 03.09.2026 | нет | база, эталон |
| BookPoint (ТВОЙ форк) | klanva/BookPoint | 1.6.5 | 30.08.2026 | нет | твой проект |
| CrossInk | uxjulia/crossink | 1.5.0 | 18.08.2026 | да | типографика, CJK, FB2/MD |
| CrossInk-TRMNL (CrossXT) | yazdipour/CrossInk-TRMNL | 1.5.0 | 08.08.2026 | нет | типографика + лёгкая статистика |
| CrossMux | 0x1abin/crossmux | 1.5.8 | 04.09.2026 | да | хаб приложений/мини-игр |
| Witch(hunt) Reader | jpirnay/witchhunt-reader | 2.26 | 03.09.2026 | нет | лучший рендер EPUB (small caps и т.п.) |
| Papyrix | bigbag/papyrix-reader | 1.28.2 | 28.08.2026 | да | лёгкий, FB2/MD/TXT, темы/шрифты |
| snapix | unbrokendub/snapix | 3.11.11 | 29.07.2026 | да | быстрый форк Papyrix |
| CrumBLE | imshentastic/CrumBLE | 1.3.0 | 07.08.2026 | нет | BLE, словарь, цитаты, EPUB-оптимизатор |
| crosspoint-reader-ble | thedrunkpenguin/crosspoint-reader-ble | 1.2-ble | 08.04.2026 | нет | BLE |
| CPR-vCodex | franssjz/cpr-vcodex | 1.5.0 | 03.09.2026 | да | ИИ-мод, привычки чтения, статистика |
| CPR-vCodex-steroids | marcoand75/cpr-vcodex-steroids | 1.5.0 | 02.09.2026 | да | надстройка над vCodex |
| Inx | obijuankenobiii/inx | 1.0.19 | 08.08.2026 | нет | чище EPUB, картинки |
| InkPointX | yokki-vans/InkPointX | 2.2.8 | 08.08.2026 | да | форк Crosspoint+CrossInk |
| CrossPet | trilwu/crosspet | 1.8.4 | 26.04.2026 | нет | виртуальная курица, VN |
| incMOD | alpzoloto-sudo/inkmod | 1.1.7 | 03.09.2026 | да | богатейшая статистика, клиппинги |
| YACP | Sichroteph/YACP | 1.3.0 | 03.09.2026 | нет | батарея + эффективность рендера |
| microreader | CidVonHighwind/microreader | — | 29.07.2026 | нет | микро-ридер |
| biscuit | yattsu/biscuit | 0.1.0 | 08.04.2026 | нет | pentest-инструмент |
| SUMI | (не на GitHub) | — | — | — | sumi.page: EPUB с картинками/таблицами, игры, флешкарты |
| shrike reader | (не найден на GitHub) | — | — | — | OTA+SD, но не шьётся |

## 2. Матрица ключевых фич

| Прошивка | серия чтения | остаток времени | синхр. часов | клиппинги | экспорт статистики |
|---|---|---|---|---|---|
| cpr-vcodex / -steroids | да | да | да | да | да |
| crossink / CrossInk-TRMNL | да | да | да | да | да |
| CrumBLE | да | да | да | да | да |
| **incMOD** | да | да | да | да | да |
| InkPointX | да | да | да | да | да |
| YACP | да | да | да | да | да |
| crossmux | да | да | нет | да | да |
| witchhunt-reader | да | нет | нет | да | да |
| crosspet | да | нет | нет | нет | нет |
| crosspoint-reader (база) | нет | нет | да | да | нет |
| inx / biscuit / ble | нет | да* | нет | нет | нет |
| papyrix / snapix / microreader | нет | нет | нет | да* | да* |

*частично.

## 3. Ключевой вывод

Самые богатые по статистике/чтению — это семейство форков CrossInk/incMOD:
все они несут один и тот же "reading analytics suite" (серия дней, остаток времени,
прогноз даты финиша, разбивка по времени суток/дням недели, клиппинги, экспорт).

## 4. Что уже есть в BookPoint (твой форк)

- Статистика чтения: ReadingStatsTypes / BookReadingStats / GlobalReadingStats / StatsStore / ReadingStatsActivity (3 вкладки) — УЖЕ ЕСТЬ.
- Серия чтения: GlobalReadingStats::currentReadingStreak / longestReadingStreak — УЖЕ ЕСТЬ.
- Остаток времени: EpubReaderActivity::openStats (темп + fallback) — УЖЕ ЕСТЬ (в т.ч. незакоммиченный fallback).
- Часы X4: HalClock + NTP-ресинк (дрейф-фикс) — УЖЕ НАПИСАН, но НЕ закоммичен (7 изменённых файлов).

## 5. Чего НЕ хватает (кандидаты на перенос, лёгкие, без FB2)

1. Экспорт/бэкап статистики (StatsBackup из incMOD/cpr-vcodex) — Tier 1.
2. Прогноз "закончишь к <дате>" (estimateFinishDateFromDailyPace из incMOD BookStatsView) — Tier 1.
3. Клиппинги/цитаты (ClippingStore + "My Clippings" из incMOD) — Tier 2 (самый "вау", ~4-6 КБ).
4. Точная синхронизация часов + дата в экране синхронизации — уже написано, нужно собрать и закоммитить.

Пропустить: FB2, BLE-синк статистики, хаб игр, pentest-модули, KOReader-авторизация.