# YouTube のポップアウトのチャットの DOM 調査

調査日: 2026-11-10。Beads の課題 `obs-comment-skin-dxs.3` の記録です。

## はじめに

CSS の生成規則で使うセレクターを決めるため、YouTube のライブチャットのページの DOM を調べました。chatv2.septapus.com の出力は参照していません。投稿者の名前とメッセージの本文は取得せず、要素名・ID・クラス・属性名だけを記録しています。

## 検証方法

Claude in Chrome で依頼者の Chrome に新しいタブを開き、ページの中で JavaScript を実行して、要素の木構造を名前だけで書き出しました。

| 対象 | URL | 用途 |
| --- | --- | --- |
| ライブ 1 | `https://www.youtube.com/live_chat?is_popout=1&v=s7VV5NhQbqc` | 通常のメッセージと全体の構造 |
| ライブ 2 | `https://www.youtube.com/live_chat?is_popout=1&v=CVVyTYmmgN8` | 投稿者の種別の追加の観測 |
| アーカイブ | `https://www.youtube.com/watch?v=K6Po87_quEM` の `#chatframe` | Super Chat、メンバーシップ、ステッカー、ティッカー |

アーカイブは収益化の記念配信です。`live_chat_replay?v=<動画 ID>` を直接開くと「Something went wrong」と表示されたため、視聴ページのチャット欄の iframe を使いました。iframe の文書は `/live_chat_replay` で、同じオリジンのため `contentDocument` で読めます。再生位置を 300 秒から 600 秒おきに動かして、出現した要素を集めました。

要件の文書が第一候補とした `jfKfPfyJRdk` は、ポップアウトに「このライブ ストリームではチャットは無効です。」と表示され、調査に使えませんでした。

Chrome は YouTube にログインした状態でした。`ytcfg.get('LOGGED_IN')` が `true` を返し、ルートの `html` 要素に `dark` 属性がありました。

## 検証結果

### 全体の構造

Shadow DOM を持つ要素は 0 件、iframe は 0 件でした。ブラウザーソースのカスタム CSS が届く構造です。`yt-live-chat-*` の要素名は現在も使われています。

ライブ 1 で `yt-live-chat-renderer` の子孫を深さ 4 まで書き出した結果の抜粋です。`(hidden)` は `hidden` 属性または `display: none` の要素です。

```text
div#promo
tp-yt-iron-pages#content-pages
  div#chat-messages
    yt-live-chat-header-renderer
      div#primary-content
      div#action-buttons
      div#viewer-leaderboard-entry-point
      yt-live-chat-button#live-chat-header-context-menu
      div#close-button
    div#contents
      div#ticker
        yt-live-chat-ticker-renderer (hidden)
      div#separator
      div#chat
        yt-live-chat-pinned-message-renderer#pinned-message (hidden)
        div#item-list
        div#action-panel (hidden)
        yt-live-chat-animation-overlay-renderer#animation-overlay (hidden)
        ytls-widget-overlay-manager
      yt-live-chat-banner-manager#live-chat-banner
      div#inline-toast-container
      tp-yt-iron-pages#panel-pages
        div#input-panel
        (中略)
  yt-live-chat-participant-list-renderer (hidden)
  (中略)
```

メッセージの並びは次の経路の `div#items` の直下です。

```text
yt-live-chat-item-list-renderer#live-chat-item-list-panel > div#contents > div#item-scroller > div#item-offset > div#items
```

計算済みのスタイルの背景色は、`yt-live-chat-renderer` が `rgb(15, 15, 15)` でした。`html`、`body`、`yt-live-chat-item-list-renderer`、`yt-live-chat-text-message-renderer` は `rgba(0, 0, 0, 0)` でした。

### 通常のメッセージ

`yt-live-chat-text-message-renderer` の構造です。

```text
yt-img-shadow#author-photo  (img#img width=24 height=24)
div#content
  span#timestamp
  yt-live-chat-author-chip
    span#prepend-chat-badges
    span#author-name.member
      span#chip-badges
    span#chat-badges
      yt-live-chat-author-badge-renderer[type=member]
        div#image
  div#before-content-buttons
  span#message-container
    span#message-prefix-icon-container
    span#message
      img#emoji-30.emoji
```

ホストの要素の属性は `modern`、`enable-improved-visibility-style`、`enable-banner-update`、`whole-message-clickable`、`author-type` でした。`author-type` の値として観測したのは `member` と空文字だけです。`#author-name` のクラスとして観測したのは `member` と `single-line` だけです。

### 投稿者の種別

配信者とモデレーターの発言は、ライブ 2 本とアーカイブの 5 か所のいずれでも観測できませんでした。そこで、ライブ 2 のページが読み込んだスタイルシート 15 件、14,873 規則のセレクターから、投稿者の種別に関わるものを抜き出しました。読めなかったスタイルシートは 0 件です。

```text
#author-name.owner
#author-name.moderator
#author-name.member
yt-live-chat-author-badge-renderer[type="owner"]
yt-live-chat-author-badge-renderer[type="moderator"]
yt-live-chat-author-badge-renderer[type="member"]
yt-live-chat-author-badge-renderer[type="verified"]
```

`author-type` 属性を使うセレクターは見つかりませんでした。

### Super Chat

`yt-live-chat-paid-message-renderer` の構造です。

