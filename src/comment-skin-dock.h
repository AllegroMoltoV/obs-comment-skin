/*
Comment Skin
Copyright (C) 2026 AllegroMoltoV <contact@allegromoltov.jp>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#pragma once

#include "core/chat-target.h"

#include <obs.h>

#include <QComboBox>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTimer>
#include <QToolButton>
#include <QWidget>

#include <string>

namespace cskin {

class PreviewPanel;
class SettingsPanel;

// 一覧を開くたびに候補を作り直すコンボボックス
class SourceComboBox : public QComboBox {
	Q_OBJECT

public:
	using QComboBox::QComboBox;
	void showPopup() override;

signals:
	void popup_requested();
};

class CommentSkinDock : public QWidget {
	Q_OBJECT

public:
	explicit CommentSkinDock(QWidget *parent = nullptr);

	void refresh_sources();
	void clear_sources();
	// OBS の終了の前に、見本のブラウザーを閉じる
	void shutdown();

private slots:
	void on_source_changed();
	void on_target_changed();
	void on_create_source();
	void on_undo();
	void on_copy_css();
	void on_settings_changed();
	void apply_pending();
	void on_preset_save();
	void on_preset_load();
	void on_preset_delete();

private:
	void refresh_presets(const QString &select_name = {});
	// 選択中のソース。参照を 1 つ持つため、呼び出し側が obs_source_release() で返す
	obs_source_t *selected_source() const;
	core::ChatTarget current_target() const;
	std::string generated_css() const;
	// チャットの種類の設定のパネル。対応していない種類では nullptr を返す
	SettingsPanel *panel_for(core::ChatTarget target) const;
	void update_view();
	void update_css_view();
	void set_status(const QString &message);

	// 1 回の操作で起きる連続した変更をまとめるための待ち時間
	static constexpr int APPLY_DELAY_MS = 150;

	SourceComboBox *source_combo_{nullptr};
	QPushButton *create_source_button_{nullptr};
	QComboBox *target_combo_{nullptr};
	QLabel *target_hint_label_{nullptr};
	SettingsPanel *youtube_panel_{nullptr};
	SettingsPanel *twitch_panel_{nullptr};
	PreviewPanel *preview_{nullptr};
	QLabel *preview_label_{nullptr};
	QWidget *preset_row_{nullptr};
	QComboBox *preset_combo_{nullptr};
	QPushButton *preset_load_button_{nullptr};
	QPushButton *preset_save_button_{nullptr};
	QPushButton *preset_delete_button_{nullptr};
	QPushButton *undo_button_{nullptr};
	QTimer *apply_timer_{nullptr};
	QString pending_uuid_; // 適用を待っているソース
	QToolButton *css_toggle_button_{nullptr};
	QPlainTextEdit *css_view_{nullptr};
	QPushButton *copy_css_button_{nullptr};
	QLabel *status_label_{nullptr};
};

} // namespace cskin
