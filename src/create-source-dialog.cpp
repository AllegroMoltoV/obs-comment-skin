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

#include "create-source-dialog.h"

#include "core/chat-target.h"

#include <obs-frontend-api.h>
#include <obs-module.h>
#include <plugin-support.h>

#include <QFormLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

namespace cskin {

namespace {

constexpr int SOURCE_WIDTH = 400;
constexpr int SOURCE_HEIGHT = 600;

QString text(const char *key)
{
	return QString::fromUtf8(obs_module_text(key));
}

bool source_name_exists(const QString &name)
{
	obs_source_t *existing = obs_get_source_by_name(name.toUtf8().constData());
	if (!existing)
		return false;
	obs_source_release(existing);
	return true;
}

// OBS のソースの名前は重複させられないため、番号を付けて空いている名前を探す
QString unique_source_name(const QString &base)
{
	if (!source_name_exists(base))
		return base;
	for (int n = 2;; ++n) {
		const QString candidate = QStringLiteral("%1 %2").arg(base).arg(n);
		if (!source_name_exists(candidate))
			return candidate;
	}
}

} // namespace

CreateSourceDialog::CreateSourceDialog(QWidget *parent) : QDialog(parent)
{
	setWindowTitle(text("CreateSource.Title"));

	kind_combo_ = new QComboBox(this);
	kind_combo_->addItem(QStringLiteral("YouTube"),
			     QString::fromUtf8(core::target_to_string(core::ChatTarget::YouTube).data()));
	kind_combo_->addItem(QStringLiteral("Twitch"),
			     QString::fromUtf8(core::target_to_string(core::ChatTarget::Twitch).data()));

	input_label_ = new QLabel(this);
	input_edit_ = new QLineEdit(this);
	input_label_->setBuddy(input_edit_);

	url_label_ = new QLabel(this);
	url_label_->setWordWrap(true);
	url_label_->setTextInteractionFlags(Qt::TextSelectableByMouse);

	buttons_ = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	buttons_->button(QDialogButtonBox::Ok)->setText(text("CreateSource.Create"));

	auto *form = new QFormLayout();
	form->addRow(text("CreateSource.Kind"), kind_combo_);
	form->addRow(input_label_, input_edit_);

	auto *layout = new QVBoxLayout(this);
	layout->addLayout(form);
	layout->addWidget(url_label_);
	layout->addWidget(buttons_);

	connect(kind_combo_, &QComboBox::currentIndexChanged, this, &CreateSourceDialog::update_state);
	connect(input_edit_, &QLineEdit::textChanged, this, &CreateSourceDialog::update_state);
	connect(buttons_, &QDialogButtonBox::accepted, this, &CreateSourceDialog::create_source);
	connect(buttons_, &QDialogButtonBox::rejected, this, &QDialog::reject);

	setMinimumWidth(420);
	update_state();
}

QString CreateSourceDialog::popout_url() const
{
	const std::string input = input_edit_->text().toStdString();
	const auto target = core::target_from_string(kind_combo_->currentData().toString().toStdString());
	const auto url = target == core::ChatTarget::YouTube ? core::youtube_popout_url(input)
							     : core::twitch_popout_url(input);
	return url ? QString::fromStdString(*url) : QString();
}

void CreateSourceDialog::update_state()
{
	const bool youtube = core::target_from_string(kind_combo_->currentData().toString().toStdString()) ==
			     core::ChatTarget::YouTube;
	input_label_->setText(youtube ? text("CreateSource.YouTubeInput") : text("CreateSource.TwitchInput"));
	input_edit_->setPlaceholderText(youtube ? text("CreateSource.YouTubePlaceholder")
						: text("CreateSource.TwitchPlaceholder"));

	const QString url = popout_url();
	if (input_edit_->text().trimmed().isEmpty())
		url_label_->clear();
	else if (url.isEmpty())
		url_label_->setText(youtube ? text("CreateSource.YouTubeInvalid") : text("CreateSource.TwitchInvalid"));
	else
		url_label_->setText(text("CreateSource.Url").arg(url));
	buttons_->button(QDialogButtonBox::Ok)->setEnabled(!url.isEmpty());
}

void CreateSourceDialog::create_source()
{
	const QString url = popout_url();
	if (url.isEmpty())
		return;

	obs_source_t *scene_source = obs_frontend_get_current_scene();
	obs_scene_t *scene = scene_source ? obs_scene_from_source(scene_source) : nullptr;
	if (!scene) {
		obs_source_release(scene_source);
		QMessageBox::warning(this, windowTitle(), text("CreateSource.NoScene"));
		return;
	}

	const bool youtube = core::target_from_string(kind_combo_->currentData().toString().toStdString()) ==
			     core::ChatTarget::YouTube;
	const QString name =
		unique_source_name(youtube ? text("CreateSource.YouTubeName") : text("CreateSource.TwitchName"));

	obs_data_t *settings = obs_data_create();
	obs_data_set_string(settings, "url", url.toUtf8().constData());
	obs_data_set_int(settings, "width", SOURCE_WIDTH);
	obs_data_set_int(settings, "height", SOURCE_HEIGHT);
	obs_source_t *source = obs_source_create("browser_source", name.toUtf8().constData(), settings, nullptr);
	obs_data_release(settings);

	if (source) {
		obs_scene_add(scene, source);
		created_uuid_ = QString::fromUtf8(obs_source_get_uuid(source));
		obs_log(LOG_INFO, "created browser source '%s'", name.toUtf8().constData());
		obs_source_release(source);
	}
	obs_source_release(scene_source);

	if (created_uuid_.isEmpty()) {
		QMessageBox::warning(this, windowTitle(), text("CreateSource.Failed"));
		return;
	}
	accept();
}

} // namespace cskin
