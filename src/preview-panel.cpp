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

#include "preview-panel.h"

#include "core/preview-script.h"

#include <browser-panel.hpp>
#include <obs-module.h>
#include <plugin-support.h>

#include <QFileInfo>
#include <QUrl>

namespace cskin {

namespace {

// obs-browser のパネルの API の版。この版の仮想関数の並びに合わせてヘッダーを写している
constexpr int REQUIRED_PANEL_VERSION = 3;
constexpr int INIT_POLL_MS = 100;
constexpr int INIT_MAX_ATTEMPTS = 300;
constexpr int PREVIEW_MIN_HEIGHT = 260;

QString text(const char *key)
{
	return QString::fromUtf8(obs_module_text(key));
}

} // namespace

PreviewPanel::PreviewPanel(QWidget *parent) : QWidget(parent)
{
	layout_ = new QVBoxLayout(this);
	layout_->setContentsMargins(0, 0, 0, 0);
	message_label_ = new QLabel(text("Preview.Loading"), this);
	message_label_->setWordWrap(true);
	layout_->addWidget(message_label_);

	const int version = obs_browser_qcef_version();
	if (version < REQUIRED_PANEL_VERSION) {
		obs_log(LOG_WARNING, "preview disabled: browser panel version %d", version);
		message_label_->setText(text("Preview.Unavailable"));
		return;
	}
	cef_ = obs_browser_init_panel();
	if (!cef_) {
		obs_log(LOG_WARNING, "preview disabled: browser panel is not available");
		message_label_->setText(text("Preview.Unavailable"));
		return;
	}

	// CEF の起動を待つ。wait_for_browser_init() は UI のスレッドを止めるため使わない
	cef_->init_browser();
	init_timer_ = new QTimer(this);
	init_timer_->setInterval(INIT_POLL_MS);
	connect(init_timer_, &QTimer::timeout, this, &PreviewPanel::wait_for_browser);
	init_timer_->start();
}

PreviewPanel::~PreviewPanel()
{
	shutdown();
	delete cef_;
}

void PreviewPanel::wait_for_browser()
{
	if (shut_down_)
		return;
	if (cef_->initialized()) {
		init_timer_->stop();
		create_browser();
		return;
	}
	if (++init_attempts_ >= INIT_MAX_ATTEMPTS) {
		init_timer_->stop();
		obs_log(LOG_WARNING, "preview disabled: browser did not start");
		message_label_->setText(text("Preview.Unavailable"));
	}
}

void PreviewPanel::create_browser()
{
	char *path = obs_module_file("preview/youtube.html");
	if (!path) {
		obs_log(LOG_WARNING, "preview disabled: preview page not found");
		message_label_->setText(text("Preview.Unavailable"));
		return;
	}
	// obs_module_file() はポータブル版などで相対パスを返すため、絶対パスにしてから URL にする
	const QString absolute = QFileInfo(QString::fromUtf8(path)).absoluteFilePath();
	const std::string url = QUrl::fromLocalFile(absolute).toString().toStdString();
	bfree(path);

	browser_ = cef_->create_widget(this, url);
	if (!browser_) {
		message_label_->setText(text("Preview.Unavailable"));
		return;
	}
	browser_->setMinimumHeight(PREVIEW_MIN_HEIGHT);
	message_label_->hide();
	layout_->addWidget(browser_);
	push_css();
	obs_log(LOG_INFO, "preview created");
}

void PreviewPanel::set_css(const std::string &css)
{
	if (css == css_)
		return;
	css_ = css;
	push_css();
}

void PreviewPanel::push_css()
{
	if (!browser_ || shut_down_)
		return;
	// 読み込みの途中でも最新の CSS が当たるよう、読み込みの完了時に走るスクリプトも差し替える
	const std::string script = core::preview_style_script(css_);
	browser_->setStartupScript(script);
	browser_->executeJavaScript(script);
}

void PreviewPanel::shutdown()
{
	if (shut_down_)
		return;
	shut_down_ = true;
	if (init_timer_)
		init_timer_->stop();
	if (browser_) {
		browser_->closeBrowser();
		delete browser_;
		browser_ = nullptr;
	}
}

} // namespace cskin
