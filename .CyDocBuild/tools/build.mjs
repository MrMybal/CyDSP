import { readFile, writeFile, copyFile } from 'node:fs/promises';
import { spawnSync } from 'node:child_process';
import { dirname, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
const work=resolve(dirname(fileURLToPath(import.meta.url)), '..');
const python=process.env.CYDSP_PYTHON || 'python';
const args=process.argv.slice(2);
if(args.some(x=>x!=='--check'))throw new Error('Usage: npm run build | npm run check');
function run(exe,args){const r=spawnSync(exe,args,{cwd:work,stdio:'inherit'});if(r.error)throw r.error;if(r.status!==0)process.exit(r.status||1);}
run(python,['tools/author.py']);
run(process.execPath,['node_modules/cydoc/tools/cli.mjs',args.includes('--check')?'check':'build','cydoc.config.json']);
await copyFile(resolve(work,'reader.html'),resolve(work,'../Documentation/Documentation.html'));
const sidecar=resolve(work,'../Documentation/DocAI.md');
await writeFile(sidecar,(await readFile(sidecar,'utf8'))+'\n'+await readFile(resolve(work,'DocAI.append.md'),'utf8'));
run(python,['tools/verify.py']);
