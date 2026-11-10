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

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace cskin::core {

struct Rgb {
	std::uint8_t r = 0;
	std::uint8_t g = 0;
	std::uint8_t b = 0;

	bool operator==(const Rgb &other) const { return r == other.r && g == other.g && b == other.b; }
};

// "#rrggbb" だけを受け付ける。生成する CSS に任意の文字列が入らないようにするため
std::optional<Rgb> parse_hex_color(std::string_view text);
std::string to_hex_color(const Rgb &color);

inline constexpr int FONT_SIZE_MIN = 8;
inline constexpr int FONT_SIZE_MAX = 96;

// YouTube のチャットの設定。値が無い項目は YouTube の既定の見た目のままにする
struct YouTubeStyle {
	std::optional<int> message_font_size; // px
	std::optional<Rgb> message_color;
	std::optional<Rgb> background_color;
	int background_opacity = 100; // 0 から 100 の百分率
};

std::string generate_youtube_css(const YouTubeStyle &style);

} // namespace cskin::core
