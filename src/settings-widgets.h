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

#include "core/youtube-style.h"

#include <QFormLayout>
#include <QPushButton>
#include <QSpinBox>
#include <QToolButton>
#include <QWidget>

#include <optional>

namespace cskin {

// 見出しを押すと中身を開閉する区画
class CollapsibleSection : public QWidget {
	Q_OBJECT

public:
	CollapsibleSection(const QString &title, QWidget *parent = nullptr);

	QFormLayout *form() const { return form_; }
	void set_expanded(bool expanded);

private:
	QToolButton *header_{nullptr};
	QWidget *body_{nullptr};
	QFormLayout *form_{nullptr};
};

// 色を選ぶボタン。未指定のときは既定と表示し、YouTube の既定の色を使う
class ColorField : public QWidget {
	Q_OBJECT

public:
	explicit ColorField(QWidget *parent = nullptr);

	std::optional<core::Rgb> color() const { return color_; }
	void set_color(const std::optional<core::Rgb> &color);

signals:
	void changed();

private:
	void choose();
	void update_view();

	QPushButton *button_{nullptr};
	QToolButton *clear_button_{nullptr};
	std::optional<core::Rgb> color_;
};

// 文字の大きさ。最小値の 1 つ下を既定として表示する
class FontSizeField : public QSpinBox {
	Q_OBJECT

public:
	explicit FontSizeField(QWidget *parent = nullptr);

	std::optional<int> size() const;
	void set_size(const std::optional<int> &size);
};

// YouTube の設定の項目
class YouTubeSettingsPanel : public QWidget {
	Q_OBJECT

public:
	explicit YouTubeSettingsPanel(QWidget *parent = nullptr);

	core::YouTubeStyle style() const;
	void set_style(const core::YouTubeStyle &style);

signals:
	void changed();

private:
	void update_enabled();

	FontSizeField *message_font_size_{nullptr};
	ColorField *message_color_{nullptr};
	ColorField *background_color_{nullptr};
	QSpinBox *background_opacity_{nullptr};
};

} // namespace cskin
