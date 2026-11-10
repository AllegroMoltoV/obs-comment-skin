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

#include "style-schema.h"

#include <string>
#include <string_view>

namespace cskin::core {

// CSS の二重引用符の文字列にする。書体の名前を font-family に入れるときに使う
std::string css_string(std::string_view text);

// YouTube のチャットの CSS を生成する。値は youtube_sections() の表にしたがう
std::string generate_youtube_css(const StyleValues &values);

} // namespace cskin::core
