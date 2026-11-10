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

#include <string>
#include <string_view>

namespace cskin::core {

// JavaScript の二重引用符の文字列リテラルにする。引用符、バックスラッシュ、制御文字、
// U+2028 と U+2029、< と > をエスケープし、CSS の中身が文字列の外へ出ないようにする
std::string js_string_literal(std::string_view text);

// 見本のページの style 要素 #cskin-generated の中身を css にし、見本のメッセージを入れ直すスクリプト
std::string preview_style_script(std::string_view css);

} // namespace cskin::core
