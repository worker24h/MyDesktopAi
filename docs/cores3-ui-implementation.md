# CoreS3 小伙伴 UI 实现方案

> 设计稿：`docs/design/cores3-ui.html`（浏览器打开，右侧可切换页面、模拟硬件事件）
> 目标：在 M5Stack CoreS3（320×240 触摸屏）上 1:1 还原设计稿，并接入真实硬件。
> 基础：在现有 `MyDesktopAi` 工程上继续开发（保留 WiFi / NTP / SD / 日志 / Web 服务）。

---

## 0. 总览

### 0.1 技术选型

| 层 | 选择 | 原因 |
|---|---|---|
| 平台 | `espressif32@8.0.0`（arduino-esp32 3.3.12） | 本机已安装，固定版本避免自动升级 |
| 屏幕/触摸/电源/音频驱动 | M5Unified + M5GFX | CoreS3 官方推荐，工程已在用 |
| UI 框架 | **LVGL 9.2.x** | 渐变、圆角、半透明、阴影、动画、图片、图表都内置，设计稿可直接还原 |
| 机器人形象 | **预渲染 PNG → LVGL 图片**（RGB565A8，带透明通道） | 3D 质感无法用基本图形实时画，用图片最省 CPU |
| 中文字体 | `lv_font_conv` 只导出用到的字 | 控制 Flash 体积 |
| 图标 | Lucide 图标字体（和设计稿同一套图标）合并进字体 | 可以随意改颜色，体积小 |

### 0.2 设计稿 → 设备的取舍

| 设计稿里有 | 设备上的做法 |
|---|---|
| 背景斜向渐变 + 白色光斑 | 一张 320×240 背景图（RGB565，150KB），零计算成本 |
| 卡片半透明 + 阴影 | `bg_opa` 70% + 小阴影；如果帧率不够，先去掉阴影 |
| 3D 机器人 + 眨眼/说话 | 每个表情一张图 + 眨眼帧 + 闭嘴帧，用定时器换图 |
| 机器人上下浮动 | `lv_anim` 改 y 坐标 |
| 机器人挥手 | v1 先不做（需要把手臂拆成单独图层），见第 9 节 |
| 气泡里的 emoji（😊🎵） | LVGL 字体不支持彩色 emoji：删掉或换成小图标 |
| 摄像头画面 | GC0308 输出 RGB565，直接喂给 `lv_image` |

### 0.3 里程碑（每一步都能独立烧录验证）

| 里程碑 | 内容 | 验收标准 |
|---|---|---|
| **M0** | 接入 LVGL：显示、触摸、时钟 | 屏幕显示按钮，点击有反应，颜色正确，帧率 ≥ 25 |
| **M1** | 素材管线：背景、机器人图片、字体、图标 | 屏幕上显示背景 + 一个机器人 + 一行中文 + 一个图标 |
| **M2** | UI 框架：主题、状态栏、导航、页面切换、Toast | 8 个页面都能进出，内容是静态假数据 |
| **M3** | 机器人组件：表情切换、眨眼、浮动、说话 | 表情页点选后，首页、头像、待机同步变化 |
| **M4** | 设备数据：电量、IMU、光照、温湿度、WiFi、亮度/音量 | 设备页显示真实数据，摇晃/遮光有反应，设置重启后保留 |
| **M5** | 陪伴模式：播放 SD 卡音频 | 能播放、暂停、切歌、跳转进度 |
| **M6** | 机器人控制：舵机 | 方向键控制舵机，平滑转动 |
| **M7** | 摄像头 | 设备页能看到实时画面，拍照存 SD |
| **M8** | AI 对话 | 按住说话 → 听到语音回答，表情联动 |
| **M9** | 打磨 | 性能、休眠、OTA、手机端联动 |

---

## 1. 工程配置（M0）

### 1.1 `platformio.ini`

```ini
[env:m5stack-cores3]
platform = espressif32@8.0.0
board = m5stack-cores3
framework = arduino
monitor_speed = 115200
board_build.filesystem = littlefs

lib_deps =
    m5stack/M5Unified@^0.2.29
    bblanchon/ArduinoJson@^7.2.0
    lvgl/lvgl@9.2.2
    madhephaestus/ESP32Servo          ; M6 再加也可以
    ; links2004/WebSockets            ; M8 / 手机联动时再加
    ; earlephilhower/ESP8266Audio     ; M5 播放 MP3 时再加

build_flags =
    -D LV_CONF_INCLUDE_SIMPLE         ; 让 LVGL 用 include/lv_conf.h
    -D LV_LVGL_H_INCLUDE_SIMPLE       ; 让转换出来的素材 .c 用 #include "lvgl.h"
    '-D LV_CONF_KCONFIG_EXTERNAL_INCLUDE="lv_kconfig_none.h"'   ; 绕过 LVGL 汇编文件找不到 esp_attr.h（配合 include/ 下的空文件）
    -I include
    -D CORE_DEBUG_LEVEL=3
```

> 注意：现在的 `main.cpp` 已经 `#include <ESP32Servo.h>`，但 `lib_deps` 里没有这个库。M0 阶段要么加上，要么先把这行删掉。

### 1.2 `include/lv_conf.h`

从 `.pio/libdeps/m5stack-cores3/lvgl/lv_conf_template.h` 复制到 `include/lv_conf.h`，把开头的 `#if 0` 改成 `#if 1`，再修改下面几项：

```c
#define LV_COLOR_DEPTH            16

/* 内存：直接用系统 malloc，大块内存会自动分到 PSRAM */
#define LV_USE_STDLIB_MALLOC      LV_STDLIB_CLIB
#define LV_USE_STDLIB_STRING      LV_STDLIB_CLIB
#define LV_USE_STDLIB_SPRINTF     LV_STDLIB_CLIB

#define LV_DEF_REFR_PERIOD        16      /* 刷新周期 ms，约 60fps 上限 */
#define LV_DPI_DEF                160

/* 调试：日志 + 左下角帧率/CPU 占用 */
#define LV_USE_LOG                1
#define LV_LOG_LEVEL              LV_LOG_LEVEL_WARN
#define LV_LOG_PRINTF             1
#define LV_USE_SYSMON             1
#define LV_USE_PERF_MONITOR       1       /* 正式版改成 0 */

/* 圆角、阴影、渐变需要复杂绘制 */
#define LV_DRAW_SW_COMPLEX        1

/* 内置字体只留一个兜底 */
#define LV_FONT_MONTSERRAT_14     1
#define LV_FONT_DEFAULT           &lv_font_montserrat_14

/* 用到的控件（模板默认大多已开，确认一下） */
#define LV_USE_LABEL   1
#define LV_USE_IMAGE   1
#define LV_USE_BUTTON  1
#define LV_USE_SLIDER  1
#define LV_USE_BAR     1
#define LV_USE_CHART   1
#define LV_USE_TABVIEW 0   /* 设计稿的标签页自己用按钮做，更好控制样式 */

/* M5 阶段如果要从 SD 卡读图片，再打开对应的文件系统驱动 */
/* #define LV_USE_FS_ARDUINO_SD 1 */
```

### 1.3 目录结构（沿用工程里的 `mda_` 前缀）

```
src/
├── main.cpp
├── app/
│   ├── mda_app_state.h/.cpp     // 全局状态：表情、传感器、播放器…
│   └── mda_events.h/.cpp        // 服务 → UI 的事件队列
├── hal/
│   └── mda_lvgl_port.h/.cpp     // LVGL 显示、触摸、时钟对接
├── ui/
│   ├── mda_ui.h/.cpp            // 初始化、页面管理、返回栈
│   ├── mda_theme.h/.cpp         // 颜色、样式、字体
│   ├── widgets/
│   │   ├── mda_robot.h/.cpp     // 机器人组件（换图、眨眼、浮动、说话）
│   │   ├── mda_statusbar.cpp    // 状态栏（放在 top layer，所有页面共用）
│   │   ├── mda_navbar.cpp       // 底部导航
│   │   ├── mda_toast.cpp
│   │   └── mda_widgets.cpp      // 卡片、入口块、气泡等小组件
│   └── screens/
│       ├── scr_home.cpp   scr_music.cpp   scr_chat.cpp   scr_control.cpp
│       └── scr_device.cpp scr_settings.cpp scr_faces.cpp  scr_idle.cpp
├── services/
│   ├── mda_power.cpp  mda_imu.cpp  mda_light.cpp  mda_env.cpp
│   ├── mda_audio.cpp  mda_servo.cpp  mda_camera.cpp  mda_ai.cpp
└── assets/
    ├── images/   // 由 PNG 转出来的 .c（见第 3 节）
    └── fonts/    // 由 lv_font_conv 生成的 .c
tools/
├── assets/png/          // 导出的原始 PNG
├── chars.txt            // 字体用到的汉字
└── make_assets.sh       // 一键转换图片和字体
```

PlatformIO 会递归编译 `src/` 下所有 `.c/.cpp`，素材 `.c` 放进去就行。

---

## 2. LVGL 对接 M5Unified（M0）

### 2.1 `hal/mda_lvgl_port.cpp`

```cpp
#include <M5Unified.h>
#include <lvgl.h>

static constexpr int W = 320, H = 240;
static constexpr int BUF_LINES = 40;              // 每块缓冲 320×40 像素

static uint32_t tick_cb() { return millis(); }

static void flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    const uint32_t w = lv_area_get_width(area);
    const uint32_t h = lv_area_get_height(area);
    M5.Display.startWrite();
    M5.Display.pushImageDMA(area->x1, area->y1, w, h, (lgfx::rgb565_t *)px_map);
    M5.Display.endWrite();
    lv_display_flush_ready(disp);
}

static void touch_cb(lv_indev_t *indev, lv_indev_data_t *data) {
    auto t = M5.Touch.getDetail();
    if (t.isPressed()) {
        data->point.x = t.x;
        data->point.y = t.y;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

void lvgl_port_init() {
    lv_init();
    lv_tick_set_cb(tick_cb);

    lv_display_t *disp = lv_display_create(W, H);
    // 绘制缓冲：放内部 RAM 且可 DMA（不要放 PSRAM，见第 8 节）
    const size_t sz = W * BUF_LINES * sizeof(uint16_t);
    void *buf1 = heap_caps_malloc(sz, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    void *buf2 = heap_caps_malloc(sz, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    lv_display_set_buffers(disp, buf1, buf2, sz, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(disp, flush_cb);

    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, touch_cb);
}

void lvgl_port_loop() {
    lv_timer_handler();
}
```

**颜色验证**：M0 第一件事是画红、绿、蓝三个色块。如果颜色不对（比如红色显示成蓝色或花屏），说明字节序反了：把 `pushImageDMA` 的第 5 个参数改成 `(uint16_t *)px_map`，并在调用前加上 `lv_draw_sw_rgb565_swap(px_map, w * h);`。以实测为准。

### 2.2 `main.cpp` 改造要点

