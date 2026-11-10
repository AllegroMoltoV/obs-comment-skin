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

// 要素名は docs/reports/2026-11-10-youtube-chat-dom.md で確かめたもの
constexpr std::string_view TEXT = "yt-live-chat-text-message-renderer";
constexpr std::string_view PAID = "yt-live-chat-paid-message-renderer";
constexpr std::string_view MEMBERSHIP = "yt-live-chat-membership-item-renderer";
constexpr std::string_view STICKER = "yt-live-chat-paid-sticker-renderer";

// 値を読む。範囲の外の値は表の範囲に収める
class Reader {
public:
	explicit Reader(const StyleValues &values) : values_(sanitize(youtube_sections(), values)) {}

	std::optional<int> number(std::string_view key) const
	{
		const auto it = values_.numbers.find(key);
		if (it != values_.numbers.end())
			return it->second;
		const ItemDef *item = find_item(youtube_sections(), key);
		return item ? item->default_value : std::nullopt;
	}

	bool toggle(std::string_view key) const { return number(key).value_or(0) != 0; }

	std::optional<Rgb> color(std::string_view key) const
	{
		const auto it = values_.colors.find(key);
		return it != values_.colors.end() ? std::optional<Rgb>(it->second) : std::nullopt;
	}

	std::string text(std::string_view key) const
	{
		const auto it = values_.texts.find(key);
		if (it != values_.texts.end())
			return it->second;
		const ItemDef *item = find_item(youtube_sections(), key);
		return item && !item->choices.empty() ? std::string(item->choices.front()) : std::string();
	}

private:
	StyleValues values_;
};

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

// 百分率を倍率にする。140 は 1.4
std::string ratio_text(int percent)
{
	std::string text = std::to_string(percent / 100);
	const int fraction = percent % 100;
	if (fraction != 0) {
		std::string digits = std::to_string(fraction);
		if (digits.size() == 1)
			digits = "0" + digits;
		if (digits.back() == '0')
			digits.pop_back();
		text += "." + digits;
	}
	return text;
}

