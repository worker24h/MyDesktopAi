#!/usr/bin/env bash
# 一键生成 UI 素材：设计稿 → PNG → LVGL 图片 .c
# 用法：在工程根目录运行  bash tools/make_assets.sh
set -euo pipefail
cd "$(dirname "$0")/.."

LVGL=.pio/libdeps/m5stack-cores3/lvgl
VENV=tools/.venv
OUT=src/assets/images

if [ ! -f "$LVGL/scripts/LVGLImage.py" ]; then
  echo "找不到 $LVGL，先运行一次 pio run 下载依赖" >&2; exit 1
fi

# 1. Python 虚拟环境（只在第一次创建）
if [ ! -x "$VENV/bin/python" ]; then
  python3 -m venv "$VENV"
  "$VENV/bin/pip" -q install pypng lz4
fi

# 2. 设计稿 → PNG
python3 tools/export_png.py

# 3. PNG → LVGL 图片 .c（先清空旧文件，避免残留已删除的素材）
mkdir -p "$OUT"
rm -f "$OUT"/*.c
"$VENV/bin/python" "$LVGL/scripts/LVGLImage.py" --ofmt C --cf RGB565A8 -o "$OUT" tools/assets/png/robot
"$VENV/bin/python" "$LVGL/scripts/LVGLImage.py" --ofmt C --cf RGB565   -o "$OUT" tools/assets/png/bg_main.png

echo "完成：$(ls "$OUT"/*.c | wc -l | tr -d ' ') 个 .c 文件 → $OUT/"