```cpp
void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);
    M5.Display.setRotation(1);           // 横屏 320×240
    M5.Display.setBrightness(200);

    sd_setup(); logger.initialize(); mda_ntp_initialize();
    wifi_start_ap_sta(AP_SSID, AP_PASSWORD);
    /* …原有 WiFi / Web 初始化保持不变… */

    lvgl_port_init();
    ui_init();                           // 创建主题、状态栏、首页
    services_init();                     // 传感器、音频等
}

void loop() {
    M5.update();                         // 刷新触摸、按键、IMU 状态
    webfs_loop(); wifi_sta_loop(); mda_ntp_loop();
    services_loop();                     // 非阻塞轮询传感器
    events_dispatch_to_ui();             // 把服务事件交给 UI（见 4.6）
    lvgl_port_loop();
    delay(2);
}
```

**必须删除**：原来直接操作 `M5.Display` 的 `drawClock()` / `drawStatus()`。接入 LVGL 后只能由 LVGL 画屏幕，否则两边会互相覆盖。AP 地址、IP 这些信息改到设置页的「网络与连接」里显示。

### 2.3 M0 验收
- 屏幕显示一个按钮和计数器，点一下加一
- 打开 `LV_USE_PERF_MONITOR` 看帧率：空白页应 ≥ 50fps，满屏动画 ≥ 25fps

---

## 3. 素材管线（M1）

### 3.1 导出机器人 PNG

设计稿里的机器人是 SVG，直接在浏览器里批量导出：

1. 用 Chrome 打开 `docs/design/cores3-ui.html`
2. 打开开发者工具 → Console，粘贴下面的脚本并回车
3. 浏览器询问「是否允许下载多个文件」时选允许

```js
(async () => {
  const defs = document.querySelector('svg defs').outerHTML;
  const SIZES = { L: 150, XL: 200, S: 44 };          // 页面里用 / 待机全屏 / 表情网格+头像
  const OPEN_EYES = ['surprised','think','listen','sad','playful','confused','talk'];
  const jobs = [];
  for (const ex of Object.keys(EXPR)) {
    jobs.push({ name: ex, ex, mod: s => s });
    if (OPEN_EYES.includes(ex))                       // 睁眼的表情需要一张“闭眼帧”
      jobs.push({ name: ex + '_blink', ex, mod: s => s.replace(/<g class="eyes">[\s\S]*?<\/g>/, `<g class="eyes">${FACE.sleep.e}</g>`) });
  }
  jobs.push({ name: 'talk_close', ex: 'talk',         // 说话的“闭嘴帧”
    mod: s => s.replace(/(class="talk-mouth"[^>]*?ry=")[\d.]+/, (_, p) => p + '1.8') });

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
    const a = Object.assign(document.createElement('a'), { download: `robot_${name}_${tag}.png`, href: c.toDataURL('image/png') });
    a.click();
    await new Promise(r => setTimeout(r, 120));
  }
})();
```

得到 20 种 × 3 个尺寸 = 60 张 PNG，放到 `tools/assets/png/robot/`。

- 20 种 = 12 个表情 + 7 张眨眼帧 + 1 张闭嘴帧
- 「开心、大笑、唱歌、睡觉、兴奋」的眼睛本来就是弯的或闭着的，不需要眨眼帧
- 不需要的组合（比如 S 尺寸的眨眼帧）可以删掉，减少 Flash 占用

### 3.2 背景图

设计稿背景是 `linear-gradient(160deg, #cfe0ff, #e2d8ff 35%, #ffcfe5 70%, #ffc2dc)` 加两团白色光斑（不透明度 55% / 45%）。这套配色是按 CoreS3 屏幕实际效果调过的：屏幕偏冷，太淡的粉色显示不出来。在浏览器里打开设计稿，把屏幕区域截成 320×240 的 PNG（或用任意图像工具按这几个颜色做一张），保存为 `bg_main.png`。

### 3.3 图片转 LVGL 格式

用 LVGL 自带的脚本（在 `.pio/libdeps/m5stack-cores3/lvgl/scripts/LVGLImage.py`）：

```bash
# 一次性准备：脚本依赖 pypng 和 lz4，装在工程自己的虚拟环境里
python3 -m venv tools/.venv
tools/.venv/bin/pip install pypng lz4

LVGL=.pio/libdeps/m5stack-cores3/lvgl
# 机器人：带透明通道。输入只能是“一个文件”或“一个文件夹”（会递归处理里面所有 PNG），不能用 *.png 传多个文件
tools/.venv/bin/python $LVGL/scripts/LVGLImage.py --ofmt C --cf RGB565A8 -o src/assets/images tools/assets/png/robot
# 背景：不透明
tools/.venv/bin/python $LVGL/scripts/LVGLImage.py --ofmt C --cf RGB565 -o src/assets/images tools/assets/png/bg_main.png
```

所以导出的 PNG 按格式分文件夹放：机器人放 `tools/assets/png/robot/`，背景单独一个文件。也可以用 LVGL 官网的在线图片转换器（选 v9）。记得把 `tools/.venv/` 加进 `.gitignore`。

生成的每个 `.c` 里会有一个 `const lv_image_dsc_t robot_happy_L`，使用时：

```cpp
LV_IMAGE_DECLARE(robot_happy_L);
lv_image_set_src(img, &robot_happy_L);
```

### 3.4 Flash 预算

RGB565A8 每像素 3 字节：

| 尺寸 | 单张 | 20 张 |
|---|---|---|
| L 150×129 | 58 KB | 1.16 MB |
| XL 200×172 | 103 KB | 2.06 MB |
| S 44×38 | 5 KB | 0.10 MB |
| 背景 320×240（RGB565） | 150 KB | — |

加起来约 3.5MB，再加代码约 1.5–2MB、字体约 0.5–1MB。默认分区 `default_16MB.csv` 的单个应用分区约 6.25MB，**能放下但比较紧**。可选的优化：
1. XL 只保留 12 个表情、不要眨眼帧（待机全屏时眨眼不那么重要）
2. 转换时加 `--compress RLE`（需要在 `lv_conf.h` 打开 `LV_USE_RLE`）：透明边缘多的图能省一半左右
3. XL 改放 SD 卡（打开 `LV_USE_FS_ARDUINO_SD`，转成 `--ofmt BIN`），只在待机页加载

### 3.5 字体

**准备字体文件**（需要可商用的授权）：
- 中文：Noto Sans SC 或 HarmonyOS Sans SC 的 Medium 字重
- 图标：`lucide-static` 包里的 `lucide.ttf`（设计稿用的就是 Lucide 图标）

**收集界面用到的汉字**：

```bash
python3 -c "import re,sys,glob;s=''.join(open(f,encoding='utf-8').read() for f in glob.glob('src/ui/**/*.cpp',recursive=True));print(''.join(sorted(set(re.findall(r'[　-〿一-鿿＀-￯]',s)))))" > tools/chars.txt
```

> AI 对话的回答文字是动态的，事先不知道会出现哪些字。所以对话气泡用的 14px 字体要额外加入「常用 3500 字」（GB2312 一级字库）。其他字号只放界面里实际用到的字。

**生成字体**（需要 Node.js：`npm i -g lv_font_conv`）：

```bash
# 14px：正文 + 对话（含常用字）+ 图标
lv_font_conv --bpp 4 --size 14 --format lvgl --no-compress --lv-include lvgl.h \
  --font NotoSansSC-Medium.otf -r 0x20-0x7E --symbols "$(cat tools/chars.txt tools/common3500.txt)" \
  --font lucide.ttf -r 0xE0F0,0xE1A8,... \
  -o src/assets/fonts/font_cn_14.c

# 11px：小字（状态栏、标签）   18px：标题、大数字
lv_font_conv --bpp 4 --size 11 ... -o src/assets/fonts/font_cn_11.c
lv_font_conv --bpp 4 --size 18 ... -o src/assets/fonts/font_cn_18.c
```

Lucide 每个图标的编码在 `lucide-static` 包的 `font/info.json` 里查。设计稿用到的图标有：`house`、`smile`、`mic`、`settings`、`music`、`message-circle`、`bot`、`briefcase`、`arrow-left`、`skip-back`、`skip-forward`、`play`、`pause`、`book-open`、`feather`、`ellipsis`、`chevron-up/down/left/right`、`crosshair`、`camera`、`scan`、`wifi`、`volume-2`、`sun`、`info`、`pencil`、`chevron-right`。

在代码里定义成宏：

```cpp
#define ICON_HOME   "\xEE\x83\xB0"   // 对应编码的 UTF-8，按 info.json 查到的值换算
```

### 3.6 一键脚本 `tools/make_assets.sh`（已生成）
`bash tools/make_assets.sh`：用无界面 Chrome 从设计稿导出机器人 PNG 和背景（`tools/export_png.py`），再转换成 `src/assets/images/*.c`。以后改了设计稿，跑一遍就行。字体部分（3.5）等界面文字定下来后再加进脚本。

### 3.7 M1 验收
屏幕显示背景图，左边一个 L 尺寸的机器人，右边一行 18px 中文「你好呀」和一个房子图标。

---

## 4. UI 框架（M2）

### 4.1 颜色和样式（`mda_theme.cpp`，数值都取自设计稿 CSS）

```cpp
namespace C {
  constexpr uint32_t INK = 0x2A2F4A, INK2 = 0x6B7194, LINE = 0xDADCEE;
  constexpr uint32_t PINK = 0xFF5B97, PINK_L = 0xFF8FBD;
  constexpr uint32_t BLUE = 0x3D8BFF, BLUE_L = 0x7CC0FF;
  constexpr uint32_t GREEN = 0x26B773, GREEN_L = 0x6FE0A2, OK = 0x36C98A;
  constexpr uint32_t VIOLET = 0x8B7CFF, VIOLET_L = 0xB7A9FF;
  constexpr uint32_t RED = 0xFF4D6D;
  // 入口块底色
  constexpr uint32_t TILE_PINK = 0xFFD6E7, TILE_BLUE = 0xD9E8FF, TILE_GREEN = 0xD4F6E0, TILE_VIOLET = 0xE6E1FF;
  // 聊天气泡
  constexpr uint32_t BUBBLE_BOT = 0xDCF6E5;
}

lv_style_t st_card, st_card_strong, st_tile, st_icon_box, st_text, st_text2, st_title;

void theme_init() {
    // 卡片 = 设计稿里的 .cardx
    lv_style_init(&st_card);
    lv_style_set_bg_color(&st_card, lv_color_white());
    lv_style_set_bg_opa(&st_card, 184);                   // 72%
    lv_style_set_radius(&st_card, 14);
    lv_style_set_border_color(&st_card, lv_color_white());
    lv_style_set_border_width(&st_card, 1);
    lv_style_set_border_opa(&st_card, 242);               // 95%
    lv_style_set_shadow_color(&st_card, lv_color_hex(0x6E64C8));
    lv_style_set_shadow_opa(&st_card, 41);                // 16%
    lv_style_set_shadow_width(&st_card, 12);
    lv_style_set_shadow_offset_y(&st_card, 4);
    lv_style_set_pad_all(&st_card, 0);

    // 图标方块：两色渐变（左上浅 → 右下深）
    lv_style_init(&st_icon_box);
    lv_style_set_radius(&st_icon_box, 9);
    lv_style_set_bg_opa(&st_icon_box, LV_OPA_COVER);
    lv_style_set_bg_grad_dir(&st_icon_box, LV_GRAD_DIR_VER);
    lv_style_set_text_color(&st_icon_box, lv_color_white());
    /* 每种颜色单独设置 bg_color / bg_grad_color */
    // …其余样式同理
}
```

另外写几个小工具函数，减少重复代码：