std::string px(int value)
{
	return std::to_string(value) + "px";
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

std::string join_selectors(std::initializer_list<std::string> selectors)
{
	std::string result;
	for (const auto &s : selectors) {
		if (!result.empty())
			result += ",\n";
		result += s;
	}
	return result;
}

std::string under(std::string_view host, std::string_view descendant)
{
	return std::string(host) + " " + std::string(descendant);
}

class CssWriter {
public:
	using Declarations = std::vector<std::pair<std::string, std::string>>;

	// 宣言を 1 つ足す。同じセレクターの宣言は 1 つの規則にまとめ、同じプロパティは後の値で置き換える
	void add(const std::string &selector, const std::string &property, const std::string &value)
	{
		auto rule = std::find_if(rules_.begin(), rules_.end(),
					 [&](const auto &entry) { return entry.first == selector; });
		if (rule == rules_.end()) {
			rules_.emplace_back(selector, Declarations());
			rule = rules_.end() - 1;
		}
		auto declaration = std::find_if(rule->second.begin(), rule->second.end(),
						[&](const auto &entry) { return entry.first == property; });
		if (declaration != rule->second.end())
			declaration->second = value;
		else
			rule->second.emplace_back(property, value);
	}

	void add_raw(const std::string &text) { raw_ += text; }

	std::string str() const
	{
		std::string css;
		for (const auto &[selector, declarations] : rules_) {
			css += selector + " {\n";
			for (const auto &[property, value] : declarations)
				css += "  " + property + ": " + value + " !important;\n";
			css += "}\n";
		}
		return css + raw_;
	}

private:
	std::vector<std::pair<std::string, Declarations>> rules_;
	std::string raw_;
};

void add_font(CssWriter &css, const Reader &r, const std::string &selector, std::string_view font_key,
	      std::string_view size_key, std::string_view color_key)
{
	const std::string font = r.text(font_key);
	if (!font.empty())
		css.add(selector, "font-family", css_string(font));
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

void write_layout(CssWriter &css, const Reader &r)
{
	if (const auto c = r.color("background_color"))
		css.add(join_selectors({"yt-live-chat-renderer", "yt-live-chat-header-renderer"}), "background-color",
			rgba_text(*c, r.number("background_opacity").value_or(100)));
	if (!r.toggle("header_visible"))
		css.add("yt-live-chat-header-renderer", "display", "none");
	if (!r.toggle("ticker_visible"))
		css.add("yt-live-chat-renderer #ticker", "display", "none");
}

void write_message(CssWriter &css, const Reader &r)
{
	const std::string message = under(TEXT, "#message");
	add_font(css, r, message, "message_font", "message_font_size", "message_color");
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
	add_font(css, r, name, "name_font", "name_font_size", {});
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
	add_font(css, r, timestamp, "timestamp_font", "timestamp_font_size", "timestamp_color");
}

void write_outline(CssWriter &css, const Reader &r)
{
	if (!r.toggle("outline_enabled"))
		return;
	const std::string w = px(r.number("outline_width").value_or(2));
	const std::string n = "-" + w;
	const std::string c = rgb_text(r.color("outline_color").value_or(Rgb{0, 0, 0}));
	std::string shadow;
	const std::pair<std::string, std::string> offsets[] = {{n, n},   {"0", n}, {w, n},   {n, "0"},
							       {w, "0"}, {n, w},   {"0", w}, {w, w}};
	for (const auto &[x, y] : offsets) {
		if (!shadow.empty())
			shadow += ", ";
		shadow += x + " " + y + " 0 " + c;
	}
	css.add(join_selectors({std::string(TEXT), std::string(PAID), std::string(MEMBERSHIP), std::string(STICKER)}),
		"text-shadow", shadow);
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
	add_font(css, r, join_selectors({under(PAID, "#author-name"), under(MEMBERSHIP, "#author-name")}), "paid_font",
		 "paid_name_font_size", "paid_name_color");
	add_font(css, r, join_selectors({under(PAID, "#purchase-amount"), under(MEMBERSHIP, "#header-subtext")}),
		 "paid_font", "paid_amount_font_size", "paid_amount_color");
	add_font(css, r, join_selectors({under(PAID, "#message"), under(MEMBERSHIP, "#message")}), "paid_font",
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

void write_animation(CssWriter &css, const Reader &r)
{
	const bool fade_in = r.toggle("fade_in");
	const std::string slide = r.text("slide_in");
	const bool slide_in = slide != "none" && !slide.empty();
	const bool remove_old = r.toggle("remove_old");
	if (!fade_in && !slide_in && !remove_old)
		return;

	// 動かすのはチャットのメッセージだけにする。YouTube の案内などの要素は消さない
	const std::string hosts =
		join_selectors({std::string(TEXT), std::string(PAID), std::string(MEMBERSHIP), std::string(STICKER)});
	std::string animations;
	if (fade_in || slide_in) {
		std::string from_transform = "none";
		if (slide == "left")
			from_transform = "translateX(-100%)";
		else if (slide == "right")
			from_transform = "translateX(100%)";
		else if (slide == "bottom")
			from_transform = "translateY(100%)";
		css.add_raw("@keyframes cskin-in {\n  from { opacity: " + std::string(fade_in ? "0" : "1") +
			    "; transform: " + from_transform + "; }\n  to { opacity: 1; transform: none; }\n}\n");
		animations +=
			"cskin-in " + std::to_string(r.number("fade_in_duration").value_or(200)) + "ms ease-out both";
	}
	if (remove_old) {
		css.add_raw("@keyframes cskin-out {\n  99% { opacity: 0; }\n"
			    "  100% { opacity: 0; max-height: 0; margin: 0; padding: 0; overflow: hidden; }\n}\n");
		if (!animations.empty())
			animations += ", ";
		animations += "cskin-out " + std::to_string(r.number("fade_out_duration").value_or(200)) +
			      "ms ease-in " + std::to_string(r.number("remove_after").value_or(30)) + "s forwards";
	}
	css.add(hosts, "animation", animations);
}

} // namespace

std::string css_string(std::string_view text)
{
	std::string out = "\"";
	for (const char ch : text) {
		const unsigned char c = static_cast<unsigned char>(ch);
		if (c < 0x20 || c == 0x7f)
			continue;
		if (ch == '"' || ch == '\\')
			out += '\\';
		out += ch;
	}
	out += '"';
	return out;
}

std::string generate_youtube_css(const StyleValues &values)
{
	const Reader r(values);
	CssWriter css;
	write_layout(css, r);
	write_message(css, r);
	write_name(css, r);
	write_avatar(css, r);
	write_timestamp(css, r);
	write_outline(css, r);
	write_message_background(css, r);
	write_paid(css, r);
	write_animation(css, r);
	return css.str();
}

} // namespace cskin::core
