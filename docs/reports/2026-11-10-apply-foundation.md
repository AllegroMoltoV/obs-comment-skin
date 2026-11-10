# 適用と保存の土台

実施日: 2026-11-10
対象の課題: obs-comment-skin-dxs.6

## 結論

ドックに、ブラウザーソースの選択・チャットの種類の判別・適用・元に戻す・生成した CSS の表示とコピー・ブラウザーソースの作成を実装した。管理範囲の置換と URL の判別は OBS に依存しない関数に分け、単体テストで確かめた。ポータブル版 OBS 33.0.1 でドックを UI Automation で操作し、適用と元に戻す操作の結果、作成したソース、再起動の後の記録の復元を確かめた。設定の項目はまだ無く、管理範囲の中身は注記だけである。

## 作成したもの

| ファイル | 内容 |
| --- | --- |
| `src/core/managed-css.h`、`.cpp` | 管理範囲 `/* comment-skin begin */` から `/* comment-skin end */` の置換。範囲が無ければ末尾に足し、目印が壊れていれば置換しない。元に戻す操作の判定 |
| `src/core/chat-target.h`、`.cpp` | URL からのチャットの種類の判別と、動画 ID・チャンネル名からのポップアウトの URL の組み立て |
| `src/source-state.h`、`.cpp` | private settings の `comment_skin` への状態の読み書きと、`css` の書き換え |
| `src/create-source-dialog.h`、`.cpp` | 現在のシーンに幅 400、高さ 600 のブラウザーソースを作るダイアログ |
| `src/comment-skin-dock.h`、`.cpp` | ドック。第 1 層のテスト用のボタンと CSS を外した |
| `tests/core-tests.cpp` | `src/core/` の単体テスト |

ビルドの設定は次のように変えた。

- `CMakeLists.txt`: `src/core/` をプラグインのソースに含めた。`ENABLE_TESTS` のオプションで、単体テストの実行ファイル `comment-skin-core-tests` と CTest のテスト `core-tests` を作る。
- `CMakePresets.json`: CI の 3 つのプリセットで `ENABLE_TESTS` を有効にした。
- `.github/workflows/build-project.yaml`: 3 つの OS のビルドの後に `ctest` を実行する手順を足した。
- `.gitignore`: `!/tests` を足した。

### 判別する URL

| 種類 | ホスト | パス |
| --- | --- | --- |
| YouTube | `youtube.com`、`www.youtube.com`、`m.youtube.com`、`studio.youtube.com` | `/live_chat` で始まる。`/live_chat_replay` を含む |
| Twitch | `twitch.tv`、`www.twitch.tv`、`m.twitch.tv` | `/popout/<チャンネル名>/chat`、`/embed/<チャンネル名>/chat` |

判別できない URL では、利用者がチャットの種類を選ぶ。選んだ種類が URL から判別した種類と同じときは記録しない。URL を変えたときに判別をやり直せるようにするためである。段階 c では、Twitch を選んでも適用ボタンを押せない。

### 保存する状態

private settings の `comment_skin` に次を置く。シーンコレクションと一緒に保存される。

| キー | 内容 |
| --- | --- |
| `target` | 利用者が選んだチャットの種類。URL から判別するときは空 |
| `has_previous` | 元に戻せる適用の記録の有無 |
| `previous_css` | 最後の適用の前の `css` |
| `applied_css` | 最後の適用の直後の `css` |

元に戻す操作では、現在の `css` が `applied_css` と違えば、適用の後に利用者が書き換えたと判断する。この場合は、書き換えも消えることを確認のダイアログで伝える。

## 検証方法

単体テストとビルドは次のコマンドで実行した。

```
cmake --preset windows-ci-x64 -DENABLE_POST_BUILD_INSTALL=OFF
cmake --build --preset windows-ci-x64
ctest --test-dir build_x64 -C RelWithDebInfo --output-on-failure
```

整形は、Visual Studio 2022 に同梱の clang-format 19.1.5 の `-i` で変更したファイルを整え、`gersemi --check CMakeLists.txt` で確かめた。

ポータブル版 OBS 33.0.1 では、次の方法でドックを操作し、結果を読んだ。

- ボタンは UI Automation の `InvokePattern` で押した。
- コンボボックスの選択は、UI Automation で求めた位置をマウスでクリックした。`SelectionItemPattern` とキーの送信では選択が反映されなかった。
- 作成のダイアログを開くボタンは、呼び出しが戻らない場合に備えて PowerShell のジョブの中で押した。
- ページに注入された CSS は、`--remote-debugging-port=9222` で起動した OBS に CDP で接続し、`head` の `style` 要素の中身を読んだ。
- 保存された状態は、OBS を正常終了した後に `config/obs-studio/basic/scenes/無題.json` を `jq` で読んだ。