```cpp
lv_obj_t *mk_card(lv_obj_t *parent, int x, int y, int w, int h);
lv_obj_t *mk_label(lv_obj_t *parent, const char *txt, const lv_font_t *font, uint32_t color);
lv_obj_t *mk_icon_box(lv_obj_t *parent, const char *icon, uint32_t c1, uint32_t c2, int size);
lv_obj_t *mk_bubble(lv_obj_t *parent, const char *txt);   // 白色圆角气泡 + 底部小三角
```

记得对所有容器调用 `lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE)`，否则 LVGL 默认可以滚动，会出现意外的拖动。

### 4.2 页面管理（`mda_ui.cpp`）

```cpp
enum class Page { Home, Music, Chat, Control, Device, Settings, Faces, Idle };

void ui_go(Page p, bool root = false);   // root=true：清空返回栈（导航栏、待机返回用）
void ui_back();
```

- 每个页面一个 `lv_obj_t *scr = lv_obj_create(NULL)`，**启动时一次性创建好、常驻内存**。总共 8 个页面，内存够用，切换也最快。
- 切换动画：`lv_screen_load_anim(scr, LV_SCR_LOAD_ANIM_FADE_IN, 200, 0, false)`
- 返回栈用一个固定大小的数组即可，规则和设计稿 `go()` 函数一致：
  - 首页、设置、表情这三个是根页面，在导航栏里切换时清空返回栈
  - 其他页面压栈，返回时出栈
- 每个页面提供 `on_enter()`，进入时刷新数据（比如首页重新设置机器人表情）

### 4.3 状态栏（放在 `lv_layer_top()`，所有页面共用）

| 元素 | 位置 | 说明 |
|---|---|---|
| 返回按钮 | x10, y2, 22×18, 圆角 9 | 只在二级页面显示 |
| 页面标题 | 返回按钮右侧 6px，11px 粗体 | 只在二级页面显示 |
| 时间 | x10, 垂直居中，11px 粗体 | 只在根页面显示 |
| WiFi 图标 | 右侧，13px | 信号弱时改成灰色 |
| 电池 | 最右，20×10，边框 1.5px | 低于 20% 填充改成红色 |

待机页隐藏整个状态栏（`lv_obj_add_flag(bar, LV_OBJ_FLAG_HIDDEN)`）。

### 4.4 Toast
也放在 top layer：顶部居中（y=28），深色半透明圆角条（`#2A2F4A` 88%），11px 白字，显示 1.6 秒后淡出。接口：`ui_toast("已保存到 SD 卡")`。

### 4.5 底部导航
x8, y200, 304×34，圆角 14，卡片样式。4 个按钮均分：首页 / 表情 / 对话 / 设置。当前页图标为粉色 `#FF5B97`，其他为 `#6F7598`。首页、设置、表情三个页面各放一份（或者也放到 top layer，在其他页面隐藏）。

### 4.6 状态和事件：服务 → UI 的数据流

**LVGL 不是线程安全的**：所有 `lv_*` 调用只能在 `loop()` 这个任务里执行。音频、AI 这些后台任务不要直接改 UI，而是发事件：

```cpp
// app/mda_events.h
enum class EvtType : uint8_t { ExprChanged, Sensor, Battery, Shake, Dark, Player, ChatAsr, ChatReply, ChatDone, Toast };
struct Evt { EvtType type; int32_t i; float f; char text[96]; };

bool events_post(const Evt &e);       // 任何任务都能调用（内部用 xQueueSend）
void events_dispatch_to_ui();         // 只在 loop() 里调用：逐条取出并更新 UI
```

```cpp
// app/mda_app_state.h
enum class Expr : uint8_t { Happy, Laugh, Surprised, Think, Listen, Sing, Sleep, Sad, Excited, Playful, Confused, Talk, COUNT };
struct AppState {
    Expr userExpr = Expr::Happy;      // 用户在表情页选的
    Expr override_ = Expr::COUNT;     // 临时反应（摇晃等），COUNT 表示没有
    uint32_t overrideUntil = 0;
    bool dark = false, chatting = false;
    float temp = NAN; int hum = -1, lux = 0, batt = 100, rssi = 0;
    int yaw = 90, pitch = 0;
    uint8_t volume = 60, brightness = 80;
    String petName = "我的小伙伴";
};
extern AppState g_state;

// 当前该显示什么表情：临时反应 > 对话中 > 光线暗 → 睡觉 > 电量低 → 难过 > 用户选择
Expr current_expr();
```

这个优先级和设计稿里 `curExpr()` 的逻辑一致。

### 4.7 M2 验收
8 个页面都按第 5 节的布局搭好（数据先写死），导航、返回、页面切换都正常。

---

## 5. 逐页实现（坐标都是屏幕绝对坐标，取自设计稿）

通用约定：状态栏占 y 0–22；内容区从 y 26 开始；左右边距 8；底部栏（导航/分类/按钮）统一在 y 200、高 34。

### 5.1 首页 `scr_home`

| 元素 | 位置 / 尺寸 | 控件 & 样式 |
|---|---|---|
| 背景 | 全屏 | `lv_image` 显示 `bg_main` |
| 问候气泡 | 左列居中，x6–152，y26，最大宽 142 | `mk_bubble`，11.5px 粗体，两行 |
| 机器人 | 气泡下方 2px，L 尺寸 150 宽 | `robot_create(parent, RobotSize::L)`，开启浮动；点击 → 待机页 |
| 入口 2×2 | x158, y30，每个 73×62，间距 7 | 圆角 14 的按钮，底色分别为 `TILE_PINK/BLUE/GREEN/VIOLET`；中间 28px 渐变图标方块 + 11px 文字 |
| 导航 | 见 4.5 | |

- 入口：唱歌故事 → 陪伴；AI 聊天 → 对话；机器人控制 → 控制；设备管理 → 设备
- 问候语随表情变化：在 `const char *GREETINGS[]` 里，和设计稿的 `greetings` 对应（去掉 emoji）。电量低时显示「电量有点低了… 帮我充充电吧」
- 按下效果：`lv_obj_set_style_transform_scale(btn, 240, LV_STATE_PRESSED)`（256 = 100%），设置 `transform_pivot` 为按钮中心

### 5.2 陪伴模式 `scr_music`

| 元素 | 位置 / 尺寸 | 说明 |
|---|---|---|
| 机器人 | x4–128, y24，124 宽（L 图缩放到 83%：`lv_image_set_scale(img, 212)`） | 播放时表情为「唱歌」，暂停时为「开心」 |
| 音符 | 机器人四周 4 个 15px 文字 ♪ ♫ | 颜色 `#C37BFF/#FFB12B/#FF7A59/#2FC6A0`；播放时做上下浮动动画，暂停时透明度 35% |
| 播放卡片 | x128–312, y26，内边距 8/10 | 卡片样式 |
| ├ 封面 | 46×46，圆角 10 | 每首歌一个底色 + 一张小图（或用分类图标代替 emoji） |
| ├ 标题 / 副标题 | 封面右侧，14px 粗体 / 10.5px 灰色 | 标题过长用 `LV_LABEL_LONG_DOT` |
| ├ 进度条 | 宽度撑满，高 5，圆角 5 | `lv_slider`：指示条粉色，隐藏圆点；拖动松手后跳转 |
| ├ 时间 | 右对齐 10px | `01:24 / 06:30` |
| └ 控制 | 上一首 18px 图标 / 播放 38px 粉色圆 / 下一首 | 间距 26 |
| 分类 | y200，4 等分，间距 6 | 选中项白底、图标方块变粉色 |

- 歌曲列表来自 SD 卡目录：`/data/media/故事/*.mp3`、`/儿歌/`、`/诗歌/`、`/更多/`，启动时扫描（见 6.5）
- 进度每秒刷新一次（`lv_timer`），不要每帧刷新

### 5.3 AI 助手 `scr_chat`

| 元素 | 位置 / 尺寸 | 说明 |
|---|---|---|
| 提示气泡 | 左列 x4–122，y26，10.5px | 有对话后隐藏 |
| 机器人 | 气泡下方，112 宽（L 缩放 75%） | 倾听 / 思考 / 说话 / 开心 |
| 消息列表 | x124–312, y26–190 | 可滚动的 `lv_obj`，flex 纵向排列，间距 6；新消息后 `lv_obj_scroll_to_view(msg, LV_ANIM_ON)` |
| ├ 我的消息 | 靠右，白底，圆角 12（右下角 4） | 最大宽 92% |
| └ 回答 | 靠左，`BUBBLE_BOT` 底色 | 打字机效果：`lv_timer` 每 45ms 多显示一个字 |
| 按住说话 | x8, y196, 304×38 | 按下：变蓝底 + 声波动画 + 文字「正在聆听…」；松开：结束录音 |

- 按钮事件：`LV_EVENT_PRESSED` 开始录音，`LV_EVENT_RELEASED` / `LV_EVENT_PRESS_LOST` 结束
- 只保留最近 20 条消息，超出删除最早的，避免内存增长
- 打字机：先 `lv_label_set_text` 空串，再用定时器逐个追加 UTF-8 字符（注意按字符而不是字节切分，用 `lv_text_encoded_next` 之类的方法）

### 5.4 机器人控制 `scr_control`

| 元素 | 位置 / 尺寸 | 说明 |
|---|---|---|
| 机器人 | x2, y28, 104 宽 | 跟随角度偏移和旋转：`lv_image_set_rotation(img, (yaw-90)*1.5)`（单位 0.1°），`lv_obj_set_x` 偏移 |
| 方向盘 | x108, y30, 132×132 圆 | 白→浅紫径向渐变用纯色 `#F4F3FF` 代替即可 |
| ├ 上下左右 | 40×40 白色圆按钮，图标蓝色 | `LV_EVENT_PRESSED` 走一步，`LV_EVENT_LONG_PRESSED_REPEAT` 持续移动 |
| └ 中间 | 36×36 蓝色圆 | 复位到 90° / 0° |
| 角度 / 俯仰 | x246, y30，66 宽，两张卡片 | 18px / 13px 数字 |
| 底部按钮 | y200，三等分：自动跟随 / 工作范围 / 停止 | 自动跟随开启时为蓝色渐变底白字 |

- 转到 0° 或 180° 极限时，机器人换成「惊讶」表情
- 角度变化通过 `servo_set_target(yaw, pitch)` 交给舵机服务，由服务做平滑（见 6.6）

### 5.5 设备管理 `scr_device`

| 元素 | 位置 / 尺寸 | 说明 |
|---|---|---|
| 标签 | x8, y26, 304×28，三个按钮 | 选中项白底；切换时显示/隐藏三个容器 |
| **传感器** | | |
| ├ 三张卡片 | y60，三等分，高 62 | 图标 + 标签 9.5px + 数值 15px（温度 / 湿度 / 光照） |
| └ 趋势图 | y128, 304×80 | `lv_chart` 折线图，30 个点，每 2 秒 `lv_chart_set_next_value`；线粉色，下方填充 22% |
| **摄像头** | y60, 304×150，圆角 12 | `lv_image` 显示摄像头帧（见 6.7）；左上 LIVE 红点闪烁；右下「拍照」「识别」两个白色圆角按钮 |
| **网络** | 列表卡片 | Wi-Fi 名称、信号、IP、AP 地址、蓝牙、电池；每行 11px，左右对齐 |

