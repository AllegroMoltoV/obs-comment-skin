# ドックを閉じるボタンで閉じた後の終了時のクラッシュ

実施日: 2026-11-10
対象の課題: obs-comment-skin-dxs.24

## 結論

主ウィンドウから切り離したドックを、右上の閉じるボタンで閉じた後に OBS を終了すると、OBS が落ちた。閉じるボタンの後のプレビューのブラウザーはメインフレームを返さなくなる。プラグインがそのブラウザーへ CSS を送ったことが原因である。OBS のブラウザーのドックと同じく、閉じるボタンでプレビューのブラウザーを閉じるよう直した。修正版で、切り離したドックで 3 回、主ウィンドウに付けたドックで 1 回試し、クラッシュは 0 回だった。

## 背景

ユーザーテスト d の後、依頼者がドックを閉じてから OBS を終了したところ、OBS が落ちた。

## 検証方法

ポータブル版 OBS 33.0.1 の crashes フォルダーのクラッシュレポートでスタックを読んだ。落ちた位置は、`dumpbin /DISASM` で `obs-browser.dll` を逆アセンブルして調べた。OBS と obs-browser のソースは GitHub の `obsproject/obs-studio` の 33.0.1 と `obsproject/obs-browser` の main を読んだ。再現は、UI Automation でドックを操作し、閉じるボタンの操作だけを依頼者に頼んだ。プレビューのページの有無は CDP の `http://127.0.0.1:9222/json` で調べた。

## 検証結果

### スタック

クラッシュレポートの抜粋を `docs/logs/2026-11-10-dock-close-crash.txt` に置いた。落ちたスレッドの呼び出しの順は次のとおりである。

```
obs-browser.dll!QCefWidgetInternal::executeJavaScript+0x48
comment-skin.dll!cskin::PreviewPanel::push_css+0x7d
comment-skin.dll!cskin::PreviewPanel::set_css+0x61
comment-skin.dll!cskin::CommentSkinDock::update_css_view+0xce
comment-skin.dll!cskin::CommentSkinDock::update_view+0x3bd
obs64.exe!OBSStudioAPI::on_event+0x56
obs64.exe!OBSBasic::ClearSceneData+0x50f
obs64.exe!OBSBasic::closeWindow+0x52a
```

OBS 33.0.1 の `OBSBasic::closeWindow` は、`ClearSceneData()` で `OBS_FRONTEND_EVENT_SCENE_COLLECTION_CLEANUP` を送ってから `OBS_FRONTEND_EVENT_EXIT` を送る。プラグインは CLEANUP でソースの一覧を空にし、プレビューの CSS を空に変えて送る。プレビューのブラウザーを閉じるのは EXIT のときである。

`executeJavaScript+0x48` は、`GetMainFrame()` の戻り値を参照する命令だった。直前で `cefBrowser` の null は確かめているため、ブラウザーの参照はあり、メインフレームが null だった。obs-browser の `executeJavaScript` は、メインフレームの null を確かめない。

```
00000001800356CC: 48 8B 49 28        mov         rcx,qword ptr [rcx+28h]
00000001800356D0: 48 85 C9           test        rcx,rcx
00000001800356D3: 0F 84 C3 01 00 00  je          000000018003589C
00000001800356D9: 48 8B 01           mov         rax,qword ptr [rcx]
00000001800356DC: 48 8D 55 B0        lea         rdx,[rbp-50h]
00000001800356E0: FF 50 70           call        qword ptr [rax+70h]
00000001800356E3: 90                 nop
00000001800356E4: 48 8B 4D B0        mov         rcx,qword ptr [rbp-50h]
00000001800356E8: 48 8B 01           mov         rax,qword ptr [rcx]
```

### 再現の条件

| 条件 | 結果 |
| --- | --- |
| ドックを開いたまま、スタイルを当てた状態で OBS を終了 | 落ちなかった |
| メニューの「ドック」からドックを閉じ、OBS を終了 | 落ちなかった。閉じた後もプレビューのページは CDP に残っていた |
| 主ウィンドウから切り離したドックを、右上の閉じるボタンで閉じ、OBS を終了 (依頼者の操作) | 落ちた。スタックは最初のクラッシュと同じだった |

最初のクラッシュも、依頼者が主ウィンドウから切り離したドックを閉じるボタンで閉じた後に起きた。ドックの配置は、UI Automation で読んだドックと主ウィンドウの位置で判別した。

OBS 33.0.1 の `OBSDock::closeEvent` は、閉じるボタンで閉じたときに `QEvent::User + QEvent::Close` を中身のウィジェットへ送る。OBS のブラウザーのドック `BrowserDock::closeEvent` は、このときに `cefWidget->closeBrowser()` を呼ぶ。obs-browser の `QCefWidgetInternal::showEvent` は、ブラウザーが無ければ作り直す。

### 修正

`CommentSkinDock::event` で `QEvent::User + QEvent::Close` を受け、`PreviewPanel::close_browser()` でプレビューのブラウザーを閉じるようにした。閉じた後の `executeJavaScript` は、obs-browser が `cefBrowser` の null を確かめて何もしない。最新の CSS は `setStartupScript` で渡しているため、作り直したブラウザーにも当たる。

### 修正版の確認

| 確認した内容 | 結果 |
| --- | --- |
| 閉じるボタンで閉じた後の CDP のターゲット | プレビューのページが消えた |
| メニューからドックを開き直した後 | 新しいターゲットで `twitch.html` が開き、閉じる前の設定の CSS が当たっていた |
| 開き直した後に「上部のランキングを表示する」をオフにする | プレビューの CSS にランキングの規則が加わり、OBS は動き続けた |
| 主ウィンドウから切り離したドックを、閉じるボタンで閉じてから OBS を終了 (依頼者の操作、3 回) | Twitch のソースで 2 回、YouTube のソースで 1 回試し、いずれも落ちなかった。新しいクラッシュレポートは無く、ログの最後は `Number of memory leaks: 0` だった |
| 主ウィンドウに付けたドックを、閉じるボタンで閉じてから OBS を終了 (依頼者の操作、1 回) | 落ちなかった。ログの最後は同じく `Number of memory leaks: 0` だった |
| 単体テスト | 成功した |

## 考察

閉じるボタンでのみページが失われる理由は、`QWidget::close()` がドックのネイティブのウィンドウを閉じ、その子の CEF のウィンドウが壊れるためだと推測しています。OBS のブラウザーのドックが閉じるボタンでブラウザーを閉じているのも、同じ事情への対応だと考えています。

修正前は、閉じるボタンでドックを閉じた後に、ソースの追加や削除でドックの表示が更新されると、終了時でなくても落ちた可能性があると考えています。この経路は修正前の版で試していません。

## 確認していないこと

- 修正前の版で、主ウィンドウに付けたドックを閉じるボタンで閉じたとき。
- macOS と Linux での動作。0.1.0 の公開で CI を動かすときに、ビルドの成否だけを確かめる。
