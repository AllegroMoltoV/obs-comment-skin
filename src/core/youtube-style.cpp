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

#include "css-builder.h"

#include <algorithm>

namespace cskin::core {

namespace {

// 要素名は docs/reports/2026-11-10-youtube-chat-dom.md で確かめたもの
constexpr std::string_view TEXT = "yt-live-chat-text-message-renderer";
constexpr std::string_view PAID = "yt-live-chat-paid-message-renderer";
constexpr std::string_view MEMBERSHIP = "yt-live-chat-membership-item-renderer";
constexpr std::string_view STICKER = "yt-live-chat-paid-sticker-renderer";

using namespace css;

void add_font(CssWriter &css, const Reader &r, const std::string &selector, std::string_view size_key,
	      std::string_view color_key)
{
	if (!size_key.empty()) {
		if (const auto size = r.number(size_key)) {
			css.add(selector, "font-size", px(*size));
			// YouTube は行の高さを px で固定しているため、文字を大きくしても行が重ならないよう比率にする
			css.add(selector, "line-height", "1.43");
		}
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
	// 書体はメッセージの中のすべての要素に当てる。YouTube は内側の要素で書体を指定し直すことがあり、
	// 外側の要素だけに当てると引き継がれないため
	css.add(join_selectors({std::string(TEXT), under(TEXT, "*"), std::string(PAID), under(PAID, "*"),
				std::string(MEMBERSHIP), under(MEMBERSHIP, "*"), std::string(STICKER),
				under(STICKER, "*"), "yt-live-chat-ticker-renderer *"}),
		"font-family", css_string(font));
}

void write_layout(CssWriter &css, const Reader &r)
{
	// 色を選ばずに不透明度だけを変えたときは、YouTube の既定の背景色 rgb(15, 15, 15) に不透明度を掛ける。
	// 不透明度を 0 にするだけで背景を透明にできるようにするため
	const int opacity = r.number("background_opacity").value_or(100);
	const auto background = r.color("background_color");
	// 下部のログインを促す欄も自前の背景で塗られているため、全体の背景色に合わせる。欄そのものは隠さない
	if (background || opacity < 100)
		css.add(join_selectors({"yt-live-chat-renderer", "yt-live-chat-header-renderer",
					"yt-live-chat-renderer #input-panel > *"}),
			"background-color", rgba_text(background.value_or(Rgb{15, 15, 15}), opacity));
	if (!r.toggle("header_visible"))
		css.add("yt-live-chat-header-renderer", "display", "none");
	if (!r.toggle("ticker_visible"))
		css.add("yt-live-chat-renderer #ticker", "display", "none");
	if (!r.toggle("input_panel_visible"))
		css.add("yt-live-chat-renderer #input-panel", "display", "none");
	if (!r.toggle("engagement_visible"))
		css.add("yt-live-chat-viewer-engagement-message-renderer", "display", "none");
}

void write_message(CssWriter &css, const Reader &r)
{
	const std::string message = under(TEXT, "#message");
	add_font(css, r, message, "message_font_size", "message_color");
	if (const auto height = r.number("message_line_height"))
		css.add(message, "line-height", ratio_text(*height));
	if (const auto spacing = r.number("message_letter_spacing"))
		css.add(message, "letter-spacing", px(*spacing));
	if (r.toggle("message_new_line"))
		css.add(message, "display", "block");
	if (r.number("message_font_size") || r.number("message_letter_spacing")) {
		// 本文を包む #content は flex の子で、空白の無い長い文字列より狭くなれない。
		// 文字を大きくしたときや字間を広げたときにメッセージの右端で切り取られないよう、幅を抑えてどこでも折り返す
		css.add(under(TEXT, "#content"), "min-width", "0");
		css.add(message, "overflow-wrap", "anywhere");
	}
}

void write_name(CssWriter &css, const Reader &r)
{
	const std::string name = under(TEXT, "#author-name");
	add_font(css, r, name, "name_font_size", {});
	if (r.number("name_font_size")) {
		// 名前を包む部品は inline-flex で、空白の無い長い名前は折り返されない。
		// 文字を大きくしたときにメッセージの右端で切り取られないよう、幅を抑えてどこでも折り返す
		css.add(under(TEXT, "yt-live-chat-author-chip"), "max-width", "100%");
		css.add(name, "min-width", "0");
		css.add(name, "overflow-wrap", "anywhere");
	}
	// 配信者・モデレーター・メンバーの名前は YouTube が色分けしているため、全体の色はそれ以外の名前に当てる
	if (const auto c = r.color("name_color"))
		css.add(name + ":not(.owner):not(.moderator):not(.member)", "color", rgb_text(*c));
	if (const auto c = r.color("owner_name_color"))
		css.add(name + ".owner", "color", rgb_text(*c));
	if (!r.toggle("owner_name_highlight")) {
		// YouTube は配信者の名前を黄色の網掛けに暗い文字で表示する。網掛けを消すと暗い背景に文字が埋もれるため、
		// 色の指定が無ければ網掛けの黄色を文字の色にする
		css.add(name + ".owner", "background-color", "transparent");
		if (!r.color("owner_name_color"))
			css.add(name + ".owner", "color", "rgb(255, 214, 0)");
	}
	if (const auto c = r.color("moderator_name_color"))
		css.add(name + ".moderator", "color", rgb_text(*c));
	if (const auto c = r.color("member_name_color"))
		css.add(name + ".member", "color", rgb_text(*c));
	if (!r.toggle("badges_visible"))
		css.add(join_selectors({under(TEXT, "#chat-badges"), under(TEXT, "#chip-badges")}), "display", "none");
	if (r.toggle("name_colon"))
		css.add_raw(std::string(TEXT) + " #author-name::after {\n  content: \":\";\n}\n");
}

void write_avatar(CssWriter &css, const Reader &r)
{
	const std::string photo = join_selectors({under(TEXT, "#author-photo"), under(PAID, "#author-photo"),
						  under(MEMBERSHIP, "#author-photo"), under(STICKER, "#author-photo")});
	if (!r.toggle("avatar_visible")) {
		css.add(photo, "display", "none");
		return;
	}
	const std::string photo_and_image =
		join_selectors({under(TEXT, "#author-photo"), under(TEXT, "#author-photo img"),
				under(PAID, "#author-photo"), under(PAID, "#author-photo img"),
				under(MEMBERSHIP, "#author-photo"), under(MEMBERSHIP, "#author-photo img"),
				under(STICKER, "#author-photo"), under(STICKER, "#author-photo img")});
	if (const auto size = r.number("avatar_size")) {
		css.add(photo_and_image, "width", px(*size));
		css.add(photo_and_image, "height", px(*size));
	}
	const std::string shape = r.text("avatar_shape");
	if (shape == "circle")
		css.add(photo_and_image, "border-radius", "50%");
	else if (shape == "rounded")
		css.add(photo_and_image, "border-radius", "25%");
	else if (shape == "square")
		css.add(photo_and_image, "border-radius", "0");
}

void write_timestamp(CssWriter &css, const Reader &r)
{
	const std::string timestamp = under(TEXT, "#timestamp");
	const std::string display = r.text("timestamp_display");
	if (display == "show")
		css.add(timestamp, "display", "inline");
	else if (display == "hide")
		css.add(timestamp, "display", "none");
	add_font(css, r, timestamp, "timestamp_font_size", "timestamp_color");
}

void write_outline(CssWriter &css, const Reader &r)
{
	if (!r.toggle("outline_enabled"))
		return;
	css.add(join_selectors({std::string(TEXT), std::string(PAID), std::string(MEMBERSHIP), std::string(STICKER)}),
		"text-shadow",
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
		paint(std::string(TEXT), *c);
	if (const auto c = r.color("member_background_color"))
		paint(std::string(TEXT) + ":has(#author-name.member)", *c);
	if (const auto c = r.color("moderator_background_color"))
		paint(std::string(TEXT) + ":has(#author-name.moderator)", *c);
	if (const auto c = r.color("owner_background_color"))
		paint(std::string(TEXT) + "[author-type=\"owner\"]", *c);
}

void write_paid(CssWriter &css, const Reader &r)
{
	// 1 行目は名前、2 行目は金額またはメンバーシップの説明、本文はメッセージ
	add_font(css, r, join_selectors({under(PAID, "#author-name"), under(MEMBERSHIP, "#author-name")}),
		 "paid_name_font_size", "paid_name_color");
	add_font(css, r, join_selectors({under(PAID, "#purchase-amount"), under(MEMBERSHIP, "#header-subtext")}),
		 "paid_amount_font_size", "paid_amount_color");
	add_font(css, r, join_selectors({under(PAID, "#message"), under(MEMBERSHIP, "#message")}),
		 "paid_message_font_size", "paid_message_color");
	if (r.number("paid_name_font_size") || r.number("paid_amount_font_size")) {
		// 名前と金額の列は flex の子で、空白の無い文字列は折り返されない。
		// 文字を大きくしたときに枠の右端で切り取られないよう、幅を抑えてどこでも折り返す
		css.add(join_selectors({under(PAID, "#header-content"), under(MEMBERSHIP, "#header-content"),
					under(PAID, "#header-content-primary-column"), under(PAID, "#author-name-chip"),
					under(PAID, "#purchase-amount-column"), under(PAID, "#purchase-amount")}),
			"min-width", "0");
		// Super Chat は名前と金額を折り返さない 1 行の枠 #single-line に並べるため、大きくすると金額が切り取られる
		css.add(under(PAID, "#single-line"), "flex-wrap", "wrap");
		css.add(under(PAID, "#single-line"), "min-width", "0");
		css.add(join_selectors({under(PAID, "#author-name"), under(MEMBERSHIP, "#author-name"),
					under(PAID, "#purchase-amount"), under(MEMBERSHIP, "#header-subtext")}),
			"overflow-wrap", "anywhere");
	}
	if (!r.toggle("membership_background_visible"))
		css.add(join_selectors({under(MEMBERSHIP, "#card"), under(MEMBERSHIP, "#header"),
					under(MEMBERSHIP, "#content")}),
			"background", "transparent");
}

void write_messages_animation(CssWriter &css, const Reader &r)
{
	// 動かすのはチャットのメッセージだけにする。YouTube の案内などの要素は消さない
	write_animation(
		css, r,
		join_selectors({std::string(TEXT), std::string(PAID), std::string(MEMBERSHIP), std::string(STICKER)}));
}

} // namespace

std::string generate_youtube_css(const StyleValues &values)
{
	const Reader r(youtube_sections(), values);
	CssWriter css;
	write_font(css, r);
	write_layout(css, r);
	write_message(css, r);
	write_name(css, r);
	write_avatar(css, r);
	write_timestamp(css, r);
	write_outline(css, r);
	write_message_background(css, r);
	write_paid(css, r);
	write_messages_animation(css, r);
	return css.str();
}

} // namespace cskin::core
