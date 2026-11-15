"""Render authored guides and public declaration references into CyDoc Markdown."""
from pathlib import Path
import hashlib, json, re, shutil
from guides import PAGES
from header_notes import HEADER_NOTES
WORK=Path(__file__).resolve().parents[1]; ROOT=WORK.parent
GROUPS={
'core':('Core and storage','Socle et stockage'),
'math':('Numerical kernels','Noyaux numériques'),
'control':('Control and quality','Contrôle et qualité'),
'processors':('Audio processors','Processeurs audio'),
'spectral':('Spectral processing','Traitement spectral'),
'synthesis':('Sound generation','Génération sonore'),
'spatial':('Spatial audio','Audio spatial'),
'analysis':('Audio analysis','Analyse audio')}

def slug(value):return re.sub(r'[^a-z0-9]+','-',value.lower()).strip('-')
def api_id(header):return 'api-'+slug(header.removeprefix('cydsp/').removesuffix('.h'))
def link(label,id):return f'[{label}](doc:{id})'
def esc(value):return str(value).replace('|','\\|').replace('\n',' ')
def code(value):return '`'+esc(value)+'`'
def write(id,titles,descs,bodies,parent=None,order=0,symbol=None):
    for i,locale in enumerate(('en','fr')):
        front={'id':id,'title':titles[i],'description':descs[i],'section':'Guides' if not id.startswith('api') else ('API reference' if i==0 else 'Référence API'),'order':order,'revision':1}
        if i:front.pop('revision');front['sourceRevision']=1
        if parent:front['parent']=parent
        if symbol:front['symbol']=symbol
        text='---\n'+'\n'.join(f'{k}: {v}' for k,v in front.items())+'\n---\n\n'+bodies[i].strip()+'\n'
        path=WORK/'manual'/locale/(id+'.md');path.write_text(text,encoding='utf-8',newline='\n')

def meaning(name,typ,i):
    n=name.lower()
    pairs=[
      (n.endswith('hz') or n in ('frequency','samplerate','rate'),('Frequency in Hz; use the supported sample-rate/Nyquist range.','Fréquence en Hz ; respecter la plage de taux/Nyquist autorisée.')),
      (n.endswith('seconds'),('Duration or position in seconds.','Durée ou position en secondes.')),
      (n.endswith('ms'),('Time in milliseconds.','Durée en millisecondes.')),
      (n.endswith('db') or n.endswith('dbtp'),('Logarithmic level in the indicated dB reference.','Niveau logarithmique selon la référence dB indiquée.')),
      (n.endswith('radians'),('Angle in radians.','Angle en radians.')),
      (n.endswith('metrespersecond'),('Velocity in metres per second.','Vitesse en mètres par seconde.')),
      (n.endswith('metres'),('Length in metres.','Longueur en mètres.')),
      (n.endswith('frames') or n=='frameoffset',('Frame count or offset; one frame contains one sample per channel.','Compte ou offset de frames ; une frame contient un échantillon par canal.')),
      (n in ('channel','channels','inputchannel','outputchannel'),('Channel index/count in the declared planar layout.','Indice/compte de canaux dans la disposition planaire déclarée.')),
      ('AudioBlock<const' in typ,('Borrowed read-only planar audio; keep samples and pointer table alive.','Audio planaire emprunté en lecture seule ; conserver échantillons et tableau de pointeurs.')),
      ('AudioBlock<' in typ,('Borrowed planar audio view; its sample constness determines writable access.','Vue audio planaire empruntée ; la constance des échantillons détermine l’accès en écriture.')),
      ('ProcessSpec' in typ,('Prepared rate, maximum block size and channel count.','Taux, bloc maximal et nombre de canaux préparés.')),
      ('CpuBackend' in typ,('Compiled CPU/OS-supported kernel choice; automatic selects during preparation.','Choix de noyaux compilés disponibles pour CPU/OS ; sélection automatique à la préparation.')),
      ('span<' in typ,('Borrowed contiguous range with the element type and constness shown; preserve its lifetime.','Plage contiguë empruntée de type/constance indiqués ; conserver sa durée de vie.')),
      (n in ('gain','outputgain','drygain','wetgain','wet','dry'),('Linear amplitude multiplier; follow the processor bounds and mixing law.','Multiplicateur d’amplitude linéaire ; respecter bornes et loi de mixage du processeur.')),
      ('void *' in typ or 'void*' in typ,('Borrowed callback context; the application owns lifetime and synchronization.','Contexte de callback emprunté ; l’application possède durée de vie et synchronisation.')),
      (n in ('seed',),('Per-instance deterministic random seed.','Graine aléatoire déterministe par instance.')),
      ('bool'==typ,('Explicit true/false option or predicate.','Option ou prédicat vrai/faux explicite.')),
      (n=='q',('Positive finite filter quality factor.','Facteur de qualité du filtre positif et fini.')),
    ]
    for match,pair in pairs:
        if match:return pair[i]
    if '&' in typ and 'const' not in typ:
        return ('Mutable reference; inspect the declaration and guide for output/state ownership.','Référence mutable ; la déclaration et le guide précisent la propriété de sortie/état.')[i]
    return ('Value/configuration in the declared type; apply the linked module contract.','Valeur/configuration du type déclaré ; appliquer le contrat du module lié.')[i]

