# 数据结构与算法课程设计

2026 / 2027 学年 第 1 学期

本仓库只有一个可执行程序 **`traffic_sim`**：启动后选择题 2 或题 7，两个题目在同一窗口内切换。

| 程序 | 包含的题目 | 切换方式 |
|---|---|---|
| `traffic_sim` | 基础题 2：快递中转栈-队列混合调度模拟<br>扩展题 7：城市公共自行车调度路径规划系统 | 任务选择页 + 顶部“任务”按钮 / `Ctrl+0/1/2` |

## 技术栈

- C++20，MinGW-w64 13.1
- Qt 6.10.1 Widgets（窗口、QPainter 自绘、QSS、定时器）
- CMake 3.21+ / Ninja
- 核心层 `core/` 为纯 C++：不包含 Qt 头文件，不使用 STL 容器和 STL 算法
- 数据结构：`Vector`、`Stack`、`Queue`、`MinHeap`、`HashMap`、邻接表 `Graph`
- 算法：Dijkstra（二叉堆 + 懒删除）、BFS 可达性、最近邻 + 2-opt 路线排序、栈+队列混合调度
- 界面：Saira 静态实例（SIL OFL 1.1）+ 微软雅黑 UI，字体和许可证在 `apps/common/fonts/`

## 界面

变体：Klein blue（accent #002FA7 / cool blue paper / standard cut / contours / standard density / full motion）
主视觉：题 2 为主通道 + 临时栈 + 三条队列；题 7 为校园路网与调度路线。
页面类型：控制台。

强调色为克莱因蓝 `#002FA7`，深蓝底上用纸白字保证对比度；深色主题把强调色提亮为 `#7FA0FF`，强调色仍只用于“当前 / 可点击 / 需要注意”。

任务台的两个页面共用一套控制台风格：

- 顶部深色命令栏 + 三色细条，下面是白板和 1px 细线分区；可切换浅色/深色。
- 任务台先显示两张编号任务卡；进入任务后顶部出现“任务”按钮，可随时返回选择页，切换不清空数据。
- 强调色只用于“当前 / 可点击 / 需要注意”，红色只用于“清空路网”；状态不再依赖红绿。
- 切角只给可点击的按钮和地图站点；面板、表格、输入框都是直角（顶部品牌徽记沿用切角形状）。
- 勾选的目标整行铺克莱因蓝并用白字、白色勾；调度告警行同样铺蓝，并带白色“!”标记。
- 英文眉标、字段和数字用 Saira 宽体，中文用微软雅黑 UI；统计数字来自实际运行数据。
- 题 2 的车辆用实心 / 描边 / 斜线纹区分类型；题 7 的路线在浅色下是墨色、深色下是浅克莱因蓝，路线中段有方向箭头。
- 缩窄窗口时，地图只保留编号方块，站名、现有车数和容量以右侧“站点状态”表为准。
- 字体许可：`apps/common/fonts/Saira-OFL.txt`。

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
│  ├─ common/theme.h/.cpp          颜色、字体、QSS、切角/斜线纹/等高线绘制
│  ├─ common/widgets.h/.cpp        切角按钮、面板、大数字统计板、命令栏
│  ├─ common/fonts/                Saira 静态实例 + OFL 许可证
│  ├─ truck/                       题 2 界面与场景画布
│  ├─ bike/                        题 7 界面与地图画布
│  └─ combined/                    任务台：任务选择页 + 单窗口切换外壳
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

启动后先显示任务选择页，两个任务在同一个窗口内切换：

```powershell
build\debug\traffic_sim.exe
```

切换方式：点击页面右上角的“任务”按钮，或按 `Ctrl+0` 返回任务选择、`Ctrl+1` 打开题 2、`Ctrl+2` 打开题 7。切换只暂停当前仿真，不清空已录入的数据。

程序支持以下参数：

- `--demo`：自动装载一组演示数据（题 2 会生成 10 辆车并完成一次让路离场；题 7 会规划并模拟一次调度）。
- `--demo-warning`：题 2 专用，装入“主通道 6 位、临时区 1 位”的边界场景并触发一次被拒绝的离场，用于展示告警条。
- `--demo-letway`：题 2 专用，装入“主通道 4 位、临时区 3 位”的场景并让最内侧车辆离场，用于截图展示挡路车辆停在临时区的中间态。
- `--task <truck|bike>`：启动后直接进入指定任务；不带此参数时显示任务选择页。
- `--smoke`：启动后约 1 秒自动退出，用于构建冒烟测试。
- `--dark`：以深色主题启动；窗口右上角的“深色 / 浅色”按钮也可以在运行时切换。
- `--screenshot <文件>`：启动后把窗口渲染成图片，按扩展名保存（PNG/JPG/BMP），父目录不存在时自动创建；配合 `--demo` 可生成报告截图。
- `--size <宽>x<高>`：按指定客户区尺寸打开窗口，用于检查最小尺寸下的布局；小于最小尺寸时按最小尺寸打开。

示例：

```powershell
build\debug\traffic_sim.exe --screenshot shots\task-select.png
build\debug\traffic_sim.exe --task truck --demo --screenshot shots\task-truck.png
build\debug\traffic_sim.exe --task bike --demo --screenshot shots\task-bike.png
build\debug\traffic_sim.exe --task truck --demo-letway --screenshot shots\task-letway.png
build\debug\traffic_sim.exe --task truck --demo-warning --screenshot shots\truck-warning.png
build\debug\traffic_sim.exe --task truck --demo --dark --screenshot shots\truck-dark.png
build\debug\traffic_sim.exe --task truck --demo --size 1200x760 --screenshot shots\truck-narrow.png
build\debug\traffic_sim.exe --task bike --demo --size 1200x760 --screenshot shots\bike-narrow.png
```

