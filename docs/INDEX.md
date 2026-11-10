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
- [2026-11-10 適用と保存の土台](reports/2026-11-10-apply-foundation.md): 管理範囲の置換、URL の判別、private settings に置く状態、単体テスト、UI Automation でのドックの操作による確認
- [2026-11-10 YouTube の最小の項目](reports/2026-11-10-youtube-minimal-items.md): YouTube の既定のスタイル、文字の大きさ・文字の色・背景色と不透明度の生成規則、ドックからの適用の確認
- [2026-11-10 ユーザーテスト c1](reports/2026-11-10-usability-c1.md): 4 つの課題の達成、適用ボタンを押すまで反映されないことへの戸惑い
- [2026-11-10 設定の変更の自動の適用](reports/2026-11-10-auto-apply.md): 自動の適用と元に戻す操作の意味、反映までの時間、連続した適用での安定性
- [2026-11-10 ドックの見本のプレビューの実現性の検証](reports/2026-11-10-dock-preview.md): obs-browser のパネルの API、同梱の見本のページ、見本への反映までの時間、通信が無いことの確認
- [2026-11-10 YouTube の残りの項目とプリセット](reports/2026-11-10-youtube-all-items.md): 項目の表、6.2 からの変更、実際の YouTube の要素でのセレクターの確認、プリセット
