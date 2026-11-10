# 文書索引

context compaction で文脈が失われた場合は、この索引から必要な文書をたどる。

| 知りたいこと | 参照先 |
|---|---|
| プロジェクトの目的と使い方 | `README.md` |
| 要件と初回セットアップの指示 | `.prompts/INIT.md` |
| 調査レポート | `docs/reports/` |
| 実行ログ (生の出力。git 追跡対象外) | `.logs/` |
| 実行ログ (残す価値があると判断したもの) | `docs/logs/` |
| 計画と決定事項 (Beads 使用時の正本) | `bd show <親課題>` |
| 計画 (Beads 非使用時) | `.prompts/PLANS/` |
| 判断を要する論点 (Beads 非使用時) | `.prompts/DISCUSSIONS/` |
| タスクの状態、依存関係、ブロッカー | `bd ready` / `bd blocked` / `bd list` |

## 調査レポート

- [2026-11-10 初期セットアップ時の追加調査](reports/2026-11-10-bootstrap-survey.md): 名前の重複の確認、検証環境、過去のロゴの作り方
- [2026-11-10 リポジトリの土台と最小のドックのプラグイン](reports/2026-11-10-scaffold-and-build.md): テンプレートの複製範囲、ローカルビルドの手順と結果、ポータブル版 OBS 33.0.1 での読み込みとドックの状態の復元の確認
- [2026-11-10 YouTube のポップアウトのチャットの DOM 調査](reports/2026-11-10-youtube-chat-dom.md): メッセージ・Super Chat・メンバーシップ・ステッカー・ティッカーの要素の構造、投稿者の種別のクラス、隠す設定を置かない要素
- [2026-11-10 css の書き換えによる再読み込みと CSS の反映の実機確認](reports/2026-11-10-css-reload-check.md): 読み込み直しの有無、CEF 150 での `:has()` の対応、未ログインの表示の `dark` 属性、配信者の発言の `author-type=owner`、CDP での調べ方
