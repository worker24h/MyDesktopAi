#!/usr/bin/env bash
# 一键生成 UI 素材：设计稿 → PNG → LVGL 图片 .c；字体 → LVGL 字体 .c
# 用法：在工程根目录运行
#   bash tools/make_assets.sh          图片 + 字体
#   bash tools/make_assets.sh images   只生成图片
#   bash tools/make_assets.sh fonts    只生成字体（改了界面文字后跑这个就行）
set -euo pipefail
cd "$(dirname "$0")/.."

WHAT=${1:-all}
LVGL=.pio/libdeps/m5stack-cores3/lvgl
VENV=tools/.venv
OUT=src/assets/images

# ---------------------------------------------------------------- 图片
make_images() {
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

  echo "图片完成：$(ls "$OUT"/*.c | wc -l | tr -d ' ') 个 .c 文件 → $OUT/"
}

# ---------------------------------------------------------------- 字体
FONT_SRC=tools/assets/fonts            # 下载的字体文件（不进 git）
FONT_OUT=src/assets/fonts
BUILD=tools/build                      # 中间文件（不进 git）
CN_FONT=$FONT_SRC/NotoSansSC-Medium.otf
ICON_FONT=$FONT_SRC/lucide.ttf
ICON_INFO=$FONT_SRC/lucide-info.json
NOTO_URL=https://github.com/notofonts/noto-cjk/raw/Sans2.004/Sans/SubsetOTF/SC/NotoSansSC-Medium.otf
LUCIDE_URL=https://cdn.jsdelivr.net/npm/lucide-static@1.62.0/font
FONT_CONV="npx -y lv_font_conv@1.5.3"

download() {   # download <url> <文件>：文件已存在就跳过
  if [ ! -s "$2" ]; then
    echo "下载 $(basename "$2") ..."
    curl -fsSL --retry 2 -o "$2.part" "$1"
    mv "$2.part" "$2"
  fi
}

make_fonts() {
  command -v npx >/dev/null || { echo "需要 Node.js（npx）来运行 lv_font_conv" >&2; exit 1; }

  # 1. 字体文件：Noto Sans SC（OFL 授权，可商用）+ Lucide 图标字体（ISC 授权）
  mkdir -p "$FONT_SRC" "$FONT_OUT" "$BUILD"
  download "$NOTO_URL"               "$CN_FONT"
  download "$LUCIDE_URL/lucide.ttf"  "$ICON_FONT"
  download "$LUCIDE_URL/info.json"   "$ICON_INFO"

  # 2. 收集字符、查图标编码、生成 mda_fonts.h
  python3 tools/make_fonts.py "$ICON_INFO"
  local ui common icons
  ui=$(cat "$BUILD/chars_ui.txt")
  common=$(cat "$BUILD/chars_common.txt")
  icons=$(cat "$BUILD/icon_ranges.txt")

  # 3. 生成三个字号：ASCII + 中文 + 图标
  #    14px 用于正文和 AI 对话，额外加入常用字；11px / 18px 只放界面里用到的字
  gen() {   # gen <字号> <中文字符>
    echo "生成 font_cn_$1.c ..."
    $FONT_CONV --bpp 4 --size "$1" --format lvgl --no-compress --lv-include lvgl.h \
      --font "$CN_FONT" -r 0x20-0x7E --symbols "$2" \
      --font "$ICON_FONT" -r "$icons" \
      -o "$FONT_OUT/font_cn_$1.c"
  }
  gen 11 "$ui"
  gen 14 "$ui$common"
  gen 18 "$ui"

  echo "字体完成：$(ls -lh "$FONT_OUT"/*.c | awk '{print $9, $5}' | tr '\n' ' ')"
}

case "$WHAT" in
  all)    make_images; make_fonts ;;
  images) make_images ;;
  fonts)  make_fonts ;;
  *)      echo "用法：bash tools/make_assets.sh [all|images|fonts]" >&2; exit 1 ;;
esac
