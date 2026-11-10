# リポジトリの土台と最小のドックのプラグイン

実施日: 2026-11-10
対象の課題: obs-comment-skin-dxs.2

## 結論

obs-loudness-balance-monitor のテンプレート由来のファイルをそろえ、ドックだけを持つ最小のプラグイン `comment-skin` 0.1.0 をローカルでビルドした。ポータブル版 OBS 33.0.1 で、プラグインの読み込み、ドックの登録、保存済みのドックの状態の再適用をログで確認した。保存済みの状態を表示に書き換えて起動すると、ドックがフローティングのウィンドウとして表示された。ドックのメニューからの開閉と、メニューで操作した後の再起動での復元は、GUI の操作が必要なため確認していない。

## 作成したもの

参照元 `D:/work/obs-loudness-balance-monitor` の git で追跡されたファイルから、次を複製した。

- `.clang-format`、`.gersemirc`、`.gitignore`、`LICENSE`
- `build-aux/`、`cmake/`
- `.github/actions/`、`.github/scripts/`、`.github/workflows/`

参照元に固有のため、次は複製しなかった。

- `.github/baseline/`、`.github/tests/`、`.github/workflows/obs33-baseline.yaml`
- `.github/scripts/Test-ObsLoad-Windows.ps1`、`.github/scripts/run-ubuntu-smoke`

複製したうえで、次を本プラグイン用に変えた。

- `buildspec.json`: `name` を `comment-skin`、`displayName` を `コメント欄の着せ替え / Comment Skin`、`version` を `0.1.0`、`bundleId` を `jp.allegromoltov.obs-comment-skin` にした。`name` は、リポジトリ名から `obs-` を除く参照元の慣例にしたがった。作者、Web サイト、メールアドレスは参照元と同じ作者の値のままである。
- `CMakePresets.json`: 参照元のテスト用の `ENABLE_PREVIEW_TESTS` を削った。
- `CMakeLists.txt`: libebur128、音声の API のリンク、検証用のプローブとテストを除いて書き直した。`ENABLE_FRONTEND_API`、`ENABLE_QT`、`ENABLE_POST_BUILD_INSTALL` は参照元と同じである。
- `.github/scripts/Build-Windows.ps1`: 検証用のプローブを有効にする `-BaselineProbe` を削った。
- `.github/scripts/.Aptfile`: 音声の再生にだけ使う `libpulse-dev` を削った。
- `src/plugin-support.c.in`、`src/plugin-support.h`: テンプレートのままだった著作権表記を本プラグインの名前と作者にした。

新たに書いたファイルは次のとおり。

- `src/plugin-main.cpp`: 読み込み完了のイベントでドックを `obs_frontend_add_dock_by_id()` で登録し、直後に `BasicWindow/DockState` を `QMainWindow::restoreState()` で再適用する。参照元の `src/plugin-main.cpp` の 47 から 54 行の処理を写した。シーンコレクションの切り替えのイベントで、ドックの候補の一覧を作り直す。
- `src/comment-skin-dock.h`、`src/comment-skin-dock.cpp`: ブラウザーソースのコンボボックスと「テスト用 CSS を書き込む」ボタンを持つドック。一覧は、一覧を開くときに `obs_enum_sources()` で型 ID が `browser_source` のソースから作り直す。ボタンは、選んだソースの `css` の末尾に、目印の注釈で囲んだテスト用の規則と書き込みの時刻を置き、`obs_source_update()` で反映する。2 回目以降は目印の範囲だけを置き換える。この操作は課題 obs-comment-skin-dxs.4 の検証用である。
- `data/locale/en-US.ini`、`data/locale/ja-JP.ini`: ドックのタイトル `DockTitle` と、ドックの文言。`DockTitle` は `.prompts/INIT.md` の 4.2 の決定にしたがい、両方とも `コメント欄の着せ替え / Comment Skin` にした。

## 検証結果

### ビルド

| コマンド | 結果 | ログ |
| --- | --- | --- |
| `cmake --preset windows-ci-x64 -DENABLE_POST_BUILD_INSTALL=OFF` | 終了コード 0。OBS のソースと obs-deps、Qt6 の取得を含めて `Configuring done (121.1s)` | `.logs/2026111002-configure.log` |
| `cmake --build --preset windows-ci-x64` | 終了コード 0。警告とエラーの行は 0 件。成果物は `build_x64/RelWithDebInfo/comment-skin.dll` | `.logs/2026111003-build.log` |

ビルドのログの警告とエラーは、`grep -i -E "warning|error"` で数えた。プリセットは `CMAKE_COMPILE_WARNING_AS_ERROR` を有効にしている。

