# YouTube の最小の項目

実施日: 2026-11-10
対象の課題: obs-comment-skin-dxs.7

## 結論

ドックに、メッセージの文字の大きさと色、全体の背景色と不透明度の 4 項目を、折りたためる区画に分けて置いた。設定から CSS を生成する処理を OBS に依存しない関数にし、単体テストで確かめた。ポータブル版 OBS 33.0.1 で、ドックから適用した文字の大きさがページに反映されることと、背景色と文字の色の規則が YouTube の要素に効くことを確かめた。

## 作成したもの

| ファイル | 内容 |
| --- | --- |
| `src/core/youtube-style.h`、`.cpp` | 設定の型と、YouTube の CSS の生成。色は `#rrggbb` だけを受け付ける |
| `src/settings-widgets.h`、`.cpp` | 折りたためる区画、色の欄、文字の大きさの欄、YouTube の設定パネル |
| `src/source-state.h`、`.cpp` | private settings の `comment_skin.youtube` への設定の読み書きを足した |
| `src/comment-skin-dock.h`、`.cpp` | 設定パネルを組み込み、設定を変えるたびにソースへ保存し、生成した CSS の表示を更新する |

項目は、値を指定しない既定の状態を持つ。既定の項目は CSS を生成せず、YouTube の見た目のままになる。不透明度は背景色を指定したときだけ使う。

## 生成する CSS

YouTube の既定の見た目は、2026-11-10 に OBS のブラウザーソースで CDP で調べた。

| 要素 | 計算済みのスタイル |
| --- | --- |
| `yt-live-chat-text-message-renderer #message` | `font-size: 14px`、`line-height: 20px`、`color: rgb(255, 255, 255)` |
| `yt-live-chat-renderer` | `background-color: rgb(15, 15, 15)` |
| `yt-live-chat-header-renderer` | `background-color: rgb(15, 15, 15)` |
| チャット欄の中央からその祖先 | `yt-live-chat-renderer` を除き `rgba(0, 0, 0, 0)` |

この結果から、次の規則を生成する。

```css
yt-live-chat-renderer,
yt-live-chat-header-renderer {
  background-color: rgba(0, 0, 0, 0.5) !important;
}
yt-live-chat-text-message-renderer #message {
  font-size: 24px !important;
  line-height: 1.43 !important;
  color: rgb(255, 255, 0) !important;
}
```

行の高さは YouTube が 20px に固定しているため、文字を大きくしたときに行が重ならないよう、既定の 20px / 14px に合わせた比率 1.43 にする。不透明度の小数は、OBS のロケールの設定で小数点がコンマにならないよう、整数から文字列を組み立てる。

## 検証方法

- 単体テストを `ctest --test-dir build_x64 -C RelWithDebInfo --output-on-failure` で実行した。
- ポータブル版 OBS で、ドックを `.tmp/2026111007-uia-do.ps1` で操作した。文字の大きさの欄には UI Automation の `RangeValuePattern` で値を入れた。
- ページの状態は `.tmp/2026111005-cdp-eval.mjs` で読んだ。
- 背景色と文字の色は、Qt の色のダイアログを自動で操作できないため、生成する規則と同じ文字列の `style` 要素を CDP で注入して確かめた。

## 検証結果

### ビルドと単体テスト

ビルドは終了コード 0 で、コンパイラーの警告は 0 件だった。ログは `.logs/2026111009-build.log`。単体テストで確かめた内容は次のとおり。

- `#rrggbb` の読み取りと書き出し。`#` の無い文字列、桁の足りない文字列、16 進数でない文字、色の名前、CSS の続きを含む文字列を拒否すること。
- 何も指定しないときに空の CSS を生成すること。
- 文字の大きさ・文字の色・背景色と不透明度が、期待した宣言になること。生成した CSS が `@import` と `url(` を含まないこと。
- 文字の大きさが上限の 96px に収まること。不透明度の 0・5・35・99・100 と範囲外の 150・-5 が、`0`・`0.05`・`0.35`・`0.99`・`1`・`1`・`0` になること。

### ポータブル版 OBS

| 操作 | 結果 |
| --- | --- |
| 「ブラウザ」を選ぶ | メッセージと背景の区画が表示され、すべての項目が既定だった。不透明度の欄は無効だった |
| 文字の大きさを 24 px にして適用 | 管理範囲に `font-size: 24px !important;` と `line-height: 1.43 !important;` が書かれた。ページの `#message` の計算済みのスタイルは `font-size: 24px`、`line-height: 34.32px` だった |
| 背景色と文字の色の規則を CDP で注入 | `yt-live-chat-renderer` とヘッダーの背景が `rgba(0, 0, 0, 0.5)`、`#message` の色が `rgb(255, 255, 0)` になった |
| 文字の大きさを 30 px に変え、元に戻して OBS を終了 | シーンコレクションの JSON の `private_settings.comment_skin.youtube` に `message_font_size: 30` と `background_opacity: 100` が保存された。`css` は管理範囲の無い適用の前の値に戻った |

## 確認していないこと

- 色のダイアログでの色の選択と、既定に戻すボタンの操作。ユーザーテスト c1 で依頼者が操作する。
- 画面への描画。計算済みのスタイルで確かめ、スクリーンショットは取っていない。
- 不透明度 50% の背景で、配信の画面の背後が透けて見えること。ユーザーテスト c1 で依頼者が確かめる。

## 考察

元に戻す操作は `css` だけを戻し、ドックの設定の値は戻しません。設定は変えるたびに保存されるため、適用せずに OBS を終了しても次回に残ります。この挙動の分かりやすさは、ユーザーテスト c1 で利用者の受け止め方を確かめたいと考えています。
