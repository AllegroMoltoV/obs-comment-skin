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

#include <optional>
#include <string>
#include <string_view>

namespace cskin::core {

enum class ChatTarget {
	Unknown,
	YouTube,
	Twitch,
};

// ブラウザーソースの URL から、表示しているチャットの種類を判別する
ChatTarget detect_target(std::string_view url);

// 保存用の文字列との相互変換。未知の文字列は Unknown にする
std::string_view target_to_string(ChatTarget target);
ChatTarget target_from_string(std::string_view text);

// 動画 ID、または動画の URL から、YouTube のポップアウトのチャットの URL を作る
std::optional<std::string> youtube_popout_url(std::string_view input);

// チャンネル名、またはチャンネルの URL から、Twitch のポップアウトのチャットの URL を作る
std::optional<std::string> twitch_popout_url(std::string_view input);

} // namespace cskin::core