## 测试

```powershell
ctest --test-dir build/debug --output-on-failure
```

测试覆盖：

- 自研容器：动态数组扩容/拷贝/删除、栈、循环队列、二叉堆有序性、哈希表 500 条键值及删除。
- 题 2：到达/补位、同优先级按到达时刻排序、栈底离场让路、begin/finish 分步离场（临时区中间态）、回装顺序、临时区不足拒绝、启动容量预警、告警事件标记、取消排队、优先级插队、计费、编号重复、事件日志上限、状态守恒断言。
- 题 7：建图/重边拒绝、Dijkstra 两种权重、断边（双向）不可达、恢复、两点查询、单目标/多目标路线规划、调度仿真、告警、断边后仿真失败且库存不变。
- 任务台：`traffic_sim --task truck/bike` 能直接进入对应页面；页面“任务”按钮与 Ctrl+0/1/2 共用 `showTask` 入口，已实际点击验证“题 2 → 选择页 → 题 7 → 选择页”的往返切换，切换时暂停当前页定时器并保留数据。

## 题 2 功能

- 启动参数：主通道容量、临时区容量、每小时费率。
- 货车到达：编号、类型（生鲜 / 普通 / 大件）、到达时刻 → 进入对应优先级队列；到达时刻默认跟随仿真时钟，可改成更早的时刻验证同优先级排序。通道有空位时按“生鲜 > 普通 > 大件、同级按到达时刻先到先服务”补位。
- 货车离场：若在通道中，先把目标车后方驶入的车辆依次移到临时栈，界面停 520 ms 展示临时区占用，再让目标车离开并原序回装、补位；输出停留时长和费用。
- 统计：累计收入、主通道占用/空位、临时区占用/峰值、各队列积压、到达/服务车次。
- 边界：通道满时只排队；临时区不足时拒绝离场，右侧立即弹出告警条（标题、所需/现有临时位、拒绝原因），同时写入事件日志并用克莱因蓝高亮；启动参数本身放不下最内侧车辆时也会提前提示。编号重复、时刻非法有提示。
- 可视化：主通道栈、临时区栈、三条优先级队列、车辆用实心/描边/斜线纹区分类型、平滑位移动画、事件日志；让路时分两帧展示，挡路车辆会真正停在临时区；临时区满时改用强调色描边。
- 自动模拟：按 700 ms（可调）推进 2 分钟并随机到达/离场；“单步推进 2 分钟”只推进时钟，不生成随机事件。

计费口径：`费用 = (离场时刻 − 到达时刻) / 60 × 每小时费率`，时间单位为分钟；这是报告中需要写明的设计假设。

## 题 7 功能

- 路网录入：示例地图、手动新增站点、手动新增无向道路；站点含编号、名称、现有车数、容量、坐标。
- 最短路径：Dijkstra + 自研二叉堆，支持“距离最短 / 耗时最少”两种权重。
- 两点查询：输出途经站点序列和总权重。
- 调度规划：从总站出发访问勾选的目标站点并回到总站；最近邻构造 + 2-opt 改进（启发式，不保证 TSP 全局最优）。
- 调度仿真：调度车按路线装卸自行车，受载量和站点容量约束；低于下限/高于上限/总站剩余未卸下都会产生告警。
- 道路阻断：选中道路后两条弧一起置为不可用（虚线 + “阻断”标记），重新计算路径；一键恢复；可检查各目标站点可达性。
- 可视化：站点为可点击的切角方块，库存告警用克莱因蓝小旗；道路权重需要在“道路阻断”栏勾选“权重”才显示；调度路线在浅色下用墨线、深色下用浅克莱因蓝，调度车沿路径动画。

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
E:\Qt\6.10.1\mingw_64\bin\windeployqt.exe --release --compiler-runtime build\release\traffic_sim.exe
```

把 exe 与同目录 DLL、`platforms/` 一起压缩提交；同时把 `apps/common/fonts/Saira-OFL.txt` 复制到 exe 同目录（字体已嵌入程序，但许可证要随包分发）。在未安装 Qt 的机器上双击验证。`dist/traffic_sim/` 是发布目录。

## 已知限制

- 题 2 的临时区溢出采用“拒绝离场 + 告警”策略：需要 7 位、临时区只有 6 位时不会移动任何车辆，也不会有车辆进入临时区；如需“强制溢出”可在 `TruckYard::beginDepart` 中调整。
- 题 7 的调度顺序是启发式，不保证最优；报告“结论/改进”中应说明。
- 调度仿真会直接修改站点车数，重复点击会基于当前车数继续；演示前可点“加载示例地图”恢复初始数据。
- 地图坐标保存在数据中，当前版本不支持拖拽站点改坐标（可后续扩展）。
- 界面按桌面控制台设计，最小窗口 1200×760；更窄的屏幕需要滚动左侧面板，地图会自动收起站名标签。
- 深色/浅色切换只在本次运行内生效，不写配置文件。
- 道路权重默认不显示，可在“道路阻断”栏勾选“权重”显示；下拉框始终显示当前道路权重。