- 离开设备页、或者不在摄像头标签时，**暂停摄像头采集**，省 CPU 和电

### 5.6 设置 `scr_settings`

| 元素 | 位置 / 尺寸 | 说明 |
|---|---|---|
| 头像 | x8–108 列居中，y28，70×70 圆 | 白→浅蓝底 + 2px 白边，里面放 S 或 L 缩放的机器人（跟随当前表情） |
| 名字 / 签名 | 头像下方，13px 粗体 / 10px 灰 | |
| 改名按钮 | 22 高的胶囊 | 设备上没键盘：改成「从手机网页改名」或者在几个预设名字里轮换 |
| 设置列表 | x114–312, y26–194，卡片，每行 33 高 | |
| ├ 角色与表情 › | | 跳表情页 |
| ├ 音量 [滑块] 60 | 滑块宽 72 | `M5.Speaker.setVolume(v * 255 / 100)` |
| ├ 亮度 [滑块] 80 | | `M5.Display.setBrightness(...)`，最低不要低于 10% |
| ├ 网络与连接 › | | 跳设备页并切到「网络」标签 |
| └ 关于设备 › | | Toast 显示固件版本 |

- 音量、亮度、名字、表情保存到 `/data/config/ui.json`（用 ArduinoJson，和现有 `mda_wifi_config` 的写法一致），滑块松手（`LV_EVENT_RELEASED`）时才写，避免频繁写 SD 卡

**实现步骤**：逐步推进，做完一步在仿真里验证，再进入下一步。

```
pio run -e sim && .pio/build/sim/program      # 仿真
pio run -t upload -t monitor                   # 真机
```

#### 总体路线

| 步骤 | 内容 | 做完能看到 | 状态 |
|---|---|---|---|
| 1 | 空的设置页 + 菜单栏切换 | 点 ⚙ 切到设置页，点 🏠 回首页 | ✅ 完成 |
| 2 | 右侧列表卡片：5 行，图标 + 文字 + 箭头 | 列表的样子 | ✅ 完成（有 2 处遗留，见第 3 步开头） |
| 3 | 音量、亮度两行加滑块和数值 | 拖动滑块，数字跟着变 | ⬜ 进行中 |
| 4 | 左侧头像、名字、改名按钮 | 页面完整 | ⬜ |
| 5 | 滑块接硬件（真机调亮度、音量，仿真只打印） | 真机亮度会变 | ⬜ |
| 6 | 保存到 SD 卡 `/data/config/ui.json`，重启后保留 | 重启后设置还在 | ⬜ |

命名规则：本项目的结构体、函数一律用下划线小写（snake_case），结构体类型加 `_t` 后缀；`if` / 循环即使一行也要加 `{}`。


#### 第 1 步：空页面 + 菜单栏切换（✅ 已完成）

做了什么：

- `src/ui/screens/mda_settings.cpp`：`mda_settings_create()` 用 `lv_obj_create(NULL)` 创建独立屏幕，关闭滚动，背景透明（透出 `lv_layer_bottom()` 上的背景图）。
- `src/ui/screens/mda_screens.h`：声明 `mda_settings_create()`。
- `src/ui/mda_ui.h`：`mda_pages_t` 保存菜单栏 4 个图标对应的页面，未实现的为 `nullptr`。
- `src/ui/mda_ui.cpp`：`ui_init()` 里**一次性创建**首页和设置页（常驻内存，切换时不再创建），再加载首页。
- `src/ui/widgets/mda_menubar.cpp`：菜单栏放在 `lv_layer_top()`；点击回调先找目标页面，页面为 `nullptr` 就直接返回（避免 `lv_screen_load(nullptr)` 崩溃），否则切高亮并用 `lv_screen_load_anim(..., MOVE_LEFT/RIGHT, 300, 0, false)` 滑动切换。

要点回顾：

- `lv_screen_load_anim` 最后一个参数必须是 `false`，否则旧页面会被删除，`mda_pages_t` 里的指针变成悬空指针。
- 页面背景透明，所以要用 `MOVE_*` 动画，`OVER_*` / `FADE_*` 会让两页内容叠在一起。
- 状态栏、菜单栏在 top 层，背景图在 bottom 层，切页时都不动，也不用重建。


#### 第 2 步：右侧设置列表卡片

只做外观：5 行，每行是图标、文字、右箭头。滑块留到第 3 步，点击留到后面。

##### 目标效果

```
         x=114                         x=312
y=26  ┌─────────────────────────────────┐
      │ (😊) 角色与表情               › │  ← 每行高 33
      │─────────────────────────────────│  ← 1px 分隔线
      │ (🔊) 音量                     › │
      │─────────────────────────────────│
      │ (☀) 亮度                      › │
      │─────────────────────────────────│
      │ (📶) 网络与连接               › │
      │─────────────────────────────────│
      │ (ⓘ) 关于设备                  › │  ← 最后一行没有分隔线
y=191 └─────────────────────────────────┘
      ═══════════ 菜单栏（y 192 起）═══════════
```

左边 x 8～108 留给第 4 步的头像。音量、亮度这两行先也放箭头，第 3 步再换成滑块。

##### 2.1 删掉测试用的 label

`mda_settings.cpp` 里显示 `"Test"` 的那几行删掉。

##### 2.2 卡片容器

在 `mda_settings_create()` 里建一个容器当卡片：

| 设置 | 写法 | 说明 |
|---|---|---|
| 创建 | `lv_obj_create(scr)` → `lv_obj_remove_style_all` → `lv_obj_add_style(card, &st_card, 0)` | `st_card` 是白色 72% 半透明 + 白边 + 阴影，对应设计稿 `.cardx` |
| 位置大小 | `lv_obj_set_pos(card, 114, 26)`，`lv_obj_set_size(card, 198, 165)` | 5 × 33 = 165 |
| 内边距 | `lv_obj_set_style_pad_hor(card, 10, 0)` | 左右各留 10px |
| 布局 | `lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN)` | 5 行从上往下自动排 |
| 不滚动 | `lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE)` | |

> 卡片底边 191 和菜单栏顶边 192 只差 1px，卡片阴影会被菜单栏盖住一点，这是正常的。觉得挤，可以把行高改成 32。

##### 2.3 用一个函数创建一行

5 行结构一样，写一个辅助函数，**返回这一行的容器**（第 3 步要往音量、亮度这两行里加滑块）：

```cpp
static lv_obj_t *mda_settings_row_create(lv_obj_t *parent, const char *icon, uint32_t icon_bg, const char *text);
```

一行由 4 部分组成：

**① 行容器**

- `lv_obj_create(parent)` → `remove_style_all`
- 宽 `LV_PCT(100)`（占满卡片内宽），高 `33`
- flex 横排：`LV_FLEX_FLOW_ROW`，交叉轴 `LV_FLEX_ALIGN_CENTER`（垂直居中）
- 子元素间距：`lv_obj_set_style_pad_column(row, 8, 0)`
- 关闭滚动
- **只画下边框**当分隔线：

  ```cpp
  lv_obj_set_style_border_side(row, LV_BORDER_SIDE_BOTTOM, 0);
  lv_obj_set_style_border_width(row, 1, 0);
  lv_obj_set_style_border_color(row, lv_color_hex(MDA_COLOR_LINE), 0);
  ```

**② 彩色圆形图标**

- 一个 22×22 的 `lv_obj`：`remove_style_all`，圆角 `LV_RADIUS_CIRCLE`，`bg_color` 用传进来的颜色，`bg_opa` 设成 `LV_OPA_COVER`
- 里面放一个 label 显示图标：字体 `font_cn_11`（里面也包含图标），白色，`lv_obj_center()`

**③ 文字**

- label，加 `st_text` 样式，再把字体换成 `font_cn_11b`（设计稿是粗体）
- **`lv_obj_set_flex_grow(label, 1)`**：让文字占满剩下的宽度，右箭头就会被挤到最右边。这是 flex 里"左边一个、右边一个"的常用写法

**④ 右箭头**

- label，`ICON_RIGHT`，字体 `font_cn_11`，颜色 `0x9AA0BF`（设计稿 `.chev` 的灰色）

##### 2.4 用表格驱动 5 行

和首页 `create_tiles()` 一样，用一个数组描述每一行，再循环创建：

```cpp
typedef struct {
    const char *icon;
    uint32_t color;
    const char *text;
} mda_settings_row_def_t;
```

| 行 | 图标 | 颜色（设计稿） |
|---|---|---|
| 角色与表情 | `ICON_SMILE` | `0xFFC93C`（黄，主题里没有，直接写数字） |
| 音量 | `ICON_VOLUME` | `MDA_COLOR_BLUE` |
| 亮度 | `ICON_SUN` | `MDA_COLOR_VIOLET` |
| 网络与连接 | `ICON_WIFI` | `MDA_COLOR_OK` |
| 关于设备 | `ICON_INFO` | `MDA_COLOR_GREEN` |

循环结束后，**最后一行**要去掉分隔线：`lv_obj_set_style_border_width(最后一行, 0, 0)`。可以把每次返回的行记下来，或者在循环里判断是不是最后一个。

##### 2.5 重新生成字体

「角色与表情」「音量」「亮度」「网络与连接」「关于设备」这些字，现在的字体里大多没有，会显示成方块。写完代码后运行：

```
bash tools/make_assets.sh fonts
```

`make_fonts.py` 会扫描 `src/ui/` 下的代码，只把出现过的中文字打包进字体。以后在 UI 里加新的中文字，都要重新生成一次。

##### 验证

点 ⚙ 切到设置页，检查：

- [ ] 卡片在右边，5 行均匀排列，文字垂直居中
- [ ] 箭头都靠最右边对齐
- [ ] 前 4 行下面有浅色分隔线，最后一行没有
- [ ] 没有方块字


#### 第 3 步：音量、亮度两行加滑块和数值

只做界面：拖动滑块，右边的数字跟着变。真正调亮度、音量放到第 5 步，保存放到第 6 步。

##### 先处理第 2 步的 2 处遗留

- 箭头颜色：`lv_obj_set_style_bg_color(icon_arrow, ...)` 设的是**背景色**（没有 `bg_opa` 所以不显示），要改成 `lv_obj_set_style_text_color(icon_arrow, lv_color_hex(0x9AA0BF), 0)`。
- 文字：「网络与链接」改成「网络与**连接**」。

##### 目标效果

```
┌─────────────────────────────────────┐
│ (😊) 角色与表情                   › │
│─────────────────────────────────────│
│ (🔊) 音量    ━━━━━━━●───────    60  │  ← 滑块宽 72，数值宽 20
│─────────────────────────────────────│
│ (☀) 亮度     ━━━━━━━━━●─────    80  │
│─────────────────────────────────────│
│ (📶) 网络与连接                   › │
│─────────────────────────────────────│
│ (ⓘ) 关于设备                     › │
└─────────────────────────────────────┘
```

宽度够不够：卡片内宽 198 − 20 = 178；圆 22 + 间距 8 + 滑块 72 + 间距 8 + 数值 20 + 间距 8 = 138，留给文字 40px，「音量」「亮度」两个 11px 字约 24px，放得下。

##### 3.1 扩展行描述：区分「箭头行」和「滑块行」

在 `mda_settings_row_def_t` 里加一个类型和滑块参数：

