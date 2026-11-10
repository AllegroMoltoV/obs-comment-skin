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

void test_youtube_css_empty()
{
	CHECK(generate_youtube_css(YouTubeStyle{}).empty());
}

void test_youtube_css_values()
{
	YouTubeStyle style;
	style.message_font_size = 24;
	style.message_color = Rgb{255, 255, 0};
	style.background_color = Rgb{0, 0, 0};
	style.background_opacity = 50;
	const std::string css = generate_youtube_css(style);
	CHECK(css.find("font-size: 24px !important;") != std::string::npos);
	CHECK(css.find("color: rgb(255, 255, 0) !important;") != std::string::npos);
	CHECK(css.find("background-color: rgba(0, 0, 0, 0.5) !important;") != std::string::npos);
	CHECK(css.find("@import") == std::string::npos);
	CHECK(css.find("url(") == std::string::npos);
}

void test_youtube_css_clamps_and_alpha()
{
	YouTubeStyle style;
	style.message_font_size = 1000;
	style.background_color = Rgb{1, 2, 3};
	const int percents[] = {0, 5, 35, 99, 100, 150, -5};
	const char *expected[] = {"0)", "0.05)", "0.35)", "0.99)", "1)", "1)", "0)"};
	for (size_t i = 0; i < 7; ++i) {
		style.background_opacity = percents[i];
		const std::string css = generate_youtube_css(style);
		CHECK(css.find("rgba(1, 2, 3, " + std::string(expected[i])) != std::string::npos);
		CHECK(css.find("font-size: " + std::to_string(FONT_SIZE_MAX) + "px") != std::string::npos);
	}
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
	test_youtube_css_values();
	test_youtube_css_clamps_and_alpha();

	if (failures > 0) {
		std::cerr << failures << " check(s) failed\n";
		return 1;
	}
	std::cout << "all checks passed\n";
	return 0;
}