```text
div#card
  div#header
    yt-img-shadow#author-photo
    div#header-content
      div#header-content-primary-column
        div#single-line
          div#author-name-chip
            yt-live-chat-author-chip
          div#leaderboard-badge
          div#purchase-amount-column
            div#purchase-amount
      span#timestamp (hidden)
      div#gradient-container
      div#menu
      div#creator-heart-button
  div#content
    div#message
    (中略)
    div#action-buttons
div#lower-bumper
```

ホストの要素の `style` 属性には次のカスタムプロパティが設定されていました。金額の帯ごとの色は、この値で決まります。

```text
--yt-live-chat-paid-message-primary-color
--yt-live-chat-paid-message-secondary-color
--yt-live-chat-paid-message-header-color
--yt-live-chat-paid-message-timestamp-color
--yt-live-chat-paid-message-color
--yt-live-chat-disable-highlight-message-author-name-color
--yt-live-chat-text-input-background-color
```

### メンバーシップ

`yt-live-chat-membership-item-renderer` の構造です。観測した要素には `show-only-header` 属性がありました。

```text
div#card
  div#header
    yt-img-shadow#author-photo
    div#header-content
      div#header-content-primary-column
        div#header-content-inner-column
          yt-live-chat-author-chip
          div#header-primary-text (hidden)
        div#header-subtext
      div#timestamp (hidden)
    div#menu
  div#content (hidden)
    div#message
```

### ステッカー

`yt-live-chat-paid-sticker-renderer` の構造です。

```text
div#card
  div#author-info
    yt-img-shadow#author-photo
    div#content
      span#timestamp
      div#content-primary-column
  div#sticker-container
    yt-img-shadow#sticker
  div#menu
  div#gradient-container
  div#creator-heart-button
div#lower-bumper
div#buy-flow-button
```

### ティッカー

`yt-live-chat-ticker-renderer` の構造と、項目の例です。

```text
div#container
  span#ticker-bar
    div#goal-ticker
    div#fanzone-ticker
    div#ticker-items
  div#left-arrow-container
  div#right-arrow-container

yt-live-chat-ticker-paid-message-item-renderer
  div#container
    div#content
      yt-img-shadow#author-photo
      div#animation-container
        div#text
        div#stats-bar
```

アーカイブで観測したティッカーの項目は、`yt-live-chat-ticker-paid-message-item-renderer`、`yt-live-chat-ticker-sponsor-item-renderer`、`yt-live-chat-ticker-paid-sticker-item-renderer` の 3 種類です。

### ライブとアーカイブの共通性

アーカイブの通常のメッセージは、ライブと同じ `yt-live-chat-text-message-renderer` で、`yt-live-chat-author-chip` の中に `span#prepend-chat-badges`、`span#author-name.member`、`span#chat-badges` を持ちました。ライブ 1 の構造と一致します。

### 広告・帰属表示・リンクに当たる要素

`div#items` の直下に、メッセージ以外の次の要素がありました。

- `yt-live-chat-viewer-engagement-message-renderer`: チャンネル登録者のみモードなどの、YouTube からの案内
- `yt-live-chat-product-picker-panel-item-view-model`: 商品の紹介に関わる要素

`div#items` の中のリンクは次の 3 種類でした。

```text
div#content > a#show-original  x92
yt-formatted-string#message > a.yt-simple-endpoint.bold  x1
yt-button-shape > a.ytSpecButtonShapeNextHost...  x1
```

このほか、`div#promo`、`yt-live-chat-banner-manager#live-chat-banner`、`yt-live-chat-pinned-message-renderer#pinned-message` があります。

## 確認できなかった事項

- `author-type` 属性の `owner` と `moderator` の値。名前の色はスタイルシートで確認した `#author-name.owner` と `#author-name.moderator` で指定できる。メッセージの背景を種別ごとに変えるには、ホストの要素での種別の判別方法が別に必要になる。
- メンバーシップのギフトの要素。スタイルシートに `#author-name.yt-gift-message-view-model` がある。実物は観測していない。
- 未ログインの表示。Chrome がログインしていたため、OBS のブラウザーソースの未ログインの表示との差は確認していない。入力欄の周辺と、`dark` 属性の有無に差が出る可能性がある。
- `show-only-header` を持たないメンバーシップの要素。メンバー歴のメッセージ付きの表示で、`div#content` が表示される場合がある。

## 考察

6.2 の群のうち、文字の縁取り、アイコン、名前、メッセージ、時刻、背景、Super Chat とメンバーシップ、ティッカーは、上の要素で対象を指定できると考えています。

配信者・モデレーター・メンバーごとの名前の色は、`#author-name` のクラスで指定できると考えています。メッセージの背景の種別ごとの色分けは、`author-type` の値が確認できないため、`:has(#author-name.owner)` のような指定が候補になります。OBS 33.0.1 の CEF が `:has()` に対応するかは、実装の段階で確かめる必要があります。

`yt-live-chat-viewer-engagement-message-renderer`、`yt-live-chat-product-picker-panel-item-view-model`、`div#promo`、バナー、固定のメッセージ、メッセージ中のリンクは、YouTube の案内や帰属表示、商品の紹介に当たると考えています。依頼者の決定 4.1 の 4 にしたがい、これらを隠す設定は用意しません。ポップアウトのヘッダーと入力欄を隠す設定の要否は、YouTube の要件の 6.2 に含まれていないため、第 2 層の計画を書き直すときに扱いを決めます。