```cpp
typedef enum {
    MDA_SETTINGS_ROW_ARROW,    // 右边是 ›，点击跳转
    MDA_SETTINGS_ROW_SLIDER,   // 右边是 滑块 + 数值
} mda_settings_row_type_t;

typedef struct {
    const char *icon;
    uint32_t color;
    const char *text;
    mda_settings_row_type_t type;
    int32_t min;      // 以下 3 个只对滑块行有意义，箭头行填 0
    int32_t max;
    int32_t value;    // 初始值
} mda_settings_row_def_t;
```

表格里：

| 行 | type | min | max | value | 说明 |
|---|---|---|---|---|---|
| 角色与表情 | ARROW | 0 | 0 | 0 | |
| 音量 | SLIDER | 0 | 100 | 60 | |
| 亮度 | SLIDER | **10** | 100 | 80 | 最低 10%，否则屏幕全黑看不见，没法再调回来 |
| 网络与连接 | ARROW | 0 | 0 | 0 | |
| 关于设备 | ARROW | 0 | 0 | 0 | |

##### 3.2 行函数里按类型创建右侧部件

`mda_settings_row_create()` 里，文字 label（`flex_grow = 1`）之后：

```cpp
if (param.type == MDA_SETTINGS_ROW_SLIDER) {
    // 创建滑块 + 数值（见 3.3、3.4）
} else {
    // 原来的箭头
}
```

文字的 `flex_grow = 1` 不变，它会把滑块和数值挤到右边，和箭头的原理一样。

##### 3.3 滑块

| 设置 | 写法 | 说明 |
|---|---|---|
| 创建 | `lv_slider_create(row)` | **不要** `remove_style_all`，保留默认主题的滑块样式，只改颜色和尺寸，省事 |
| 尺寸 | `lv_obj_set_size(slider, 72, 4)` | 高 4 是轨道粗细 |
| 范围和初始值 | `lv_slider_set_range(slider, min, max)`，`lv_slider_set_value(slider, value, LV_ANIM_OFF)` | |
| 轨道颜色 | `lv_obj_set_style_bg_color(slider, lv_color_hex(MDA_COLOR_LINE), LV_PART_MAIN)` | 没填充的部分 |
| 填充颜色 | `lv_obj_set_style_bg_color(slider, lv_color_hex(MDA_COLOR_VIOLET), LV_PART_INDICATOR)` | 设计稿 `accent-color: var(--violet)` |
| 圆钮 | `bg_color` 设成白色，`LV_PART_KNOB`；`lv_obj_set_style_pad_all(slider, 4, LV_PART_KNOB)` | 圆钮直径 = 轨道高 4 + 上下 pad 各 4 = 12 |
| 圆钮描边 | `border_width 1` + `border_color MDA_COLOR_VIOLET`，`LV_PART_KNOB` | 白底上看得清 |
| 点击范围 | `lv_obj_set_ext_click_area(slider, 10)` | 轨道只有 4px 高，手指很难按准，四周扩大 10px |

这里第一次用到选择器的 **part**：同一个滑块有 3 个部位，用 `LV_PART_MAIN` / `LV_PART_INDICATOR` / `LV_PART_KNOB` 分别设样式。

##### 3.4 数值 label

- `lv_label_create(row)`，字体 `font_cn_11`，颜色 `MDA_COLOR_INK2`（次要文字的灰）
- 固定宽度 `lv_obj_set_width(value_label, 20)`，右对齐 `lv_obj_set_style_text_align(value_label, LV_TEXT_ALIGN_RIGHT, 0)`
  - 固定宽度是为了数字从 9 变 10、从 99 变 100 时，滑块**不会左右跳**
- 初始文字：`lv_label_set_text_fmt(value_label, "%d", (int)param.value)`

##### 3.5 拖动时更新数字

给滑块注册 `LV_EVENT_VALUE_CHANGED` 事件，把数值 label 作为 `user_data` 传进去：

```cpp
lv_obj_add_event_cb(slider, mda_settings_slider_changed_cb, LV_EVENT_VALUE_CHANGED, value_label);
```

回调里：

1. `lv_event_get_current_target_obj(e)` 拿到滑块
2. `lv_event_get_user_data(e)` 拿到数值 label
3. `lv_slider_get_value(slider)` 读当前值，`lv_label_set_text_fmt` 更新数字

拖动过程中会连续触发 `VALUE_CHANGED`。第 5 步会在这里调硬件（亮度要跟手实时变化）；第 6 步保存则放到 `LV_EVENT_RELEASED`（松手时才写 SD 卡）。

##### 3.6 不需要重新生成字体

数字 0～9 是 ASCII，字体里本来就有。只有改了「网络与连接」的「连」字，才需要运行一次 `bash tools/make_assets.sh fonts`。

##### 验证

- [ ] 音量、亮度两行右边是滑块 + 数字，其他 3 行还是箭头
- [ ] 初始值是 60 和 80
- [ ] 拖动滑块，数字实时跟着变；亮度最小只能拖到 10
- [ ] 数字从两位变三位（100）时，滑块位置不跳
- [ ] 箭头是灰色的；「网络与连接」没有方块字

#### 第 4 步：左侧头像、名字、改名按钮

左边 x 8～108 这一列，从上往下：头像、名字、签名、改名按钮。

##### 目标效果

```
x=8          x=108
   ┌─────────┐
   │  ╭───╮  │  ← 头像：70×70 圆，y=28 起
   │  │ 🤖 │  │     白→浅蓝渐变 + 2px 白边 + 阴影，里面是 M 尺寸机器人（80px）
   │  ╰───╯  │
   │我的小伙伴│  ← 名字：粗体，深色
   │ 陪伴每一天～│ ← 签名：小字，灰色
   │ ( ✏ 改名 ) │ ← 胶囊按钮：高 22
   └─────────┘
```

##### 4.1 左列容器（照抄首页 `create_left()` 的写法）

- `lv_obj_create(scr)` → `remove_style_all`，`lv_obj_set_pos(col, 8, 28)`，`lv_obj_set_size(col, 100, LV_SIZE_CONTENT)`
- flex 竖排 `LV_FLEX_FLOW_COLUMN`，主轴 `START`，交叉轴 **`CENTER`**（每个元素水平居中）
- 行间距 `lv_obj_set_style_pad_row(col, 4, 0)`；关闭滚动

##### 4.2 头像

| 设置 | 写法 | 说明 |
|---|---|---|
| 圆形底 | `lv_obj_create(col)` → `remove_style_all`，`size 70×70`，`radius LV_RADIUS_CIRCLE` | |
| 渐变 | `bg_color` 白色，`bg_grad_color 0xDCECFF`，`bg_grad_dir LV_GRAD_DIR_VER`，`bg_opa COVER` | 设计稿是径向渐变，用竖向渐变近似，效果差不多，渲染也便宜 |
| 白边 | `border_width 2`，`border_color` 白色 | |
| 阴影 | `shadow_width 12`，`shadow_offset_y 4`，`shadow_color MDA_COLOR_SHADOW`，`shadow_opa` 约 40 | 和卡片一致；角大小 12+35=47，**超过阴影缓存 40**，会每次重算，但头像只有一个，影响不大 |
| 机器人 | `lv_image_create(avatar)`，`lv_image_set_src(img, &robot_happy_M)`，`lv_obj_center(img)` | 文件顶部加 `LV_IMAGE_DECLARE(robot_happy_M);`。M 尺寸（80×69）专门给头像用，S（44）留给表情页网格 |
| 不滚动 | `lv_obj_remove_flag(avatar, LV_OBJ_FLAG_SCROLLABLE)` | |

后续表情系统做好后，头像要跟着当前表情换图，那时再把 `img` 存成静态变量。

##### 4.3 名字和签名

- 名字：label，`st_text` + 字体 `font_cn_14`，文字「我的小伙伴」。设计稿是 13px 粗体，项目里没有这个字号，14px 最接近
- 签名：label，`st_text2`（11px 灰色），文字「陪伴每一天～」
- 名字的 label 存成**静态变量** `s_name_label`，改名时要更新它

##### 4.4 改名按钮

| 设置 | 写法 |
|---|---|
| 胶囊 | `lv_button_create(col)` → `remove_style_all` → `add_style(&st_card_strong)`，高 22，宽 `LV_SIZE_CONTENT`，`radius 11`，`pad_hor 10` |
| 内容 | 横排 flex，交叉轴居中，`pad_column 3`；里面一个图标 label（`ICON_PEN`）+ 一个文字 label「改名」，字体都用 `font_cn_11b` |
| 按下效果 | `lv_obj_set_style_transform_scale(btn, 240, LV_STATE_PRESSED)`，按下时缩到 94%，`transform_pivot_x/y` 设成按钮中心 |

**设备上没有键盘**，改名先用最简单的方案：**点一下，在几个预设名字里轮换**：

```cpp
static const char *NAMES[] = {"我的小伙伴", "小星星", "豆豆", "团子"};
```

点击回调（`LV_EVENT_CLICKED`）里下标加 1、取模，`lv_label_set_text(s_name_label, NAMES[i])`。以后可以改成「在手机网页上改名」。

##### 4.5 重新生成字体

「我的小伙伴」「陪伴每一天」「改名」和预设名字里的字都是新的，运行 `bash tools/make_assets.sh fonts`。

> 「～」是全角波浪号，`make_fonts.py` 的字符范围包含全角标点，会被一起打包。

##### 验证

- [ ] 头像、名字、签名、按钮在左列居中，上下间距均匀
- [ ] 头像是圆的，有白边和阴影，机器人在正中间
- [ ] 点「改名」，名字在几个预设之间轮换；按下时按钮有缩小效果
- [ ] 没有方块字

#### 第 5 步：滑块接硬件

拖动亮度滑块，真机屏幕亮度实时变化；拖动音量滑块，松手时响一声提示音，能听出音量大小。仿真里只打印数值。

##### 5.1 先加一层硬件接口，UI 不直接调 `M5`

UI 代码在仿真里也要编译，仿真没有 `M5.Display` / `M5.Speaker`。所以加一层很薄的接口，真机和仿真各实现一份（和状态栏以后要做的拆分是同一个思路）：

| 文件 | 内容 | 参与哪个环境 |
|---|---|---|
| `src/hal/mda_device.h` | 声明 `void mda_device_set_brightness(int percent);`、`void mda_device_set_volume(int percent);`、`void mda_device_beep();` | 两边都 include |
| `src/hal/mda_device.cpp` | 真机实现，调用 `M5` | 只有 `m5stack-cores3`（`env:sim` 的 `build_src_filter` 不包含 `hal/`，自动排除） |
| `src/sim/sim_device.cpp` | 仿真实现，`printf` 打印数值 | 只有 `sim`，文件用 `#ifdef MDA_SIM` 包起来 |

真机实现：

| 函数 | 实现 | 注意 |
|---|---|---|
| 亮度 | `M5.Display.setBrightness(percent * 255 / 100)` | 函数里再做一次下限保护：小于 10 就按 10 处理 |
| 音量 | `M5.Speaker.setVolume(percent * 255 / 100)` | |
| 提示音 | `M5.Speaker.tone(2000, 50)` | 2kHz 响 50ms |

##### 5.2 滑块回调怎么知道调哪个硬件

