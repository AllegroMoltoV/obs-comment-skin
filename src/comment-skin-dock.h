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

#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QWidget>

namespace cskin {

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

private slots:
	void on_write_test_css();

private:
	SourceComboBox *source_combo_{nullptr};
	QPushButton *write_test_css_button_{nullptr};
	QLabel *status_label_{nullptr};
};

} // namespace cskin
