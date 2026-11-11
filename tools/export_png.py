#!/usr/bin/env python3
"""
从设计稿 docs/design/cores3-ui.html 导出 UI 素材 PNG（用无界面 Chrome 渲染）。

输出：
  tools/assets/png/robot/robot_<表情>[_blink]_<L|XL|S>.png   机器人（透明背景）
  tools/assets/png/bg_main.png                                 320x240 背景

用法：python3 tools/export_png.py            （在工程根目录运行）
依赖：Google Chrome（macOS 默认路径，或用环境变量 CHROME 指定）
"""
import base64
import html
import json
import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DESIGN = ROOT / "docs/design/cores3-ui.html"
OUT_ROBOT = ROOT / "tools/assets/png/robot"
OUT_BG = ROOT / "tools/assets/png/bg_main.png"
CHROME = os.environ.get("CHROME", "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome")

# 尺寸：页面里用 / 待机全屏 / 表情网格+头像
SIZES = {"L": 150, "XL": 200, "S": 44}

# 在设计稿页面里运行：渲染每个机器人，结果写到 <body data-out="...">
EXPORT_JS = r"""
(async () => {
  const SIZES = __SIZES__;
  const defs = document.querySelector('svg defs').outerHTML;
  const OPEN_EYES = ['surprised','think','listen','sad','playful','confused','talk'];
  const jobs = [];
  for (const ex of Object.keys(EXPR)) {
    jobs.push({ name: ex, ex, mod: s => s });
    if (OPEN_EYES.includes(ex))
      jobs.push({ name: ex + '_blink', ex, mod: s => s.replace(/<g class="eyes">[\s\S]*?<\/g>/, `<g class="eyes">${FACE.sleep.e}</g>`) });
  }
  jobs.push({ name: 'talk_close', ex: 'talk', mod: s => s.replace(/(class="talk-mouth"[^>]*?ry=")[\d.]+/, (_, p) => p + '1.8') });

  const out = {};
  for (const { name, ex, mod } of jobs) for (const [tag, w] of Object.entries(SIZES)) {
    const h = Math.round(w * 206 / 240);
    const svg = mod(robotSVG(ex, w))
      .replace('<svg ', `<svg xmlns="http://www.w3.org/2000/svg" height="${h}" `)
      .replace(/(<svg[^>]*>)/, m => m + defs);
    const img = new Image();
    img.src = 'data:image/svg+xml;charset=utf-8,' + encodeURIComponent(svg);
    await img.decode();
    const c = Object.assign(document.createElement('canvas'), { width: w, height: h });
    c.getContext('2d').drawImage(img, 0, 0, w, h);
    out[`robot_${name}_${tag}.png`] = c.toDataURL('image/png');
  }
  document.body.setAttribute('data-out', JSON.stringify(out));
})().catch(e => document.body.setAttribute('data-out', JSON.stringify({ __error: String(e) })));
"""

# 背景：和设计稿 .lcd 的背景完全一致（渐变 + 两团白色光斑）
BG_HTML = """<!doctype html><html><head><style>
html,body{margin:0;width:320px;height:240px;overflow:hidden}
.lcd{position:relative;width:320px;height:240px;overflow:hidden;
  background:linear-gradient(160deg,#cfe0ff 0%,#e6dcff 50%,#ffe2ef 100%)}
.lcd::before{content:"";position:absolute;width:220px;height:80px;left:-50px;top:150px;border-radius:50%;
  background:radial-gradient(closest-side,rgba(255,255,255,.85),transparent)}
.lcd::after{content:"";position:absolute;width:200px;height:70px;right:-60px;top:100px;border-radius:50%;
  background:radial-gradient(closest-side,rgba(255,255,255,.7),transparent)}
</style></head><body><div class="lcd"></div></body></html>"""


def chrome(*args):
    if not Path(CHROME).exists():
        sys.exit(f"找不到 Chrome：{CHROME}（可用环境变量 CHROME 指定路径）")
    return subprocess.run([CHROME, "--headless=new", "--disable-gpu", "--hide-scrollbars", *args],
                          capture_output=True, text=True, timeout=300)


def export_robots(tmp: Path):
    page = DESIGN.read_text(encoding="utf-8")
    js = EXPORT_JS.replace("__SIZES__", json.dumps(SIZES))
    (tmp / "export.html").write_text(page.replace("</body>", f"<script>{js}</script></body>"), encoding="utf-8")
    dom = chrome("--virtual-time-budget=60000", "--dump-dom", (tmp / "export.html").as_uri()).stdout
    m = re.search(r'data-out="([^"]*)"', dom)
    if not m:
        sys.exit("导出失败：页面没有返回结果")
    out = json.loads(html.unescape(m.group(1)))
    if "__error" in out:
        sys.exit("导出失败：" + out["__error"])
    OUT_ROBOT.mkdir(parents=True, exist_ok=True)
    for name, url in out.items():
        (OUT_ROBOT / name).write_bytes(base64.b64decode(url.split(",", 1)[1]))
    print(f"机器人：{len(out)} 张 → {OUT_ROBOT.relative_to(ROOT)}/")


def export_bg(tmp: Path):
    (tmp / "bg.html").write_text(BG_HTML, encoding="utf-8")
    OUT_BG.parent.mkdir(parents=True, exist_ok=True)
    chrome("--window-size=320,240", "--force-device-scale-factor=1",
           f"--screenshot={OUT_BG}", (tmp / "bg.html").as_uri())
    if not OUT_BG.exists():
        sys.exit("背景导出失败")
    print(f"背景：{OUT_BG.relative_to(ROOT)}")


if __name__ == "__main__":
    with tempfile.TemporaryDirectory() as d:
        export_robots(Path(d))
        export_bg(Path(d))
