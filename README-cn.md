# LCD2USB++ —— 专业 C++ 库

一个现代化的 C++ 库，用于控制 LCD2USB 设备。本库为 USB 字符液晶显示屏提供简洁的面向对象接口，并具备完善的字体管理与字符动画能力。

**版本：** `v-0.1.0` · **作者：** Yifan Yang · **许可证：** MIT OR Apache-2.0 · [English README](README.md)

## 🚀 功能特性

- 🖥️ **完整的 LCD 控制**：文本显示、光标定位、屏幕管理
- 🌍 **UTF-8 多语言文本**：解码、渲染并滚动显示任意语言文本（每个码点最多占 2 个 HD44780 字符位）。完整设计见 `NOTES-utf8.md`，简述为：UTF-8 解码器 → 码点到字形表 → 显存差异比对 → CGRAM LRU 缓存，字体来源为 TTF 或预打包的 `.glyphs` 文件。
- 🧰 **`tools/` 目录**：`tools/ufo_to_glyphs.py` 将 UFO 像素字体转换为紧凑的 24 字节/字形二进制包；`tools/glyph_inspect` 以 ASCII 艺术预览任意码点。离线可用 —— 若使用字形包，运行时无需 FreeType。
- 🎨 **自定义字符**：创建并上传 5x8 像素自定义字符
- 🔄 **字符动画**：基于帧的自定义字符动画
- 📚 **字体管理**：专业字体收藏系统，支持二进制文件读写
- 🖼️ **图像处理**：BMP 转 LCD 转换器，支持自动阈值检测
- 🎬 **GIF 动画**：GIF 动画播放器，支持灵活的网格尺寸与帧率控制
- 🚀 **现代 C++17**：干净的 API，RAII 与完善的错误处理
- 🔧 **CMake 构建系统**：专业构建配置
- 📱 **跨平台**：支持 macOS 与 Linux
- ✅ **硬件实测**：已通过真实 LCD2USB 设备验证

## 📦 项目结构

```
lcd2usb++/
├── CMakeLists.txt              # 根构建配置
├── README.md                   # 英文说明（本文件的中文版为 README-cn.md）
├── Manual-FontEditor.md        # 字体编辑器文档
├── include/lcd2usb/            # 公共库头文件
│   ├── lcd2usb_lib.hpp        # 库主头文件
│   ├── lcd2usb.hpp            # 旧版兼容头文件
│   ├── lcd_display.hpp        # 核心 LCD 通信
│   ├── custom_char.hpp        # 自定义字符系统
│   ├── font_editor.hpp        # 字体管理系统
│   ├── multi_char_display.hpp # 网格显示辅助类
│   └── scrolling_text.hpp     # 文本滚动系统
├── src/                        # 库实现
│   ├── lcd_display.cpp        # LCD 通信代码
│   ├── custom_char.cpp        # 自定义字符代码
│   ├── font_editor.cpp        # 字体管理代码
│   ├── multi_char_display.cpp # 网格显示实现
│   ├── scrolling_text.cpp     # 文本滚动代码
│   └── CMakeLists.txt         # 库构建配置
├── examples/                   # 示例程序
│   ├── basic_test.cpp         # 基础 LCD 功能
│   ├── custom_char_demo.cpp   # 自定义字符
│   ├── animated_demo.cpp      # 字符动画
│   ├── font_editor_demo.cpp   # 高级字体管理
│   ├── font_editor_demo_simple.cpp # 简单字体示例
│   ├── storyteller.cpp        # 带滚动的文本阅读器
│   ├── bmp_to_lcd_demo.cpp    # BMP 图像查看器
│   ├── gif_to_lcd_demo.cpp    # GIF 动画播放器
│   ├── full_demo.cpp          # 完整功能展示
│   ├── scrolling_demo.cpp     # 文本滚动示例
│   ├── system_monitor.cpp     # 系统状态显示
│   └── CMakeLists.txt         # 示例构建配置
├── assets/                     # 演示文件
│   ├── ChillBitmap7x.ttf      # OFL 位图字体（UTF-8 示例用）
│   ├── boutique7x7.glyphs     # 预打包字形文件
│   └── story_utf8.txt         # 多语言示例故事
├── cmake/                      # CMake 配置
│   └── lcd2usbConfig.cmake.in
└── trunc/                      # 旧代码存档
```