def method_note(s,i):
    name=s['name'].split('::')[-1]
    if s.get('deleted'):return ('This operation is explicitly deleted; do not copy this owning object.','Cette opération est explicitement supprimée ; ne pas copier cet objet propriétaire.')[i]
    if s['kind']=='CXXConstructorDecl':return ('Construct an instance; prepare it before processing when the API requires preparation.','Construire une instance ; la préparer avant traitement lorsque son API exige une préparation.')[i]
    if s['kind']=='CXXDestructorDecl':return ('Destroy only when no processing or borrowed users remain; destruction may release storage.','Détruire seulement après arrêt des traitements/utilisateurs empruntés ; la destruction peut libérer du stockage.')[i]
    if name=='prepare' or name.startswith('prepare'):
        return ('Prepare outside the audio callback; check Status before using the new configuration.','Préparer hors callback audio ; vérifier Status avant d’utiliser la nouvelle configuration.')[i]
    if name in ('process','processMixed','processGain','render','next'):
        return ('Advances signal state for the supplied samples. Respect prepared dimensions, alias rules and owner serialization.','Avance l’état du signal pour les échantillons fournis. Respecter dimensions préparées, alias autorisés et sérialisation du propriétaire.')[i]
    if name=='reset':return ('Clear/restart signal history while retaining prepared storage; parameters and transition behavior follow the module contract.','Effacer/redémarrer l’historique en conservant le stockage préparé ; paramètres et transitions suivent le contrat du module.')[i]
    if name in ('flush','finish'):return ('Drain the documented tail and inspect completion; reset before starting a new stream.','Vider la queue documentée et vérifier la fin ; réinitialiser avant un nouveau flux.')[i]
    if name.startswith('stage'):return ('Stage outside realtime processing; retained assets/contexts must satisfy the transition lifetime.','Préparer le stage hors traitement temps réel ; respecter la durée de vie des données/contextes de transition.')[i]
    if name.startswith('set') or name in ('trigger','start','noteOn','noteOff'):
        return ('Apply control on the processing owner between calls, or under the explicit producer contract. Check any returned Status.','Appliquer le contrôle sur le propriétaire entre appels, ou selon le contrat producteur explicite. Vérifier le Status éventuel.')[i]
    return ('Use the declared inputs and returned type with the module contract below; getters do not synchronize access.','Utiliser les entrées et le type retourné selon le contrat du module lié ; les getters ne synchronisent pas les accès.')[i]