现在两个滑块共用一个回调，`user_data` 已经用来传数值 label 了。办法：在行描述里加一个函数指针，创建时存到**滑块对象自己**的 user_data 上：

```cpp
typedef void (*mda_settings_slider_cb_t)(int32_t value);

typedef struct {
    ...
    mda_settings_slider_cb_t on_change;   // 拖动时调用；箭头行填 nullptr
} mda_settings_row_def_t;
```

- 创建滑块时：`lv_obj_set_user_data(slider, (void *)param.on_change);`
- 回调里：`auto fn = (mda_settings_slider_cb_t)lv_obj_get_user_data(slider);`，不为空就 `fn(value)`
- 表格里音量填 `mda_device_set_volume`，亮度填 `mda_device_set_brightness`，箭头行填 `nullptr`

这样回调里不用写 `if (是音量) ... else if (是亮度)`，以后加别的滑块也只改表格。

> 区分两个 user_data：`lv_obj_add_event_cb(..., user_data)` 是**事件**的；`lv_obj_set_user_data(obj, ...)` 是**对象**的，一个对象只有一个。

##### 5.3 什么时候调用

| 事件 | 做什么 |
|---|---|
| `LV_EVENT_VALUE_CHANGED`（拖动中，连续触发） | 更新数字；调 `on_change`。亮度要跟手变化 |
| `LV_EVENT_RELEASED`（松手） | 音量滑块响一声 `mda_device_beep()`；第 6 步在这里保存 |

拖动时 `VALUE_CHANGED` 一秒可能触发几十次。`setBrightness` 走 I2C 调电源芯片，量不大，可以接受；如果以后发现卡，再改成"值变化超过 2 才调用"。

##### 5.4 开机时应用一次

开机时屏幕亮度、音量是 M5Unified 的默认值，和滑块显示的 60 / 80 对不上。`mda_settings_create()` 最后对两个滑块的初始值各调用一次 `on_change`。第 6 步改成从配置文件读。

##### 验证

- [ ] 仿真：拖动滑块，终端打印 `brightness=xx` / `volume=xx`，松手打印 `beep`
- [ ] 真机：拖亮度，屏幕亮度跟着变；拖到最左边屏幕不会全黑
- [ ] 真机：拖音量后松手响一声，音量大小有区别；音量 0 时不响
- [ ] 开机后屏幕亮度就是 80%

#### 第 6 步：保存到 SD 卡，重启后保留

##### 6.1 配置文件和接口

照 `src/config/mda_wifi_config.*` 的写法，新建 `src/config/mda_ui_config.h/.cpp`：

```
/data/config/ui.json
{ "volume": 60, "brightness": 80, "name": "我的小伙伴" }
```

```cpp
#define MDA_UI_CONFIG_PATH "/data/config/ui.json"

typedef struct {
    int volume = 60;            // 默认值：文件不存在或字段缺失时用
    int brightness = 80;
    char name[32] = "我的小伙伴";  // UTF-8，一个汉字 3 字节，32 字节够 10 个字
} mda_ui_config_t;

bool mda_ui_config_load(mda_ui_config_t &cfg);         // 失败时 cfg 保持默认值
bool mda_ui_config_save(const mda_ui_config_t &cfg);   // 先写 .tmp 再替换，和 wifi 配置一样
mda_ui_config_t &mda_ui_config();                      // 全局唯一一份，供各页面读写
```

要点：

- 读取时每个字段都用 `doc["volume"] | 60` 这种带默认值的写法，字段缺失也不会出错
- 读出来的值要**限制范围**：音量 0～100，亮度 10～100。防止有人手改文件写了 0，开机屏幕全黑
- 名字用 `strlcpy` 复制，防止超长溢出

##### 6.2 仿真里怎么办

仿真没有 SD 卡。`env:sim` 的 `build_src_filter` 不包含 `config/`，所以 `mda_ui_config.cpp` 不会参与编译。在 `src/sim/` 里加一份 `sim_ui_config.cpp`：`load` 直接返回 false（用默认值），`save` 只 `printf`。

##### 6.3 接入流程

| 时机 | 做什么 | 位置 |
|---|---|---|
| 开机 | `mda_ui_config_load(mda_ui_config())`，然后 `mda_device_set_brightness/volume` 应用 | `main.cpp`，`sd_setup()` 之后、`ui_init()` 之前 |
| 创建设置页 | 滑块初始值、名字从 `mda_ui_config()` 读，不再用表格里写死的 60 / 80 | `mda_settings_create()` |
| 滑块松手 `LV_EVENT_RELEASED` | 更新 `mda_ui_config()` 里对应字段，调用 `mda_ui_config_save` | 滑块回调 |
| 改名 | 更新 `name`，保存 | 改名按钮回调 |

**为什么只在松手时保存**：拖动时 `VALUE_CHANGED` 会触发几十次，每次都写 SD 卡，既慢又磨损卡。松手只写一次。

滑块回调怎么知道该改 `volume` 还是 `brightness`：和 5.2 一样，在行描述里再加一个 `int *config_field`，指向 `mda_ui_config()` 里的字段。也可以把 5.2 的 `on_change` 和这个字段合并成一个小结构体，两种都行。

##### 6.4 SD 卡和屏幕共用 SPI 总线

CoreS3 的 SD 卡和屏幕在同一条 SPI 总线上。现在 flush 用的是同步写屏（`startWrite` … `endWrite`），而且保存配置和 LVGL 刷屏都在 `loop()` 这一个任务里**依次执行**，不会同时占用总线，所以不用额外加锁。

以后如果把 flush 改成异步 DMA，或者把 LVGL 放到单独的任务里，就必须处理总线互斥，见本节前面的性能分析。

##### 验证

- [ ] 拖动滑块、改名后，串口能看到保存日志；SD 卡上出现 `/data/config/ui.json`，内容正确
- [ ] 重启后亮度、音量、名字都保持上次的值
- [ ] 删掉 `ui.json` 后重启，用默认值 60 / 80 / 我的小伙伴，不报错
- [ ] 手动把文件里 `brightness` 改成 0，重启后亮度是 10%，屏幕没有全黑
- [ ] 仿真里松手时打印 `save`，不崩溃

#### 待办 / 备注

- `include/lv_conf.h` 的 `LV_USE_SYSMON` 还是 `1`（性能测量用），测量已结束，可以改回 `0`。
- 设置列表里「角色与表情 ›」「网络与连接 ›」「关于设备 ›」的点击还没做：前两个要等表情页、设备页实现后再跳转；「关于设备」等 Toast（4.4 节）实现后显示固件版本。
- 状态栏的硬件读取（电池、WiFi、时间）也可以照第 5 步的 `mda_device` 思路抽成接口，这样仿真里也能看到状态栏。

### 5.7 表情系统 `scr_faces`

- 6×2 网格，x8–312，y26 开始，每格约 46×70，间距 5
- 每格：卡片 + S 尺寸机器人 + 10px 文字；选中项白底 + 1.5px 紫色描边 `#8B7CFF`
- 点击：`g_state.userExpr = …`，保存配置，Toast「已切换为「开心」」

**实现步骤**：和设置页一样逐步推进，每步做完在仿真里验证。

#### 总体路线

| 步骤 | 内容 | 做完能看到 | 状态 |
|---|---|---|---|
| 1 | 表情表：12 种表情的名字和各尺寸图片集中到一处 | 编译通过 | ✅ 完成 |
| 2 | 空的表情页 + 菜单栏 😊 切换 | 点 😊 切到表情页 | ✅ 完成 |
| 3 | 6×2 网格：卡片 + 机器人 + 文字 | 网格的样子 | ✅ 完成（用 grid 布局） |
| 4 | 选中状态：点哪个，哪个变成白底 + 紫边 | 点击切换选中 | ✅ 完成 |
| 5 | Toast 提示组件（4.4 节） | 点击后顶部弹出提示，1.6 秒后淡出 | ⬜ |
| 6 | 当前表情全局共享：首页机器人、设置页头像跟着变 | 切回首页，机器人换了表情 | ⬜ |
| 7 | 保存到 `ui.json`，重启后保留；设置页「角色与表情 ›」跳转到这里 | 重启后表情还在 | ⬜ |

第 6、7 步依赖设置页第 6 步的 `mda_ui_config`，可以先做 1～5。

#### 第 1 步：表情表

表情要在多个页面用到：表情页（S）、设置页头像（M）、首页（L）、待机页（XL）。把"表情 → 名字、图片"集中放在一个地方，各页面只认一个编号。

新建 `src/ui/mda_expr.h` / `mda_expr.cpp`：

```cpp
typedef enum {
    MDA_EXPR_HAPPY,      // 开心
    MDA_EXPR_LAUGH,      // 大笑
    MDA_EXPR_SURPRISED,  // 惊讶
    MDA_EXPR_THINK,      // 思考
    MDA_EXPR_LISTEN,     // 倾听
    MDA_EXPR_SING,       // 唱歌
    MDA_EXPR_SLEEP,      // 睡觉
    MDA_EXPR_SAD,        // 难过
    MDA_EXPR_EXCITED,    // 兴奋
    MDA_EXPR_PLAYFUL,    // 调皮
    MDA_EXPR_CONFUSED,   // 疑惑
    MDA_EXPR_TALK,       // 说话
    MDA_EXPR_COUNT,      // 总数，用来写循环和数组大小，不是一种表情
} mda_expr_t;

typedef struct {
    const char *key;               // 保存到配置文件用的英文名："happy"
    const char *name;              // 显示用的中文名："开心"
    const lv_image_dsc_t *img_s;   // 44px，表情网格
    const lv_image_dsc_t *img_m;   // 80px，设置页头像
    const lv_image_dsc_t *img_l;   // 120px，首页
} mda_expr_info_t;

const mda_expr_info_t *mda_expr_info(mda_expr_t expr);   // 越界时返回 HAPPY，避免空指针
mda_expr_t mda_expr_from_key(const char *key);           // "happy" → MDA_EXPR_HAPPY，找不到返回 HAPPY
```

要点：

- 顺序和名字照设计稿 `EXPR` 对象（`docs/design/cores3-ui.html` 第 498 行）
- `.cpp` 里用 `LV_IMAGE_DECLARE` 声明 12 × 3 张图，再写一个 `static const mda_expr_info_t TABLE[MDA_EXPR_COUNT]`
- `_blink`、`talk_close` 是眨眼、说话动画用的帧，网格里不需要，先不放进表里
- **配置文件里存 `key` 字符串，不存编号**：以后在中间插入新表情，编号会变，字符串不会

> Flash：表里引用了图片，链接器就会把它们打包。12 张 S（约 5KB）+ 12 张 M（约 16KB）+ 12 张 L（约 37KB）≈ 700KB，Flash 从约 36% 涨到约 47%，空间够。XL（200px，每张约 100KB）等做待机页时再加。

#### 第 2 步：空页面 + 菜单栏切换

和设置页第 1 步完全一样：

- 新建 `src/ui/screens/mda_faces.cpp`，`mda_faces_create()`：`lv_obj_create(NULL)`，关滚动，背景透明
- `mda_screens.h` 声明
- `mda_ui.cpp` 的 `ui_init()` 里 `s_pages.smile = mda_faces_create();`

菜单栏 😊 对应的就是 `pages->smile`，之前为空所以点了没反应，赋值之后自动就能切换，**菜单栏代码不用改**。

