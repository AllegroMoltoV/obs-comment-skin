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

#include "preview-script.h"

namespace cskin::core {

namespace {

void append_unicode_escape(std::string &out, unsigned value)
{
	static constexpr char DIGITS[] = "0123456789abcdef";
	out += "\\u";
	for (int shift = 12; shift >= 0; shift -= 4)
		out += DIGITS[(value >> shift) & 0x0f];
}

} // namespace

std::string js_string_literal(std::string_view text)
{
	std::string out = "\"";
	for (size_t i = 0; i < text.size(); ++i) {
		const unsigned char c = static_cast<unsigned char>(text[i]);
		// U+2028 と U+2029 は UTF-8 で E2 80 A8 と E2 80 A9
		if (c == 0xe2 && i + 2 < text.size() && static_cast<unsigned char>(text[i + 1]) == 0x80 &&
		    (static_cast<unsigned char>(text[i + 2]) == 0xa8 ||
		     static_cast<unsigned char>(text[i + 2]) == 0xa9)) {
			append_unicode_escape(out, static_cast<unsigned char>(text[i + 2]) == 0xa8 ? 0x2028 : 0x2029);
			i += 2;
			continue;
		}
		switch (c) {
		case '"':
			out += "\\\"";
			break;
		case '\\':
			out += "\\\\";
			break;
		case '\n':
			out += "\\n";
			break;
		case '\r':
			out += "\\r";
			break;
		case '\t':
			out += "\\t";
			break;
		case '<':
		case '>':
			append_unicode_escape(out, c);
			break;
		default:
			if (c < 0x20)
				append_unicode_escape(out, c);
			else
				out += static_cast<char>(c);
		}
	}
	out += '"';
	return out;
}

std::string preview_style_script(std::string_view css)
{
	// 見本のメッセージを入れ直し、現れる・消えるアニメーションを最初から見られるようにする。
	// 見本は新しいメッセージを下に詰めて並べるため、上端で一部しか見えないメッセージは隠す。
	// 位置はアニメーションの変形の影響を受けない offsetTop で比べ、見本の大きさが変わったときも判定し直す
	return "(function(){var s=document.getElementById(\"cskin-generated\");if(s)s.textContent=" +
	       js_string_literal(css) +
	       ";var i=document.getElementById(\"items\");if(!i)return;"
	       "i.replaceChildren.apply(i,Array.from(i.children));"
	       "var fit=function(){var c=document.getElementById(\"item-scroller\");if(!c)return;"
	       "var top=c.offsetTop;Array.from(i.children).forEach(function(e){"
	       "e.style.visibility=\"\";});Array.from(i.children).forEach(function(e){"
	       "if(e.offsetTop<top-1)e.style.visibility=\"hidden\";});};"
	       "requestAnimationFrame(function(){requestAnimationFrame(fit);});"
	       "if(!window.cskinFitBound){window.cskinFitBound=true;window.addEventListener(\"resize\",function(){"
	       "requestAnimationFrame(window.cskinFit);});}window.cskinFit=fit;})();";
}

} // namespace cskin::core