> 说明：`cc.bmp`、`cc.gif` 等展示图片与长篇示例文本（如 `lr-book2.md`）属于个人本地文件：已被 git 忽略，不在仓库中 —— 请通过 `--file` 或图像查看器的文件参数传入你自己的文件。

## 🛠️ 前置要求

- **CMake 3.16** 或更高
- 支持 **C++17** 的编译器（GCC 7+、Clang 5+、MSVC 2019+）
- **libusb** 开发库
- **giflib**（GIF 动画支持）
- **ncurses**（GIF 播放器终端界面）
- 测试需要 **LCD2USB 硬件**

### 安装依赖

**macOS：**
```bash
brew install cmake libusb-compat giflib ncurses
```

**Ubuntu/Debian：**
```bash
sudo apt install cmake build-essential libusb-dev libusb-1.0-0-dev libgif-dev libncurses-dev
```

**Fedora/RHEL：**
```bash
sudo dnf install cmake gcc-c++ libusb-devel libusb1-devel giflib-devel ncurses-devel
```

## 🔨 构建

```bash
# 克隆或下载本项目
cd lcd2usb++

# 创建构建目录
mkdir build && cd build

# 用 CMake 配置
cmake ..

# 编译库与示例
make -j4

# 编译产物位置：
# - src/liblcd2usb.a     （静态库）
# - examples/            （示例可执行文件）
```

## 📚 库的使用

### 基本 LCD 操作

```cpp
#include <lcd2usb/lcd2usb_lib.hpp>
using namespace LCD2USB;

int main() {
    // 初始化 LCD 显示
    LCDDisplay display;
    
    // 基本文本输出
    display.clear();
    display.setCursor(0, 0);
    display.print("Hello, LCD2USB++!");
    display.setCursor(0, 1);
    display.print("Modern C++ API");
    
    return 0;
}
```

### 自定义字符

```cpp
#include <lcd2usb/lcd2usb_lib.hpp>
using namespace LCD2USB;

int main() {
    LCDDisplay display;
    
    // 使用工厂方法创建自定义字符
    auto heart = CharacterFactory::createHeart();
    auto spinner = CharacterFactory::createSpinner(0);
    
    // 上传到 LCD 的自定义字符存储区（CGRAM）
    display.uploadCustomCharacter(0, heart);
    display.uploadCustomCharacter(1, spinner);
    
    // 显示自定义字符
    display.clear();
    display.setCursor(0, 0);
    display.print("Custom chars: ");
    display.print(static_cast<char>(0)); // 爱心
    display.print(static_cast<char>(1)); // 旋转器
    
    return 0;
}
```

### 字体管理

```cpp
#include <lcd2usb/lcd2usb_lib.hpp>
using namespace LCD2USB;
using namespace LCD2USB::FontEdit;

int main() {
    LCDDisplay display;
    FontManager font_manager("My Collection");
    
    // 创建并添加字符
    auto heart = CharacterFactory::createHeart();
    CharacterBitmap heart_bitmap("Heart");
    heart_bitmap.setDescription("Love symbol");
    heart_bitmap.setFrame(0, heart.toBytes());
    
    font_manager.addCharacter('H', heart_bitmap);
    
    // 保存为二进制文件
    font_manager.saveToFile("my_font.lcf");
    
    // 从文件加载
    FontManager loaded_font;
    loaded_font.loadFromFile("my_font.lcf");
    
    return 0;
}
```

## 🎯 示例导览

本库附带的完整示例，演示 LCD2USB++ 的各个方面：

### 1. 基础测试（`examples/basic_test.cpp`）

**用途**：演示 LCD 基本操作与 USB 通信。

**特性**：
- USB 设备检测与初始化
- 基本文本显示与光标定位
- 回显（echo）测试验证通信
- 错误处理与优雅关闭

**运行**：
```bash
cd build/examples
./basic_test
```