#### 第 3 步：6×2 网格

##### 布局

```
x=8                                                     x=312
y=26  ┌────┐ ┌────┐ ┌────┐ ┌────┐ ┌────┐ ┌────┐
      │ 🤖 │ │ 🤖 │ │ 🤖 │ │ 🤖 │ │ 🤖 │ │ 🤖 │  ← 每格 46×70，间距 5
      │开心│ │大笑│ │惊讶│ │思考│ │倾听│ │唱歌│
      └────┘ └────┘ └────┘ └────┘ └────┘ └────┘
      ┌────┐ ┌────┐ ┌────┐ ┌────┐ ┌────┐ ┌────┐
      │ 🤖 │ │ 🤖 │ │ 🤖 │ │ 🤖 │ │ 🤖 │ │ 🤖 │
      │睡觉│ │难过│ │兴奋│ │调皮│ │疑惑│ │说话│
y=171 └────┘ └────┘ └────┘ └────┘ └────┘ └────┘
```

宽：6 × 46 + 5 × 5 = 301 ≤ 304；高：2 × 70 + 5 = 145，到 y=171，离菜单栏（y 192）还有空隙。

##### 网格容器：flex 自动换行

| 设置 | 写法 | 说明 |
|---|---|---|
| 容器 | `lv_obj_create(scr)` → `remove_style_all`，`pos(8, 26)`，`size(304, 145)` | |
| 布局 | `lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP)` | **横排，一行放不下自动换到下一行**。6 个之后第 7 个放不下，自动去第二行 |
| 间距 | `pad_column 5`，`pad_row 5` | 列间距、行间距各 5 |
| 不滚动 | 关闭 `SCROLLABLE` | |

不用自己算每格的坐标，循环创建 12 个格子，flex 会自动排成两行。（也可以用 LVGL 的 grid 布局，但这里 flex 换行就够了）

##### 一格：`mda_faces_cell_create(parent, expr)`

| 部分 | 写法 |
|---|---|
| 格子 | `lv_obj_create(parent)` → `remove_style_all` → `add_style(&st_card)`；`size 46×70`；`radius 12`；flex 竖排，主轴 `CENTER`，交叉轴 `CENTER`；`pad_row 2`；关滚动 |
| 机器人 | `lv_image_create(cell)`，`lv_image_set_src(img, info->img_s)` |
| 文字 | label，`st_text`，字体 `font_cn_11b`（设计稿 10px 粗体，最接近的是 11px） |
| 可点击 | `lv_obj_t` 默认就能点击，不用加 flag；**但图片和 label 要保证不抢点击**：label 默认不可点击，image 也是，不用处理 |
| 记住编号 | `lv_obj_add_event_cb(cell, cb, LV_EVENT_CLICKED, (void *)(intptr_t)expr)` |

12 个格子用 `for (int i = 0; i < MDA_EXPR_COUNT; i++)` 创建，表情名从 `mda_expr_info()` 取。

##### 字体

12 个表情名里的字是新加的，运行 `bash tools/make_assets.sh fonts`。

#### 第 4 步：选中状态

这里第一次用到 **状态（state）**：不用在点击时手动改背景和边框，而是提前定义好"选中时长什么样"，点击时只切换状态。

##### 4.1 定义一个"选中"样式

```cpp
static lv_style_t st_face_on;   // 文件内静态变量，只初始化一次

lv_style_init(&st_face_on);
lv_style_set_bg_color(&st_face_on, lv_color_white());
lv_style_set_bg_opa(&st_face_on, LV_OPA_COVER);
lv_style_set_border_width(&st_face_on, 2);          // 设计稿 1.5px，LVGL 只能整数，用 2
lv_style_set_border_color(&st_face_on, lv_color_hex(MDA_COLOR_VIOLET));
```

`lv_style_init` 只能调用一次，放在 `mda_faces_create()` 开头即可（页面只创建一次）。也可以放进 `mda_theme.cpp` 的 `theme_init()`，统一管理。

##### 4.2 只在 CHECKED 状态下生效

```cpp
lv_obj_add_style(cell, &st_face_on, LV_STATE_CHECKED);
```

选择器写 `LV_STATE_CHECKED`：**平时用 `st_card` 的半透明白底，进入 CHECKED 状态时，`st_face_on` 叠加上去**，变成实白底 + 紫边。

##### 4.3 点击时切换

```cpp
lv_obj_add_state(cell, LV_STATE_CHECKED);          // 选中
lv_obj_remove_state(old_cell, LV_STATE_CHECKED);   // 取消上一个
```

和菜单栏一样，用一个 `static lv_obj_t *s_checked_cell` 记住当前选中的格子。点的就是当前格子时直接返回。

> 为什么不在回调里直接 `set_style_bg_color`：用状态的话，样式定义集中在一处；以后想加按下效果（`LV_STATE_PRESSED`）也只是多加一个样式，不用改回调。

##### 4.4 开机时的选中项

创建完 12 个格子后，给"当前表情"那一格加上 CHECKED。第 6 步之前先固定选中「开心」。

#### 第 5 步：Toast 提示组件

4.4 节定义的通用组件，表情页是第一个用到的地方。新建 `src/ui/widgets/mda_toast.h/.cpp`，接口：

```cpp
void mda_toast_create();                 // ui_init() 里调用一次，放在菜单栏之后创建（保证盖在最上面）
void mda_toast_show(const char *text);   // 显示 1.6 秒后淡出
```

##### 外观

- 放在 `lv_layer_top()`，顶部居中 `lv_obj_align(toast, LV_ALIGN_TOP_MID, 0, 28)`
- 宽高 `LV_SIZE_CONTENT`（跟着文字走），`pad_hor 12`，`pad_ver 5`，圆角 `LV_RADIUS_CIRCLE`（胶囊形）
- 背景 `MDA_COLOR_INK`，`bg_opa` 约 224（88%）；文字白色，字体 `font_cn_11`
- **默认隐藏**：`lv_obj_add_flag(toast, LV_OBJ_FLAG_HIDDEN)`；不可点击（`remove_flag CLICKABLE`），不挡下面的按钮

##### 显示和自动消失

```cpp
void mda_toast_show(const char *text) {
    lv_label_set_text(s_label, text);
    lv_anim_delete(s_toast, NULL);                   // 连续调用时，先取消上一次还没结束的淡出
    lv_obj_set_style_opa(s_toast, LV_OPA_COVER, 0);  // 恢复不透明
    lv_obj_remove_flag(s_toast, LV_OBJ_FLAG_HIDDEN);
    lv_obj_fade_out(s_toast, 300, 1300);             // 等 1300ms 后，用 300ms 淡出，总共 1.6 秒
}
```

`lv_obj_fade_out(obj, 时长, 延迟)` 是 LVGL 自带的淡出动画。淡出完对象还在，只是透明，下次 show 时把透明度恢复即可。

> 淡出动画每帧都要重画 Toast 那一小块区域（DIRECT 模式下只重画这块），开销很小。

##### 表情页里调用

```cpp
char buf[48];
snprintf(buf, sizeof(buf), "已切换为「%s」", info->name);
mda_toast_show(buf);
```

`buf` 用局部数组没问题：`lv_label_set_text` 会把文字复制一份。「已切换为」「」」这几个字是新的，要重新生成字体。

#### 第 6 步：当前表情全局共享

##### 6.1 状态放在哪里

设计文档里的 `g_state.userExpr`，在本项目里就是 `mda_ui_config()` 里的一个字段（设置页第 6 步定义的配置结构体），加一项：

```cpp
char expr[16] = "happy";   // 当前表情的 key
```

提供两个函数（放在 `mda_expr.cpp` 里）：

```cpp
mda_expr_t mda_expr_current();               // 读：mda_expr_from_key(mda_ui_config().expr)
void mda_expr_set_current(mda_expr_t expr);  // 写：strlcpy 到 config.expr
```

设置页第 6 步还没做的话，可以先用一个 `static mda_expr_t s_current = MDA_EXPR_HAPPY;` 代替，接口不变，以后再换成读写配置。

##### 6.2 其他页面怎么跟着变：进入页面时刷新

不要在表情页里去改首页、设置页的图片（那样表情页就要知道别的页面的内部控件）。改成**每个页面进入时自己刷新**：

```cpp
// 首页：在 mda_home_create() 里注册
lv_obj_add_event_cb(scr, mda_home_on_load, LV_EVENT_SCREEN_LOAD_START, NULL);

static void mda_home_on_load(lv_event_t *e) {
    lv_image_set_src(s_robot, mda_expr_info(mda_expr_current())->img_l);
}
```

- `LV_EVENT_SCREEN_LOAD_START`：页面开始切换进来时触发，滑动动画一开始图片就已经是新的
- 首页的机器人、设置页的头像图片都要存成文件内的 `static` 变量
- 设置页同样注册一个，换成 `img_m`
- 页面之间完全解耦：表情页只负责"改当前表情"，谁要显示谁自己读

> 进入页面时才刷新，所以要保证"改表情"和"看到效果"之间一定会经过一次页面切换。表情页在菜单栏里，从表情页回首页一定会切换，满足条件。

#### 第 7 步：保存 + 设置页跳转

##### 7.1 点击时保存

点击回调里：`mda_expr_set_current(expr)` → `mda_ui_config_save(mda_ui_config())`。点击只触发一次，可以直接写 SD 卡（不像滑块拖动会连续触发）。

开机读配置后，`mda_expr_current()` 就是上次保存的表情，表情页、首页、设置页创建时都按它显示。

##### 7.2 设置页「角色与表情 ›」跳转到表情页

现在页面切换的代码在菜单栏里：菜单栏知道当前高亮哪个图标。设置页的行直接调用 `lv_screen_load_anim` 跳过去的话，**菜单栏的高亮还停在 ⚙**，对不上。

所以这里要把切换抽成统一入口（4.2 节提到的 `ui_go`）：

| 位置 | 新增 |
|---|---|
| `mda_ui.h` | `typedef enum { MDA_PAGE_HOME, MDA_PAGE_SMILE, MDA_PAGE_MIC, MDA_PAGE_SETTINGS } mda_page_t;` 和 `void mda_ui_go(mda_page_t page);` |
| `mda_menubar.h` | `void mda_menubar_set_active(mda_page_t page);`：只改高亮，不切页面 |
| `mda_ui.cpp` | `mda_ui_go()`：选出页面对象 → 判断方向 → `lv_screen_load_anim` → `mda_menubar_set_active(page)` |
| `mda_menubar.cpp` | 点击回调改成只调用 `mda_ui_go(对应页面)` |
| `mda_settings.cpp` | 「角色与表情」行注册 `LV_EVENT_CLICKED`，调用 `mda_ui_go(MDA_PAGE_SMILE)` |

这样所有切换都走 `mda_ui_go`，高亮、方向、动画只在一个地方维护。

#### 验证

- [ ] 点菜单栏 😊 切到表情页，6×2 网格整齐，12 个名字都没有方块字
- [ ] 默认「开心」是白底 + 紫边，其他格是半透明卡片
- [ ] 点「大笑」：选中框移过去，顶部弹出「已切换为「大笑」」，约 1.6 秒后淡出
- [ ] 快速连点几个表情：Toast 文字跟着变，不会闪烁或提前消失
- [ ] 切回首页：机器人变成大笑；切到设置页：头像也是大笑
- [ ] 设置页点「角色与表情 ›」跳到表情页，菜单栏 😊 同时高亮
- [ ] 重启后表情还是大笑

