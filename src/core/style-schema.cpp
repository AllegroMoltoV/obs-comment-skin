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

#include "style-schema.h"

#include <algorithm>

namespace cskin::core {

namespace {

int hex_digit(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return -1;
}

ItemDef number(std::string_view key, std::string_view label, int min, int max, std::string_view unit,
	       std::optional<int> default_value = std::nullopt)
{
	return ItemDef{key, ItemKind::Number, label, min, max, default_value, unit, {}};
}

ItemDef opacity(std::string_view key, std::string_view label)
{
	return ItemDef{key, ItemKind::Opacity, label, 0, 100, 100, "%", {}};
}

ItemDef toggle(std::string_view key, std::string_view label, bool default_on)
{
	return ItemDef{key, ItemKind::Toggle, label, 0, 1, default_on ? 1 : 0, {}, {}};
}

ItemDef color(std::string_view key, std::string_view label)
{
	return ItemDef{key, ItemKind::Color, label, 0, 0, std::nullopt, {}, {}};
}

ItemDef choice(std::string_view key, std::string_view label, std::vector<std::string_view> choices)
{
	return ItemDef{key, ItemKind::Choice, label, 0, 0, std::nullopt, {}, std::move(choices)};
}

ItemDef font(std::string_view key, std::string_view label)
{
	return ItemDef{key, ItemKind::Font, label, 0, 0, std::nullopt, {}, {}};
}

ItemDef when(ItemDef item, std::vector<Condition> conditions)
{
	item.enabled_when = std::move(conditions);
	return item;
}

// メッセージの背景の不透明度と形は、いずれかの背景色を選んだときだけ効く
const std::vector<Condition> MESSAGE_BACKGROUND_SET = {{"message_background_color", "set"},
						       {"owner_background_color", "set"},
						       {"moderator_background_color", "set"},
						       {"member_background_color", "set"}};

} // namespace

std::optional<Rgb> parse_hex_color(std::string_view text)
{
	if (text.size() != 7 || text[0] != '#')
		return std::nullopt;
	int values[6];
	for (size_t i = 0; i < 6; ++i) {
		values[i] = hex_digit(text[i + 1]);
		if (values[i] < 0)
			return std::nullopt;
	}
	return Rgb{static_cast<std::uint8_t>(values[0] * 16 + values[1]),
		   static_cast<std::uint8_t>(values[2] * 16 + values[3]),
		   static_cast<std::uint8_t>(values[4] * 16 + values[5])};
}

std::string to_hex_color(const Rgb &color)
{
	static constexpr char DIGITS[] = "0123456789abcdef";
	std::string text = "#";
	for (const std::uint8_t v : {color.r, color.g, color.b}) {
		text += DIGITS[v >> 4];
		text += DIGITS[v & 0x0f];
	}
	return text;
}

const std::vector<SectionDef> &youtube_sections()
{
	// キーは保存とプリセットに使うため、変えるときは古いキーの読み込みも考える
	static const std::vector<SectionDef> sections = {
		{"Section.General",
		 {
			 font("font", "Item.Font"),
		 }},
		{"Section.Message",
		 {
			 number("message_font_size", "Item.FontSize", 8, 96, "px"),
			 number("message_line_height", "Item.LineHeight", 100, 300, "%"),
			 number("message_letter_spacing", "Item.LetterSpacing", 0, 20, "px"),
			 color("message_color", "Item.TextColor"),
			 toggle("message_new_line", "Item.MessageNewLine", false),
		 }},
		{"Section.Name",
		 {
			 number("name_font_size", "Item.FontSize", 8, 96, "px"),
			 color("name_color", "Item.TextColor"),
			 color("owner_name_color", "Item.OwnerColor"),
			 toggle("owner_name_highlight", "Item.OwnerHighlight", true),
			 color("moderator_name_color", "Item.ModeratorColor"),
			 color("member_name_color", "Item.MemberColor"),
			 toggle("badges_visible", "Item.BadgesVisible", true),
			 toggle("name_colon", "Item.NameColon", false),
		 }},
		{"Section.Avatar",
		 {
			 toggle("avatar_visible", "Item.AvatarVisible", true),
			 when(number("avatar_size", "Item.AvatarSize", 12, 96, "px"), {{"avatar_visible", "on"}}),
			 when(choice("avatar_shape", "Item.AvatarShape", {"default", "circle", "rounded", "square"}),
			      {{"avatar_visible", "on"}}),
		 }},
		{"Section.Timestamp",
		 {
			 choice("timestamp_display", "Item.TimestampDisplay", {"default", "show", "hide"}),
			 when(number("timestamp_font_size", "Item.FontSize", 8, 48, "px"),
			      {{"timestamp_display", "show"}}),
			 when(color("timestamp_color", "Item.TextColor"), {{"timestamp_display", "show"}}),
		 }},
		{"Section.Outline",
		 {
			 toggle("outline_enabled", "Item.OutlineEnabled", false),
			 when(number("outline_width", "Item.OutlineWidth", 1, 8, "px", 2), {{"outline_enabled", "on"}}),
			 when(color("outline_color", "Item.OutlineColor"), {{"outline_enabled", "on"}}),
		 }},
		{"Section.Background",
		 {
			 color("background_color", "Item.BackgroundColor"),
			 opacity("background_opacity", "Item.Opacity"),
			 color("message_background_color", "Item.MessageBackgroundColor"),
			 color("owner_background_color", "Item.OwnerBackgroundColor"),
			 color("moderator_background_color", "Item.ModeratorBackgroundColor"),
			 color("member_background_color", "Item.MemberBackgroundColor"),
			 when(opacity("message_background_opacity", "Item.MessageBackgroundOpacity"),
			      MESSAGE_BACKGROUND_SET),
			 when(choice("message_background_style", "Item.MessageBackgroundStyle", {"fill", "bar"}),
			      MESSAGE_BACKGROUND_SET),
		 }},
		{"Section.Paid",
		 {
			 number("paid_name_font_size", "Item.PaidNameFontSize", 8, 96, "px"),
			 color("paid_name_color", "Item.PaidNameColor"),
			 number("paid_amount_font_size", "Item.PaidAmountFontSize", 8, 96, "px"),
			 color("paid_amount_color", "Item.PaidAmountColor"),
			 number("paid_message_font_size", "Item.PaidMessageFontSize", 8, 96, "px"),
			 color("paid_message_color", "Item.PaidMessageColor"),
			 toggle("membership_background_visible", "Item.MembershipBackgroundVisible", true),
		 }},
		{"Section.Layout",
		 {
			 toggle("header_visible", "Item.HeaderVisible", true),
			 toggle("ticker_visible", "Item.TickerVisible", true),
			 toggle("input_panel_visible", "Item.InputPanelVisible", true),
			 toggle("engagement_visible", "Item.EngagementVisible", true),
		 }},
		{"Section.Animation",
		 {
			 toggle("fade_in", "Item.FadeIn", false),
			 when(number("fade_in_duration", "Item.FadeInDuration", 0, 3000, "ms", 200),
			      {{"fade_in", "on"}, {"slide_in", "!none"}}),
			 choice("slide_in", "Item.SlideIn", {"none", "left", "right", "bottom"}),
			 toggle("remove_old", "Item.RemoveOld", false),
			 when(number("remove_after", "Item.RemoveAfter", 1, 600, "s", 30), {{"remove_old", "on"}}),
			 when(number("fade_out_duration", "Item.FadeOutDuration", 0, 3000, "ms", 200),
			      {{"remove_old", "on"}}),
		 }},
	};
	return sections;
}

std::string sanitize_font_name(std::string_view name)
{
	static constexpr std::string_view REJECTED = "()\"\\;{}<>@";
	std::string result;
	for (const char ch : name) {
		const unsigned char c = static_cast<unsigned char>(ch);
		if (c < 0x20 || c == 0x7f || REJECTED.find(ch) != std::string_view::npos)
			continue;
		result += ch;
	}
	const size_t first = result.find_first_not_of(' ');
	if (first == std::string::npos)
		return {};
	return result.substr(first, result.find_last_not_of(' ') - first + 1);
}

bool item_effective(const ItemDef &item, const std::vector<SectionDef> &sections, const StyleValues &values)
{
	if (item.enabled_when.empty())
		return true;
	for (const auto &condition : item.enabled_when) {
		const ItemDef *target = find_item(sections, condition.key);
		if (!target)
			continue;
		switch (target->kind) {
		case ItemKind::Toggle: {
			const auto it = values.numbers.find(condition.key);
			const int value = it != values.numbers.end() ? it->second : target->default_value.value_or(0);
			if ((value != 0) == (condition.value == "on"))
				return true;
			break;
		}
		case ItemKind::Color:
			if (values.colors.count(condition.key) > 0)
				return true;
			break;
		case ItemKind::Choice: {
			const auto it = values.texts.find(condition.key);
			const std::string current = it != values.texts.end() ? it->second
									     : std::string(target->choices.front());
			const bool negate = !condition.value.empty() && condition.value.front() == '!';
			const std::string_view expected = negate ? condition.value.substr(1) : condition.value;
			if ((current == expected) != negate)
				return true;
			break;
		}
		case ItemKind::Number:
		case ItemKind::Opacity:
		case ItemKind::Font:
			break;
		}
	}
	return false;
}

const ItemDef *find_item(const std::vector<SectionDef> &sections, std::string_view key)
{
	for (const auto &section : sections)
		for (const auto &item : section.items)
			if (item.key == key)
				return &item;
	return nullptr;
}

StyleValues sanitize(const std::vector<SectionDef> &sections, const StyleValues &values)
{
	StyleValues result;
	for (const auto &[key, value] : values.numbers) {
		const ItemDef *item = find_item(sections, key);
		if (!item || (item->kind != ItemKind::Number && item->kind != ItemKind::Opacity &&
			      item->kind != ItemKind::Toggle))
			continue;
		result.numbers[key] = std::clamp(value, item->min, item->max);
	}
	for (const auto &[key, value] : values.colors) {
		const ItemDef *item = find_item(sections, key);
		if (item && item->kind == ItemKind::Color)
			result.colors[key] = value;
	}
	for (const auto &[key, value] : values.texts) {
		const ItemDef *item = find_item(sections, key);
		if (!item)
			continue;
		if (item->kind == ItemKind::Choice &&
		    std::find(item->choices.begin(), item->choices.end(), value) != item->choices.end())
			result.texts[key] = value;
		else if (item->kind == ItemKind::Font) {
			const std::string name = sanitize_font_name(value);
			if (!name.empty())
				result.texts[key] = name;
		}
	}
	return result;
}

} // namespace cskin::core
