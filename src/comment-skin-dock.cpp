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

#include "comment-skin-dock.h"

#include "core/managed-css.h"
#include "core/youtube-style.h"
#include "create-source-dialog.h"
#include "preset-store.h"
#include "preview-panel.h"
#include "settings-widgets.h"
#include "source-state.h"

#include <obs-module.h>
#include <plugin-support.h>

#include <QApplication>
#include <QClipboard>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QScrollArea>
#include <QString>
#include <QVBoxLayout>

#include <algorithm>
#include <cstring>
#include <utility>
#include <vector>

namespace cskin {

namespace {

constexpr const char *BROWSER_SOURCE_ID = "browser_source";

QString text(const char *key)
{
	return QString::fromUtf8(obs_module_text(key));
}

QString target_display_name(core::ChatTarget target)
{
	switch (target) {
	case core::ChatTarget::YouTube:
		return QStringLiteral("YouTube");
	case core::ChatTarget::Twitch:
		return QStringLiteral("Twitch");
	case core::ChatTarget::Unknown:
		break;
	}
	return {};
}

QString target_data(core::ChatTarget target)
{
	return QString::fromUtf8(core::target_to_string(target).data());
}

// 候補の名前と UUID。UUID で選ぶと、名前を変えられても同じソースを指せる
using SourceEntry = std::pair<QString, QString>;

bool collect_browser_source(void *param, obs_source_t *source)
{
	const char *id = obs_source_get_unversioned_id(source);
	if (!id || std::strcmp(id, BROWSER_SOURCE_ID) != 0)
		return true;

	auto *entries = static_cast<std::vector<SourceEntry> *>(param);
	entries->emplace_back(QString::fromUtf8(obs_source_get_name(source)),
			      QString::fromUtf8(obs_source_get_uuid(source)));
	return true;
}

QString source_name(obs_source_t *source)
{
	return QString::fromUtf8(obs_source_get_name(source));
}

} // namespace

void SourceComboBox::showPopup()
{
	emit popup_requested();
	QComboBox::showPopup();
}

CommentSkinDock::CommentSkinDock(QWidget *parent) : QWidget(parent)
{
	auto *content = new QWidget();
	auto *layout = new QVBoxLayout(content);

	// ブラウザーソース
	auto *source_label = new QLabel(text("BrowserSource"), content);
	source_combo_ = new SourceComboBox(content);
	source_combo_->setPlaceholderText(text("NoBrowserSource"));
	source_combo_->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
	source_label->setBuddy(source_combo_);
	create_source_button_ = new QPushButton(text("CreateSource.Button"), content);
	auto *source_row = new QHBoxLayout();
	source_row->addWidget(source_combo_, 1);
	source_row->addWidget(create_source_button_);

	// チャットの種類
	auto *target_label = new QLabel(text("ChatTarget"), content);
	target_combo_ = new QComboBox(content);
	target_combo_->addItem(target_display_name(core::ChatTarget::YouTube), target_data(core::ChatTarget::YouTube));
	target_combo_->addItem(target_display_name(core::ChatTarget::Twitch), target_data(core::ChatTarget::Twitch));
	target_combo_->setPlaceholderText(text("ChatTarget.Select"));
	target_label->setBuddy(target_combo_);
	target_hint_label_ = new QLabel(content);
	target_hint_label_->setWordWrap(true);

	// 設定の項目
	settings_label_ = new QLabel(content);
	settings_label_->setWordWrap(true);
	youtube_panel_ = new SettingsPanel(core::youtube_sections(), content);
	youtube_panel_->setVisible(false);
	preview_ = new PreviewPanel(content);
	preview_->setVisible(false);

	// プリセット
	preset_row_ = new QWidget(content);
	auto *preset_layout = new QHBoxLayout(preset_row_);
	preset_layout->setContentsMargins(0, 0, 0, 0);
	auto *preset_label = new QLabel(text("Preset"), preset_row_);
	preset_combo_ = new QComboBox(preset_row_);
	preset_combo_->setPlaceholderText(text("Preset.None"));
	preset_combo_->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
	preset_label->setBuddy(preset_combo_);
	preset_load_button_ = new QPushButton(text("Preset.Load"), preset_row_);
	preset_save_button_ = new QPushButton(text("Preset.Save"), preset_row_);
	preset_delete_button_ = new QPushButton(text("Preset.Delete"), preset_row_);
	preset_layout->addWidget(preset_label);
	preset_layout->addWidget(preset_combo_, 1);
	preset_layout->addWidget(preset_load_button_);
	preset_layout->addWidget(preset_save_button_);
	preset_layout->addWidget(preset_delete_button_);
	preset_row_->setVisible(false);

	// 自動の適用と元に戻す
	undo_button_ = new QPushButton(text("Undo"), content);
	undo_button_->setToolTip(text("Undo.Tooltip"));
	auto *action_row = new QHBoxLayout();
	action_row->addStretch();
	action_row->addWidget(undo_button_);
	auto *apply_hint_label = new QLabel(text("ApplyHint"), content);
	apply_hint_label->setWordWrap(true);
	apply_timer_ = new QTimer(this);
	apply_timer_->setSingleShot(true);
	apply_timer_->setInterval(APPLY_DELAY_MS);

	// 生成した CSS
	css_toggle_button_ = new QToolButton(content);
	css_toggle_button_->setText(text("GeneratedCss"));
	css_toggle_button_->setCheckable(true);
	css_toggle_button_->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	css_toggle_button_->setArrowType(Qt::RightArrow);
	css_toggle_button_->setAutoRaise(true);
	css_view_ = new QPlainTextEdit(content);
	css_view_->setReadOnly(true);
	css_view_->setMinimumHeight(120);
	copy_css_button_ = new QPushButton(text("CopyCss"), content);
	css_view_->setVisible(false);
	copy_css_button_->setVisible(false);

	status_label_ = new QLabel(content);
	status_label_->setWordWrap(true);

	layout->addWidget(source_label);
	layout->addLayout(source_row);
	layout->addWidget(target_label);
	layout->addWidget(target_combo_);
	layout->addWidget(target_hint_label_);
	layout->addWidget(settings_label_);
	layout->addWidget(preview_);
	layout->addWidget(preset_row_);
	layout->addWidget(youtube_panel_);
	layout->addLayout(action_row);
	layout->addWidget(apply_hint_label);
	layout->addWidget(css_toggle_button_);
	layout->addWidget(css_view_);
	layout->addWidget(copy_css_button_);
	layout->addWidget(status_label_);
	layout->addStretch();

	auto *scroll = new QScrollArea(this);
	scroll->setWidgetResizable(true);
	scroll->setFrameShape(QFrame::NoFrame);
	scroll->setWidget(content);
	auto *outer = new QVBoxLayout(this);
	outer->setContentsMargins(0, 0, 0, 0);
	outer->addWidget(scroll);

	connect(source_combo_, &SourceComboBox::popup_requested, this, &CommentSkinDock::refresh_sources);
	connect(source_combo_, &QComboBox::currentIndexChanged, this, &CommentSkinDock::on_source_changed);
	connect(create_source_button_, &QPushButton::clicked, this, &CommentSkinDock::on_create_source);
	connect(target_combo_, &QComboBox::currentIndexChanged, this, &CommentSkinDock::on_target_changed);
	connect(apply_timer_, &QTimer::timeout, this, &CommentSkinDock::apply_pending);
	connect(undo_button_, &QPushButton::clicked, this, &CommentSkinDock::on_undo);
	connect(copy_css_button_, &QPushButton::clicked, this, &CommentSkinDock::on_copy_css);
	connect(youtube_panel_, &SettingsPanel::changed, this, &CommentSkinDock::on_settings_changed);
	connect(preset_load_button_, &QPushButton::clicked, this, &CommentSkinDock::on_preset_load);
	connect(preset_save_button_, &QPushButton::clicked, this, &CommentSkinDock::on_preset_save);
	connect(preset_delete_button_, &QPushButton::clicked, this, &CommentSkinDock::on_preset_delete);
	connect(preset_combo_, &QComboBox::currentIndexChanged, this, [this](int index) {
		preset_load_button_->setEnabled(index >= 0);
		preset_delete_button_->setEnabled(index >= 0);
	});
	connect(css_toggle_button_, &QToolButton::toggled, this, [this](bool open) {
		css_toggle_button_->setArrowType(open ? Qt::DownArrow : Qt::RightArrow);
		css_view_->setVisible(open);
		copy_css_button_->setVisible(open);
	});

	refresh_sources();
}

void CommentSkinDock::refresh_sources()
{
	const QString selected_uuid = source_combo_->currentData().toString();

	std::vector<SourceEntry> entries;
	obs_enum_sources(collect_browser_source, &entries);

	source_combo_->blockSignals(true);
	source_combo_->clear();
	for (const auto &[name, uuid] : entries)
		source_combo_->addItem(name, uuid);
	source_combo_->setCurrentIndex(source_combo_->findData(selected_uuid));
	source_combo_->blockSignals(false);

	update_view();
}

void CommentSkinDock::clear_sources()
{
	// シーンコレクションを片付けている間に、消えていくソースへ適用しないようにする
	apply_timer_->stop();
	pending_uuid_.clear();
	source_combo_->blockSignals(true);
	source_combo_->clear();
	source_combo_->blockSignals(false);
	update_view();
}

obs_source_t *CommentSkinDock::selected_source() const
{
	const QString uuid = source_combo_->currentData().toString();
	if (uuid.isEmpty())
		return nullptr;
	return obs_get_source_by_uuid(uuid.toUtf8().constData());
}

core::ChatTarget CommentSkinDock::current_target() const
{
	return core::target_from_string(target_combo_->currentData().toString().toStdString());
}

std::string CommentSkinDock::generated_css() const
{
	if (current_target() == core::ChatTarget::YouTube)
		return core::generate_youtube_css(youtube_panel_->values());
	return {};
}

void CommentSkinDock::update_view()
{
	obs_source_t *source = selected_source();

	target_combo_->blockSignals(true);
	if (!source) {
		target_combo_->setCurrentIndex(-1);
		target_combo_->setEnabled(false);
		target_hint_label_->clear();
		settings_label_->clear();
		settings_label_->setVisible(false);
		youtube_panel_->setVisible(false);
		preview_->setVisible(false);
		preset_row_->setVisible(false);
		undo_button_->setEnabled(false);
	} else {
		const SourceState state = load_source_state(source);
		const core::ChatTarget detected = core::detect_target(get_source_url(source));
		const core::ChatTarget target = state.chosen_target != core::ChatTarget::Unknown ? state.chosen_target
												 : detected;
		target_combo_->setEnabled(true);
		target_combo_->setCurrentIndex(target_combo_->findData(target_data(target)));

		if (state.chosen_target != core::ChatTarget::Unknown)
			target_hint_label_->setText(text("ChatTarget.Chosen"));
		else if (detected != core::ChatTarget::Unknown)
			target_hint_label_->setText(text("ChatTarget.Detected").arg(target_display_name(detected)));
		else
			target_hint_label_->setText(text("ChatTarget.Undetected"));

		const bool youtube = target == core::ChatTarget::YouTube;
		if (youtube)
			youtube_panel_->set_values(load_youtube_style(source));
		youtube_panel_->setVisible(youtube);
		preview_->setVisible(youtube);
		preset_row_->setVisible(youtube);
		if (youtube)
			refresh_presets(preset_combo_->currentText());
		settings_label_->setVisible(target == core::ChatTarget::Twitch);
		settings_label_->setText(text("Settings.TwitchUnsupported"));

		undo_button_->setEnabled(state.has_previous);
		obs_source_release(source);
	}
	target_combo_->blockSignals(false);

	update_css_view();
}

void CommentSkinDock::update_css_view()
{
	const std::string css = generated_css();
	css_view_->setPlainText(css.empty() ? text("GeneratedCss.Empty") : QString::fromStdString(css));
	copy_css_button_->setEnabled(!css.empty());
	preview_->set_css(css);
}

void CommentSkinDock::refresh_presets(const QString &select_name)
{
	const core::ChatTarget target = current_target();
	const QSignalBlocker blocker(preset_combo_);
	preset_combo_->clear();
	for (const auto &preset : load_presets())
		if (preset.target == target)
			preset_combo_->addItem(QString::fromStdString(preset.name));
	preset_combo_->setCurrentIndex(select_name.isEmpty() ? -1 : preset_combo_->findText(select_name));
	preset_load_button_->setEnabled(preset_combo_->currentIndex() >= 0);
	preset_delete_button_->setEnabled(preset_combo_->currentIndex() >= 0);
}

void CommentSkinDock::on_preset_save()
{
	const core::ChatTarget target = current_target();
	if (target != core::ChatTarget::YouTube)
		return;
	bool ok = false;
	const QString name = QInputDialog::getText(this, text("Preset.SaveTitle"), text("Preset.SaveName"),
						   QLineEdit::Normal, preset_combo_->currentText(), &ok)
				     .trimmed();
	if (!ok || name.isEmpty())
		return;

	std::vector<Preset> presets = load_presets();
	const std::string key = name.toStdString();
	auto it = std::find_if(presets.begin(), presets.end(),
			       [&](const Preset &p) { return p.target == target && p.name == key; });
	if (it != presets.end()) {
		if (QMessageBox::question(this, text("Preset.SaveTitle"), text("Preset.Overwrite").arg(name)) !=
		    QMessageBox::Yes)
			return;
		it->values = youtube_panel_->values();
	} else {
		presets.push_back(Preset{key, target, youtube_panel_->values()});
	}
	if (!save_presets(presets)) {
		set_status(text("Preset.SaveFailed"));
		return;
	}
	refresh_presets(name);
	set_status(text("Preset.Saved").arg(name));
}

void CommentSkinDock::on_preset_load()
{
	const QString name = preset_combo_->currentText();
	if (preset_combo_->currentIndex() < 0)
		return;
	const core::ChatTarget target = current_target();
	for (const auto &preset : load_presets()) {
		if (preset.target != target || preset.name != name.toStdString())
			continue;
		// 欄に写したうえで、設定を変えたときと同じ経路で保存と自動の適用を行う
		youtube_panel_->set_values(preset.values);
		on_settings_changed();
		set_status(text("Preset.Loaded").arg(name));
		return;
	}
	set_status(text("Preset.NotFound"));
	refresh_presets();
}

void CommentSkinDock::on_preset_delete()
{
	const QString name = preset_combo_->currentText();
	if (preset_combo_->currentIndex() < 0)
		return;
	if (QMessageBox::question(this, text("Preset"), text("Preset.ConfirmDelete").arg(name)) != QMessageBox::Yes)
		return;
	const core::ChatTarget target = current_target();
	std::vector<Preset> presets = load_presets();
	presets.erase(
		std::remove_if(presets.begin(), presets.end(),
			       [&](const Preset &p) { return p.target == target && p.name == name.toStdString(); }),
		presets.end());
	if (!save_presets(presets)) {
		set_status(text("Preset.SaveFailed"));
		return;
	}
	refresh_presets();
	set_status(text("Preset.Deleted").arg(name));
}

void CommentSkinDock::shutdown()
{
	apply_timer_->stop();
	pending_uuid_.clear();
	preview_->shutdown();
}

void CommentSkinDock::on_settings_changed()
{
	obs_source_t *source = selected_source();
	if (!source)
		return;
	const bool youtube = current_target() == core::ChatTarget::YouTube;
	if (youtube)
		save_youtube_style(source, youtube_panel_->values());
	obs_source_release(source);
	update_css_view();

	if (youtube) {
		pending_uuid_ = source_combo_->currentData().toString();
		apply_timer_->start();
	}
}

void CommentSkinDock::set_status(const QString &message)
{
	status_label_->setText(message);
}

void CommentSkinDock::on_source_changed()
{
	set_status({});
	update_view();
}

void CommentSkinDock::on_target_changed()
{
	obs_source_t *source = selected_source();
	if (!source)
		return;

	// URL から判別できる種類と同じなら記録しない。URL を変えたときに判別をやり直せるようにするため
	const core::ChatTarget chosen = current_target();
	const core::ChatTarget detected = core::detect_target(get_source_url(source));
	save_chosen_target(source, chosen == detected ? core::ChatTarget::Unknown : chosen);
	obs_source_release(source);
	update_view();
}

void CommentSkinDock::on_create_source()
{
	CreateSourceDialog dialog(this);
	if (dialog.exec() != QDialog::Accepted)
		return;

	refresh_sources();
	const int index = source_combo_->findData(dialog.created_uuid());
	if (index >= 0)
		source_combo_->setCurrentIndex(index);
	set_status(text("CreateSource.Created"));
}

void CommentSkinDock::apply_pending()
{
	const QString uuid = std::exchange(pending_uuid_, QString());
	if (uuid.isEmpty())
		return;
	obs_source_t *source = obs_get_source_by_uuid(uuid.toUtf8().constData());
	if (!source)
		return;
	const QString name = source_name(source);

	// 選択が変わっていても、変更したソースの保存済みの設定から CSS を作る
	const SourceState state = load_source_state(source);
	const core::ChatTarget target = state.chosen_target != core::ChatTarget::Unknown
						? state.chosen_target
						: core::detect_target(get_source_url(source));
	if (target != core::ChatTarget::YouTube) {
		obs_source_release(source);
		return;
	}

	const std::string current = get_source_css(source);
	const auto updated = core::replace_block(current, core::generate_youtube_css(load_youtube_style(source)));
	if (!updated) {
		obs_source_release(source);
		set_status(text("Apply.BrokenMarkers"));
		return;
	}
	if (*updated == current) {
		obs_source_release(source);
		return;
	}

	// 元に戻す先は、プラグインが最初に書き換える前の css にする
	save_apply_record(source, state.has_previous ? state.previous_css : current, *updated);
	set_source_css(source, *updated);
	obs_log(LOG_INFO, "applied CSS to browser source '%s'", name.toUtf8().constData());
	obs_source_release(source);

	set_status(text("Apply.Done").arg(name));
	if (uuid == source_combo_->currentData().toString())
		undo_button_->setEnabled(true);
}

void CommentSkinDock::on_undo()
{
	// 待っている適用が後から走ると、戻した css をまた書き換えてしまう。
	// 同じソースへの適用は戻すため捨て、別のソースへの適用はその場で済ませる
	if (apply_timer_->isActive()) {
		apply_timer_->stop();
		if (pending_uuid_ == source_combo_->currentData().toString())
			pending_uuid_.clear();
		else
			apply_pending();
	}

	obs_source_t *source = selected_source();
	if (!source) {
		set_status(text("SourceNotFound"));
		refresh_sources();
		return;
	}
	const QString name = source_name(source);
	const SourceState state = load_source_state(source);
	const std::string current = get_source_css(source);

	switch (core::decide_undo(state.has_previous, current, state.applied_css)) {
	case core::UndoDecision::Nothing:
		obs_source_release(source);
		set_status(text("Undo.Nothing"));
		update_view();
		return;
	case core::UndoDecision::RestoreWithConfirm:
		if (QMessageBox::question(this, text("DockTitle"), text("Undo.ConfirmEdited")) != QMessageBox::Yes) {
			obs_source_release(source);
			return;
		}
		break;
	case core::UndoDecision::Restore:
		break;
	}

	clear_apply_record(source);
	save_youtube_style(source, core::StyleValues{});
	set_source_css(source, state.previous_css);
	obs_log(LOG_INFO, "restored previous CSS of browser source '%s'", name.toUtf8().constData());
	obs_source_release(source);

	set_status(text("Undo.Done").arg(name));
	update_view();
}

void CommentSkinDock::on_copy_css()
{
	const std::string css = generated_css();
	if (css.empty())
		return;
	QApplication::clipboard()->setText(QString::fromStdString(css));
	set_status(text("CopyCss.Done"));
}

} // namespace cskin
