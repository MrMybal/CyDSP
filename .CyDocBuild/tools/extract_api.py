"""Extract public C++ declarations with Clang; never export implementation bodies."""
import argparse, hashlib, json, os, re, subprocess
from pathlib import Path

WORK = Path(__file__).resolve().parents[1]
ROOT = WORK.parent

def extract(ast):
    dec = json.JSONDecoder(); pos = 0; records = []
    while pos < len(ast):
        if ast[pos].isspace(): pos += 1; continue
        obj, pos = dec.raw_decode(ast, pos); records.append(obj)
    groups = {p.relative_to(ROOT / 'include').as_posix(): {'header':p.relative_to(ROOT/'include').as_posix(), 'sha256':hashlib.sha256(p.read_bytes()).hexdigest(), 'symbols':[]} for p in sorted((ROOT/'include/cydsp').rglob('*.h'))}
    def clean(s):
        s = re.sub(r'/\*.*?\*/|//[^\n]*', '', s, flags=re.S)
        return re.sub(r'\s+', ' ', s).strip().encode('latin1').decode('utf-8')
    for namespace in records:
        file = namespace.get('loc', {}).get('file')
        if not file: continue
        path = Path(file).resolve()
        if not path.is_relative_to(ROOT/'include/cydsp'): continue
        key = path.relative_to(ROOT/'include').as_posix(); source = path.read_bytes().decode('latin1'); seen=set()
        def location(loc): return loc.get('expansionLoc',loc.get('spellingLoc',loc))
        def at(loc): return location(loc).get('offset',0)
        def raw(n):
            r=n.get('range',{}); e=r.get('end',{}); return source[at(r.get('begin',{})):at(e)+location(e).get('tokLen',0)]
        def default(p):
            if 'init' not in p and not p.get('hasInClassInitializer'): return ''
            nodes=[c for c in p.get('inner',[]) if c.get('range') and not c.get('kind','').endswith('Attr')]
            return clean(raw(nodes[-1])) if nodes else ''
        def add(n, scope, template=''):
            kind=n.get('kind',''); name=n.get('name','')
            if n.get('isImplicit') or not name: return
            loc=n.get('loc',{}); offset=at(loc)
            if 'offset' not in loc and 'spellingLoc' not in loc: return
            uid=(kind,name,offset)
            if uid in seen: return
            seen.add(uid)
            fullname='::'.join(scope+[name])
            item={'name':fullname,'kind':kind,'line':source.count('\n',0,offset)+1}
            if kind in ('CXXRecordDecl','RecordDecl'):
                if not n.get('completeDefinition'): return
                item['signature']=template+(n.get('tagUsed','struct')+' '+name)+';'
                item['members']=[]
                access='public' if n.get('tagUsed') in ('struct','union') else 'private'
                saved=output[0];output[0]=item['members']
                for child in n.get('inner',[]):
                    if child.get('kind')=='AccessSpecDecl': access=child.get('access',access)
                    elif access=='public': visit(child,scope+[name])
                output[0]=saved
            elif kind=='EnumDecl':
                values=[]
                for c in n.get('inner',[]):
                    if c.get('kind')=='EnumConstantDecl': values.append(clean(raw(c)))
                item['signature']='enum '+('class ' if n.get('scopedEnumTag') else '')+name+' { '+', '.join(values)+' };'
            elif kind in ('FunctionDecl','CXXMethodDecl','CXXConstructorDecl','CXXDestructorDecl','CXXConversionDecl'):
                body=next((c for c in n.get('inner',[]) if c.get('kind') in ('CompoundStmt','CXXTryStmt')),None)
                begin=at(n.get('range',{}).get('begin',{}))
                sig=source[begin:at(body['range']['begin'])] if body else raw(n)
                # Constructor initializer expressions are implementation, not API.
                if kind=='CXXConstructorDecl': sig=re.split(r'(?<!:):(?!:)',sig,maxsplit=1)[0]
                item['signature']=template+clean(sig).rstrip(';')+';'
                item['parameters']=[{'name':p.get('name',''), 'type':p.get('type',{}).get('qualType',''), 'default':default(p)} for p in n.get('inner',[]) if p.get('kind')=='ParmVarDecl']
                item['returnType']=n.get('type',{}).get('qualType','').split('(',1)[0].strip() if kind not in ('CXXConstructorDecl','CXXDestructorDecl') else ''
                item['deleted']=bool(n.get('explicitlyDeleted'))
            elif kind in ('FieldDecl','VarDecl'):
                typ=n.get('type',{}).get('qualType',''); val=default(n)
                prefix=('inline ' if n.get('inline') else '')+('static ' if n.get('storageClass')=='static' else '')+('constexpr ' if n.get('constexpr') else '')
                if n.get('constexpr'):typ=typ.removeprefix('const ')
                item['signature']=prefix+typ+' '+name+(' = '+val if val else '')+';'
                item['type']=typ;item['default']=val
            elif kind in ('TypeAliasDecl','TypedefDecl','ConceptDecl'):
                item['signature']=template+clean(raw(n)).rstrip(';')+';'
            else: return
            assert not re.search(r'\breturn\b',item['signature']), (key,item['signature'])
            output[0].append(item)
        def visit(n,scope,template=''):
            kind=n.get('kind','')
            if kind=='NamespaceDecl':
                if n.get('name')=='detail': return
                for c in n.get('inner',[]): visit(c,scope+[n.get('name','')])
            elif kind in ('ClassTemplateDecl','FunctionTemplateDecl','TypeAliasTemplateDecl'):
                params=[clean(raw(c)) for c in n.get('inner',[]) if c.get('kind') in ('TemplateTypeParmDecl','NonTypeTemplateParmDecl','TemplateTemplateParmDecl')]
                prefix='template <'+', '.join(params)+'> '
                wanted=('CXXRecordDecl',) if kind=='ClassTemplateDecl' else ('TypeAliasDecl',) if kind=='TypeAliasTemplateDecl' else ('FunctionDecl','CXXMethodDecl','CXXConstructorDecl')
                for c in n.get('inner',[]):
                    if c.get('kind') in wanted and not c.get('isImplicit'):
                        begin=at(n.get('range',{}).get('begin',{})); end=at(c.get('range',{}).get('begin',{}))
                        exact=clean(source[begin:end])
                        add(c,scope,(exact+' ') if exact else prefix);break
            elif kind=='ClassTemplateSpecializationDecl': return
            else: add(n,scope,template)
        output=[groups[key]['symbols']]
        for child in namespace.get('inner',[]): visit(child,['cydsp'])
    # Version macros have no namespace AST declaration. The umbrella exports headers.
    groups['cydsp/core/Version.h']['macros']=re.findall(r'^#define\s+(CYDSP_VERSION_\w+)\s+(.+)$',(ROOT/'include/cydsp/core/Version.h').read_text(),re.M)
    return {'schemaVersion':1,'libraryVersion':'0.3.2','headers':list(groups.values())}

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--ast',type=Path);parser.add_argument('--clang',type=Path);args=parser.parse_args()
    if args.ast: ast=args.ast.read_text(encoding='utf-8')
    else:
        clang=args.clang or Path(os.environ.get('CLANG_CL','C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/Llvm/x64/bin/clang-cl.exe'))
        unit=WORK/'internal/api.cpp';unit.parent.mkdir(parents=True,exist_ok=True);unit.write_text('#include <cydsp/CyDSP.h>\n')
        result=subprocess.run([str(clang),'/nologo','/std:c++20','/EHsc','/Zs','/I'+str(ROOT/'include'),'/clang:-Xclang','/clang:-ast-dump=json','/clang:-Xclang','/clang:-ast-dump-filter=cydsp',str(unit)],cwd=ROOT,text=True,encoding='utf-8',capture_output=True,check=True)
        ast=result.stdout
    catalog=extract(ast);(WORK/'api').mkdir(exist_ok=True)
    (WORK/'api/catalog.json').write_text(json.dumps(catalog,indent=2,ensure_ascii=False)+'\n',encoding='utf-8',newline='\n')
    def count(items):return sum(1+count(i.get('members',[])) for i in items)
    print(f"Public API: {len(catalog['headers'])} headers; {sum(count(h['symbols']) for h in catalog['headers'])} declarations")
if __name__=='__main__': main()