**预期输出**：
```
=== LCD2USB++ Basic Test ===
Found LCD2USB device on bus 008 device 017
LCD2USB device opened and communication verified
LCD2USB device found and initialized!
Echo test successful!
Firmware version 1.9
Basic text displayed. Waiting 3 seconds...
Basic test completed successfully!
```

### 2. 自定义字符演示（`examples/custom_char_demo.cpp`）

**用途**：展示自定义字符创建系统。

**特性**：
- 创建爱心、进度条与箭头字符
- 基于图案的字符定义
- 十六进制字节显示与校验
- 多字符上传与显示

**运行**：
```bash
./custom_char_demo
```

**LCD 上将看到**：
- 爱心字符（❤️）
- 显示进度的动画进度条
- 方向箭头
- 图案拆解及十六进制值

### 3. 动画演示（`examples/animated_demo.cpp`）

**用途**：演示基于帧的字符动画。

**特性**：
- 多帧字符动画
- 跳跃小人动画
- 旋转加载效果
- 信号处理实现优雅退出（Ctrl+C）

**运行**：
```bash
./animated_demo
# 按 Ctrl+C 停止
```

### 4. 字体编辑器演示（`examples/font_editor_demo_simple.cpp`）

**用途**：展示专业字体管理系统。

**特性**：
- 字体收藏集的创建与管理
- 二进制文件读写（.lcf 格式）
- 字符元数据（名称、描述、时间戳）
- 与显示系统集成

**运行**：
```bash
./font_editor_demo
```

**演示流程**：
1. 创建包含爱心与音符字符的字体集
2. 在 LCD 上显示字符
3. 将字体集保存为 `demo_font.lcf`
4. 加载已保存的字体并校验完整性
5. 显示元数据信息（作者、创建时间、字符数）

### 5. 故事讲述者（`examples/storyteller.cpp`）

**用途**：高级文本文件阅读器，带滚动显示与进度可视化。

**特性**：
- 以 256 字节分块加载任意文本文件（txt、md 等），节省内存
- 可配置屏幕宽度（2–40 字符）
- 可调滚动速度（1 秒以上/次）
- 使用自定义字符显示可视化进度条：
  - 上行：当前块进度
  - 下行：全文进度
- 自动换行的垂直滚动
- 命令行参数解析
- 信号处理实现优雅关闭

**命令行选项**：
```bash
./storyteller [options]
Options:
  --file <path>        Text file to display (default: assets/story.txt)
  --width <n>          Screen width in characters, 2-20 (default: 16)
  --rows <n>           Number of rows, 1-4 (default: 2)
  --speed <seconds>    Scroll speed in seconds (default: 2)
  --help              Show this help message
```

**用法示例**：
```bash
# 使用默认参数
./storyteller

# 自定义文件与更快的滚动
./storyteller --file my_story.txt --speed 1

# 20 字符宽 4 行显示，慢速滚动
./storyteller --file novel.md --width 20 --rows 4 --speed 3

# 显示帮助
./storyteller --help
```

### 6. BMP 图像查看器（`examples/bmp_to_lcd_demo.cpp`）

**用途**：在 LCD 上以自定义字符网格显示静态 BMP 图像，带自动阈值检测。

**特性**：
- 灵活的网格尺寸（--width、--height）
- 双线性插值平滑缩放
- Otsu 法自动阈值检测
- 手动阈值控制（--threshold 0-255）
- 图像缩放系数（--scale）
- 支持 24 位与 32 位 BMP
- Ctrl+C 优雅退出
- 启动时清空 CGRAM 以清除残留图案

**命令行选项**：
```bash
./bmp_to_lcd_demo [options] <bmp_file>
Options:
  --width <n>         Grid width (default: 2)
  --height <n>        Grid height (default: 2)
  --scale <n>         Image scale factor (default: 1)
  --threshold <0-255> Manual threshold (default: auto-detect)
```

**用法示例**：
```bash
# 以 2x4 网格显示（8 个字符，推荐）
./bmp_to_lcd_demo --width 2 --height 4 image.bmp

# 2x2 网格 + 自动阈值
./bmp_to_lcd_demo --width 2 --height 2 image.bmp
```

