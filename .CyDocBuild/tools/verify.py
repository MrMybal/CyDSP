"""Verify the complete CyDSP CyDoc export without executing DSP code."""
from pathlib import Path
from html.parser import HTMLParser
import hashlib,json,re
WORK=Path(__file__).resolve().parents[1]; ROOT=WORK.parent; OUT=ROOT/'Documentation'
class Links(HTMLParser):
    def __init__(self):super().__init__();self.links=[]
    def handle_starttag(self,tag,attrs):self.links += [(k,v) for k,v in attrs if k in ('href','src') and v]
def main():
    config=json.loads((WORK/'cydoc.config.json').read_text(encoding='utf-8-sig'))
    data=json.loads((OUT/'DocData/manifest.json').read_text(encoding='utf-8'))
    version=re.search(r'project\(CyDSP VERSION (\S+)',(ROOT/'CMakeLists.txt').read_text()).group(1)
    assert data['version']==config['version']==version
    assert data['defaultLocale']=='en' and {x['code'] for x in data['locales']}=={'en','fr'}
    pages={(p['locale'],p['id']):p for p in data['pages']}
    en={p['id'] for p in data['pages'] if p['locale']=='en'}
    fr={p['id'] for p in data['pages'] if p['locale']=='fr'}
    assert en==fr and len(en)==140,(len(en),len(fr))
    links=0;headings=0
    for id in en:
        a=pages['en',id];b=pages['fr',id]
        assert a['text'].strip() and b['text'].strip()
        assert a['parent'] == b['parent'] if 'parent' in a else 'parent' not in b
        assert {h['id'] for h in a['headings']}=={h['id'] for h in b['headings']},id
        headings+=len(a['headings'])+len(b['headings'])
        assert a.get('revision')==b.get('sourceRevision'),id
        for p in (a,b):
            assert p.get('kind')!='source' and not p.get('sourcePageId'),id
            assert not any(marker in p['text'] for marker in ('\u00c3', '\u00e2\u20ac', '\u00c2\u00a9', '\u00c2\u00b7')), ('Invalid UTF-8 text',id)
            assert not any(x in p['html'] for x in ('D:\\_Project','Source Files','separation-*','Migration.fr.md'))
            html=Links();html.feed(p['html'])
            for attr,target in html.links:
                if target.startswith('#/'):
                    parts=target[2:].split('/'); assert len(parts)>=2,(id,target)
                    locale,dest=parts[:2]; assert (locale,dest) in pages,(id,target)
                    if len(parts)>2: assert parts[2] in {x['id'] for x in pages[locale,dest]['headings']} or parts[2] in pages[locale,dest].get('anchors',[]),(id,target)
                    links+=1
                elif target.startswith('doc:'):
                    dest=target[4:].split('#')[0].split('?')[0];assert (p['locale'],dest) in pages,(id,target);links+=1
                elif target.startswith(('http:','https:','mailto:','#')):continue
                elif target.startswith('assets/'):
                    assert (OUT/'DocData'/target).is_file(),target
    catalog=json.loads((WORK/'api/catalog.json').read_text())
    headers=list((ROOT/'include/cydsp').rglob('*.h'));assert len(headers)==len(catalog['headers'])==88
    def count(items):return sum(1+count(i.get('members',[])) for i in items)
    declarations=sum(count(h['symbols']) for h in catalog['headers'])
    for h in catalog['headers']:
        assert hashlib.sha256((ROOT/'include'/h['header']).read_bytes()).hexdigest()==h['sha256']
        def public(items):
            for s in items:
                assert not re.search(r'\b(return|private|protected)\b',s['signature']),s['name']
                if 'members' in s:public(s['members'])
        public(h['symbols'])
    html=Links();html.feed((OUT/'Documentation.html').read_text())
    for attr,target in html.links:
        assert not re.match(r'^(https?:)?//',target),target
        assert (OUT/target).is_file(),target
    assert not re.search(r'<script[^>]+type=["\']module',(OUT/'Documentation.html').read_text())
    assert (OUT/'DocAI.md').is_file()
    assert all(p['id'].lower()!='docai' for p in data['pages'])
    assert not any(p.name in ('node_modules','src','include','.CyDocBuild') for p in OUT.rglob('*'))
    for name in ('CyDSP-MIT.txt','CyDoc-MIT.txt','DSPFilters.txt','FFTConvolver.txt','r8brain-free-src.txt','signalsmith-linear.txt','signalsmith-stretch.txt'):
        assert (OUT/'DocData/assets/notices'/name).is_file(),name
    report=json.loads((OUT/'DocData/translation-report.json').read_text())
    assert not report.get('missing') and not report.get('outdated') and not report.get('untracked'),report
    print(f'PASS: {len(data["pages"])} pages, {len(en)} English/French pairs, {len(headers)} headers, {declarations} public declarations, {links} internal links, {headings} paired section anchors; local scripts/assets/notices')
if __name__=='__main__':main()
