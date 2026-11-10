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

#include "youtube-style.h"

#include <algorithm>

namespace cskin::core {

namespace {

int hex_digit(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return -1;
}

// ロケールに依存しないよう、小数を自前で書く。0 から 100 の百分率を 0 から 1 にする
std::string alpha_text(int percent)
{
	percent = std::clamp(percent, 0, 100);
	if (percent == 100)
		return "1";
	if (percent == 0)
		return "0";
	std::string digits = std::to_string(percent);
	if (digits.size() == 1)
		digits = "0" + digits;
	if (digits.back() == '0')
		digits.pop_back();
	return "0." + digits;
}

std::string rgb_text(const Rgb &c)
{
	return "rgb(" + std::to_string(c.r) + ", " + std::to_string(c.g) + ", " + std::to_string(c.b) + ")";
}

std::string rgba_text(const Rgb &c, int percent)
{
	return "rgba(" + std::to_string(c.r) + ", " + std::to_string(c.g) + ", " + std::to_string(c.b) + ", " +
	       alpha_text(percent) + ")";
}

void add_rule(std::string &css, std::string_view selector, std::string_view declarations)
{
	css += selector;
	css += " {\n";
	css += declarations;
	css += "}\n";
}

} // namespace

std::optional<Rgb> parse_hex_color(std::string_view text)
{
	if (text.size() != 7 || text[0] != '#')
		return std::nullopt;
	int values[6];
	for (size_t i = 0; i < 6; ++i) {
		values[i] = hex_digit(text[i + 1]);
		if (values[i] < 0)
			return std::nullopt;
	}
	return Rgb{static_cast<std::uint8_t>(values[0] * 16 + values[1]),
		   static_cast<std::uint8_t>(values[2] * 16 + values[3]),
		   static_cast<std::uint8_t>(values[4] * 16 + values[5])};
}

std::string to_hex_color(const Rgb &color)
{
	static constexpr char DIGITS[] = "0123456789abcdef";
	std::string text = "#";
	for (const std::uint8_t v : {color.r, color.g, color.b}) {
		text += DIGITS[v >> 4];
		text += DIGITS[v & 0x0f];
	}
	return text;
}

std::string generate_youtube_css(const YouTubeStyle &style)
{
	std::string css;

	// 全体の背景。YouTube は yt-live-chat-renderer とヘッダーを塗り、その内側は透明にしている
	if (style.background_color) {
		add_rule(css, "yt-live-chat-renderer,\nyt-live-chat-header-renderer",
			 "  background-color: " + rgba_text(*style.background_color, style.background_opacity) +
				 " !important;\n");
	}

	// メッセージの本文。YouTube は行の高さを 20px に固定しているため、文字を大きくしたときに重ならないよう比率にする。
	// 1.43 は YouTube の既定の 20px / 14px に合わせた値

	std::string message;
	if (style.message_font_size) {
		const int size = std::clamp(*style.message_font_size, FONT_SIZE_MIN, FONT_SIZE_MAX);
		message += "  font-size: " + std::to_string(size) + "px !important;\n";
		message += "  line-height: 1.43 !important;\n";
	}
	if (style.message_color)
		message += "  color: " + rgb_text(*style.message_color) + " !important;\n";
	if (!message.empty())
		add_rule(css, "yt-live-chat-text-message-renderer #message", message);

	return css;
}

} // namespace cskin::core
