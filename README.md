# Abyss Restaurant 深海食堂 — v0.1 简化版

Deep-sea exploration + underwater restaurant management. UE 5.6, C++.
Everything (seabed, dome, sub, fish, UI) is generated from code and engine basic shapes,
so the game is playable before any Tripo / Fab art is imported.

## 运行

1. 双击 `build_and_setup.bat`（编译 C++ + 自动生成材质和地图，编辑器会自动打开再关闭）
   - 只改了 C++：可以只编译，或在编辑器里按 Ctrl+Alt+F11 (Live Coding)
2. 打开 `AbyssRestaurant.uproject` → 地图 `/Game/Maps/L_Abyss` → 点 **Play**

## 操作

| 探索（白天） | |
|---|---|
| W/A/S/D | 前后左右（朝镜头方向） |
| 鼠标 | 转向 / 俯仰 |
| Space / Ctrl(C) | 上浮 / 下潜 |
| Shift | 加速（更耗电） |
| 按住左键 | 探照灯锁定鱼后捕捉 |
| F | 开关探照灯 |
| E | 在泊位附近：停靠并开始营业 |

| 餐厅（夜晚） | |
|---|---|
| 1 – 4 | 选择订单开始烹饪 |
| Space | 指针在绿色区域时出菜（Perfect = 1.5 倍价格） |
| B / L | 结算界面：升级电池 / 探照灯 |
| Enter | 结算界面：开始新的一天 |

## 玩法循环

白天：从泊位出发 → 浅礁（约40 m）黄尾鲷 → 外围金枪鱼 → **东边海沟**边缘的稀有石斑鱼（越深越黑）→
电量耗尽前回泊位按 E。
夜晚：顾客入座点单（根据你冰箱里的鱼）→ 按 1-4 烹饪 → 时机条按空格 → 收钱 → 顾客等太久会生气离开。
结算：升级 → Enter 进入下一天，顾客数量逐日增加。

## 代码结构 (`Source/AbyssRestaurant`)

| 文件 | 作用 |
|---|---|
| `AbyssGameMode` | 日夜循环、金钱、库存、升级 |
| `SubmarinePawn` | 潜艇操控、电池、探照灯捕鱼、海洋雪粒子 |
| `CatchableFish` / `FishSpawner` | 可捕捉的鱼（游荡/逃跑/挣扎），开局生成 |
| `FishSchool` | 装饰鱼群（实例化渲染，会躲开潜艇） |
| `SeabedGenerator` | 程序化海底地形 + 礁石/珊瑚/海带/发光管虫 |
| `UnderwaterEnvironment` | 雾、焦散光、环境光、调色；按深度变暗 |
| `RestaurantDome` / `RestaurantCustomer` | 穹顶餐厅（程序化搭建）+ 营业逻辑 + 顾客 |
| `AbyssHUD` | 全部 UI（指南针、深度表、电量、声呐、锁定框、订单、烹饪条、结算） |
| `Scripts/setup_project.py` | 生成材质 + 搭建并保存地图 |

## 常用调参（选中关卡里的 Actor → Details 面板）

- **画面太暗/太亮**：`Underwater_Environment` → Exposure Compensation
- **雾太浓**：`Underwater_Environment` → Fog Density
- **地形**：`Seabed` → Seed / Trench / Scatter Density（改完自动重新生成）
- **鱼的数量**：`Fish_Spawner`；鱼群：`FishSchool_*`
- **营业难度**：`Restaurant_Dome` → Cook Speed / Base Patience / Spawn Interval

## 下一步（第 2 周）

- Tripo 模型替换：潜艇、鱼、餐厅家具、厨师/顾客（Mixamo 动画）
- Fab 写实素材替换礁石/珊瑚/沙地；海底转静态网格以支持 Lumen 全局光照
- 深渊巨兽结尾演出、音效、中文字体
