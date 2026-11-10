// 見た目の崩れの検査 (チャットのページ)
// 使い方: node page-check.mjs <webSocketDebuggerUrl> <CSS のファイル>
// CDP で CSS を style 要素として注入し、メッセージの文字の要素の位置と大きさを調べて JSON で書き出す。
// 崩れが 1 件でもあれば終了コード 1 で終わる。注入した style 要素は最後に取り除く
import { readFileSync } from 'node:fs';

const [wsUrl, cssFile] = process.argv.slice(2);
if (!wsUrl || !cssFile) {
	console.error('usage: node page-check.mjs <webSocketDebuggerUrl> <css file>');
	process.exit(2);
}
const css = readFileSync(cssFile, 'utf8');

const pageScript = (cssText) => `(async () => {
	const style = document.createElement('style');
	style.id = 'cskin-layout-check';
	style.textContent = ${JSON.stringify(cssText)};
	document.head.appendChild(style);
	await new Promise((r) => requestAnimationFrame(() => requestAnimationFrame(r)));

	const HOSTS = ['yt-live-chat-text-message-renderer', 'yt-live-chat-paid-message-renderer',
		'yt-live-chat-membership-item-renderer', 'yt-live-chat-paid-sticker-renderer'];
	const TEXT_PARTS = ['#message', '#author-name', '#timestamp', '#purchase-amount', '#header-subtext'];
	const visible = (e) => { const r = e.getBoundingClientRect(); return r.width > 0 && r.height > 0 && getComputedStyle(e).display !== 'none'; };
	const px = (v) => parseFloat(v);
	const issues = [];
	const add = (kind, host, part, detail) => issues.push({ kind, host: host.tagName.toLowerCase(), part, detail });
	let checked = 0;

	for (const hostName of HOSTS) {
		for (const host of document.querySelectorAll(hostName)) {
			if (!visible(host)) continue;
			for (const part of TEXT_PARTS) {
				for (const el of host.querySelectorAll(part)) {
					if (!visible(el)) continue;
					checked++;
					const s = getComputedStyle(el);
					const fontSize = px(s.fontSize);
					if (s.lineHeight !== 'normal' && px(s.lineHeight) < fontSize)
						add('line-height-smaller-than-font', host, part, s.lineHeight + ' < ' + s.fontSize);
					// overflow で切り取る祖先 (メッセージの要素まで) から、文字の要素がはみ出していないか
					const r = el.getBoundingClientRect();
					for (let a = el.parentElement; a && a !== host.parentElement; a = a.parentElement) {
						const as = getComputedStyle(a);
						if (as.overflowX === 'visible' && as.overflowY === 'visible') continue;
						const ar = a.getBoundingClientRect();
						if (r.top < ar.top - 1 || r.bottom > ar.bottom + 1 || r.left < ar.left - 1 || r.right > ar.right + 1)
							add('clipped-by-ancestor', host, part, a.tagName.toLowerCase() + (a.id ? '#' + a.id : ''));
					}
					// 1 行の文字の高さに対して、要素の高さが足りないか
					if (r.height + 1 < fontSize)
						add('shorter-than-font', host, part, r.height.toFixed(1) + 'px < ' + s.fontSize);
				}
			}
			// アイコンと本文の重なり
			const photo = host.querySelector('#author-photo');
			const content = host.querySelector(':scope > #content, #header-content');
			if (photo && content && visible(photo) && visible(content)) {
				const p = photo.getBoundingClientRect();
				const c = content.getBoundingClientRect();
				const w = Math.min(p.right, c.right) - Math.max(p.left, c.left);
				const h = Math.min(p.bottom, c.bottom) - Math.max(p.top, c.top);
				if (w > 1 && h > 1) add('avatar-overlaps-text', host, '#author-photo', w.toFixed(1) + 'x' + h.toFixed(1));
			}
		}
	}
	const doc = document.documentElement;
	if (doc.scrollWidth > doc.clientWidth + 1)
		issues.push({ kind: 'horizontal-overflow', detail: doc.scrollWidth + ' > ' + doc.clientWidth });
	style.remove();

	const summary = {};
	for (const i of issues) summary[i.kind + ' ' + (i.host ?? '') + ' ' + (i.part ?? '')] = (summary[i.kind + ' ' + (i.host ?? '') + ' ' + (i.part ?? '')] ?? 0) + 1;
	return { url: location.href.slice(0, 80), checked, issueCount: issues.length, summary, examples: issues.slice(0, 5) };
})()`;

const ws = new WebSocket(wsUrl);
ws.onopen = () =>
	ws.send(JSON.stringify({ id: 1, method: 'Runtime.evaluate', params: { expression: pageScript(css), returnByValue: true, awaitPromise: true } }));
ws.onmessage = (ev) => {
	const msg = JSON.parse(ev.data);
	if (msg.id !== 1) return;
	ws.close();
	const value = msg.result?.result?.value;
	if (!value) {
		console.log(JSON.stringify(msg.result ?? msg.error, null, 2));
		process.exit(2);
	}
	console.log(JSON.stringify(value, null, 2));
	process.exit(value.issueCount > 0 || value.checked === 0 ? 1 : 0);
};
ws.onerror = (e) => {
	console.error('websocket error', e.message ?? e);
	process.exit(2);
};