整形の検査は次のとおり。

- `gersemi --check CMakeLists.txt` が終了コード 0 だった。
- Visual Studio 2022 に同梱の clang-format 19.1.5 で `--dry-run --Werror` を `src/` の C++ のファイルとヘッダーに実行し、終了コード 0 だった。CI が求める版は 19.1.1 で、版が一致しない。

### ポータブル版 OBS 33.0.1

`D:/work/obs-loudness-balance-monitor/.tmp/2026110301-OBS-Studio-33.0.1-Windows-x64.zip` を `.tmp/obs-33.0.1-portable/` に展開し、直下に `portable_mode` を作った。プラグインは `obs-plugins/64bit/comment-skin.dll` と `data/obs-plugins/comment-skin/locale/` に置いた。起動は PowerShell の `Start-Process -PassThru`、終了は `CloseMainWindow()` と `WM_CLOSE` による正常終了で行った。3 回とも終了後に、展開先の配下で動くプロセスが 0 件であることを `Get-Process` で確かめた。

| 回 | 条件 | ログ | 確認した内容 |
| --- | --- | --- | --- |
| 1 | 初回。設定なし | `2026-11-10 15-40-46.txt` | 読み込みと登録。保存済みの状態が無いため再適用の行は無い。自動構成ウィザードは閉じた |
| 2 | 1 回目の終了で保存された状態 | `2026-11-10 15-42-07.txt` | 再適用の成功。ドックのウィンドウは無い |
| 3 | 保存済みの状態を表示に書き換え | `2026-11-10 15-42-54.txt` | 再適用の成功。ドックのウィンドウがある |

ログは `.tmp/obs-33.0.1-portable/config/obs-studio/logs/` にある。3 回目のログの該当行は次のとおり。1 回目と 2 回目も、再適用の行を除いて同じ内容の行がある。

```
15:42:55.045: Portable mode: true
15:42:56.425: [comment-skin] plugin loaded successfully (version 0.1.0)
15:42:56.425:     comment-skin.dll
15:42:56.707: [comment-skin] Dock registered successfully
15:42:56.721: [comment-skin] Restored OBS dock state after registration
15:43:09.805: [comment-skin] plugin unloaded
15:43:10.090: Number of memory leaks: 0
```

ドックの表示の状態は、次の方法で確かめた。

- `config/obs-studio/user.ini` の `DockState` を Base64 で復号し、ドックの名前の直後の 1 バイトを読んだ。1 回目の終了後は `comment-skin-dock` が `0x02` で、同じく非表示の `statsDock` と同じ値、表示中の `scenesDock` などは `0x01` だった。
- 2 回目の起動後に、OBS のプロセスの表示中の最上位ウィンドウを `EnumWindows` で列挙すると、OBS の主ウィンドウだけだった。
- `comment-skin-dock` の値を `0x03` に書き換えて 3 回目を起動すると、`コメント欄の着せ替え / Comment Skin` という題の最上位ウィンドウが列挙された。3 回目の終了後も `user.ini` の値は `0x03` のままだった。

書き換えに使ったスクリプトはセッションの一時領域に置いたもので、リポジトリには含めていない。

展開した ZIP では、標準のプラグインが `core/<名前>/` の形式で置かれていた。`obs-plugins/64bit/` は ZIP に含まれず、今回作成した。3 回のログとも、読み込み済みのモジュールの一覧に `comment-skin.dll` がある。

### 確認していないこと

- ドックのメニューに項目が表示され、操作でドックを開閉できること。
- メニューで開いて終了した後の再起動で、ドックが表示された状態に戻ること。3 回目の検証は `user.ini` を直接書き換えた代用である。
- ドックのコンボボックスへのブラウザーソースの表示と、「テスト用 CSS を書き込む」ボタンの動作。課題 obs-comment-skin-dxs.4 で確かめる。
- macOS と Linux のビルドと、CI の整形の検査。GitHub への push をしていないため、CI は実行していない。

## 考察

- `DockState` のフラグは、表示が 1、フローティングが 2 を表すと解釈しました。既知の表示中と非表示のドックの値がこの解釈と一致したためです。Qt のソースとは照合していないため、この解釈は推測にとどまると考えています。
- 3 回目でドックがフローティングのウィンドウとして表示されたのは、`obs_frontend_add_dock_by_id()` が登録の直後にドックをフローティングにし、その状態が保存されていたためと推測します。
- 標準のプラグインが `core/<名前>/` に置かれていても、従来の `obs-plugins/64bit/` に置いた本プラグインは読み込まれました。このため、親課題の design に書いた配置の手順は 33.0.1 で使えると考えています。