def render_symbol(s,i,anchors,level=2):
    name=s['name']; short=name.removeprefix('cydsp::'); base='symbol-'+slug(short); n=anchors.get(base,0);anchors[base]=n+1;anchor=base+('-'+str(n+1) if n else '')
    out=f"{'#'*level} {code(short)} {{#{anchor}}}\n\n```cpp\n{s['signature']}\n```\n\n"
    if 'members' in s:
        fields=[m for m in s['members'] if m['kind'] in ('FieldDecl','VarDecl')]
        if fields:
            labels=('Field | Type | Default | Meaning','Champ | Type | Défaut | Sens')[i]
            out+='| '+labels+' |\n| --- | --- | --- | --- |\n'
            for f in fields:out+='| '+code(f['name'].split('::')[-1])+' | '+code(f['type'])+' | '+(code(f['default']) if f['default'] else '—')+' | '+meaning(f['name'].split('::')[-1],f['type'],i)+' |\n'
            out+='\n'
        for member in s['members']:
            if member['kind'] not in ('FieldDecl','VarDecl'):out+=render_symbol(member,i,anchors,min(level+1,5))
    elif 'parameters' in s:
        out+=method_note(s,i)+'\n\n'
        params=s['parameters']
        if params:
            out+='| '+('Parameter | Type | Default | Meaning','Paramètre | Type | Défaut | Sens')[i]+' |\n| --- | --- | --- | --- |\n'
            for index,p in enumerate(params):
                label=p['name'] or ('argument '+str(index+1))
                out+='| '+code(label)+' | '+code(p['type'])+' | '+(code(p['default']) if p['default'] else '—')+' | '+meaning(p['name'],p['type'],i)+' |\n'
            out+='\n'
        ret=s.get('returnType','')
        if ret and ret!='void':
            description=('Check Status; ok reports success.','Vérifier Status ; ok indique le succès.')[i] if ret.endswith('Status') else ('Returned value/view; respect borrowed lifetime and the units named by this API.','Valeur/vue retournée ; respecter la durée de vie empruntée et les unités indiquées par l’API.')[i]
            out+=('Return: ','Retour : ')[i]+code(ret)+' — '+description+'\n\n'
    return out

