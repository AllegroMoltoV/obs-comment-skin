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

// チャットの種類ごとに、項目の表と CSS の生成を選ぶ

#include "chat-target.h"
#include "style-schema.h"
#include "twitch-style.h"
#include "youtube-style.h"

#include <string>
#include <vector>

namespace cskin::core {

// 種類の項目の表。対応していない種類では nullptr を返す
inline const std::vector<SectionDef> *sections_for(ChatTarget target)
{
	switch (target) {
	case ChatTarget::YouTube:
		return &youtube_sections();
	case ChatTarget::Twitch:
		return &twitch_sections();
	case ChatTarget::Unknown:
		break;
	}
	return nullptr;
}

inline std::string generate_css(ChatTarget target, const StyleValues &values)
{
	switch (target) {
	case ChatTarget::YouTube:
		return generate_youtube_css(values);
	case ChatTarget::Twitch:
		return generate_twitch_css(values);
	case ChatTarget::Unknown:
		break;
	}
	return {};
}

} // namespace cskin::core
