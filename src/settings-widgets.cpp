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

#include "settings-widgets.h"

#include <obs-module.h>

#include <QColorDialog>
#include <QHBoxLayout>
#include <QIcon>
#include <QPixmap>
#include <QVBoxLayout>

namespace cskin {

namespace {

QString locale_text(const char *key)
{
	return QString::fromUtf8(obs_module_text(key));
}

QColor to_qcolor(const core::Rgb &c)
{
	return QColor(c.r, c.g, c.b);
}

} // namespace

CollapsibleSection::CollapsibleSection(const QString &title, QWidget *parent) : QWidget(parent)
{
	header_ = new QToolButton(this);
	header_->setText(title);
	header_->setCheckable(true);
	header_->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	header_->setAutoRaise(true);

	body_ = new QWidget(this);
	form_ = new QFormLayout(body_);
	form_->setContentsMargins(16, 0, 0, 0);

	auto *layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(header_);
	layout->addWidget(body_);

	connect(header_, &QToolButton::toggled, this, &CollapsibleSection::set_expanded);
	set_expanded(true);
}

void CollapsibleSection::set_expanded(bool expanded)
{
	header_->blockSignals(true);
	header_->setChecked(expanded);
	header_->blockSignals(false);
	header_->setArrowType(expanded ? Qt::DownArrow : Qt::RightArrow);
	body_->setVisible(expanded);
}

ColorField::ColorField(QWidget *parent) : QWidget(parent)
{
	button_ = new QPushButton(this);
	clear_button_ = new QToolButton(this);
	clear_button_->setText(locale_text("Field.Reset"));
	clear_button_->setToolTip(locale_text("Field.ResetTooltip"));

	auto *layout = new QHBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(button_, 1);
	layout->addWidget(clear_button_);

	connect(button_, &QPushButton::clicked, this, &ColorField::choose);
	connect(clear_button_, &QToolButton::clicked, this, [this]() {
		set_color(std::nullopt);
		emit changed();
	});
	update_view();
}

void ColorField::set_color(const std::optional<core::Rgb> &color)
{
	color_ = color;
	update_view();
}

void ColorField::choose()
{
	const QColor initial = color_ ? to_qcolor(*color_) : QColor(Qt::white);
	const QColor chosen = QColorDialog::getColor(initial, this);
	if (!chosen.isValid())
		return;
	set_color(core::Rgb{static_cast<std::uint8_t>(chosen.red()), static_cast<std::uint8_t>(chosen.green()),
			    static_cast<std::uint8_t>(chosen.blue())});
	emit changed();
}

void ColorField::update_view()
{
	if (color_) {
		QPixmap swatch(16, 16);
		swatch.fill(to_qcolor(*color_));
		button_->setIcon(QIcon(swatch));
		button_->setText(QString::fromStdString(core::to_hex_color(*color_)));
	} else {
		button_->setIcon(QIcon());
		button_->setText(locale_text("Field.Default"));
	}
	clear_button_->setEnabled(color_.has_value());
}

FontSizeField::FontSizeField(QWidget *parent) : QSpinBox(parent)
{
	setRange(core::FONT_SIZE_MIN - 1, core::FONT_SIZE_MAX);
	setSpecialValueText(locale_text("Field.Default"));
	setSuffix(QStringLiteral(" px"));
	setValue(minimum());
}

std::optional<int> FontSizeField::size() const
{
	if (value() == minimum())
		return std::nullopt;
	return value();
}

void FontSizeField::set_size(const std::optional<int> &size)
{
	setValue(size ? *size : minimum());
}

YouTubeSettingsPanel::YouTubeSettingsPanel(QWidget *parent) : QWidget(parent)
{
	auto *message = new CollapsibleSection(locale_text("Section.Message"), this);
	message_font_size_ = new FontSizeField(message);
	message_color_ = new ColorField(message);
	message->form()->addRow(locale_text("Item.FontSize"), message_font_size_);
	message->form()->addRow(locale_text("Item.TextColor"), message_color_);

	auto *background = new CollapsibleSection(locale_text("Section.Background"), this);
	background_color_ = new ColorField(background);
	background_opacity_ = new QSpinBox(background);
	background_opacity_->setRange(0, 100);
	background_opacity_->setSuffix(QStringLiteral(" %"));
	background_opacity_->setValue(100);
	background->form()->addRow(locale_text("Item.BackgroundColor"), background_color_);
	background->form()->addRow(locale_text("Item.Opacity"), background_opacity_);

	auto *layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(message);
	layout->addWidget(background);

	const auto notify = [this]() {
		update_enabled();
		emit changed();
	};
	connect(message_font_size_, &QSpinBox::valueChanged, this, notify);
	connect(message_color_, &ColorField::changed, this, notify);
	connect(background_color_, &ColorField::changed, this, notify);
	connect(background_opacity_, &QSpinBox::valueChanged, this, notify);
	update_enabled();
}

core::YouTubeStyle YouTubeSettingsPanel::style() const
{
	core::YouTubeStyle style;
	style.message_font_size = message_font_size_->size();
	style.message_color = message_color_->color();
	style.background_color = background_color_->color();
	style.background_opacity = background_opacity_->value();
	return style;
}

void YouTubeSettingsPanel::set_style(const core::YouTubeStyle &style)
{
	const QSignalBlocker block_size(message_font_size_);
	const QSignalBlocker block_opacity(background_opacity_);
	message_font_size_->set_size(style.message_font_size);
	message_color_->set_color(style.message_color);
	background_color_->set_color(style.background_color);
	background_opacity_->setValue(style.background_opacity);
	update_enabled();
}

void YouTubeSettingsPanel::update_enabled()
{
	// 不透明度は背景色を指定したときだけ使う
	background_opacity_->setEnabled(background_color_->color().has_value());
}

} // namespace cskin
