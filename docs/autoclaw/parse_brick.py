import re, io, sys, gzip, csv
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
t = open('.openclaw/tmp/brick_wb_dec.html', encoding='utf-8').read()
for m in re.finditer(r'fetch\(([^)]+)\)', t):
    print('FETCH:', m.group(1))
urls = re.findall(r'https?://[^\s"\'<>)]+', t)
for u in set(urls):
    if 'cloudflare' not in u and 'creativecommons' not in u:
        print('URL:', u[:200])
# find array constants with firmware rows
m = re.search(r'(const|let|var)\s+(github\w*|other\w*|data\w*|rows\w*)\s*=\s*[\[\[]', t)
print('CONST-HIT:', m.group(0) if m else None)
# search for known firmware names in the html
for name in ['CrossPoint','CrossInk','CrossMux','Witch','Papyrix','CrumBLE','sumi','shrike','microreader','Inx','InkPoint','CrossPet','incMOD','YACP','snapix','biscuit','Codex','BLE']:
    hits = len(re.findall(name, t, re.I))
    print(name, '->', hits)
