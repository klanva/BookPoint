# -*- coding: utf-8 -*-
# Анализ выгрузки темы 4PDA "[X]Помощник" (9939 постов).
# 1) когда впервые упомянут X4 Pro; 2) с этого момента — что говорят о прошивках;
# 3) жалобы/пожелания; 4) упоминания BookPoint.
import io, re, json, collections

SRC = r'C:\Users\andre\Downloads\[X]Помощник_9939_posts.txt'
OUT = r'C:\xteinkx4\TOPIC_RESEARCH.md'

text = io.open(SRC, encoding='utf-8', errors='ignore').read()

# --- парсинг постов ---
post_re = re.compile(
    r'^\[#(\d+)\] \(\#?\d*\) (\d{2}\.\d{2}\.\d{2}), (\d{2}:\d{2}) \| Автор: (.+)$',
    re.M)
url_re = re.compile(r'^Ссылка: (\S+)', re.M)
SEP_LINE = '-' * 80
BIG_SEP = '=' * 80

# найдём все заголовки постов с позициями
heads = list(post_re.finditer(text))
posts = []
for i, m in enumerate(heads):
    start = m.end()
    end = heads[i + 1].start() if i + 1 < len(heads) else len(text)
    block = text[start:end]
    um = url_re.search(block)
    url = um.group(1) if um else ''
    # тело: после строки-разделителя
    sep_idx = block.find(SEP_LINE)
    if sep_idx != -1:
        body = block[sep_idx + len(SEP_LINE):]
    else:
        body = block
    # убираем хвостовой большой сепаратор
    body = body.replace(BIG_SEP, '').strip()
    posts.append({
        'num': int(m.group(1)),
        'date': m.group(2),
        'time': m.group(3),
        'author': m.group(4).strip(),
        'url': url,
        'text': body,
    })

print('parsed posts:', len(posts))
if len(posts) < 100:
    raise SystemExit('парсер сломался — слишком мало постов')

# --- дата в ключ для сортировки/сравнения (DD.MM.YY) ---
def dkey(d):
    dd, mm, yy = d.split('.')
    return (2000 + int(yy), int(mm), int(dd))

# --- словарь прошивок: синонимы ---
FIRMWARES = {
    'CrossPoint': [r'кросспойнт', r'кросспоинт', r'crosspoint', r'кросспоинт', r'кросспонт'],
    'inkMOD': [r'инкмод', r'inkmod', r'ink mod'],
    'Papyrix': [r'папирикс', r'papyrix', r'папирих'],
    'CrossInk': [r'кроссинк', r'crossink', r'крос инк'],
    'CrumBLE': [r'crumble', r'крамбл'],
    'vCodex': [r'vcodex', r'вкодекс', r'cpr-vcodex'],
    'WitchHunt': [r'witchhunt', r'witch hunt', r'вичхант', r'витчхант'],
    'InkPointX': [r'inkpointx', r'inkpoint', r'инкпоинт'],
    'XTOS': [r'xtos'],
    'BookPoint': [r'bookpoint', r'букпоинт', r'бук пойнт'],
    'Snapix': [r'snapix', r'снапикс'],
    'MicroReader': [r'microreader', r'микроридер'],
    'YACP': [r'yacp'],
    'biscuit': [r'biscuit', r'бисквит'],
}
FW_RE = {name: re.compile('|'.join(pats), re.I) for name, pats in FIRMWARES.items()}

# --- упоминания X4 Pro ---
PRO_RE = re.compile(r'x\s*4\s*pro|x4pro|четыре\s+про|4\s*про', re.I)
pro_posts = [p for p in posts if PRO_RE.search(p['text'])]
print('X4 Pro mentions:', len(pro_posts))
first_pro = pro_posts[0] if pro_posts else None
if first_pro:
    print('first X4 Pro mention: #%d, %s, автор %s' % (first_pro['num'], first_pro['date'], first_pro['author']))

# --- посты с упоминанием прошивок ---
fw_posts = []
for p in posts:
    hits = [name for name, rx in FW_RE.items() if rx.search(p['text'])]
    if hits:
        fw_posts.append((p, hits))

print('posts mentioning any firmware:', len(fw_posts))

# ---.timeline: упоминания прошивок по месяцам ---
month_stats = collections.defaultdict(collections.Counter)
for p, hits in fw_posts:
    dd, mm, yy = p['date'].split('.')
    month = '20' + yy + '-' + mm
    for h in set(hits):
        month_stats[month][h] += 1

# --- срез после первого X4 Pro ---
if first_pro:
    cutoff = dkey(first_pro['date'])
    after = [p for p in posts if dkey(p['date']) >= cutoff]
else:
    after = posts
print('posts after first X4 Pro mention:', len(after))

