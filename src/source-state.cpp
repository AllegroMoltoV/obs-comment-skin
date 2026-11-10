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

#include "source-state.h"

namespace cskin {

namespace {

constexpr const char *STATE_KEY = "comment_skin";
constexpr const char *TARGET_KEY = "target";
constexpr const char *HAS_PREVIOUS_KEY = "has_previous";
constexpr const char *PREVIOUS_CSS_KEY = "previous_css";
constexpr const char *APPLIED_CSS_KEY = "applied_css";

constexpr const char *YOUTUBE_KEY = "youtube";
constexpr const char *MESSAGE_FONT_SIZE_KEY = "message_font_size";
constexpr const char *MESSAGE_COLOR_KEY = "message_color";
constexpr const char *BACKGROUND_COLOR_KEY = "background_color";
constexpr const char *BACKGROUND_OPACITY_KEY = "background_opacity";

std::optional<core::Rgb> get_color(obs_data_t *data, const char *key)
{
	return core::parse_hex_color(obs_data_get_string(data, key));
}

void set_color(obs_data_t *data, const char *key, const std::optional<core::Rgb> &color)
{
	if (color)
		obs_data_set_string(data, key, core::to_hex_color(*color).c_str());
	else
		obs_data_erase(data, key);
}

// private settings の中の状態のオブジェクトを読み書きする。無ければ作る
template<typename Fn> void edit_state(obs_source_t *source, Fn &&fn)
{
	obs_data_t *priv = obs_source_get_private_settings(source);
	obs_data_t *state = obs_data_get_obj(priv, STATE_KEY);
	if (!state)
		state = obs_data_create();
	fn(state);
	obs_data_set_obj(priv, STATE_KEY, state);
	obs_data_release(state);
	obs_data_release(priv);
}

std::string get_string_setting(obs_source_t *source, const char *key)
{
	obs_data_t *settings = obs_source_get_settings(source);
	std::string value = obs_data_get_string(settings, key);
	obs_data_release(settings);
	return value;
}

} // namespace

SourceState load_source_state(obs_source_t *source)
{
	SourceState result;
	obs_data_t *priv = obs_source_get_private_settings(source);
	obs_data_t *state = obs_data_get_obj(priv, STATE_KEY);
	if (state) {
		result.chosen_target = core::target_from_string(obs_data_get_string(state, TARGET_KEY));
		result.has_previous = obs_data_get_bool(state, HAS_PREVIOUS_KEY);
		result.previous_css = obs_data_get_string(state, PREVIOUS_CSS_KEY);
		result.applied_css = obs_data_get_string(state, APPLIED_CSS_KEY);
		obs_data_release(state);
	}
	obs_data_release(priv);
	return result;
}

void save_chosen_target(obs_source_t *source, core::ChatTarget target)
{
	const std::string value(core::target_to_string(target));
	edit_state(source, [&](obs_data_t *state) { obs_data_set_string(state, TARGET_KEY, value.c_str()); });
}

void save_apply_record(obs_source_t *source, const std::string &previous_css, const std::string &applied_css)
{
	edit_state(source, [&](obs_data_t *state) {
		obs_data_set_bool(state, HAS_PREVIOUS_KEY, true);
		obs_data_set_string(state, PREVIOUS_CSS_KEY, previous_css.c_str());
		obs_data_set_string(state, APPLIED_CSS_KEY, applied_css.c_str());
	});
}

void clear_apply_record(obs_source_t *source)
{
	edit_state(source, [](obs_data_t *state) {
		obs_data_set_bool(state, HAS_PREVIOUS_KEY, false);
		obs_data_erase(state, PREVIOUS_CSS_KEY);
		obs_data_erase(state, APPLIED_CSS_KEY);
	});
}

core::YouTubeStyle load_youtube_style(obs_source_t *source)
{
	core::YouTubeStyle style;
	obs_data_t *priv = obs_source_get_private_settings(source);
	obs_data_t *state = obs_data_get_obj(priv, STATE_KEY);
	obs_data_t *youtube = state ? obs_data_get_obj(state, YOUTUBE_KEY) : nullptr;
	if (youtube) {
		if (obs_data_has_user_value(youtube, MESSAGE_FONT_SIZE_KEY))
			style.message_font_size = static_cast<int>(obs_data_get_int(youtube, MESSAGE_FONT_SIZE_KEY));
		style.message_color = get_color(youtube, MESSAGE_COLOR_KEY);
		style.background_color = get_color(youtube, BACKGROUND_COLOR_KEY);
		if (obs_data_has_user_value(youtube, BACKGROUND_OPACITY_KEY))
			style.background_opacity = static_cast<int>(obs_data_get_int(youtube, BACKGROUND_OPACITY_KEY));
		obs_data_release(youtube);
	}
	if (state)
		obs_data_release(state);
	obs_data_release(priv);
	return style;
}

void save_youtube_style(obs_source_t *source, const core::YouTubeStyle &style)
{
	edit_state(source, [&](obs_data_t *state) {
		obs_data_t *youtube = obs_data_create();
		if (style.message_font_size)
			obs_data_set_int(youtube, MESSAGE_FONT_SIZE_KEY, *style.message_font_size);
		set_color(youtube, MESSAGE_COLOR_KEY, style.message_color);
		set_color(youtube, BACKGROUND_COLOR_KEY, style.background_color);
		obs_data_set_int(youtube, BACKGROUND_OPACITY_KEY, style.background_opacity);
		obs_data_set_obj(state, YOUTUBE_KEY, youtube);
		obs_data_release(youtube);
	});
}

std::string get_source_url(obs_source_t *source)
{
	return get_string_setting(source, "url");
}

std::string get_source_css(obs_source_t *source)
{
	return get_string_setting(source, "css");
}

void set_source_css(obs_source_t *source, const std::string &css)
{
	obs_data_t *update = obs_data_create();
	obs_data_set_string(update, "css", css.c_str());
	obs_source_update(source, update);
	obs_data_release(update);
}

} // namespace cskin
