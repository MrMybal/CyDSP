# CyDSP optional SADIE II HRTF selection

Three original SOFA files from SADIE II version 2-2: KU100 (D1), KEMAR (D2)
and one human listener (H10). All are 48 kHz, float64, 256 samples per ear.
D1/D2 each contain 8802 directions; H10 contains 2114. The signal files total
81,375,249 bytes (81.38 decimal MB). The whole folder stays below 100,000,000
bytes, including the manifest and notices.

These measurements are Apache-2.0, separately from the original CyDSP MIT code.
Keep LICENSE.txt, NOTICE.txt and provenance with redistributed data; identify
any later processing/modifications. Nothing is embedded in the C++ archive.
This is a selection of alternative listener profiles, not three physical parts
that must be combined. Listener preference and localization require evaluation.

To install the folder, explicitly configure CYDSP_INSTALL_HRTF_DATA=ON.
The destination is share/CyDSP/hrtf/sadie-ii-v2-2 (or the configured GNUInstallDirs
data directory). A data-only component install uses --component HrtfData.
The installed CMake package exposes CyDSP_HRTF_DATA_DIR when enabled.

Have the host read a selected SOFA file into memory and call the matching loadSofa overload
for SofaHrtf or SofaHrtf64, then prepareSofaHrtf and
HrirBinaural. Select the reader with SofaLoadOptions: the compact reader stores
float32, while the complete reader preserves float64. For D1/D2/H10 increase
maximumPoints above the default 2048 (the realtimeHigh preset allows 16384).
At 48 kHz the supplied 256 samples are retained; other processing may change
length/latency. SOFA reading/preparation belongs outside the audio callback.

Verify the folder with:
~~~sh
python scripts/verify_hrtf_data.py
~~~
From an installed copy, pass --directory with this folder's absolute path.

An optional native integration test is available only with
CYDSP_BUILD_TESTS=ON, CYDSP_BUILD_SOFA=ON and CYDSP_TEST_HRTF_DATA=ON.
The larger training corpus is fetched explicitly with:
~~~sh
python scripts/fetch_hrtf_training.py
~~~
By default it selects the original 96 kHz/512-sample files of all 20 subjects.
Archives and extracted files stay under ignored build/hrtf-training, outside
this selection and production installation. No model is trained by this command.

Source: https://zenodo.org/records/12092466
Manual: https://mrmybal.github.io/CyDSP/Documentation.html#/en/sofa/bundled
