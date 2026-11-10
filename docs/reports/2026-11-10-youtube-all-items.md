# YouTube の残りの項目とプリセット

実施日: 2026-11-10
対象の課題: obs-comment-skin-dxs.9

## 結論

`.prompts/INIT.md` の 6.2 の各群の項目と、ヘッダーを隠す設定、プリセットの保存と読み込みを作った。項目は表で定義し、ドックの欄、保存、CSS の生成を表から作る。生成するセレクターは、Chrome で YouTube のアーカイブのチャットのリプレイに当て、すべて実際の要素に一致し、計算済みのスタイルが変わることを確かめた。ポータブル版 OBS では、ドックで変えた値が見本と実物の両方に反映されることと、プリセットを別のソースへ読み込めることを確かめた。

## 作成したもの

| ファイル | 内容 |
| --- | --- |
| `src/core/style-schema.h`、`.cpp` | 項目の表、設定の値の型、値の検査、書体の名前の検査 |
| `src/core/youtube-style.h`、`.cpp` | 表の値からの CSS の生成 |
| `src/style-data.h`、`.cpp` | 設定の値と `obs_data` の相互変換 |
| `src/settings-widgets.h`、`.cpp` | 表から欄を作る設定のパネル |
| `src/preset-store.h`、`.cpp` | プリセットの `presets.json` の読み書き |
| `data/preview/youtube.html` | 見本に Super Chat、メンバーシップ、ティッカー、時刻を足した |

### 項目

| 区画 | 項目 |
| --- | --- |
| メッセージ | 書体、文字の大きさ、行の高さ、字間、文字の色、名前の次の行に表示する |
| 名前 | 書体、文字の大きさ、文字の色、配信者・モデレーター・メンバーの色、バッジを表示する、名前の後に「:」を付ける |
| アイコン | アイコンを表示する、大きさ、形 |
| 時刻 | 時刻の表示、書体、文字の大きさ、文字の色 |
| 文字の縁取り | 文字を縁取る、縁の太さ、縁の色 |
| 背景 | 全体の背景色と不透明度、メッセージの背景色、配信者・モデレーター・メンバーの背景色、メッセージの背景の不透明度、メッセージの背景の形 |
| Super Chat とメンバーシップ | 書体、名前・金額と説明・本文の大きさと色、メンバーシップの背景を表示する |
| 表示する部分 | 上部の見出しを表示する、上部の Super Chat の帯を表示する |
| アニメーション | 新しいメッセージを浮かび上がらせる、現れる時間、現れる向き、古いメッセージを消す、消すまでの時間、消える時間 |

最初の版から使っている 4 項目の保存のキーは変えていない。値が無い項目は CSS を生成せず、YouTube の既定の見た目のままにする。

### 6.2 からの変更