**网格尺寸建议**：静态图像推荐 2×3（6 字符）、2×2（4 字符）、1×7（7 字符）；更大的网格仅适用于静态图像。原因见下文「HD44780 硬件限制」。

### 7. GIF 动画播放器（`examples/gif_to_lcd_demo.cpp`）

**用途**：在 LCD 上播放 GIF 动画，终端通过 ncurses 实时显示状态。

**特性**：
- 可配置帧率的 GIF 动画播放
- 灵活的网格尺寸
- 终端实时状态：帧计数、帧率、进度条
- 自动/手动阈值
- 帧数限制与循环控制
- Ctrl+C 优雅退出

**命令行选项**：
```bash
./gif_to_lcd_demo [options] <gif_file>
Options:
  --width <n>         Grid width (default: 2)
  --height <n>        Grid height (default: 2)
  --fps <n>           Frames per second (default: 10)
  --scale <n>         Image scale factor (default: 1)
  --threshold <0-255> Manual threshold (default: auto-detect)
  -m <n>              Maximum frames to display
  --loop              Loop animation continuously
```

**⚠️ 动画关键限制**：推荐网格 ≤7 字符（2×3、2×2、1×7）；更大的网格在动画中会出现视觉伪影，因为每帧最多使用 7 个不同的字符图案（我们跳过 0 号槽位以避免显示问题）。

### 8. RsTask 看板（`examples/lcd-rstasks-kanban.cpp`）

