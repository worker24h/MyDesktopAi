#!/usr/bin/env python3
"""字体素材准备（由 make_assets.sh 调用）

输出到 tools/build/：
  chars_ui.txt      界面用到的字：扫描 src/ui、src/app 下的源码 + tools/chars_extra.txt
  chars_common.txt  GB2312 一级字库（3755 个常用字），给对话用的 14px 字体
  icon_ranges.txt   图标编码，逗号分隔，直接传给 lv_font_conv -r
输出到 src/assets/fonts/：
  mda_fonts.h       LV_FONT_DECLARE + ICON_xxx 宏
用法：python3 tools/make_fonts.py <lucide info.json>
"""
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BUILD = ROOT / "tools/build"
FONT_OUT = ROOT / "src/assets/fonts"
SCAN_DIRS = [ROOT / "src/ui", ROOT / "src/app"]
FONT_NAMES = ["font_cn_11", "font_cn_14", "font_cn_18"]

# 需要导出的非 ASCII 字符：中文、全角标点、常用符号（° · … — “” ‘’ ♪ ♫ ×）
CJK_RE = re.compile(r"[°·×‐-‧♪♫"
                    r"　-〿一-鿿＀-￯]")


def collect_ui_chars() -> str:
    text = (ROOT / "tools/chars_extra.txt").read_text(encoding="utf-8")
    for d in SCAN_DIRS:
        if not d.exists():
            continue
        for f in d.rglob("*"):
            if f.suffix in (".c", ".cpp", ".h", ".hpp"):
                text += f.read_text(encoding="utf-8", errors="ignore")
    return "".join(sorted(set(CJK_RE.findall(text))))


def gb2312_level1() -> str:
    """GB2312 一级汉字：区 0xB0–0xD7，位 0xA1–0xFE（0xD7FA 之后为空位）"""
    out = []
    for hi in range(0xB0, 0xD8):
        for lo in range(0xA1, 0xFF):
            try:
                out.append(bytes([hi, lo]).decode("gb2312"))
            except UnicodeDecodeError:
                pass
    return "".join(out)


def load_icons(info_path: Path):
    info = json.loads(info_path.read_text(encoding="utf-8"))
    icons = []
    for line in (ROOT / "tools/icons.txt").read_text(encoding="utf-8").splitlines():
        line = line.split("#", 1)[0].strip()
        if not line:
            continue
        macro, name = line.split()
        if name not in info:
            sys.exit(f"tools/icons.txt：Lucide 里没有图标「{name}」，去 https://lucide.dev/icons 核对名字")
        code = int(info[name]["encodedCode"].lstrip("\\"), 16)
        icons.append((macro, name, code))
    return icons


def write_header(icons):
    lines = [
        "// 由 tools/make_assets.sh 自动生成，不要手动修改",
        "#pragma once",
        "#include \"lvgl.h\"",
        "",
    ]
    lines += [f"LV_FONT_DECLARE({n});" for n in FONT_NAMES]
    lines.append("")
    lines.append("// Lucide 图标（UTF-8 字符串），三个字号都包含，用法：lv_label_set_text(lbl, ICON_HOME);")
    for macro, name, code in icons:
        utf8 = "".join(f"\\x{b:02X}" for b in chr(code).encode("utf-8"))
        lines.append(f"#define ICON_{macro:<10} \"{utf8}\"   // {name} U+{code:04X}")
    FONT_OUT.mkdir(parents=True, exist_ok=True)
    (FONT_OUT / "mda_fonts.h").write_text("\n".join(lines) + "\n", encoding="utf-8")


def main():
    if len(sys.argv) != 2:
        sys.exit("用法：python3 tools/make_fonts.py <lucide info.json>")
    BUILD.mkdir(parents=True, exist_ok=True)

    ui = collect_ui_chars()
    common = gb2312_level1()
    icons = load_icons(Path(sys.argv[1]))

    (BUILD / "chars_ui.txt").write_text(ui, encoding="utf-8")
    (BUILD / "chars_common.txt").write_text(common, encoding="utf-8")
    (BUILD / "icon_ranges.txt").write_text(",".join(f"0x{c:04X}" for _, _, c in icons), encoding="utf-8")
    write_header(icons)
    print(f"界面字符 {len(ui)} 个，常用字 {len(common)} 个，图标 {len(icons)} 个")


if __name__ == "__main__":
    main()
