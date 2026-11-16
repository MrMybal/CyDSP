## CyDSP authoring and API rules

The source project is CyDSP 0.4.0, a standalone C++20 numerical DSP library.
CyAudioCore may be mentioned as a consumer; do not turn this manual into an engine
migration guide. Work only in CyDSP and respect its AGENTS.md.

- Human guide originals are maintained in .CyDocBuild/tools/guides.py. Translate
  every guide into French with the same page and anchor IDs.
- Public header explanations live in .CyDocBuild/tools/header_notes.py. The API
  catalog in .CyDocBuild/api/catalog.json records public declarations only. It
  must match all public header hashes before publication.
- To refresh declarations, run tools/extract_api.py with Clang in a C++20 developer
  environment. Never export private members, implementation bodies or raw sources.
- tools/author.py writes bilingual source Markdown under manual/en and manual/fr.
  Review those files and improve the authored explanations when contracts change.
- npm run build generates the real reader and runs tools/verify.py; npm run check
  regenerates it through CyDoc's check command. Both commands write the export.
- Keep caches and evidence under .CyDocBuild/internal, outside human pages. Do not
  expose benchmarks, audit history or documentation-maintenance steps in navigation.
- Distribute all of Documentation/, including notices. Keep node_modules, the
  generator archive, raw AST data and authoring inputs out of installed manuals.
- README links target Documentation/Documentation.html#/en/overview and the French
  equivalent. The full DocData tree must accompany the reader.