after_fw = [(p, hits) for p, hits in fw_posts if (not first_pro or dkey(p['date']) >= cutoff)]
after_counter = collections.Counter()
for p, hits in after_fw:
    for h in set(hits):
        after_counter[h] += 1
print('mentions after cutoff:', dict(after_counter))

# --- жалобы/пожелания: посты с негативными/просительными паттернами ---
complaint_re = re.compile(r'жрёт|жрет|батарея|разряд|автономность|виснет|зависает|лагает|тормоз|глючит|гостинг|выцвет|медленно|долго грузит|перезагружа|краш|пал')
wish_re = re.compile(r'хотел(ось)? бы|добавьте|добавить бы|не хватает|сделайте|нужна поддержка|предлагаю|просят|добавлена фича|пожелание')
fb2_re = re.compile(r'fb2|фб2', re.I)

complaints = [p for p in after if complaint_re.search(p['text'])]
wishes = [p for p in after if wish_re.search(p['text'])]
fb2_after = [p for p in after if fb2_re.search(p['text'])]

# --- BookPoint упоминания ---
bp_posts = [p for p in posts if FW_RE['BookPoint'].search(p['text'])]

# --- отчёт ---
out = []
out.append('# Ресёрч темы [X]Помощник (4PDA) — 9939 постов, 22.02.26 – 12.09.26')
out.append('')
out.append('## 1. Когда появился X4 Pro')
out.append('')
if first_pro:
    out.append('Первое упоминание X4 Pro в теме: пост #%d от %s (автор %s).' % (first_pro['num'], first_pro['date'], first_pro['author']))
    # первые 3 упоминания с коротким контекстом
    out.append('')
    out.append('Первые упоминания:')
    for p in pro_posts[:5]:
        idx = p['text'].lower().find('pro')
        ctx = p['text'][max(0, idx - 120): idx + 150].replace('\n', ' ')
        out.append('- #%d (%s, %s): …%s…' % (p['num'], p['date'], p['author'], ctx))
out.append('')
out.append('Всего постов с упоминанием X4 Pro: %d из %d.' % (len(pro_posts), len(posts)))
out.append('')

out.append('## 2. Прошивки: о чём говорят с момента появления X4 Pro')
out.append('')
out.append('Постов в этот период: %d. Упоминания прошивок:' % len(after))
out.append('')
out.append('| Прошивка | Упоминаний |')
out.append('|---|---|')
for name, cnt in after_counter.most_common():
    out.append('| %s | %d |' % (name, cnt))
out.append('')

# --- последние 40 постов с прошивками — полный список для ручного чтения ---
out.append('## 3. Посты с обсуждением прошивок (после появления X4 Pro), по датам')
out.append('')
for p, hits in after_fw:
    out.append('**#%d (%s, %s)** — %s' % (p['num'], p['date'], p['author'], ', '.join(sorted(set(hits)))))
    # компактно: до 500 символов текста
    t = re.sub(r'\s+', ' ', p['text'])[:500]
    out.append('> ' + t)
    out.append('')

out.append('## 4. Жалобы и проблемы (после появления X4 Pro): %d постов' % len(complaints))
out.append('')
for p in complaints[:60]:
    t = re.sub(r'\s+', ' ', p['text'])[:220]
    out.append('- #%d (%s): %s' % (p['num'], p['date'], t))
out.append('')

out.append('## 5. Пожелания/запросы фич: %d постов' % len(wishes))
out.append('')
for p in wishes[:60]:
    t = re.sub(r'\s+', ' ', p['text'])[:220]
    out.append('- #%d (%s): %s' % (p['num'], p['date'], t))
out.append('')

out.append('## 6. FB2-упоминания после появления X4 Pro: %d' % len(fb2_after))
out.append('')
for p in fb2_after[:30]:
    t = re.sub(r'\s+', ' ', p['text'])[:200]
    out.append('- #%d (%s): %s' % (p['num'], p['date'], t))
out.append('')

out.append('## 7. Упоминания BookPoint: %d' % len(bp_posts))
out.append('')
for p in bp_posts[:30]:
    t = re.sub(r'\s+', ' ', p['text'])[:300]
    out.append('- #%d (%s, %s): %s' % (p['num'], p['date'], p['author'], t))
out.append('')

io.open(OUT, 'w', encoding='utf-8', newline='\n').write('\n'.join(out))
print('report written:', OUT)

# краткая статистика в консоль
print('---')
print('first pro:', first_pro and (first_pro['num'], first_pro['date']))
print('after cutoff posts:', len(after))
print('fw mentions after:', dict(after_counter.most_common()))
print('complaints:', len(complaints), 'wishes:', len(wishes), 'fb2:', len(fb2_after), 'bookpoint mentions:', len(bp_posts))
