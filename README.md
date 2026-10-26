# 数据结构与算法课程设计

2026 / 2027 学年 第 1 学期

本仓库包含两个可独立运行的可视化程序：

| 程序 | 题目 | 类型 |
|---|---|---|
| `truck_sim` | 快递中转栈-队列混合调度模拟 | 基础题 2 |
| `bike_sim` | 城市公共自行车调度路径规划系统 | 扩展题 7 |

## 技术栈

- C++20，MinGW-w64 13.1
- Qt 6.10.1 Widgets（只用于界面、绘图、定时器、文件对话框）
- CMake 3.21+ / Ninja
- 核心层 `core/` 为纯 C++：不包含 Qt 头文件，不使用 STL 容器和 STL 算法
- 数据结构：`Vector`、`Stack`、`Queue`、`MinHeap`、`HashMap`、邻接表 `Graph`
- 算法：Dijkstra（二叉堆 + 懒删除）、BFS 可达性、最近邻 + 2-opt 路线排序、栈+队列混合调度

## 目录

```
.
├─ CMakeLists.txt
├─ CMakePresets.json
├─ core/
│  ├─ include/ds/containers.h      Vector / Stack / Queue / MinHeap / HashMap
│  ├─ include/ds/graph.h           邻接表图
│  ├─ include/ds/dijkstra.h        最短路
│  ├─ include/ds/truck_yard.h      题 2 核心模型
│  ├─ include/ds/bike_system.h     题 7 核心模型
│  └─ src/                         对应实现
├─ apps/
│  ├─ common/theme.h               统一浅色主题
│  ├─ truck/                       题 2 界面与场景画布
│  └─ bike/                        题 7 界面与地图画布
├─ tests/test_main.cpp             控制台测试（不链接 Qt）
```

## 构建

环境要求：Windows 10/11，Qt 6.10.1 MinGW 套件，CMake ≥ 3.21，Ninja。

```powershell
$env:PATH = 'E:\Qt\6.10.1\mingw_64\bin;E:\Qt\Tools\mingw1310_64\bin;E:\Qt\Tools\Ninja;' + $env:PATH
cmake --preset mingw-debug
cmake --build build/debug -j 4
```

## 运行

```powershell
build\debug\truck_sim.exe
build\debug\bike_sim.exe
```

两个程序都支持：

- `--demo`：自动装载一组演示数据（题 2 会生成 10 辆车并完成一次让路离场；题 7 会规划并模拟一次调度）。
- `--smoke`：启动后约 1 秒自动退出，用于构建冒烟测试。
- `--screenshot <文件>`：启动后把窗口渲染成图片（默认 BMP，Qt 在本机未加载 PNG 写入插件）。配合 `--demo` 可生成报告截图。

示例：

```powershell
build\debug\truck_sim.exe --demo --screenshot shots\truck_demo.bmp
build\debug\bike_sim.exe  --demo --screenshot shots\bike_demo.bmp
```

## 测试

```powershell
ctest --test-dir build/debug --output-on-failure
```

测试覆盖：

- 自研容器：动态数组扩容/拷贝/删除、栈、循环队列、二叉堆有序性、哈希表 500 条键值及删除。
- 题 2：到达/补位、栈底离场让路、回装顺序、临时区不足拒绝、取消排队、优先级插队、计费、统计守恒、编号重复。
- 题 7：建图/重边拒绝、Dijkstra 两种权重、断边不可达、恢复、两点查询、单目标/多目标路线规划、调度仿真、断边后仿真失败。

## 题 2 功能

- 启动参数：主通道容量、临时区容量、每小时费率。
- 货车到达：编号、类型（生鲜 / 普通 / 大件）、到达时刻 → 进入对应优先级队列；通道有空位时按“生鲜 > 普通 > 大件、同级先到先服务”补位。
- 货车离场：若在通道中，自动把目标车上方的车依次移到临时栈，目标车离开后原序回装；输出停留时长和费用。
- 统计：累计收入、主通道占用/空位、临时区占用/峰值、各队列积压、到达/服务车次。
- 边界：通道满时只排队；临时区不足时拒绝离场并写入告警日志；编号重复、时刻非法有提示。
- 可视化：主通道栈、临时区栈、三条优先级队列、车辆按类型着色、平滑位移动画、事件日志。
- 自动模拟：按 700 ms（可调）推进 2 分钟，随机到达/离场。

计费口径：`费用 = (离场时刻 − 到达时刻) / 60 × 每小时费率`，时间单位为分钟；这是报告中需要写明的设计假设。

## 题 7 功能

- 路网录入：示例地图、手动新增站点、手动新增无向道路；站点含编号、名称、现有车数、容量、坐标。
- 最短路径：Dijkstra + 自研二叉堆，支持“距离最短 / 耗时最少”两种权重。
- 两点查询：输出途经站点序列和总权重。
- 调度规划：从总站出发访问勾选的目标站点并回到总站；最近邻构造 + 2-opt 改进（启发式，不保证 TSP 全局最优）。
- 调度仿真：调度车按路线装卸自行车，受载量和站点容量约束；低于下限/高于上限/超容都会产生告警。
- 道路阻断：选中道路后两条弧一起置灰，重新计算路径；一键恢复；可检查各目标站点可达性。
- 可视化：站点圆点按容量和库存比例着色、道路权重标注、路线黄色高亮、调度车沿路径动画、告警列表、站点状态表。

## 复杂度

| 操作 | 复杂度 |
|---|---|
| 题 2 到达 + 补位 | O(C)，C 为主通道容量 |
| 题 2 离场 | O(C) |
| 题 2 按编号查车 | 平均 O(1)（哈希表） |
| 题 7 Dijkstra | O((V + E) log V) |
| 题 7 断边/恢复 | O(deg) |
| 题 7 目标间距离矩阵 | O(T · (V + E) log V) |
| 题 7 最近邻 + 2-opt | O(T²) 起 |

## 打包

```powershell
cmake --preset mingw-release
cmake --build build/release -j 4
E:\Qt\6.10.1\mingw_64\bin\windeployqt.exe --release --compiler-runtime build\release\truck_sim.exe
E:\Qt\6.10.1\mingw_64\bin\windeployqt.exe --release --compiler-runtime build\release\bike_sim.exe
```

把 exe 与同目录 DLL、`platforms/` 一起压缩提交；在未安装 Qt 的机器上双击验证。

## 已知限制

- 题 2 的临时区溢出采用“拒绝离场 + 告警”策略；如需“强制溢出”可在 `TruckYard::depart` 中调整。
- 题 7 的调度顺序是启发式，不保证最优；报告“结论/改进”中应说明。
- 调度仿真会直接修改站点车数，重复点击会基于当前车数继续；演示前可点“加载示例地图”恢复初始数据。
- 地图坐标保存在数据中，当前版本不支持拖拽站点改坐标（可后续扩展）。
