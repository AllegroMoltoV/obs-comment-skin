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

#include "chat-target.h"

#include <algorithm>
#include <cctype>
#include <vector>

namespace cskin::core {

namespace {

struct UrlParts {
	std::string host; // 小文字
	std::string path; // 先頭の / を含む。大文字と小文字を保つ
	std::string query;
};

std::string to_lower(std::string_view text)
{
	std::string result(text);
	std::transform(result.begin(), result.end(), result.begin(),
		       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	return result;
}

std::string_view trim(std::string_view text)
{
	while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front())))
		text.remove_prefix(1);
	while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back())))
		text.remove_suffix(1);
	return text;
}

bool starts_with(std::string_view text, std::string_view prefix)
{
	return text.substr(0, prefix.size()) == prefix;
}

// スキームの無い "youtube.com/..." も URL として扱う
std::optional<UrlParts> split_url(std::string_view url)
{
	url = trim(url);
	const std::string lower = to_lower(url);
	size_t start = 0;
	if (starts_with(lower, "https://"))
		start = 8;
	else if (starts_with(lower, "http://"))
		start = 7;

	const std::string_view rest = url.substr(start);
	const size_t host_end = rest.find_first_of("/?#");
	UrlParts parts;
	parts.host = to_lower(rest.substr(0, host_end));
	if (parts.host.empty() || parts.host.find('.') == std::string::npos)
		return std::nullopt;
	if (host_end == std::string_view::npos)
		return parts;

	std::string_view tail = rest.substr(host_end);
	const size_t fragment = tail.find('#');
	if (fragment != std::string_view::npos)
		tail = tail.substr(0, fragment);
	const size_t query = tail.find('?');
	parts.path = std::string(tail.substr(0, query));
	if (query != std::string_view::npos)
		parts.query = std::string(tail.substr(query + 1));
	return parts;
}

bool is_youtube_host(std::string_view host)
{
	return host == "youtube.com" || host == "www.youtube.com" || host == "m.youtube.com" ||
	       host == "studio.youtube.com";
}

bool is_twitch_host(std::string_view host)
{
	return host == "twitch.tv" || host == "www.twitch.tv" || host == "m.twitch.tv";
}

std::string query_value(std::string_view query, std::string_view key)
{
	while (!query.empty()) {
		const size_t amp = query.find('&');
		const std::string_view pair = query.substr(0, amp);
		const size_t eq = pair.find('=');
		if (pair.substr(0, eq) == key && eq != std::string_view::npos)
			return std::string(pair.substr(eq + 1));
		if (amp == std::string_view::npos)
			break;
		query.remove_prefix(amp + 1);
	}
	return {};
}

// パスを / で区切った要素。空の要素は除く
std::vector<std::string> path_segments(std::string_view path)
{
	std::vector<std::string> segments;
	size_t pos = 0;
	while (pos < path.size()) {
		const size_t end = path.find('/', pos);
		const std::string_view segment =
			path.substr(pos, end == std::string_view::npos ? std::string_view::npos : end - pos);
		if (!segment.empty())
			segments.emplace_back(segment);
		if (end == std::string_view::npos)
			break;
		pos = end + 1;
	}
	return segments;
}

std::string segment_at(const std::vector<std::string> &segments, size_t index)
{
	return index < segments.size() ? segments[index] : std::string();
}

bool is_video_id(std::string_view text)
{
	return text.size() == 11 && std::all_of(text.begin(), text.end(), [](unsigned char c) {
		       return std::isalnum(c) || c == '-' || c == '_';
	       });
}

bool is_twitch_login(std::string_view text)
{
	return !text.empty() && text.size() <= 25 &&
	       std::all_of(text.begin(), text.end(), [](unsigned char c) { return std::isalnum(c) || c == '_'; });
}

} // namespace

ChatTarget detect_target(std::string_view url)
{
	const auto parts = split_url(url);
	if (!parts)
		return ChatTarget::Unknown;

	const std::string path = to_lower(parts->path);
	if (is_youtube_host(parts->host) && starts_with(path, "/live_chat"))
		return ChatTarget::YouTube;

	if (is_twitch_host(parts->host)) {
		// /popout/<チャンネル名>/chat と /embed/<チャンネル名>/chat
		const auto segments = path_segments(path);
		const std::string first = segment_at(segments, 0);
		if ((first == "popout" || first == "embed") && segments.size() >= 3 && segments[2] == "chat")
			return ChatTarget::Twitch;
	}
	return ChatTarget::Unknown;
}

std::string_view target_to_string(ChatTarget target)
{
	switch (target) {
	case ChatTarget::YouTube:
		return "youtube";
	case ChatTarget::Twitch:
		return "twitch";
	case ChatTarget::Unknown:
		break;
	}
	return "";
}

ChatTarget target_from_string(std::string_view text)
{
	if (text == "youtube")
		return ChatTarget::YouTube;
	if (text == "twitch")
		return ChatTarget::Twitch;
	return ChatTarget::Unknown;
}

std::optional<std::string> youtube_popout_url(std::string_view input)
{
	input = trim(input);
	std::string id;
	if (is_video_id(input)) {
		id = std::string(input);
	} else if (const auto parts = split_url(input)) {
		const auto segments = path_segments(parts->path);
		if (parts->host == "youtu.be") {
			id = segment_at(segments, 0);
		} else if (is_youtube_host(parts->host)) {
			const std::string first = segment_at(segments, 0);
			if (first == "live" || first == "shorts")
				id = segment_at(segments, 1);
			else
				id = query_value(parts->query, "v");
		}
	}
	if (!is_video_id(id))
		return std::nullopt;
	return "https://www.youtube.com/live_chat?is_popout=1&v=" + id;
}

std::optional<std::string> twitch_popout_url(std::string_view input)
{
	input = trim(input);
	std::string login;
	if (is_twitch_login(input)) {
		login = std::string(input);
	} else if (const auto parts = split_url(input)) {
		if (is_twitch_host(parts->host)) {
			const auto segments = path_segments(parts->path);
			const std::string first = segment_at(segments, 0);
			login = (first == "popout" || first == "embed") ? segment_at(segments, 1) : first;
		}
	}
	if (!is_twitch_login(login))
		return std::nullopt;
	return "https://www.twitch.tv/popout/" + to_lower(login) + "/chat?popout=";
}

} // namespace cskin::core
