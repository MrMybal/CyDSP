# CyDSP documentation workspace

Node.js 22+ and Python 3 generate the manual; readers need neither runtime.
The public reader is ../Documentation/Documentation.html. All authoring and
maintenance instructions belong here, outside manual/en and manual/fr.

## Install the documentation toolchain

CyDoc is not substituted with a registry package of the same name. The package
manifest and lockfile use vendor/cydoc-0.1.0.tgz, built from the official CyDoc
checkout. Obtain/build CyDoc 0.1.0 from https://github.com/MrMybal/CyDoc, then run
`npm pack --ignore-scripts --pack-destination` with this workspace's absolute
vendor directory. The archive and node_modules are ignored; retain the archive
as a separate build input. Install with `npm ci --ignore-scripts --legacy-peer-deps`.
For the current environment, offline installation from the local npm cache worked.
The optional Three.js peer is not needed for this text/static-diagram manual.

## Author and generate

- tools/guides.py: 43 English/French usage guides.
- tools/header_notes.py: explanation and usage-guide mapping for every public header.
- api/catalog.json: Clang-extracted public declarations, defaults and header hashes.
- tools/author.py: writes reviewable source Markdown, copies exact legal notices.
- cydoc.config.json: English default, complete French locale, no optional media modules.

Run `npm run build` or `npm run check` here. Set CYDSP_PYTHON if Python is not on
PATH. Both regenerate the export, including the separate DocAI.md sidecar. Do not
edit generated manifests, viewer bundles or data.js by hand.

After a public API change, use a C++20 Clang developer environment and run
`python tools/extract_api.py --clang` with the clang-cl executable path. INCLUDE
must resolve the standard library and platform headers. The extractor does not
execute DSP code. Review its catalog and translated notes, then regenerate.

The build rejects stale header hashes. Verification checks complete page/anchor
parity, all 88 headers and 2006 current public declarations, local assets, ignored
internal material and self-contained reader resources. Browser checks remain
separate: open the real file URL and exercise search, tree, anchors and both locales.
No commit, push, GitHub workflow or deployment is part of this generator.

## Hosted manual

The online reader is https://mrmybal.github.io/CyDSP/Documentation.html.
GitHub Pages publishes the root of the dedicated gh-pages branch. That branch
contains only the complete Documentation export, an index.html entry that keeps
the locale/page fragment, and .nojekyll. Main keeps the offline Documentation/
layout. Source changes on main do not automatically regenerate or publish Pages.

To update the site, regenerate/check the manual locally, review and scan the
complete export, then publish its contents to gh-pages with a normal fast-forward
commit. Preserve the index entry, .nojekyll and all notices. The GitHub Pages
built-in deployment serves this prebuilt static export; no custom CI or remote
compilation is used. Never publish the repository root or authoring workspace.
