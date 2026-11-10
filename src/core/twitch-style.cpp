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

#include "twitch-style.h"

#include "css-builder.h"

namespace cskin::core {

namespace {

using namespace css;

// セレクターは docs/reports/2026-11-10-twitch-chat-dom.md で確かめたもの。
// 版ごとに変わりうる生成された名前のクラスは使わず、意味の分かるクラス名と data 属性だけを使う
constexpr std::string_view LINE = ".chat-line__message";
constexpr std::string_view NOTICE = "[data-test-selector=\"user-notice-line\"]";
constexpr std::string_view BODY = "[data-a-target=\"chat-line-message-body\"]";
constexpr std::string_view LOG = ".chat-scrollable-area__message-container";

// 種別のバッジ。画像の ID か、表示の言語の alt の文字列のどちらかで見分ける。
// モデレーターの ID は観測で確かめた。配信者と VIP の ID は観測できておらず、alt と併せて指定する
std::string badge_has(std::initializer_list<std::string_view> ids, std::initializer_list<std::string_view> alts)
{
	std::string list;
	for (const auto id : ids) {
		if (!list.empty())
			list += ", ";
		list += "img.chat-badge[src*=\"" + std::string(id) + "\"]";
	}
	for (const auto alt : alts) {
		if (!list.empty())
			list += ", ";
		list += "img.chat-badge[alt*=\"" + std::string(alt) + "\"]";
	}
	return std::string(LINE) + ":has(" + list + ")";
}

std::string broadcaster_line()
{
	return badge_has({"5527c58c-fb7d-422d-b71b-f309dcb85cc1"}, {"配信者", "Broadcaster"});
}

std::string moderator_line()
{
	return badge_has({"3267646d-33f0-4b17-b3df-f923a41db1d0"}, {"モデレーター", "Moderator"});
}

std::string vip_line()
{
	return badge_has({"b817aba4-fad8-49e2-b88a-7cc744dfa6ec"}, {"VIP"});
}

std::string subscriber_line()
{
	return badge_has({}, {"サブスクライバー", "Subscriber"});
}

std::string names(const std::string &line)
{
	return join_selectors({under(line, ".chat-author__display-name"), under(line, ".chat-author__intl-login")});
}

void add_text(CssWriter &css, const Reader &r, const std::string &selector, std::string_view size_key,
	      std::string_view color_key)
{
	if (const auto size = r.number(size_key)) {
		css.add(selector, "font-size", px(*size));
		// Twitch は行の高さを px で固定しているため、文字を大きくしても行が重ならないよう比率にする。
		// 1.57 は Twitch の既定の 22px / 14px に合わせた値
		css.add(selector, "line-height", "1.57");
	}
	if (!color_key.empty())
		if (const auto c = r.color(color_key))
			css.add(selector, "color", rgb_text(*c));
}

void write_font(CssWriter &css, const Reader &r)
{
	const std::string font = r.text("font");
	if (font.empty())
		return;
	css.add(join_selectors({under(LOG, "*")}), "font-family", css_string(font));
}

void write_layout(CssWriter &css, const Reader &r)
{
	// 色を選ばずに不透明度だけを変えたときは、Twitch の既定の背景色 rgb(24, 24, 27) に不透明度を掛ける
	const int opacity = r.number("background_opacity").value_or(100);
	const auto background = r.color("background_color");
	if (background || opacity < 100) {
		css.add(join_selectors({"html", "body"}), "background-color", "transparent");
		css.add(join_selectors({".chat-room", ".stream-chat-header"}), "background-color",
			rgba_text(background.value_or(Rgb{24, 24, 27}), opacity));
	}
	if (!r.toggle("header_visible"))
		css.add(".stream-chat-header", "display", "none");
	if (!r.toggle("input_panel_visible"))
		css.add(".chat-input", "display", "none");
	if (!r.toggle("engagement_visible"))
		css.add("[data-a-target=\"chat-welcome-message\"]", "display", "none");
}

void write_message(CssWriter &css, const Reader &r)
{
	const std::string body = under(LINE, BODY);
	add_text(css, r, body, "message_font_size", {});
	if (const auto c = r.color("message_color"))
		css.add(join_selectors({body, under(body, ".text-fragment")}), "color", rgb_text(*c));
	if (const auto height = r.number("message_line_height"))
		css.add(body, "line-height", ratio_text(*height));
	if (const auto spacing = r.number("message_letter_spacing"))
		css.add(body, "letter-spacing", px(*spacing));
	if (r.toggle("message_new_line"))
		css.add(body, "display", "block");
	if (r.number("message_font_size") || r.number("message_letter_spacing"))
		// 空白の無い長い文字列がメッセージの右端で切り取られないよう、どこでも折り返す
		css.add(body, "overflow-wrap", "anywhere");
}

void write_name(CssWriter &css, const Reader &r)
{
	const std::string name = names(std::string(LINE));
	add_text(css, r, name, "name_font_size", {});
	// Twitch は利用者ごとの色をインラインの style で付けるため、色を指定するときは !important で上書きする
	if (!r.toggle("use_user_color"))
		if (const auto c = r.color("name_color"))
			css.add(name, "color", rgb_text(*c));
	// 種別ごとの色を後に書き、利用者ごとの色と全体の色より優先させる
	if (const auto c = r.color("subscriber_name_color"))
		css.add(names(subscriber_line()), "color", rgb_text(*c));
	if (const auto c = r.color("vip_name_color"))
		css.add(names(vip_line()), "color", rgb_text(*c));
	if (const auto c = r.color("moderator_name_color"))
		css.add(names(moderator_line()), "color", rgb_text(*c));
	if (const auto c = r.color("broadcaster_name_color"))
		css.add(names(broadcaster_line()), "color", rgb_text(*c));
	if (!r.toggle("badges_visible"))
		css.add(under(LINE, "img.chat-badge"), "display", "none");
	if (!r.toggle("name_colon_visible"))
		// 名前の後の「: 」は、名前を包む要素の次の span にある
		css.add(under(LINE, ".chat-line__username-container + span"), "display", "none");
}

void write_outline(CssWriter &css, const Reader &r)
{
	if (!r.toggle("outline_enabled"))
		return;
	css.add(join_selectors({std::string(LINE), std::string(NOTICE)}), "text-shadow",
		outline_shadow(r.number("outline_width").value_or(2), r.color("outline_color").value_or(Rgb{0, 0, 0})));
}

void write_message_background(CssWriter &css, const Reader &r)
{
	const int opacity = r.number("message_background_opacity").value_or(100);
	const bool bar = r.text("message_background_style") == "bar";
	const auto paint = [&](const std::string &selector, const Rgb &c) {
		if (bar)
			css.add(selector, "box-shadow", "inset 4px 0 0 " + rgba_text(c, opacity));
		else
			css.add(selector, "background-color", rgba_text(c, opacity));
	};
	// 種別ごとの色を後に書き、全体の色より優先させる
	if (const auto c = r.color("message_background_color"))
		paint(std::string(LINE), *c);
	if (const auto c = r.color("subscriber_background_color"))
		paint(subscriber_line(), *c);
	if (const auto c = r.color("vip_background_color"))
		paint(vip_line(), *c);
	if (const auto c = r.color("moderator_background_color"))
		paint(moderator_line(), *c);
	if (const auto c = r.color("broadcaster_background_color"))
		paint(broadcaster_line(), *c);
}

void write_notice(CssWriter &css, const Reader &r)
{
	add_text(css, r, std::string(NOTICE), "notice_font_size", "notice_color");
}

} // namespace

std::string generate_twitch_css(const StyleValues &values)
{
	const Reader r(twitch_sections(), values);
	CssWriter css;
	write_font(css, r);
	write_layout(css, r);
	write_message(css, r);
	write_name(css, r);
	write_outline(css, r);
	write_message_background(css, r);
	write_notice(css, r);
	// 動かすのはメッセージとサブスクの通知の行だけにする。案内の行は動かさない
	write_animation(css, r,
			std::string("[role=\"log\"] > *:has(") + std::string(LINE) + ", " + std::string(NOTICE) + ")");
	return css.str();
}

} // namespace cskin::core
