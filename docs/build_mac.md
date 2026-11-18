# build_mac.md — 新项目地址构建 MapTool（Mac）

> 项目根：`/Users/a11/地图工具bag回放/`；源码：`maptool-sim/mac-tool-src/`（引入自 `~/Documents/Codex/2026-10-12/n-h/修图工具/地图工具/`）。
> 工具链零安装：复用离线工具链（cmake 4.4.4 + Qt6 全套含 WebSockets + OpenCV 5.0）。

```bash
export PATH="$HOME/Documents/Codex/2026-10-12/n-h/work/toolchain/env/bin:$PATH"
TC="$HOME/Documents/Codex/2026-10-12/n-h/work/toolchain/env/lib/cmake/Qt6"

cd "/Users/a11/地图工具bag回放"
cmake -S maptool-sim/mac-tool-src -B build -DCMAKE_PREFIX_PATH="$TC" -DCMAKE_BUILD_TYPE=Release
cmake --build build -j4
# 产物：build/dist/bin/MapTool.app（2026-11-18 实测 100% 通过，仅 1 个无害警告）
open build/dist/bin/MapTool.app
```

## 注意
- 本机无全局 cmake/Qt，**必须先前置 PATH**，否则 `command not found: cmake`
- CMake 自动用 Qt6（工具链内含 Qt6Core5Compat / Qt6Svg / Qt6WebSockets——直播 WS 客户端可直接用）
- build/ 已入 .gitignore（建议后续补）；新增模块放 `maptool-sim/` 根下独立子目录，构建时在 CMakeLists 里 add_subdirectory 合入