使ったスクリプトは `.tmp/2026111005-cdp-eval.mjs`、`.tmp/2026111006-cdp-navigate.mjs`、`.tmp/2026111007-uia-do.ps1` にある。

## 検証結果

### ビルドと単体テスト

| 項目 | 結果 | ログ |
| --- | --- | --- |
| 構成 | 終了コード 0 | `.logs/2026111004-configure.log` |
| ビルド | 終了コード 0。警告とエラーの行は 0 件 | `.logs/2026111006-build.log` |
| 単体テスト | `core-tests` が成功 | なし |

単体テストで確かめた内容は次のとおり。

- 範囲が無いときに末尾へ足し、既存の CSS が先頭に残ること。空の `css` にも足せること。
- 範囲の前後の利用者の CSS が、置換の後も同じ文字列で残ること。
- 2 回置換しても範囲が 1 組のままで、前の中身が残らないこと。
- 開始だけ、終了だけ、逆順、2 組の 4 通りの壊れた目印で、置換を拒否すること。
- 元に戻す操作の判定が、記録なし、書き換えなし、書き換えありの 3 通りで分かれること。
- URL の判別。`www` と `studio` と `live_chat_replay`、大文字の URL、Twitch の `popout` と `embed`、`chat` で始まるチャンネル名の別のパス、動画の視聴ページ、ほかのサイト、空の文字列。
- 動画 ID と、視聴ページ・`youtu.be`・`/live/` の URL からの YouTube の URL の組み立て。不正な入力の拒否。
- チャンネル名と、チャンネルの URL・ポップアウトの URL からの Twitch の URL の組み立て。不正な入力の拒否。

### ポータブル版 OBS での操作

対象は、依頼者が第 1 層の確認で作ったブラウザーソース「ブラウザ」で、URL は `https://studio.youtube.com/live_chat?is_popout=1&v=<動画 ID>` だった。`css` には、既定の CSS と第 1 層のテスト用の CSS があった。

| 操作 | 結果 |
| --- | --- |
| ソースを選ぶ | チャットの種類が YouTube になり、「URL から YouTube と判別しました。」と表示された。適用ボタンが有効になった |
| 適用 | 注入された CSS の末尾に管理範囲が足され、既定の CSS とテスト用の CSS は前に残った |
| 同じ設定で再び適用 | 「変更はありません。」と表示され、`css` は書き換わらなかった |
| 元に戻す | 注入された CSS が、適用の前と同じ文字列に戻った。元に戻すボタンが無効になった |
| 新規作成で `https://www.youtube.com/watch?v=<11 文字の ID>` を入力して作成 | 「YouTube チャット」が現在のシーンに追加され、ドックで選ばれた。種類は YouTube と判別された |
| 再び適用して OBS を終了 | シーンコレクションの JSON の「ブラウザ」の `private_settings.comment_skin` に、`has_previous: true`、`previous_css`、`applied_css` が保存された。「YouTube チャット」の `url`、`width: 400`、`height: 600` も保存された |
| OBS を起動し直してソースを選ぶ | 元に戻すボタンが有効だった |

OBS のログには、適用、元に戻す、作成の行があった。2 回の終了とも `Number of memory leaks: 0` だった。

```
17:50:47.339: [comment-skin] applied CSS to browser source 'ブラウザ'
17:51:09.200: [comment-skin] restored previous CSS of browser source 'ブラウザ'
17:52:06.370: [comment-skin] applied CSS to browser source 'ブラウザ'
17:52:48.784: [comment-skin] created browser source 'YouTube チャット'
17:52:57.935: Number of memory leaks: 0
```

確認の後、元に戻す操作で「ブラウザ」の `css` を適用の前に戻した。

## 確認していないこと

- 目印が壊れているときの警告のダイアログと、適用の後に書き換えた `css` を戻すときの確認のダイアログの表示。判定の処理は単体テストで確かめた。
- シーンコレクションの切り替えとソースの複製の後の状態。保存と写しは OBS のソースで確かめた設計の前提で、実機では確かめていない。
- Twitch のソースと、URL から判別できないソースでの種類の選択。
- macOS と Linux のビルドと、CI の単体テストの実行。GitHub に push していない。
- CI の clang-format は 19.1.1 で、手元の 19.1.5 と版が違う。

## 考察

ドックの操作は UI Automation で自動化できたため、以降の課題でも、適用の経路は開発のセッションが自分で確かめられると考えています。コンボボックスの選択だけはマウスのクリックが必要で、ほかのアプリが前面にあると失敗します。そのため、スクリプトは前面が OBS でないときに操作を中止するようにしています。