- 「ティッカー以外の表示の有無」は置かなかった。チャット欄を隠すと、隠す設定を置かないと決めた YouTube の案内、商品の紹介、固定のメッセージも隠れるためである。
- アニメーションの対象は、通常のメッセージ、Super Chat、メンバーシップ、ステッカーの要素だけにした。YouTube の案内などの要素を、古いメッセージを消す設定で消さないためである。
- Super Chat とメンバーシップの行の高さは、項目として置かなかった。文字の大きさを変えたときに、行が重ならない比率 1.43 を自動で当てる。
- 書体の名前から、`(`、`)`、`"`、`\`、`;`、`{`、`}`、`<`、`>`、`@` と制御文字を取り除く。プリセットのファイルは手で編集できるため、生成する CSS に `url(` や `@import` が入る経路を断つためである。

### プリセット

プラグインの設定のディレクトリの `presets.json` に、名前、チャットの種類、値を保存する。ドックには、選んでいるソースと同じ種類のプリセットだけを表示する。読み込みは、欄に値を写したうえで、設定を変えたときと同じ経路で保存と自動の適用を行う。

## 検証方法

- 単体テストで、表の値からの生成、値の検査、書体の名前の検査、CSS の文字列の逃がし方を確かめた。すべての項目に値を入れたときに、`@import` と `url(` を生成しないことも確かめた。
- Chrome で `https://www.youtube.com/watch?v=K6Po87_quEM` のチャット欄の iframe を開き、再生位置を 600 秒から 3,000 秒まで 600 秒おきに動かして、生成するセレクターの一致する要素の数を数えた。続けて、生成する規則と同じ形の `style` 要素を注入し、計算済みのスタイルを読んだ。
- ポータブル版 OBS で、ドックを `.tmp/2026111007-uia-do.ps1` で操作し、見本と実物のページを `.tmp/2026111016-style-probe.js` で読んだ。

## 検証結果

### 単体テスト

`ctest` の `core-tests` が成功した。ビルドの警告は 0 件だった。

### 実際の YouTube の要素

| セレクター | 一致した要素の数の最大 |
| --- | --- |
| `yt-live-chat-text-message-renderer #message` | 47 |
| `yt-live-chat-text-message-renderer #chat-badges` と `#chip-badges` | 47 |
| `yt-live-chat-text-message-renderer #author-photo img` | 47 |
| `yt-live-chat-text-message-renderer #timestamp` | 47 |
| `yt-live-chat-text-message-renderer:has(#author-name.member)` | 38 |
| `yt-live-chat-paid-message-renderer` の `#author-name`、`#purchase-amount`、`#message`、`#author-photo img` | 15 |
| `yt-live-chat-membership-item-renderer` の `#author-name`、`#header-subtext`、`#message`、`#card`、`#header` | 3 |
| `yt-live-chat-paid-sticker-renderer #author-photo img` | 1 |
| `yt-live-chat-renderer #ticker` | 1 |
| `yt-live-chat-header-renderer` | 1 |

規則を注入した後の計算済みのスタイルは、次のとおり期待した値になった。

- メッセージの `font-size`、`line-height`、`letter-spacing`、`display: block`。
- メンバーの名前の色と、種別の無い名前の色。名前の後の `::after` の `":"`。
- バッジの `display: none`。
- アイコンの大きさと角の丸み。
- 時刻の `display: inline` と色。YouTube の既定では時刻は `display: none` だった。
- メンバーの発言の背景色。
- Super Chat とメンバーシップの名前・金額と説明・本文の大きさと色。
- メンバーシップの `#header` の背景が透明になった。注入の前は `#header` と `#content` が `rgb(15, 157, 88)` だった。
- ティッカーとヘッダーの `display: none`。
- 文字の縁取りの `text-shadow`。

Super Chat のアイコンの画像は、`img` だけに大きさを当てても 32 px のままだった。画像に `max-width: 100%` があり、外側の `#author-photo` が 32 px のままだったためである。外側にも当てると 40 px になった。生成する CSS は外側と画像の両方に当てている。

投稿者の種別の属性 `author-type` には、メンバーの発言で `member` を観測した。モデレーターの発言は観測できなかったため、モデレーターの指定は `:has(#author-name.moderator)` のままにした。

### ポータブル版 OBS

| 操作 | 結果 |
| --- | --- |
| 「ブラウザ」で行の高さ 150 %、字間 2 px、名前の次の行に表示、見出しを隠すに変える | 見本と実物の両方で `line-height: 22.5px`、`letter-spacing: 2px`、`display: block`、見出しの `display: none` になった。見本の資源の読み込みは 0 件だった |
| プリセット「大きめテスト」に保存 | `plugin_config/comment-skin/presets.json` に名前、`youtube`、7 つの値が保存された |
| 元に戻した後に読み込む | 欄と見本が保存した値に戻った |
| 「YouTube チャット」を選んで読み込む | 行の高さが 150 % になり、実物に適用された |
| 2 つのソースを元に戻して OBS を終了 | 2 つとも管理範囲が消え、設定は空になった。`Number of memory leaks: 0` |

## 確認していないこと

- アニメーションと文字の縁取りの画面での見え方。計算済みのスタイルと生成した文字列だけを確かめた。
- 書体の選択、色のダイアログ、プリセットの削除の操作。
- モデレーターの発言での背景色と名前の色。
- 区画を閉じた状態の欄の UI Automation での操作。ドックのスクロールは、スクロールバーの値を 0 にして戻した。

## 考察

確認していない見え方と操作は、ユーザーテスト c2 で依頼者が全群を操作するため、そこで確かめるのが効率的だと考えています。
