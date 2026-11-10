// 設定の漏れの検査 (背景)
// 使い方: node opaque-scan.mjs <webSocketDebuggerUrl> <CSS のファイル>
// CSS を注入した後に、チャット欄の中で背景が透明でない要素と、その背景色を一覧にする。
// 全体の背景を透明にする設定を当てたときに、まだ塗られて残る要素を見つけるために使う
import { readFileSync } from 'node:fs';

const [wsUrl, cssFile] = process.argv.slice(2);
const css = readFileSync(cssFile, 'utf8');
const expression = `(async () => {
	const style = document.createElement('style');
	style.textContent = ${JSON.stringify(css)};
	document.head.appendChild(style);
	await new Promise((r) => requestAnimationFrame(() => requestAnimationFrame(r)));
	const root = document.querySelector('yt-live-chat-renderer') ?? document.body;
	const found = {};
	for (const e of [root, ...root.querySelectorAll('*')]) {
		const r = e.getBoundingClientRect();
		if (r.width < 1 || r.height < 1) continue;
		const s = getComputedStyle(e);
		if (s.display === 'none' || s.visibility === 'hidden') continue;
		// ボタンを押したときに光る層のように、祖先を含めた不透明度が 0 の要素は見えないため数えない
		let opacity = 1;
		for (let a = e; a && opacity > 0; a = a.parentElement) opacity *= parseFloat(getComputedStyle(a).opacity);
		if (opacity === 0) continue;
		const bg = s.backgroundColor;
		const transparent = bg === 'transparent' || /rgba\\(.*,\\s*0\\)$/.test(bg);
		if (transparent && s.backgroundImage === 'none') continue;
		const key = e.tagName.toLowerCase() + (e.id ? '#' + e.id : '') + ' ' + bg + (s.backgroundImage !== 'none' ? ' image' : '');
		found[key] = (found[key] ?? 0) + 1;
	}
	style.remove();
	return found;
})()`;
const ws = new WebSocket(wsUrl);
ws.onopen = () => ws.send(JSON.stringify({ id: 1, method: 'Runtime.evaluate', params: { expression, returnByValue: true, awaitPromise: true } }));
ws.onmessage = (ev) => {
	const m = JSON.parse(ev.data);
	if (m.id !== 1) return;
	console.log(JSON.stringify(m.result?.result?.value ?? m.result, null, 1));
	ws.close();
	process.exit(0);
};
