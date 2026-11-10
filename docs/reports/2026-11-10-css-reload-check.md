# css の書き換えによる再読み込みと CSS の反映の実機確認

実施日: 2026-11-10
対象の課題: obs-comment-skin-dxs.4

## 結論

ドックから `css` を書き換えると、ブラウザーソースのチャットは読み込み直され、書き込んだ CSS がページに取り込まれた。OBS 33.0.1 の CEF 150 は CSS の `:has()` に対応し、`yt-live-chat-text-message-renderer:has(#author-name.owner)` の規則で配信者の発言の背景が変わった。OBS のブラウザーソースは YouTube に未ログインで、未ログインでも `html` に `dark` 属性があった。

依頼者の目視では、テスト用 CSS の枠線と緑の背景が見えなかった。原因は CSS の反映の失敗ではなく、テスト用 CSS の作りとテストの条件である。根拠は後述の「目視で変化が見えなかった原因」に書く。

## 検証方法

1. 依頼者が、手順書 `.prompts/MANUALS/2026111001-visual-check.md` の B にしたがい、ポータブル版 OBS 33.0.1 でブラウザーソースを追加し、ドックの「テスト用 CSS を書き込む」を押した。
2. 開発のセッションが、同じポータブル版 OBS を `obs64.exe --remote-debugging-port=9222` で起動し直した。`http://127.0.0.1:9222/json` でブラウザーソースのページを取得し、Chrome DevTools Protocol の `Runtime.evaluate` でページの中の状態を読んだ。

Windows の obs-browser は `CefMainArgs` に引数を渡さないが、CEF がプロセスの起動オプションを読むため、この方法で接続できた。obs-browser のソースは `D:/work/obs-loudness-balance-monitor/.tmp/2026110305-obs-browser.zip` の `obs-browser-plugin.cpp` の 291 から 299 行で確かめた。

## 検証結果

### 依頼者の目視

| 項目 | 結果 |
| --- | --- |
| チャットの読み込み直し | あり |
| 枠線の色 | 変化なし |
| メンバーの発言の緑の背景 | 変化なし |

依頼者が OBS のプロパティから写した `css` の値には、目印の範囲が想定どおり書き込まれていた。書き込みの時刻は 2026-11-10T17:25:08 で、OBS のログの `wrote test CSS` の行と一致する。

### ページの中の状態

OBS のログの obs-browser の版は次のとおり。

```
[obs-browser]: Version 2.27.5
[obs-browser]: CEF Version 150.0.7871.252 (runtime), ...
```

ブラウザーソースの URL は `https://studio.youtube.com/live_chat?is_popout=1&v=<動画 ID>` だった。依頼者自身の配信で、チャットのメッセージは配信者の発言 1 件だった。

| 確認した内容 | 結果 |
| --- | --- |
| obs-browser が注入した `<style>` の数と位置 | 1 件、`head` の直下 |
| 注入した `<style>` の `cssRules` | 4 規則すべてが解釈され、`:has()` の規則も捨てられていない |
| `CSS.supports('selector(:has(a))')` | `true` |
| `body` の計算済みの `outline` | `solid 4px rgb(229, 57, 53)`、`outline-offset: -4px` |
| 左上の座標 (2, 2) の最前面の要素 | `yt-live-chat-header-renderer` |
| `#author-name.member` を持つメッセージ | 0 件 |
| `yt-live-chat-text-message-renderer:has(#author-name.owner)` | 1 件 |
| 上の要素に `background-color: rgb(0, 200, 0) !important` の `<style>` を足した前後の計算済みの背景 | `rgba(0, 0, 0, 0)` から `rgb(0, 200, 0)` |
| `ytcfg.get('LOGGED_IN')` | `false` |
| `html` の属性 | `dark` を含む |
| `#input-panel` の子 | `yt-live-chat-message-renderer` のみ。入力欄は無い |

配信者の発言のホストの要素には、`author-type=owner` と `author-is-owner` の属性があった。2026-11-10 の DOM 調査では、配信者の発言を観測できず、この属性を確認できていなかった。

同じ動画の `https://www.youtube.com/live_chat?is_popout=1&v=<動画 ID>` へ `Page.navigate` で移しても、`LOGGED_IN` は `false`、`html` に `dark` 属性があり、注入した `<style>` は 1 件だった。

## 目視で変化が見えなかった原因

- 枠線: `body` の枠線は計算上は付いていた。`body` の上に YouTube の要素が重なって描かれ、枠線が見えなかったと考えています。左上の最前面の要素が `yt-live-chat-header-renderer` だったことが根拠です。重なりの仕組みまでは確かめていません。
- 緑の背景: メンバーの発言が 0 件だったため、規則に一致する要素がありませんでした。

## 確認していないこと

- `:has()` の規則による背景の変化は、obs-browser が注入した `<style>` ではなく、CDP で足した `<style>` で確かめた。注入の仕組みは同じ `head` の `<style>` で、注入された `:has()` の規則が解釈済みであることも確かめている。
- メンバーとモデレーターの発言の `author-type` の値。今回のチャットには配信者の発言しかなかった。
- 画面への描画。計算済みのスタイルで確かめ、スクリーンショットは取っていない。

## 考察

css の書き換えで読み込み直しが起きるため、親課題の設計どおり、適用ボタンで `css` を書き換える方式を採れると考えています。読み込み直しでチャットの表示が最初からになる点は、ユーザーテスト c1 で受け止め方を確かめます。

配信者の発言は `[author-type=owner]` で指定でき、`:has()` を使わずに済みます。メンバーとモデレーターも同じ属性で指定できる可能性がありますが、値を観測するまでは、`:has(#author-name.member)` と `:has(#author-name.moderator)` を候補として残します。

OBS のブラウザーソースは未ログインでも `dark` 属性を持つため、YouTube の暗い配色を前提に CSS を生成してよいと考えています。未ログインでは入力欄が無く、ログインを促す `yt-live-chat-message-renderer` が表示されます。この要素を隠す設定の要否は、第 2 層の計画の書き直しで扱います。
