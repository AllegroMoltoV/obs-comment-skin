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

#include <obs-module.h>
#include <plugin-support.h>

#include <QDateTime>
#include <QString>
#include <QVBoxLayout>

#include <cstring>
#include <utility>
#include <vector>

namespace cskin {

namespace {

constexpr const char *BROWSER_SOURCE_ID = "browser_source";

// テスト用 CSS の範囲を示す目印。書き込むたびにこの範囲だけを置き換える
const QString TEST_CSS_BEGIN = QStringLiteral("/* comment-skin test begin */");
const QString TEST_CSS_END = QStringLiteral("/* comment-skin test end */");

QString text(const char *key)
{
	return QString::fromUtf8(obs_module_text(key));
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

QString replace_test_block(const QString &css, const QString &block)
{
	QString result = css;
	const qsizetype begin = result.indexOf(TEST_CSS_BEGIN);
	if (begin >= 0) {
		const qsizetype end = result.indexOf(TEST_CSS_END, begin);
		if (end >= 0)
			result.remove(begin, end + TEST_CSS_END.size() - begin);
	}
	while (result.endsWith(QLatin1Char('\n')))
		result.chop(1);
	if (!result.isEmpty())
		result += QLatin1Char('\n');
	return result + block;
}

} // namespace

void SourceComboBox::showPopup()
{
	emit popup_requested();
	QComboBox::showPopup();
}

CommentSkinDock::CommentSkinDock(QWidget *parent) : QWidget(parent)
{
	auto *layout = new QVBoxLayout(this);

	auto *source_label = new QLabel(text("BrowserSource"), this);
	source_combo_ = new SourceComboBox(this);
	source_label->setBuddy(source_combo_);

	write_test_css_button_ = new QPushButton(text("WriteTestCss"), this);

	status_label_ = new QLabel(this);
	status_label_->setWordWrap(true);

	layout->addWidget(source_label);
	layout->addWidget(source_combo_);
	layout->addWidget(write_test_css_button_);
	layout->addWidget(status_label_);
	layout->addStretch();

	connect(source_combo_, &SourceComboBox::popup_requested, this, &CommentSkinDock::refresh_sources);
	connect(write_test_css_button_, &QPushButton::clicked, this, &CommentSkinDock::on_write_test_css);

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
	const int index = source_combo_->findData(selected_uuid);
	if (index >= 0)
		source_combo_->setCurrentIndex(index);
	source_combo_->setPlaceholderText(text("NoBrowserSource"));
	source_combo_->blockSignals(false);

	write_test_css_button_->setEnabled(!entries.empty());
}

void CommentSkinDock::clear_sources()
{
	source_combo_->clear();
	write_test_css_button_->setEnabled(false);
}

void CommentSkinDock::on_write_test_css()
{
	const QString uuid = source_combo_->currentData().toString();
	if (uuid.isEmpty()) {
		status_label_->setText(text("SelectBrowserSource"));
		return;
	}

	obs_source_t *source = obs_get_source_by_uuid(uuid.toUtf8().constData());
	if (!source) {
		status_label_->setText(text("SourceNotFound"));
		refresh_sources();
		return;
	}

	obs_data_t *settings = obs_source_get_settings(source);
	const QString current_css = QString::fromUtf8(obs_data_get_string(settings, "css"));
	obs_data_release(settings);

	const QString stamp = QDateTime::currentDateTime().toString(Qt::ISODate);
	const QString block = TEST_CSS_BEGIN + QStringLiteral("\n/* written at ") + stamp +
			      QStringLiteral(" */\nbody { outline: 4px solid #e53935; outline-offset: -4px; }\n"
					     "html:not([dark]) body { outline-color: #1e88e5; }\n"
					     "yt-live-chat-text-message-renderer:has(#author-name.member) "
					     "{ background-color: rgba(0, 200, 0, 0.35) !important; }\n") +
			      TEST_CSS_END;
	const QByteArray new_css = replace_test_block(current_css, block).toUtf8();

	obs_data_t *update = obs_data_create();
	obs_data_set_string(update, "css", new_css.constData());
	obs_source_update(source, update);
	obs_data_release(update);

	const QString name = QString::fromUtf8(obs_source_get_name(source));
	obs_log(LOG_INFO, "wrote test CSS to browser source '%s' (%s)", name.toUtf8().constData(),
		stamp.toUtf8().constData());
	obs_source_release(source);

	status_label_->setText(text("WroteTestCss").arg(name, stamp));
}

} // namespace cskin
