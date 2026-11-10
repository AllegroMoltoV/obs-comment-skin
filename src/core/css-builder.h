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

// YouTube と Twitch の CSS の生成で共通に使う部品

#include "style-schema.h"

#include <algorithm>
#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace cskin::core {

// CSS の二重引用符の文字列にする。書体の名前を font-family に入れるときに使う
inline std::string css_string(std::string_view text)
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

} // namespace cskin::core

namespace cskin::core::css {

// 設定の値を読む。範囲の外の値は表の範囲に収め、値が無い項目は表の既定値を返す
class Reader {
public:
	Reader(const std::vector<SectionDef> &sections, const StyleValues &values)
		: sections_(sections),
		  values_(sanitize(sections, values))
	{
	}

	std::optional<int> number(std::string_view key) const
	{
		const auto it = values_.numbers.find(key);
		if (it != values_.numbers.end())
			return it->second;
		const ItemDef *item = find_item(sections_, key);
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
		const ItemDef *item = find_item(sections_, key);
		return item && !item->choices.empty() ? std::string(item->choices.front()) : std::string();
	}

private:
	const std::vector<SectionDef> &sections_;
	StyleValues values_;
};

// ロケールに依存しないよう、小数を自前で書く。0 から 100 の百分率を 0 から 1 にする
inline std::string alpha_text(int percent)
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
inline std::string ratio_text(int percent)
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

inline std::string px(int value)
{
	return std::to_string(value) + "px";
}

inline std::string rgb_text(const Rgb &c)
{
	return "rgb(" + std::to_string(c.r) + ", " + std::to_string(c.g) + ", " + std::to_string(c.b) + ")";
}

inline std::string rgba_text(const Rgb &c, int percent)
{
	return "rgba(" + std::to_string(c.r) + ", " + std::to_string(c.g) + ", " + std::to_string(c.b) + ", " +
	       alpha_text(percent) + ")";
}

inline std::string join_selectors(std::initializer_list<std::string> selectors)
{
	std::string result;
	for (const auto &s : selectors) {
		if (!result.empty())
			result += ",\n";
		result += s;
	}
	return result;
}

inline std::string under(std::string_view host, std::string_view descendant)
{
	return std::string(host) + " " + std::string(descendant);
}

// 文字を縁取る text-shadow の値。8 方向に影を置く
inline std::string outline_shadow(int width, const Rgb &color)
{
	const std::string w = px(width);
	const std::string n = "-" + w;
	const std::string c = rgb_text(color);
	const std::pair<std::string, std::string> offsets[] = {{n, n},   {"0", n}, {w, n},   {n, "0"},
							       {w, "0"}, {n, w},   {"0", w}, {w, w}};
	std::string shadow;
	for (const auto &[x, y] : offsets) {
		if (!shadow.empty())
			shadow += ", ";
		shadow += x + " " + y + " 0 " + c;
	}
	return shadow;
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

// 現れる・消えるアニメーションを hosts に当てる。設定が無ければ何もしない
inline void write_animation(CssWriter &css, const Reader &r, const std::string &hosts)
{
	const bool fade_in = r.toggle("fade_in");
	const std::string slide = r.text("slide_in");
	const bool slide_in = slide != "none" && !slide.empty();
	const bool remove_old = r.toggle("remove_old");
	if (!fade_in && !slide_in && !remove_old)
		return;

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

} // namespace cskin::core::css
