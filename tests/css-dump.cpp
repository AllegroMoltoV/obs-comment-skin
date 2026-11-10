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

// 見た目の崩れの検査で当てる CSS を書き出す。
// 使い方: comment-skin-css-dump <max|min>
// 数値の項目をすべて最大または最小にし、文字の縁取り、時刻、名前の後の「:」を有効にする。
// アニメーションは、計測の途中で要素が動かないよう無効のままにする

#include "core/style-schema.h"
#include "core/youtube-style.h"

#include <iostream>
#include <string>

using namespace cskin::core;

int main(int argc, char **argv)
{
	const std::string profile = argc > 1 ? argv[1] : "";
	if (profile != "max" && profile != "min") {
		std::cerr << "usage: comment-skin-css-dump <max|min>\n";
		return 2;
	}
	const bool use_max = profile == "max";

	StyleValues values;
	for (const auto &section : youtube_sections()) {
		for (const auto &item : section.items) {
			const std::string key(item.key);
			if (item.kind == ItemKind::Number) {
				// アニメーションの時間は計測に関わらないため変えない
				if (item.unit == "ms" || item.unit == "s")
					continue;
				values.numbers[key] = use_max ? item.max : item.min;
			}
		}
	}
	values.numbers["outline_enabled"] = 1;
	values.numbers["name_colon"] = 1;
	values.texts["timestamp_display"] = "show";
	values.texts["avatar_shape"] = "square";
	values.colors["message_background_color"] = Rgb{40, 40, 40};
	values.numbers["message_background_opacity"] = 80;

	std::cout << generate_youtube_css(values);
	return 0;
}