def main():
    catalog=json.loads((WORK/'api/catalog.json').read_text(encoding='utf-8'))
    assert len(catalog['headers'])==len(list((ROOT/'include/cydsp').rglob('*.h')))
    for h in catalog['headers']:
        assert hashlib.sha256((ROOT/'include'/h['header']).read_bytes()).hexdigest()==h['sha256'], 'Stale API catalog: '+h['header']
        assert Path(h['header']).stem in HEADER_NOTES,h['header']
    for order,p in enumerate(PAGES):
        bodies=list(p['body'])
        headers=[h for h in catalog['headers'] if HEADER_NOTES[Path(h['header']).stem][2]==p['id']]
        for i in range(2):
            if headers:bodies[i]+='\n\n## '+('Public API','API publique')[i]+' {#api}\n\n'+'\n'.join('- '+link(h['header'],api_id(h['header'])) for h in headers)
        write(p['id'],p['title'],p['description'],bodies,p['parent'],order)
    api_bodies=[]
    for i in range(2):
        body=('Choose a module below. Each header page contains its public types, fields, method signatures, parameters and defaults. Implementation bodies and private members are excluded. Unnamed C++ parameters are identified by their positional argument number.','Choisissez un module ci-dessous. Chaque page d’en-tête contient types publics, champs, signatures de méthodes, paramètres et défauts. Corps d’implémentation et membres privés sont exclus. Les paramètres C++ non nommés sont identifiés par leur numéro de position.')[i]
        body+='\n\n## '+('Modules','Modules')[i]+' {#modules}\n\n'+'\n'.join('- '+link(t[i],'api-'+group) for group,t in GROUPS.items())
        body+='\n- '+link('cydsp/CyDSP.h',api_id('cydsp/CyDSP.h'))
        body+='\n\n'+link(('Processing contracts','Contrats de traitement')[i],'contracts')
        api_bodies.append(body)
    write('api',('Public C++ API','API C++ publique'),('Every public header and its declarations.','Tous les en-têtes publics et leurs déclarations.'),api_bodies,None,1000)
    for order,(group,titles) in enumerate(GROUPS.items()):
        headers=[h for h in catalog['headers'] if h['header'].split('/')[1]==group]
        bodies=[]
        for i in range(2):
            body='| '+('Header | Purpose','En-tête | Rôle')[i]+' |\n| --- | --- |\n'
            body+='\n'.join('| '+link(Path(h['header']).name,api_id(h['header']))+' | '+HEADER_NOTES[Path(h['header']).stem][i]+' |' for h in headers)
            bodies.append(body)
        write('api-'+group,titles,titles,bodies,'api',1001+order)
    for order,h in enumerate(catalog['headers']):
        stem=Path(h['header']).stem;note=HEADER_NOTES[stem];group=h['header'].split('/')[1]; parent='api-'+group if group in GROUPS else 'api';bodies=[]
        for i in range(2):
            body=note[i]+'\n\n## '+('Include','Inclusion')[i]+' {#include}\n\n```cpp\n#include <'+h['header']+'>\n```\n\n'
            body+=link(('Usage and contracts','Utilisation et contrats')[i],note[2])+' · '+link(('Common processing rules','Règles communes de traitement')[i],'contracts')+'\n\n'
            if not h['symbols']:body+=('This umbrella header includes every public CyDSP module. Use individual headers to reduce inclusion costs.','Cet en-tête regroupe tous les modules publics CyDSP. Utilisez les en-têtes individuels pour limiter le coût d’inclusion.')[i]+'\n'
            anchors={}
            for symbol in h['symbols']:body+=render_symbol(symbol,i,anchors)
            if h.get('macros'):
                body+='\n## '+('Version macros','Macros de version')[i]+' {#macros}\n\n```cpp\n'+'\n'.join('#define '+k+' '+v for k,v in h['macros'])+'\n```\n'
            bodies.append(body)
        write(api_id(h['header']),(stem,stem),(note[0],note[1]),bodies,parent,1100+order,symbol=(next((s['name'] for s in h['symbols'] if s['name']=='cydsp::'+stem),h['symbols'][0]['name']) if h['symbols'] else None))
    assets=WORK/'assets/notices';assets.mkdir(parents=True,exist_ok=True)
    shutil.copy2(ROOT/'LICENSE',assets/'CyDSP-MIT.txt')
    for p in (ROOT/'third_party/licenses').glob('*.txt'):shutil.copy2(p,assets/p.name)
    shutil.copy2(WORK/'node_modules/cydoc/LICENSE',assets/'CyDoc-MIT.txt')
    # Legal texts remain exact originals in both locales; surrounding prose is translated.
    for locale in ('en','fr'):
        path=WORK/'manual'/locale/'licensing.md';text=path.read_text(encoding='utf-8')
        text+='\n## '+('Complete notices' if locale=='en' else 'Notices complètes')+' {#notices}\n\n'
        for p in sorted(assets.glob('*.txt')):text+='### '+p.name+'\n\n```text\n'+p.read_text(encoding='utf-8').strip()+'\n```\n\n'
        path.write_text(text.rstrip()+'\n',encoding='utf-8',newline='\n')
    expected={p['id']+'.md' for p in PAGES}|{'api.md'}|{'api-'+x+'.md' for x in GROUPS}|{api_id(h['header'])+'.md' for h in catalog['headers']}
    for locale in ('en','fr'):
        actual={p.name for p in (WORK/'manual'/locale).glob('*.md')}
        assert actual==expected,(locale,actual^expected)
    print(f'Authored {len(PAGES)} guide pairs + {len(catalog["headers"])} header pairs + {len(GROUPS)+1} API index pairs = {len(expected)*2} pages')
if __name__=='__main__': main()
