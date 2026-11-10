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

#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include <string>

class QCefWidget;
struct QCef;

namespace cskin {

// プラグインに同梱した見本のページを obs-browser のパネルで表示し、生成した CSS を即座に当てる。
// 見本のページは外部の資源を読み込まないため、プラグインは通信しない
class PreviewPanel : public QWidget {
	Q_OBJECT

public:
	explicit PreviewPanel(QWidget *parent = nullptr);
	~PreviewPanel() override;

	void set_css(const std::string &css);

	// CEF が止まる前に、ブラウザーを閉じる。OBS の終了のイベントで呼ぶ
	void shutdown();

private:
	void wait_for_browser();
	void create_browser();
	void push_css();

	QVBoxLayout *layout_{nullptr};
	QLabel *message_label_{nullptr};
	QTimer *init_timer_{nullptr};
	QCef *cef_{nullptr};
	QCefWidget *browser_{nullptr};
	int init_attempts_{0};
	std::string css_;
	bool shut_down_{false};
};

} // namespace cskin