**用途**：在 LCD 上显示 [RsTask](https://github.com/yfyang86/rsTask) 工作区（`rstasks` crate / CLI）的实时看板 —— 第一行显示 queue / work / review / done 计数，下方为活动任务，每个任务都带自定义字符图标。需要 `rstasks` 二进制在 `PATH` 中。

**特性**：
- 通过运行 `rstasks --db <db> board` 获取看板；`--rstasks-path` 接受工作区目录（含 `.rstask/`）或直接的 `rstask.sqlite` 文件路径，可多次指定 —— 以第一个存在的路径为准
- 第 0 行始终是摘要行：`Q:n W:n R:n D:n` 计数，空间足够时显示翻页标记与时钟
- 每个任务都有一个确定性的 5×8「identicon」自定义字符（由任务 id 的 FNV-1a 哈希生成，水平镜像对称），字形底部两行烙有泳道标记：queue = 底边两角，work = 向上箭头，review = 菱形
- 默认模式分页显示活动任务（work → review → queue，done 只计数）；每次数据刷新翻页
- `--details` 跑马灯模式 —— 仅对超出屏幕的行生效；内容能放下的行保持静止：
  - 2–3 行显示屏让所有活动任务逐行向上滚动流过
  - 4 行显示屏每行一个泳道（WORK | REVIEW | QUEUE），任务横向走马灯，图标随文字一起滚动
- 看板数据每 `--duration` 分钟刷新；动画在两次刷新之间以约 4 fps 运行
- `--dry-run` 终端预览（图标显示为 `#`），`--once` 渲染单帧，SIGINT/SIGTERM 优雅退出

**命令行选项**：
```bash
./lcd_rstasks_kanban [options]
Options:
  --rstasks-path PATH  RsTask 工作区目录（含 .rstask/）或 rstask.sqlite
                       文件路径。可多次指定；以第一个匹配为准（默认: .）
  --rows N             LCD 行数（默认 4）
  --width N            LCD 列数（默认 20）
  --duration MIN       看板刷新周期，单位分钟（默认 10）
  --details            跑马灯详情模式（见上文）
  --once               渲染单帧后退出
  --dry-run            在终端打印帧，不驱动 LCD
  --help               显示帮助
```

**用法示例**：
```bash
# 4x20 LCD 上显示某工作区的实时看板
./lcd_rstasks_kanban --rstasks-path ~/projects/my-workspace

# 跑马灯详情模式，每 10 分钟刷新
./lcd_rstasks_kanban --rstasks-path ~/projects/my-workspace --details --duration 10

# 在终端预览布局（无需硬件）
./lcd_rstasks_kanban --rstasks-path . --details --dry-run --once
```

**预期输出**（4×20，`--details` 模式；`#` 表示任务的 identicon 图标）：

```
+--------------------+
|Q:4 W:1 R:1 D:1     |
|W # 184e Write kanb|
|R # 89c2 Audit UTF-|
|Q # 1d29 Fix LCD ro|
+--------------------+
```

**技术细节**：
- **CLI 集成**：解析 `rstasks board` 的 `== status (n) ==` 小节与任务表；工作区目录自动解析到 `.rstask/rstask.sqlite`
- **Identicon**：由任务 id 的 FNV-1a 哈希生成 `CustomChar` 5×8 图案；上传到 CGRAM 槽位 1–7（跳过 0 号槽位，原因见 HD44780 一节），仅在上传内容变化时重新上传；单帧需要的不同图标超过 7 个时，多余的退化为空格
- **行宽精确**：每行填充/截断到恰好 `--width` 字节，避免 HD44780 光标折行到下一行
- **双时钟设计**：看板数据按 `--duration`（分钟）刷新，而跑马灯动画每 250 ms 走一格

---

## ⚠️ HD44780 硬件限制

### 8 个 CGRAM 槽位的硬约束

HD44780 LCD 控制器有一个影响自定义字符显示的硬件限制：

**关键事实**：
- 只有 **8 个 CGRAM 槽位**（编号 0–7）
- 每个槽位存储一个 5×8 像素自定义字符图案（8 字节）
- 显示位置存的是**槽位编号**，不是像素图案本身
- 所有引用同一槽位的位置**实时显示同一图案**
- **字符代码 0 在某些 LCD 控制器上可能无法正常显示**，因此我们只用槽位 1–7（实际可用 7 个）

### 为什么这很重要

显示自定义字符时：
1. 将 8 字节图案上传到某个 CGRAM 槽位（0–7）
2. 告诉 LCD 在某个位置显示该槽位编号
3. LCD 只是**引用**该槽位 —— 并不复制图案
4. 如果你改写了槽位 7，**所有显示槽位 7 的位置都会立即变化**

### 对网格显示的影响

**静态图像**（bmp_to_lcd_demo）：
- ✅ **≤7 个字符**：每个字符独占一个槽位（1–7），显示完美
- ⚠️ **>7 个字符**：使用「槽位 7 复用」技术 —— 字符 0–5 放入槽位 1–6，然后逐个把字符 6 及之后的图案动态上传到槽位 7 并立即显示。静态图像显示后不再变化，因此可行

**动画**（gif_to_lcd_demo）：
- ✅ **≤7 个字符**：动画流畅，无伪影
- ❌ **>7 个字符**：严重伪影 —— 为字符 6 更新槽位 7 时，之前显示的字符 7、8…… 都会被改变

### 推荐配置

**最佳选择**（≤7 个字符）：
- **2×3 网格** = 6 个字符 ✅（推荐）
- **2×2 网格** = 4 个字符 ✅
- **1×7 网格** = 7 个字符 ✅（用满全部可用槽位）

**仅适用于静态图像**（>7 个字符）：
- **2×4** = 8 个字符 ⚠️、**3×3** = 9 个字符 ⚠️、**4×4** = 16 个字符 ⚠️（会有警告，使用槽位复用）

### 技术原理

`MultiCharDisplay` 辅助类实现了两种策略：

**策略 1：直接显示**（≤7 个字符）：所有字符分别上传到槽位 1–7（跳过 0），然后显示字符代码 1–7。

**策略 2：槽位 7 复用**（>7 个字符）：前 6 个字符放入槽位 1–6；从第 7 个字符起，动态复用槽位 7 —— 每次上传后**立即**在对应位置显示，再上传下一个。

**为什么跳过 0 号槽位？** 字符代码 0（空字符）在某些 HD44780 控制器上可能无法正常显示或被特殊处理。只用槽位 1–7 可以确保在所有 HD44780 兼容 LCD 上可靠显示。

**为什么策略 2 对动画无效？** 动画帧需要更新所有位置；更新位置 (3,3) 的槽位 7 时，位置 (2,3)、(1,3) 等也会同时改变，造成闪烁与错误图案。

## 🔧 API 参考

### 核心类

#### `LCD2USB::LCDDisplay`
LCD 通信与控制的主类。

**主要方法**：
- `clear()` —— 清屏
- `setCursor(int col, int row)` —— 设置光标位置
- `print(const std::string& text)` —— 显示文本
- `uploadCustomCharacter(int slot, const CustomChar& char)` —— 上传自定义字符

#### `LCD2USB::CustomChar`
用于创建 5x8 像素自定义字符。

**主要方法**：
- `setPixel(int x, int y, bool value)` —— 设置单个像素
- `toBytes()` —— 转换为字节数组以便上传
- `clear()` —— 清除所有像素

#### `LCD2USB::CharacterFactory`
预置字符模板的工厂类。

**可用字符**：
- `createHeart()` —— 爱心
- `createNote()` —— 音符
- `createSpinner(int frame)` —— 加载旋转器（4 帧）
- `createMatchGuy(int frame)` —— 跳跃小人（4 帧）
- `createDancer(int frame)` —— 跳舞小人（2 帧）

#### `LCD2USB::FontEdit::FontManager`
专业字体收藏集管理。

**主要方法**：
- `addCharacter(uint8_t index, const CharacterBitmap& char)` —— 添加字符
- `saveToFile(const std::string& filename)` —— 保存为 .lcf 文件
- `loadFromFile(const std::string& filename)` —— 从 .lcf 文件加载
- `getCharacterCount()` —— 获取字符数量

## 🔗 集成到你的项目

### 在 CMake 项目中使用

在你的 `CMakeLists.txt` 中添加：

```cmake
# 查找库
find_package(LCD2USB REQUIRED)

# 链接到你的目标
target_link_libraries(your_target PRIVATE LCD2USB::lcd2usb)
```

### 手动编译

```bash
# 编译你的程序
g++ -std=c++17 your_app.cpp -I/path/to/lcd2usb/include -L/path/to/lcd2usb/build/src -llcd2usb -lusb
```

## 🐛 故障排除

### 找不到设备
```
Error: LCD2USB device not found
```
**解决方法**：
- 检查 USB 连接
- 换一个 USB 端口
- Linux 下用 sudo 运行：`sudo ./your_app`
- 检查设备权限

### 编译问题
```
libusb not found
```
**解决方法**：
- 安装 libusb 开发包
- 查看 CMake 输出中缺失的具体依赖
- 确认编译器支持 C++17

### Linux 下权限不足
```bash
# 将用户加入 dialout 组
sudo usermod -a -G dialout $USER
# 重新登录

# 或创建 udev 规则
echo 'SUBSYSTEM=="usb", ATTRS{idVendor}=="0403", ATTRS{idProduct}=="c630", MODE="0666"' | sudo tee /etc/udev/rules.d/99-lcd2usb.rules
sudo udevadm control --reload-rules
```

### GIF/BMP 演示问题

**"Warning: Grid has >7 characters..."**
- 网格超过 7 个字符时的预期行为，详见「HD44780 硬件限制」
- 动画请使用 2×3、2×2 或 1×7 网格

**动画中出现伪影**
- 网格尺寸超过 7 个字符（HD44780 CGRAM 限制，加上我们避开 0 号槽位）
- 解决：`--width 2 --height 3`（6 字符）或更小

**BMP 文件不支持**
- 仅支持 24 位与 32 位 BMP
- 转换图片：`convert input.png -type truecolor output.bmp`

## 📖 字体文件格式

本库使用自定义二进制格式（`.lcf`）存储字体：

**结构**：
- 文件头：魔数签名、版本、作者、时间戳
- 字符数据：索引、名称、描述、帧数据
- 元数据：创建信息、字符数

**文件扩展名**：`.lcf`（LCD Character Font）

## 🤝 参与贡献

欢迎贡献！可改进的方向：

1. **ScrollingText 系统**：实现文本滚动与跑马灯效果
2. **高级动画**：更多字符动画预设
3. **字体导入/导出**：支持其他字体格式
4. **文档**：用 Doxygen 生成 API 文档
5. **平台支持**：增强 Windows 支持

## 📄 许可证

采用以下许可证**任选其一**：

- Apache License, Version 2.0（见 [LICENSE-Apache-2.0](LICENSE-Apache-2.0)）
- MIT License（见 [LICENSE-MIT](LICENSE-MIT)）

即 `MIT OR Apache-2.0`。

### 许可说明

虽然本库驱动 LCD2USB 设备，但其中**不含**原始 LCD2USB 项目主机端工具（其固件与工具为 GPL）的任何代码：`include/lcd2usb/lcd2usb.hpp` 中的 USB 标识符与命令码属于功能性的硬件接口定义，而本库的每个实现文件都是原创的 C++ 设计。通过设备公开的 USB 协议与设备交互，并不会使主机程序成为设备固件的衍生作品，因此本库可以采用宽松许可证发布。感谢 Till Harbaum 设计出 LCD2USB 硬件及本库所遵循的协议。

### 第三方组件

以下组件随项目发布或被本项目链接，保留各自许可证：

- `assets/ChillBitmap7x.ttf` —— SIL Open Font License 1.1
- `assets/boutique7x7.glyphs`、`fonts/uni668E_.glif` —— 衍生字形数据，遵循其来源字体的许可条款
- [giflib](https://sourceforge.net/projects/giflib/) —— MIT 风格许可证（GIF 示例）
- [FreeType](https://freetype.org/) —— FTL / GPL-2.0 带字体例外（UTF-8 管线）
- [ncurses](https://invisible-island.net/ncurses/) —— MIT 风格许可证（GIF 示例界面）
- [libusb / libusb-compat](https://libusb.info/) —— LGPL-2.1，动态链接

## 🎯 硬件兼容性

**已测试**：
- LCD2USB v1.9 固件
- 16x2 与 20x4 字符显示屏
- HD44780 兼容 LCD 控制器

**要求**：
- USB 2.0 或更高
- LCD2USB 适配板
- 兼容的字符 LCD 显示屏

## 🔍 版本历史

- **v-0.1.0**：首个公开发布（git 标签 `v-0.1.0`）
  - RsTask 看板示例、MIT OR Apache-2.0 双许可证、个人展示资源移出仓库、跨平台临时文件忽略规则

- **v1.2.0**：RsTask 看板示例
  - 通过 `rstasks` CLI 显示 RsTask 工作区的实时看板
  - 每任务 CGRAM identicon 图标，含泳道页脚图案
  - 跑马灯详情模式（能放下的行保持静止）
  - 可配置的显示几何参数与分钟级刷新调度

- **v1.2.0（法律）**：以 MIT OR Apache-2.0 双许可证重新授权
  - 新增 LICENSE-MIT 与 LICENSE-Apache-2.0；README 中加入许可理由与第三方声明（字体 OFL、giflib、FreeType、ncurses、libusb LGPL）

- **v1.1.0**：图像与动画支持
  - 灵活网格的 BMP 图像查看器
  - 带 ncurses 界面的 GIF 动画播放器
  - MultiCharDisplay 网格管理辅助类
  - Otsu 法自动阈值检测
  - 双线性插值图像缩放
  - 完整的 HD44780 限制文档
  - 信号处理实现优雅清理
  - 简化初始化以避免显示伪影
  - 7 个可用的示例程序

- **v1.0.0**：首个专业库版本
  - 核心 LCD 通信
  - 自定义字符系统
  - 支持二进制读写的字体管理
  - 5 个可用的示例程序（含 storyteller）
  - CMake 构建系统
  - 真实硬件测试完成

## 📚 更多文档

- **[Manual-FontEditor.md](Manual-FontEditor.md)**：完整的字体编辑器指南
  - `.lcf` 二进制格式规范
  - 完整 API 参考与示例
  - 代码分步讲解
  - 高级技巧与最佳实践
  - 故障排除与调试工具

---

**LCD2USB++ 库为 LCD2USB 设备提供了现代化的专业接口，具备完善的字符管理、图像处理与动画能力。特性包括 BMP/GIF 支持、灵活的网格显示以及带自动阈值检测的图像处理。适用于嵌入式项目、信息展示屏、交互应用与创意可视化。注意：设计基于字符的动画时，请留意 HD44780 的 8 个 CGRAM 槽位限制（见「HD44780 硬件限制」一节）。**
