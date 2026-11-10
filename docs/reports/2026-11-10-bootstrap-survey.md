# 初期セットアップ時の追加調査

調査日: 2026-11-10

## はじめに

`.prompts/INIT.md` の要件に記載済みの事実は再調査していません。本レポートには、要件の文書が着手前の確認を求めた事項と、計画を立てるために追加で確認した事項だけを書きます。

## 検証結果

### 名前の重複

要件の 4.2 が求める、GitHub と倉庫くんでの同名の有無を確認しました。

| 確認先 | 方法 | 結果 |
| --- | --- | --- |
| GitHub 全体 | `gh search repos "obs-comment-skin"` と `gh search repos "comment skin obs"` | 0 件 |
| 依頼者のアカウント | `gh api repos/AllegroMoltoV/obs-comment-skin` | HTTP 404 |
| 倉庫くん | MCP の `search_plugins` で `comment`、`コメント`、`skin`、`chat` を検索 | いずれも 0 件 |

0 件が検索の失敗でないことを、同じ経路の対照で確認しました。`gh search repos "obs-simple-drop-shadow"` は 1 件を返し、倉庫くんの `search_plugins` に `drop shadow` を渡すと `allegromoltov/obs-simple-drop-shadow` の 1 件が返りました。

死角として、倉庫くんの `search_plugins` は掲載済みのプラグインだけを返します。検査待ちや未掲載の登録は、この経路では見えません。

### 検証環境

- この PC には OBS Studio 33.0.1 が導入されています。レジストリのアンインストール情報で確認しました。
- `D:/work/obs-loudness-balance-monitor/.tmp/2026110301-OBS-Studio-33.0.1-Windows-x64.zip` に、OBS 33.0.1 のポータブル版の ZIP があります。依頼者の普段の設定を汚さずに検証する隔離環境として使えます。
- CMake、Git、GitHub CLI が PATH にあります。Visual Studio は `C:/Program Files/Microsoft Visual Studio/` に `2022` と `18` のフォルダーがあります。`vswhere.exe -all -products *` は結果を返さなかったため、ローカルでのビルドの可否は未確認です。
- 既存のプラグインでは、配布物を GitHub Actions でビルドし、その成果物を隔離版の OBS に入れて確認していました。`D:/work/obs-loudness-balance-monitor/docs/reports/2026-09-20-v012-build.md` に手順と結果があります。

### テンプレート

obs-plugintemplate の最新のコミットは 2025-12-09 の README の誤字の修正です。`gh api repos/obsproject/obs-plugintemplate/commits` で確認しました。既存の 3 本のプラグインが作られた後に、構成の変更はありません。

### ソースの追加・名前の変更・削除のシグナル

要件の 12 節が未確認としていた事項です。`D:/work/obs-loudness-balance-monitor/.tmp/2026110302-obs-studio-33.0.1-source.zip` の `docs/sphinx/reference-core.rst` を読みました。Core OBS Signals として次が定義されています。

```text
657: **source_create** (ptr source)
661: **source_destroy** (ptr source)
665: **source_remove** (ptr source)
670: **source_update** (ptr source)
702: **source_rename** (ptr source, string new_name, string prev_name)
```

ドックの候補の一覧は、これらのシグナルで更新できます。実機での発火の順序と、シーンコレクションの切り替え時の挙動は未確認です。

### 過去のロゴの作り方

既存の 3 本はいずれも `docs/images/logo.svg` に原本を置いています。obs-loudness-balance-monitor では `.appendix/logo-proposals/` に A・B・C の 3 案と比較の SVG を作り、依頼者のレビューを経て採用版を決めました。採用版は 256 x 256 の座標系の SVG で、512 x 512 の PNG に変換しています。経緯は `D:/work/obs-loudness-balance-monitor/docs/reports/2026-11-07-v100-release.md` の 37 行目にあります。

共通の意匠は、角丸の正方形の地に、単純な図形を少数の色で置く形です。外部の書体に依存せず、図形だけで描いています。

## まとめ

`obs-comment-skin` の名前は、GitHub と倉庫くんの掲載済みのプラグインに重複がありません。検証には、導入済みの OBS 33.0.1 とは別に、ポータブル版の 33.0.1 を隔離環境として使えます。
