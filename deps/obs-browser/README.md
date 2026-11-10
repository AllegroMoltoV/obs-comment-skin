# obs-browser のパネルの API のヘッダー

`browser-panel.hpp` は、[obs-browser](https://github.com/obsproject/obs-browser) の `panel/browser-panel.hpp` を変更せずに写したものです。ライセンスは obs-browser と同じ GPL-2.0 です。

写した元は、obs-browser の 2.27.5 のソースで、コミット `d4db73e7e5d5b51d853177c8cddc796715a8ac84` です。OBS Studio 33.0.1 に同梱される obs-browser も 2.27.5 です。パネルの API の版 `BROWSER_PANEL_VERSION` は 3 で、ドックの見本のプレビューは実行時にこの版を確かめてから使います。

このヘッダーは仮想関数の並びで obs-browser と結び付くため、obs-browser の版を上げたときは、元のヘッダーと差分が無いことを確かめてください。