### 5.8 待机全屏 `scr_idle`

- 全屏背景图，左上 20px 粗体大时钟（`#2A2F4A` 55% 透明度），中间 XL 机器人（浮动 + 眨眼），底部 10px「轻触屏幕返回」
- 进入条件：
  1. 首页点击机器人
  2. 无操作 40 秒：在 `loop` 里检查 `lv_display_get_inactive_time(NULL) > 40000`（正在播放或对话中除外）
  3. 光线变暗（LTR-553）
- 点任意位置回首页

---

## 6. 机器人组件和硬件服务（M3–M8）

### 6.1 机器人组件 `widgets/mda_robot.cpp`（M3）

```cpp
enum class RobotSize { S, L, XL };
lv_obj_t *robot_create(lv_obj_t *parent, RobotSize size, bool bob = true);
void robot_set_expr(lv_obj_t *robot, Expr e);
void robot_set_talking(lv_obj_t *robot, bool on);
```

实现要点：
- 内部就是一个 `lv_image`，用一张查找表把「表情 × 尺寸」映射到图片：
  ```cpp
  struct RobotImgs { const lv_image_dsc_t *normal, *blink; };
  static const RobotImgs IMG_L[(int)Expr::COUNT] = { { &robot_happy_L, nullptr }, { &robot_surprised_L, &robot_surprised_blink_L }, … };
  ```
- **浮动**：`lv_anim` 改 y，幅度 4px，单程 1.6s，`lv_anim_path_ease_in_out`，往返无限循环
- **眨眼**：每个机器人一个 `lv_timer`，随机 2.5–5.5 秒触发一次，换成 blink 图，120ms 后换回；表情没有 blink 图就跳过
- **说话**：`robot_set_talking(true)` 时启动 120ms 定时器，在 `talk` 和 `talk_close` 两张图之间切换
- 页面隐藏时暂停这些定时器（`lv_timer_pause`），只让当前页的机器人动

### 6.2 电源和 WiFi（M4）
- 电量：`M5.Power.getBatteryLevel()`，是否在充电：`M5.Power.isCharging()`，每 5 秒读一次
- WiFi 信号：`WiFi.RSSI()`，每 2 秒读一次
- 状态变化时发 `EvtType::Battery`，由 UI 更新状态栏

### 6.3 IMU 摇晃检测（M4）
- `M5.Imu.getAccel(&ax, &ay, &az)`，每 20ms 读一次
- 计算加速度模长 `|a|`，在 0.3 秒窗口里有 3 次以上超过 2.0g，就判定为摇晃
- 触发后表情依次变成「惊讶」(1.2s) → 「疑惑」(1.4s)，同时让机器人图片左右抖动（`lv_anim` 改 x，±4px，6 次）
- 加 3 秒冷却时间，避免连续触发

### 6.4 光照和温湿度（M4）
- **光照 LTR-553**：CoreS3 板载，在内部 I2C 上（`M5.In_I2C`）。可以用 M5 官方 `M5CoreS3` 库里的 LTR5xx 驱动，或者照数据手册直接读寄存器（地址和寄存器以手册为准）
  - 光照持续 2 秒低于 10 lx → `dark = true`（进入待机 + 睡觉）；高于 30 lx 恢复（两个阈值防止来回跳）
- **温湿度**：板上没有，需要接 ENV Unit（SHT40）到 Port A（`M5.Ex_I2C`），用 `m5stack/M5Unit-ENV` 库；没接的时候设备页显示「--」
- 每 2 秒读一次，推进趋势图

### 6.5 陪伴模式：音频播放（M5）
- 参考 M5Unified 自带的示例 `examples/Advanced/MP3_with_ESP8266Audio`：它实现了一个把 ESP8266Audio 的输出接到 `M5.Speaker` 的类，直接拿来用
- 播放放在**单独的 FreeRTOS 任务**里（解码比较耗时），通过事件告诉 UI 进度和状态
- 播放列表：启动时扫描 `/data/media/<分类>/` 目录，文件名就是标题（`小王子-星星的故事.mp3` 按 `-` 拆成标题和副标题）
- **坑**：CoreS3 的扬声器和麦克风共用 I2S，**不能同时工作**。录音前 `M5.Speaker.end()` 再 `M5.Mic.begin()`，播放前反过来。对话开始时要先暂停音乐

### 6.6 舵机（M6）
- 两个舵机（水平、俯仰）接 Port B 或 Port C 的 GPIO，具体引脚以 CoreS3 官方引脚图为准；**舵机单独接 5V 电源，和 CoreS3 共地**
- `ESP32Servo`：`servo.attach(pin, 500, 2400)`
- 平滑：每 20ms 让当前角度朝目标角度走一小步（比如每步最多 3°），不要直接跳到目标
- 限位：水平 0–180°，俯仰 −30–+30°（俯仰舵机实际角度 = 90 + pitch）
- 自动跟随：v1 先做设计稿里的左右扫动；以后接人脸检测（见第 9 节）

### 6.7 摄像头（M7）
- 用 esp32-camera（arduino-esp32 3.x 已内置），CoreS3 的引脚配置参考 M5 官方 CoreS3 摄像头示例
- 格式：`PIXFORMAT_RGB565`，分辨率 `FRAMESIZE_QVGA`（320×240），帧缓冲放 PSRAM（`fb_location = CAMERA_FB_IN_PSRAM`）
- 显示：准备一个 `lv_image_dsc_t`（`LV_COLOR_FORMAT_RGB565`，320×240），每帧把摄像头数据拷进来，然后 `lv_image_set_src` + `lv_obj_invalidate`。图片放在 304×150 的容器里，`lv_obj_set_y(img, -45)` 取中间部分
- 摄像头输出的 RGB565 字节序可能和 LVGL 相反，颜色不对就用 `lv_draw_sw_rgb565_swap()` 转一下
- 帧率目标 10–15fps 就够了。只在摄像头标签可见时采集
- 拍照：`esp_camera_fb_get()` 后转 JPEG（`frame2jpg`）写入 `/data/photos/时间戳.jpg`，然后 Toast 提示

### 6.8 AI 对话（M8）
```
按下按钮 → 停止音乐 → M5.Mic 以 16kHz 录音 → 通过 WebSocket 分片上传 PCM
松开按钮 → 发送结束标记 → 中转服务：语音识别 → 大模型 → 语音合成
        → 返回 {识别文字} {回答文字} {音频}
设备：显示我的话 → 机器人「思考」→ 收到回答后打字机显示 + 播放音频 + 机器人「说话」→ 播放完回到「开心」
```
- 设备端用 `links2004/WebSockets` 的 `WebSocketsClient`，在单独任务里收发；UI 只通过事件 `ChatAsr / ChatReply / ChatDone` 更新
- 中转服务先在电脑上用 Python（FastAPI + WebSocket）实现，语音识别、大模型、语音合成选哪家都可以替换；API Key 只放服务端
- 录音缓冲：16kHz × 16bit × 10 秒 = 320KB，放 PSRAM；或者边录边发，只用几 KB 的环形缓冲

---

## 7. 和手机网页联动（可选，M9）

设计稿右侧的「WebSocket 日志」就是建议的协议，设备端在 81 端口开 WebSocket 服务：

| 方向 | 消息 |
|---|---|
| 设备 → 手机 | `{"type":"state","expr":"happy","temp":26.5,"hum":62,"lux":120,"batt":78}`（每 6 秒） |
| 设备 → 手机 | `{"type":"page","v":"music"}` `{"type":"player","playing":true,"title":"小王子","pos":84}` |
| 设备 → 手机 | `{"type":"servo","yaw":100,"pitch":5}` `{"type":"audio","action":"start","rate":16000}` |
| 手机 → 设备 | `{"cmd":"expr","v":"sing"}` `{"cmd":"servo","yaw":100,"pitch":5}` `{"cmd":"play","title":"小王子"}` |

手机端直接用之前的 `index.html`，把模拟数据换成 WebSocket 收到的数据。放到 SD 卡 `/data/www/`，由现有 Web 服务器提供。

---

## 8. 性能和内存清单

| 项目 | 建议 |
|---|---|
| LVGL 绘制缓冲 | 2 × 320×40×2 = 50KB，**放内部 RAM 且可 DMA**（PSRAM 做 DMA 慢，还可能出错） |
| 图片 | 编译进固件的 `const` 数据直接从 Flash 读，不占 RAM |
| 摄像头帧、录音缓冲、MP3 解码缓冲 | 放 PSRAM |
| 阴影 | 最耗性能的效果。帧率不够时先把 `shadow_width` 改成 0，或者只给少数卡片加 |
| 半透明 | 有成本但可以接受；背景用图片，不要实时渲染渐变 |
| 动画 | 同一时间只让当前页面的机器人动；浮动动画只重绘机器人那一小块区域 |
| 图片缩放/旋转 | `lv_image_set_scale/rotation` 很耗 CPU。常用尺寸直接导出对应大小的图，只在控制页用旋转 |
| 刷新频率低的数据 | 传感器、进度条、时钟用 `lv_timer` 按秒刷新，不要在每帧里改 |
| 观察 | 开发时开着 `LV_USE_PERF_MONITOR`，每个里程碑记录一次帧率和 `ESP.getFreeHeap()` / `ESP.getFreePsram()` |

---

## 9. 进阶（M9 之后）

1. **挥手动画**：把机器人拆成三层图片（身体、右臂、头部和表情），右臂用 `lv_image_set_pivot` + 旋转动画来回摆
2. **人脸跟随**：摄像头做人脸检测（ESP-DL，或者把画面发给中转服务），根据人脸在画面中的水平位置调整舵机目标角度
3. **语音唤醒**：arduino-esp32 3.x 自带 `ESP_SR`，可以做「小伙伴」唤醒词，替代按住说话
4. **OTA 升级**：工程里已经有 Web 服务器，可以加一个上传固件的页面；需要的话改分区表留出两个应用分区
5. **素材放 SD 卡**：图片越来越多时，改成 `--ofmt BIN` 放到 SD 卡，打开 `LV_USE_FS_ARDUINO_SD` 按需加载

---

## 10. 常见问题

| 现象 | 可能原因 |
|---|---|
| 颜色发紫、红蓝颠倒、花屏 | RGB565 字节序不对，见 2.1 |
| 屏幕闪烁、有残影 | 还有代码直接调用 `M5.Display` 绘图，和 LVGL 冲突 |
| 触摸没反应 | `loop()` 里没有调用 `M5.update()` |
| 点击位置偏移 | `setRotation` 和 LVGL 分辨率不一致（必须是 1 → 320×240） |
| 中文显示成方框 | 这个字没有导出到字体里，重新跑 `make_assets.sh` |
| 随机重启 | 舵机没单独供电；或者在后台任务里调用了 `lv_*` 函数 |
| 播放音乐时录不了音 | 扬声器和麦克风共用 I2S，见 6.5 |
| 编译后固件太大 | 见 3.4 的优化；`pio run -t size` 查看各部分占用 |
| 动画卡顿 | 打开性能监视看帧率，先去阴影，再检查是否在每帧里做了重活 |
