# ドックの見本のプレビューの実現性の検証

実施日: 2026-11-10
対象の課題: obs-comment-skin-dxs.16

## 結論

obs-browser のパネルの API で、プラグインに同梱した見本のページをドックに表示し、生成した CSS を読み込み直しなしで差し替えられることを、Windows のポータブル版 OBS 33.0.1 で確かめた。設定を変えてから見本に反映されるまでの時間は 133 ms から 233 ms で、実物のブラウザーソースの 785 ms から 881 ms より短い。見本のページの資源の読み込みは 0 件で、OBS の終了時のクラッシュとメモリーリークは無かった。macOS と Linux のビルドは確かめていない。

## 背景

ユーザーテスト c1 の後、依頼者は、プレビューしながらでないとサイズの釣り合いを調整しづらいという問題を重く見るべきだと指摘した。実物のブラウザーソースは、`css` を変えるたびにページを作り直す。そのため、反映に時間がかかり、コメントの少ない配信では見比べる対象が残らない。依頼者は、ドックに見本のプレビューを置く案を、この検証が通ることを条件に採った。

## 作成したもの

| ファイル | 内容 |
| --- | --- |
| `deps/obs-browser/browser-panel.hpp` | obs-browser 2.27.5 の `panel/browser-panel.hpp` を変更せずに写したもの。由来は同じディレクトリの `README.md` に書いた |
| `data/preview/youtube.html` | 見本のページ。YouTube のポップアウトのチャットと同じ要素名と ID で組み、土台の見た目は自作した。CSP の `default-src 'none'` で通信を禁じている |
| `src/core/preview-script.h`、`.cpp` | CSS を JavaScript の文字列リテラルにする処理と、見本の `style` 要素を差し替えるスクリプトの生成 |
| `src/preview-panel.h`、`.cpp` | ドックの見本の部品 |

見本の部品の動きは次のとおり。

- `obs_browser_qcef_version()` が 3 以上で、`obs_browser_init_panel()` が `QCef` を返すときだけ見本を作る。それ以外では「この環境では見本を表示できません」と表示する。
- CEF の起動は `init_browser()` で求め、`initialized()` を 100 ms ごとに確かめる。`wait_for_browser_init()` は UI のスレッドを止めるため使わない。30 秒たっても起動しなければ見本を諦める。
- 見本のページの URL は、`obs_module_file()` のパスを絶対パスにしてから `file:` の URL にする。ポータブル版では `obs_module_file()` が相対パスを返し、そのままでは `file://../data/...` となって読み込めなかった。
- CSS は `executeJavaScript()` で即座に当て、同じスクリプトを `setStartupScript()` にも渡す。読み込みの途中で設定を変えても、読み込みの完了時に最新の CSS が当たるようにするためである。
- OBS の終了のイベントで `closeBrowser()` を呼んでブラウザーを閉じる。obs-browser が CEF を止める前に閉じるためである。
- Linux では `ENABLE_WAYLAND` を定義し、ヘッダーの Wayland の判定を有効にする。

## 検証方法

- 単体テストで、引用符・バックスラッシュ・改行・`<` と `>`・制御文字・U+2028 と U+2029 のエスケープを確かめた。
- ポータブル版 OBS を `--remote-debugging-port=9222` で起動した。見本のページは、ブラウザーソースと同じく CDP の対象に現れた。
- 見本の状態は `.tmp/2026111014-preview-probe.js` で読んだ。反映までの時間は、文字の大きさの欄に値を入れる直前の時刻から、見本の `#message` の計算済みの `font-size` が新しい値になるまでを、`.tmp/2026111015-preview-wait.mjs` で 10 ms ごとに読んで測った。この時間には、UI Automation の呼び出しと、計測用の Node.js の起動の時間が含まれる。

## 検証結果

### 表示と通信

| 確認した内容 | 結果 |
| --- | --- |
| 見本のページの URL | `file:///D:/work/obs-comment-skin/.tmp/obs-33.0.1-portable/data/obs-plugins/comment-skin/preview/youtube.html` |
| ドックの中の見本の大きさ | 幅 324 px、高さ 260 px |
| `performance.getEntriesByType('resource')` | 設定を変える前と、4 回変えた後のどちらも 0 件 |
| `#cskin-generated` の中身 | ドックの生成した CSS と同じ文字列 |
| 見本の `#message` の `font-size` | 最後に入れた 28 px |

### 反映までの時間

| 回 | 値 | UI Automation の呼び出し | 見本への反映までの時間 |
| --- | --- | --- | --- |
| 1 | 24 px | 149 ms | 233 ms |
| 2 | 32 px | 57 ms | 137 ms |
| 3 | 20 px | 52 ms | 135 ms |
| 4 | 28 px | 50 ms | 133 ms |

同じ条件で測った実物のブラウザーソースへの反映は 785 ms から 881 ms だった。結果は `docs/reports/2026-11-10-auto-apply.md`。

### 終了

元に戻す操作の後に OBS を `CloseMainWindow()` で終了した。プロセスは終了し、展開先の配下で動くプロセスは 0 件だった。ログの最後は `Number of memory leaks: 0` で、`config/obs-studio/crashes/` は空だった。

## 確認していないこと

- macOS と Linux のビルドと動作。Linux の `ENABLE_WAYLAND` の定義に必要な `obs-nix-platform.h` が、CI のビルドで見つかることも確かめていない。GitHub に push していないため、CI は実行していない。
- Wayland で見本の代わりに案内が表示されること。
- ドックを閉じた状態や、ドックをフローティングにした状態での見本の表示。
- 依頼者の目視による見本の見た目と使い心地。

## 考察

Windows では、見本のプレビューを採用できると考えています。見本への反映は実物より 0.6 秒以上短く、見本のコメントは消えないため、依頼者が求めたサイズの釣り合いの調整に使えると考えています。

ヘッダーは obs-browser の仮想関数の並びに依存します。OBS の版を上げるときは、obs-browser の版とヘッダーの差分を確かめる必要があると考えています。macOS と Linux の動作は、0.1.0 の公開の準備で CI を動かすときに確かめるのが適切だと考えています。
