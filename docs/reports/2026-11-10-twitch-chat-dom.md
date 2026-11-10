# Twitch のポップアウトのチャットの DOM 調査

調査日: 2026-11-10。Beads の課題 `obs-comment-skin-dxs.13` の記録です。

## はじめに

段階 d で Twitch のチャットの CSS を生成するため、ポップアウトのチャットのページの DOM を調べました。依頼者の承認を得て、0.1.0 の公開の準備と並行して前倒しで行いました。投稿者の名前とメッセージの本文は記録せず、要素名・クラス名・属性名・バッジの画像の ID だけを記録しています。

## 検証方法

Claude in Chrome で依頼者の Chrome に新しいタブを開き、ページの中で JavaScript を実行して読みました。Chrome は Twitch に未ログインの状態でした。依頼者の指示で OBS を起動しない期間だったため、OBS のブラウザーソースでの確認は行っていません。

| 対象 | URL の形 | 用途 |
| --- | --- | --- |
| ライブ 1 | `https://www.twitch.tv/popout/<チャンネル名>/chat?popout=` | 全体の構造、通常のメッセージ、サブスクの通知。約 80 秒、行 1,200 件ほどを観測 |
| ライブ 2 | 同上 | モデレーターのバッジ、案内の行。約 40 秒を観測 |

チャンネルは、`https://www.twitch.tv/directory/all` の上位から選びました。

配信者・VIP のバッジの画像の ID を確かめるため、Twitch のページが使う GraphQL の窓口にグローバルバッジの一覧を問い合わせようとしましたが、Claude Code の安全上の判断で許可されませんでした。この方法は使っていません。

## 検証結果

### 全体の構造

Shadow DOM を持つ要素は 0 件でした。iframe は 1 件あり、`aax-fe.amazon-adsystem.com` の広告の iframe でした。

```text
div.stream-chat
  div.stream-chat-header[role=region]          高さ 40 px。「チャット」の見出しとボタン 1 つ
  section.chat-room[data-a-target=chat-theme-dark]
    div.chat-room__content
      (中略)
        div.chat-scrollable-area__message-container[role=log]   メッセージの並び
      div.chat-room__notifications
      div.chat-input                           高さ 141 px。入力欄とボタン
      div.chat-room__viewer-card[data-a-target=chat-user-card]
```

クラス名には、`Layout-sc-1xcs6mc-0` のような生成された名前と、`chat-line__message` のような意味の分かる名前が並んでいます。生成された名前は版ごとに変わる可能性があるため、意味の分かる名前と `data-a-target`、`data-test-selector` の属性だけをセレクターの候補にします。

計算済みの背景色は、`body` が `rgb(14, 14, 16)`、`section.chat-room` が `rgb(24, 24, 27)`、`div.stream-chat` と `div.chat-room__content` が透明でした。

### 通常のメッセージ

```text
div.chat-line__message[role=group][data-a-target=chat-line-message]
  div
    div.chat-line__message-highlight
    div.chat-line__message-container
      div
      div
        div.chat-line__no-background
          div
            div.chat-line__username-container.chat-line__username-container--hoverable
              span                                    バッジ。img.chat-badge[data-a-target=chat-badge]
              span.chat-line__username[role=button]
                span
                  span.chat-author__display-name[data-a-target=chat-message-username]
            span                                      名前の後の「: 」
            span[data-a-target=chat-line-message-body]
              span.text-fragment[data-a-target=chat-message-text]
    div.chat-line__icons
      div.chat-line__reply-icon
```

- 名前の色は、`span.chat-author__display-name` のインラインの `style` に `color: rgb(255, 69, 0);` の形で書かれていました。利用者ごとの色です。
- 本文は `font-size: 14px`、`line-height: 22px` で、書体は `Inter` から始まる一覧でした。
- 時刻の要素 `.chat-line__timestamp` は 0 件でした。既定では表示されていません。
- スタンプだけのメッセージには `chat-line__message--emote` があり、スタンプの名前に `data-a-target=emote-name` がありました。

### バッジ

バッジは `img.chat-badge` で、画像の URL の `badges/v1/<ID>/` に ID があります。`alt` は表示の言語の文字列で、日本語では「モデレーターバッジ」のような形でした。サブスクライバーのバッジの `alt` は、継続の月数と年数のバッジ名を含む文字列でした。

| 種別 | 観測 | ID |
| --- | --- | --- |
| モデレーター | ライブ 2 で観測 | `3267646d-33f0-4b17-b3df-f923a41db1d0` |
| サブスクライバー | ライブ 1 で多数観測 | チャンネルごとに違う。`alt` に「サブスクライバー」を含む |
| 配信者、VIP | 観測できなかった | 未確認 |

ライブ 1 の約 1,200 行と、ライブ 2 の約 140 行に、配信者と VIP のバッジは出ませんでした。

### 特別な行

| 行 | 見分け方 | 観測 |
| --- | --- | --- |
| サブスクの通知 | `[data-test-selector=user-notice-line]`。中の再サブスクのメッセージは通常のメッセージと同じ `chat-line__message` の作り | ライブ 1 で 12 件 |
| 案内「〜さんのチャットルームにようこそ！」 | `div.chat-line__status[data-a-target=chat-welcome-message]`。リンクは 0 件 | ライブ 2 で 8 件 |
| Bits の応援、強調表示のメッセージ、告知 | 未確認 | 観測できなかった |

## 確認できなかった事項

- OBS のブラウザーソースでの表示。未ログインの Chrome で調べたため近い条件と考えますが、OBS の CEF での DOM と計算済みのスタイルは確かめていません。
- 配信者と VIP のバッジの ID。
- Bits の応援、強調表示のメッセージ、告知の要素。
- 入力欄の中のリンクの有無。ボタンは 6 つで、リンクは数えていません。

## 考察

Twitch のチャットも、Shadow DOM と iframe の中に無いため、カスタム CSS で見た目を変えられると考えています。広告の iframe は、YouTube と同じ方針で隠す設定を置かない対象だと考えています。

メッセージ、名前、本文、バッジ、ヘッダー、入力欄、サブスクの通知、案内は、意味の分かるクラス名と `data-a-target` の属性で指定できると考えています。配信者・モデレーター・VIP・サブスクライバーごとの色分けは、`:has()` でバッジの画像の ID を手がかりにする方法が候補です。サブスクライバーは ID がチャンネルごとに違うため、`alt` の文字列に頼るしかなく、表示の言語に依存します。

段階 d の計画では、配信者と VIP のバッジの ID、Bits の応援、強調表示のメッセージを観測する手段と、OBS のブラウザーソースでの確認を最初に置く必要があると考えています。
