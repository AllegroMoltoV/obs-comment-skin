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

// OBS に依存しない処理の単体テスト。失敗した検査を標準エラーに書き、終了コード 1 で終わる

#include "core/chat-target.h"
#include "core/managed-css.h"
#include "core/preview-script.h"
#include "core/youtube-style.h"

#include <iostream>
#include <optional>
#include <string>

namespace {

int failures = 0;

void check(bool condition, const char *what, int line)
{
	if (condition)
		return;
	++failures;
	std::cerr << "FAILED (line " << line << "): " << what << '\n';
}

#define CHECK(expr) check((expr), #expr, __LINE__)

using namespace cskin::core;

const std::string DEFAULT_CSS = "body { background-color: rgba(0, 0, 0, 0); margin: 0px auto; overflow: hidden; }";

size_t count(const std::string &text, const std::string &needle)
{
	size_t n = 0;
	for (size_t pos = text.find(needle); pos != std::string::npos; pos = text.find(needle, pos + 1))
		++n;
	return n;
}

void test_replace_appends_when_absent()
{
	const auto result = replace_block(DEFAULT_CSS, ".a { color: red; }");
	CHECK(result.has_value());
	CHECK(result->rfind(DEFAULT_CSS + "\n", 0) == 0);
	CHECK(result->find(".a { color: red; }") != std::string::npos);
	CHECK(inspect_block(*result) == BlockState::Intact);
}

void test_replace_on_empty_css()
{
	const auto result = replace_block("", ".a { color: red; }");
	CHECK(result.has_value());
	CHECK(result->rfind(std::string(BLOCK_BEGIN), 0) == 0);
}

void test_replace_keeps_user_css_outside_block()
{
	const std::string before = "/* mine */\n.user { color: blue; }\n";
	const std::string after = "\n.user2 { color: green; }\n";
	const std::string css =
		before + std::string(BLOCK_BEGIN) + "\n.old { color: red; }\n" + std::string(BLOCK_END) + after;

	const auto result = replace_block(css, ".new { color: white; }");
	CHECK(result.has_value());
	CHECK(result->rfind(before, 0) == 0);
	CHECK(result->size() >= after.size() &&
	      result->compare(result->size() - after.size(), after.size(), after) == 0);
	CHECK(result->find(".old") == std::string::npos);
	CHECK(result->find(".new { color: white; }") != std::string::npos);
}

void test_replace_twice_keeps_single_block()
{
	const auto first = replace_block(DEFAULT_CSS, ".a { color: red; }");
	const auto second = replace_block(*first, ".b { color: red; }");
	CHECK(second.has_value());
	CHECK(count(*second, std::string(BLOCK_BEGIN)) == 1);
	CHECK(count(*second, std::string(BLOCK_END)) == 1);
	CHECK(second->find(".a {") == std::string::npos);
	CHECK(second->rfind(DEFAULT_CSS, 0) == 0);
}

void test_replace_refuses_broken_markers()
{
	const std::string begin(BLOCK_BEGIN);
	const std::string end(BLOCK_END);
	const std::string cases[] = {
		DEFAULT_CSS + "\n" + begin + "\n.x {}\n",
		DEFAULT_CSS + "\n.x {}\n" + end + "\n",
		DEFAULT_CSS + "\n" + end + "\n.x {}\n" + begin + "\n",
		DEFAULT_CSS + "\n" + begin + "\n" + end + "\n" + begin + "\n" + end + "\n",
	};
	for (const auto &css : cases) {
		CHECK(inspect_block(css) == BlockState::Broken);
		CHECK(!replace_block(css, ".y {}").has_value());
	}
}

void test_undo_decision()
{
	CHECK(decide_undo(false, "a", "a") == UndoDecision::Nothing);
	CHECK(decide_undo(true, "applied", "applied") == UndoDecision::Restore);
	CHECK(decide_undo(true, "applied + edit", "applied") == UndoDecision::RestoreWithConfirm);
}

void test_detect_target()
{
	CHECK(detect_target("https://www.youtube.com/live_chat?is_popout=1&v=abcdefghijk") == ChatTarget::YouTube);
	CHECK(detect_target("https://studio.youtube.com/live_chat?is_popout=1&v=abcdefghijk") == ChatTarget::YouTube);
	CHECK(detect_target("https://www.youtube.com/live_chat_replay?v=abcdefghijk") == ChatTarget::YouTube);
	CHECK(detect_target("HTTPS://WWW.YOUTUBE.COM/LIVE_CHAT?v=abcdefghijk") == ChatTarget::YouTube);
	CHECK(detect_target("https://www.twitch.tv/popout/somechannel/chat?popout=") == ChatTarget::Twitch);
	CHECK(detect_target("https://www.twitch.tv/embed/somechannel/chat?parent=localhost") == ChatTarget::Twitch);
	CHECK(detect_target("https://www.twitch.tv/popout/chatterbox/viewercard/x") == ChatTarget::Unknown);
	CHECK(detect_target("https://www.youtube.com/watch?v=abcdefghijk") == ChatTarget::Unknown);
	CHECK(detect_target("https://example.com/live_chat") == ChatTarget::Unknown);
	CHECK(detect_target("https://obsproject.com/browser-source") == ChatTarget::Unknown);
	CHECK(detect_target("") == ChatTarget::Unknown);
}

void test_target_string_round_trip()
{
	CHECK(target_from_string(target_to_string(ChatTarget::YouTube)) == ChatTarget::YouTube);
	CHECK(target_from_string(target_to_string(ChatTarget::Twitch)) == ChatTarget::Twitch);
	CHECK(target_from_string("") == ChatTarget::Unknown);
	CHECK(target_from_string("other") == ChatTarget::Unknown);
}

void test_youtube_popout_url()
{
	const std::string expected = "https://www.youtube.com/live_chat?is_popout=1&v=abc-_DEF123";
	CHECK(youtube_popout_url("abc-_DEF123") == expected);
	CHECK(youtube_popout_url("  abc-_DEF123 ") == expected);
	CHECK(youtube_popout_url("https://www.youtube.com/watch?v=abc-_DEF123&t=10") == expected);
	CHECK(youtube_popout_url("https://youtu.be/abc-_DEF123?si=x") == expected);
	CHECK(youtube_popout_url("https://www.youtube.com/live/abc-_DEF123") == expected);
	CHECK(youtube_popout_url("youtube.com/watch?v=abc-_DEF123") == expected);
	CHECK(!youtube_popout_url("short").has_value());
	CHECK(!youtube_popout_url("https://example.com/watch?v=abc-_DEF123").has_value());
	CHECK(!youtube_popout_url("abc def ghij").has_value());
}

void test_twitch_popout_url()
{
	const std::string expected = "https://www.twitch.tv/popout/some_channel/chat?popout=";
	CHECK(twitch_popout_url("some_channel") == expected);
	CHECK(twitch_popout_url("Some_Channel") == expected);
	CHECK(twitch_popout_url("https://www.twitch.tv/some_channel") == expected);
	CHECK(twitch_popout_url("https://www.twitch.tv/popout/some_channel/chat?popout=") == expected);
	CHECK(!twitch_popout_url("bad name").has_value());
	CHECK(!twitch_popout_url("https://example.com/some_channel").has_value());
	CHECK(!twitch_popout_url("").has_value());
}

void test_hex_color()
{
	CHECK((parse_hex_color("#1a2B3c") == Rgb{0x1a, 0x2b, 0x3c}));
	CHECK((to_hex_color(Rgb{0x1a, 0x2b, 0x3c}) == "#1a2b3c"));
	CHECK(!parse_hex_color("1a2b3c").has_value());
	CHECK(!parse_hex_color("#1a2b3").has_value());
	CHECK(!parse_hex_color("#1a2b3g").has_value());
	CHECK(!parse_hex_color("red").has_value());
	CHECK(!parse_hex_color("#000000; } @import url(x) {").has_value());
}

bool contains(const std::string &text, const std::string &needle)
{
	return text.find(needle) != std::string::npos;
}

void test_youtube_css_empty()
{
	CHECK(generate_youtube_css(StyleValues{}).empty());
}

void test_youtube_css_first_items()
{
	// 最初の版から使っている 4 項目のキー。保存済みの設定を読めるよう、名前を変えない
	StyleValues values;
	values.numbers["message_font_size"] = 24;
	values.colors["message_color"] = Rgb{255, 255, 0};
	values.colors["background_color"] = Rgb{0, 0, 0};
	values.numbers["background_opacity"] = 50;
	const std::string css = generate_youtube_css(values);
	CHECK(contains(css, "font-size: 24px !important;"));
	CHECK(contains(css, "line-height: 1.43 !important;"));
	CHECK(contains(css, "color: rgb(255, 255, 0) !important;"));
	CHECK(contains(css, "background-color: rgba(0, 0, 0, 0.5) !important;"));
}

void test_youtube_css_clamps_and_alpha()
{
	StyleValues values;
	values.numbers["message_font_size"] = 1000;
	values.colors["background_color"] = Rgb{1, 2, 3};
	const int percents[] = {0, 5, 35, 99, 100, 150, -5};
	const char *expected[] = {"0)", "0.05)", "0.35)", "0.99)", "1)", "1)", "0)"};
	for (size_t i = 0; i < 7; ++i) {
		values.numbers["background_opacity"] = percents[i];
		const std::string css = generate_youtube_css(values);
		CHECK(contains(css, "rgba(1, 2, 3, " + std::string(expected[i])));
		CHECK(contains(css, "font-size: 96px"));
	}
}

void test_youtube_css_line_height_overrides_ratio()
{
	StyleValues values;
	values.numbers["message_font_size"] = 30;
	values.numbers["message_line_height"] = 140;
	const std::string css = generate_youtube_css(values);
	CHECK(contains(css, "line-height: 1.4 !important;"));
	CHECK(!contains(css, "line-height: 1.43"));
}

void test_youtube_css_roles()
{
	StyleValues values;
	values.colors["name_color"] = Rgb{1, 1, 1};
	values.colors["owner_name_color"] = Rgb{2, 2, 2};
	values.colors["member_background_color"] = Rgb{3, 3, 3};
	values.colors["owner_background_color"] = Rgb{4, 4, 4};
	values.texts["message_background_style"] = "bar";
	const std::string css = generate_youtube_css(values);
	CHECK(contains(css, "#author-name:not(.owner):not(.moderator):not(.member) {\n  color: rgb(1, 1, 1)"));
	CHECK(contains(css, "#author-name.owner {\n  color: rgb(2, 2, 2)"));
	CHECK(contains(css, "yt-live-chat-text-message-renderer:has(#author-name.member) {\n  box-shadow:"));
	CHECK(contains(css, "yt-live-chat-text-message-renderer[author-type=\"owner\"] {\n  box-shadow:"));
	// 種別ごとの色は、全体の色より後に書いて優先させる
	values.colors["message_background_color"] = Rgb{5, 5, 5};
	const std::string ordered = generate_youtube_css(values);
	CHECK(ordered.find("rgba(5, 5, 5") < ordered.find("rgba(3, 3, 3"));
}

void test_youtube_css_unified_font()
{
	// 書体は 1 つに統一し、メッセージの中のすべての要素に当てる
	StyleValues values;
	values.texts["font"] = "Yu Gothic";
	const std::string css = generate_youtube_css(values);
	CHECK(contains(css, "yt-live-chat-text-message-renderer *"));
	CHECK(contains(css, "yt-live-chat-paid-message-renderer *"));
	CHECK(contains(css, "font-family: \"Yu Gothic\" !important;"));
}

void test_item_effective()
{
	const auto &sections = youtube_sections();
	const ItemDef *outline_width = find_item(sections, "outline_width");
	const ItemDef *timestamp_size = find_item(sections, "timestamp_font_size");
	const ItemDef *message_opacity = find_item(sections, "message_background_opacity");
	const ItemDef *fade_in_duration = find_item(sections, "fade_in_duration");
	const ItemDef *message_size = find_item(sections, "message_font_size");
	StyleValues values;
	CHECK(!item_effective(*outline_width, sections, values));
	CHECK(!item_effective(*timestamp_size, sections, values));
	CHECK(!item_effective(*message_opacity, sections, values));
	CHECK(!item_effective(*fade_in_duration, sections, values));
	CHECK(item_effective(*message_size, sections, values));
	values.numbers["outline_enabled"] = 1;
	values.texts["timestamp_display"] = "show";
	values.colors["member_background_color"] = Rgb{1, 1, 1};
	values.texts["slide_in"] = "left";
	CHECK(item_effective(*outline_width, sections, values));
	CHECK(item_effective(*timestamp_size, sections, values));
	CHECK(item_effective(*message_opacity, sections, values));
	CHECK(item_effective(*fade_in_duration, sections, values));
}

void test_youtube_css_hide_input_and_engagement()
{
	StyleValues values;
	const std::string defaults = generate_youtube_css(values);
	CHECK(!contains(defaults, "#input-panel {"));
	CHECK(!contains(defaults, "viewer-engagement"));
	values.numbers["input_panel_visible"] = 0;
	values.numbers["engagement_visible"] = 0;
	const std::string css = generate_youtube_css(values);
	CHECK(contains(css, "yt-live-chat-renderer #input-panel {\n  display: none !important;"));
	CHECK(contains(css, "yt-live-chat-viewer-engagement-message-renderer {\n  display: none !important;"));
	// 商品の紹介は隠さない
	CHECK(!contains(css, "product-picker"));
}

void test_youtube_css_opacity_without_color()
{
	// 色を選ばずに不透明度だけを 0 にすると、YouTube の既定の色で透明にする
	StyleValues values;
	values.numbers["background_opacity"] = 0;
	CHECK(contains(generate_youtube_css(values), "background-color: rgba(15, 15, 15, 0) !important;"));
}

void test_youtube_css_owner_highlight()
{
	StyleValues values;
	CHECK(!contains(generate_youtube_css(values), "#author-name.owner"));
	values.numbers["owner_name_highlight"] = 0;
	const std::string css = generate_youtube_css(values);
	CHECK(contains(css,
		       "#author-name.owner {\n  background-color: transparent !important;\n  color: rgb(255, 214, 0)"));
	values.colors["owner_name_color"] = Rgb{1, 2, 3};
	const std::string colored = generate_youtube_css(values);
	CHECK(contains(colored, "color: rgb(1, 2, 3)"));
	CHECK(!contains(colored, "rgb(255, 214, 0)"));
}

void test_youtube_css_toggles_and_animation()
{
	StyleValues values;
	CHECK(!contains(generate_youtube_css(values), "yt-live-chat-header-renderer {\n  display: none"));
	values.numbers["header_visible"] = 0;
	values.numbers["remove_old"] = 1;
	values.numbers["remove_after"] = 10;
	const std::string css = generate_youtube_css(values);
	CHECK(contains(css, "yt-live-chat-header-renderer {\n  display: none !important;"));
	CHECK(contains(css, "@keyframes cskin-out"));
	CHECK(contains(css, "cskin-out 200ms ease-in 10s forwards"));
	// 消すのはメッセージの要素だけにし、YouTube の案内などの要素を含む #items の子すべては対象にしない
	CHECK(!contains(css, "#items"));
}

void test_youtube_css_never_loads_resources()
{
	// すべての項目に値を入れても、外部の資源を読み込む記述を生成しない
	StyleValues values;
	for (const auto &section : youtube_sections()) {
		for (const auto &item : section.items) {
			const std::string key(item.key);
			switch (item.kind) {
			case ItemKind::Number:
			case ItemKind::Opacity:
				values.numbers[key] = item.max;
				break;
			case ItemKind::Toggle:
				values.numbers[key] = item.default_value.value_or(0) == 0 ? 1 : 0;
				break;
			case ItemKind::Color:
				values.colors[key] = Rgb{10, 20, 30};
				break;
			case ItemKind::Choice:
				values.texts[key] = std::string(item.choices.back());
				break;
			case ItemKind::Font:
				values.texts[key] = "Evil\"; } @import url(http://example.com/x.css); a {\"";
				break;
			}
		}
	}
	const std::string css = generate_youtube_css(values);
	CHECK(!css.empty());
	CHECK(!contains(css, "@import"));
	CHECK(!contains(css, "url("));
}

void test_sanitize()
{
	StyleValues values;
	values.numbers["message_font_size"] = 1;
	values.numbers["unknown_key"] = 5;
	values.texts["avatar_shape"] = "triangle";
	values.texts["slide_in"] = "left";
	values.colors["message_font_size"] = Rgb{1, 2, 3};
	const StyleValues clean = sanitize(youtube_sections(), values);
	CHECK(clean.numbers.at("message_font_size") == 8);
	CHECK(clean.numbers.count("unknown_key") == 0);
	CHECK(clean.texts.count("avatar_shape") == 0);
	CHECK(clean.texts.at("slide_in") == "left");
	CHECK(clean.colors.empty());
}

void test_sanitize_font_name()
{
	CHECK(sanitize_font_name("Yu Gothic UI") == "Yu Gothic UI");
	CHECK(sanitize_font_name("  游ゴシック  ") == "游ゴシック");
	CHECK(sanitize_font_name("a\"; } @import url(x) {") == "a  import urlx");
	CHECK(sanitize_font_name("();{}").empty());
}

void test_css_string()
{
	CHECK(css_string("Yu Gothic") == "\"Yu Gothic\"");
	CHECK(css_string("a\"b\\c\nd") == "\"a\\\"b\\\\cd\"");
}

void test_js_string_literal()
{
	CHECK(js_string_literal("a { color: red; }") == "\"a { color: red; }\"");
	CHECK(js_string_literal("q\"b\\") == "\"q\\\"b\\\\\"");
	CHECK(js_string_literal("x\ny\r\tz") == "\"x\\ny\\r\\tz\"");
	CHECK(js_string_literal("</style><script>") == "\"\\u003c/style\\u003e\\u003cscript\\u003e\"");
	CHECK(js_string_literal(std::string("\x01", 1)) == "\"\\u0001\"");
	CHECK(js_string_literal("\xe2\x80\xa8\xe2\x80\xa9") == "\"\\u2028\\u2029\"");
	CHECK(js_string_literal("日本語") == "\"日本語\"");
}

void test_preview_style_script()
{
	const std::string script = preview_style_script("a{}");
	CHECK(script.find("cskin-generated") != std::string::npos);
	CHECK(script.find("textContent=\"a{}\"") != std::string::npos);
}

} // namespace

int main()
{
	test_replace_appends_when_absent();
	test_replace_on_empty_css();
	test_replace_keeps_user_css_outside_block();
	test_replace_twice_keeps_single_block();
	test_replace_refuses_broken_markers();
	test_undo_decision();
	test_detect_target();
	test_target_string_round_trip();
	test_youtube_popout_url();
	test_twitch_popout_url();
	test_hex_color();
	test_youtube_css_empty();
	test_youtube_css_first_items();
	test_youtube_css_clamps_and_alpha();
	test_youtube_css_line_height_overrides_ratio();
	test_youtube_css_roles();
	test_youtube_css_toggles_and_animation();
	test_youtube_css_unified_font();
	test_item_effective();
	test_youtube_css_opacity_without_color();
	test_youtube_css_hide_input_and_engagement();
	test_youtube_css_owner_highlight();
	test_youtube_css_never_loads_resources();
	test_sanitize();
	test_sanitize_font_name();
	test_css_string();
	test_js_string_literal();
	test_preview_style_script();

	if (failures > 0) {
		std::cerr << failures << " check(s) failed\n";
		return 1;
	}
	std::cout << "all checks passed\n";
	return 0;
}
