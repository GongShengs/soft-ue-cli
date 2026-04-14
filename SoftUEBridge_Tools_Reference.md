# SoftUEBridge MCP 工具完整参考手册

> **版本**: SoftUEBridge v1.x  
> **生成日期**: 2025-04-09  
> **工具总数**: 54 个原生 C++ 工具 + 5 个 Skill 扩展脚本 = **59 个工具**  
> **传输协议**: HTTP JSON-RPC (`http://127.0.0.1:8080/bridge`)

---

## 目录

- [一、世界场景与关卡操作](#一世界场景与关卡操作)
- [二、Actor 属性与函数](#二actor-属性与函数)
- [三、资产查询与管理](#三资产查询与管理)
- [四、Blueprint 图表操作](#四blueprint-图表操作)
- [五、Material 系统](#五material-系统)
- [六、Widget / UMG](#六widget--umg)
- [七、StateTree 状态树](#七statetree-状态树)
- [八、截图与视口](#八截图与视口)
- [九、PIE / 游戏控制](#九pie--游戏控制)
- [十、编译与构建](#十编译与构建)
- [十一、日志与控制台](#十一日志与控制台)
- [十二、性能分析 (Insights)](#十二性能分析-insights)
- [十三、项目信息](#十三项目信息)
- [十四、依赖与引用](#十四依赖与引用)
- [十五、Python 脚本执行](#十五python-脚本执行)
- [十六、Skill 扩展：动画资产查询工具](#十六skill-扩展动画资产查询工具)

---

## 一、世界场景与关卡操作

### 1. `query-level`
**功能**: 列出并检查当前游戏世界中的 Actor。支持按类名、标签、名称模式过滤。  
**关键参数**:
| 参数 | 类型 | 说明 |
|------|------|------|
| `actor_name` | string | 按名称或标签查找 (支持通配符 `*pattern*`) |
| `class_filter` | string | 按类名过滤 |
| `tag_filter` | string | 按 Actor 标签过滤 |
| `include_components` | bool | 包含组件列表 |
| `include_properties` | bool | 包含 Actor 和组件属性值 |
| `include_transform` | bool | 包含变换信息 (默认 true) |
| `world` | string | 世界上下文: `editor` / `pie` / `game` |
| `limit` | int | 最大结果数 (默认 100) |
| `include_foliage` | bool | 包含 Foliage 实例计数 |
| `include_grass` | bool | 包含 Landscape 草地/材质信息 |

### 2. `spawn-actor`
**功能**: 在编辑器关卡或 PIE 世界中生成 Actor。支持原生类和 Blueprint Actor。  
**关键参数**:
| 参数 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `actor_class` | string | ✅ | 类名或 Blueprint 路径 |
| `location` | [x,y,z] | | 位置 (厘米) |
| `rotation` | [p,y,r] | | 旋转 (度) |
| `world` | string | | `editor` (默认, 支持撤销) 或 `pie` |
| `label` | string | | World Outliner 中的标签 |

### 3. `add-component`
**功能**: 向关卡中已存在的 Actor 添加组件。  
**关键参数**: `actor_name` (必需), `component_class` (必需), `component_name`, `attach_to`

---

## 二、Actor 属性与函数

### 4. `get-property`
**功能**: 通过反射获取 Actor 或其组件的属性值。支持点语法: `ComponentName.PropertyName`。  
**关键参数**: `actor_name` (必需), `property_name` (必需), `world`

### 5. `set-property`
**功能**: 通过反射设置 Actor 或其组件的属性。值以字符串形式传入 (如 `'100'`, `'true'`, `'(X=0,Y=0,Z=100)'`)。  
**关键参数**: `actor_name` (必需), `property_name` (必需), `value` (必需), `world`

### 6. `call-function`
**功能**: 调用 Actor 上的 Blueprint 或原生函数。支持 bool/int/float/string 参数类型。  
**关键参数**: `actor_name` (必需), `function_name` (必需), `args` (object), `world`

### 7. `trigger-input`
**功能**: 在运行中的游戏 (PIE 或打包构建) 中模拟玩家输入。  
**动作类型**:
- `key`: 按下/释放按键
- `action`: 触发命名输入动作
- `move-to`: 寻路到位置
- `look-at`: 旋转朝向目标

---

## 三、资产查询与管理

### 8. `query-asset`
**功能**: 按模式/类/路径搜索资产，或检查特定资产的详细信息。  
**两种模式**:
- **搜索模式**: 使用 `query` 参数，支持 `*` 和 `?` 通配符
- **检查模式**: 使用 `asset_path` 参数，返回资产详情

**关键参数**: `query`, `class`, `path`, `asset_path`, `depth`, `include_defaults`, `property_filter`, `row_filter`

### 9. `create-asset`
**功能**: 按类名创建新资产。支持 Blueprint, Material, DataTable, DataAsset, WidgetBlueprint, AnimBlueprint 等。  
**关键参数**: `asset_path` (必需), `asset_class` (必需), `parent_class`, `skeleton`, `row_struct`

### 10. `delete-asset`
**功能**: 删除资产。对 Blueprint 会运行垃圾回收确保生成的类被完全清理。  
**关键参数**: `asset_path`

### 11. `save-asset`
**功能**: 保存修改后的资产到磁盘。在执行修改命令后使用以防止编辑器崩溃导致数据丢失。  
**关键参数**: `asset_path` (必需), `checkout` (bool)

### 12. `open-asset`
**功能**: 在 UE 编辑器中打开资产或编辑器窗口。  
**关键参数**: `asset_path`, `window_name`, `bring_to_front`

### 13. `get-asset-preview`
**功能**: 生成资产的可视化预览图 (纹理、网格体、材质等)。  
**关键参数**: `asset_path`, `resolution` (64-1024, 默认 256), `format` (png/jpeg), `output` (file/base64)

### 14. `get-asset-diff`
**功能**: 获取二进制 UE 资产 (Blueprint, Material, DataTable) 相对于版本控制基准版本的结构化差异。  
**关键参数**: `asset_path` (必需), `scm_type` (git/perforce/auto), `base_revision`

### 15. `set-asset-property`
**功能**: 使用 UE 反射设置任意资产上的任意属性。支持嵌套路径、数组索引、TArray、TMap、TSet、对象引用、结构体、向量/旋转/颜色。  
**关键参数**: `asset_path` (必需), `property_path` (必需), `value`, `component_name`, `clear_override`

### 16. `inspect-details-panel`
**功能**: 检查 DataAsset 或任意 UObject 的 Details 面板结构。返回完整的面板布局，含类别、属性、元数据、控件类型和嵌套信息。  
**关键参数**: `asset_path` (必需), `include_values` (默认 true), `depth` (默认 2, 最大 4)

### 17. `get-class-hierarchy`
**功能**: 浏览类继承树，显示父类和子类。支持 C++ 和 Blueprint 类。  
**关键参数**: `class_name` (必需), `direction` (parents/children/both), `include_blueprints`, `depth`

---

## 四、Blueprint 图表操作

### 18. `query-blueprint`
**功能**: 查询 Blueprint 结构：函数、变量、组件、默认值、事件图、组件覆盖。  
**关键参数**: `asset_path` (必需), `include` (functions/variables/components/defaults/graph/interfaces/component_overrides/all), `property_filter`, `category_filter`, `search`

### 19. `query-blueprint-graph`
**功能**: 查询 Blueprint 图表：列出所有图、按 GUID 获取特定节点、获取可调用详情。对 AnimBlueprint 还返回 anim_graph、state_machine、state、transition、blend_stack 图。  
**关键参数**: `asset_path` (必需), `node_guid`, `callable_name`, `list_callables`, `graph_name`, `graph_type`, `include_positions`, `include_anim_node_properties`

### 20. `add-graph-node`
**功能**: 向 Blueprint 或 Material 图添加节点。支持智能自动定位。  
**节点类型示例**:
- Material: `MaterialExpressionAdd`, `MaterialExpressionSceneTexture`
- Blueprint: `K2Node_CallFunction`, `K2Node_VariableGet`, `K2Node_Event`
- AnimLayer: `AnimLayerFunction`

**关键参数**: `asset_path` (必需), `node_class` (必需), `graph_name`, `position`, `auto_position`, `connect_to_node`, `properties`

### 21. `remove-graph-node`
**功能**: 从 Blueprint 或 Material 图中移除节点。  
**关键参数**: `asset_path` (必需), `node_id` (必需)

### 22. `insert-graph-node`
**功能**: 原子性地在两个已连接节点之间插入新节点。断开源→目标连线，创建新节点，连接源→新输入和新输出→目标。所有操作在单个撤销事务中完成。  
**关键参数**: `asset_path` (必需), `node_class` (必需), `source_node` (必需), `source_pin` (必需), `target_node` (必需), `target_pin` (必需), `properties`

### 23. `connect-graph-pins`
**功能**: 连接 Blueprint 或 Material 图中的两个引脚。  
**关键参数**: `asset_path` (必需), `source_node` (必需), `source_pin` (必需), `target_node` (必需), `target_pin` (必需)

### 24. `disconnect-graph-pin`
**功能**: 断开 Blueprint 或 Material 图中引脚的连接。默认断开所有连接；指定 `target_node` 和 `target_pin` 可仅断开特定连接。  
**关键参数**: `asset_path` (必需), `node_id` (必需), `pin_name` (必需), `target_node`, `target_pin`

### 25. `set-node-position`
**功能**: 批量设置 Material/Blueprint/AnimBlueprint 图中节点的编辑器位置。所有移动在单个撤销事务中完成。  
**关键参数**: `asset_path` (必需), `positions` (必需, [{guid, x, y}]), `graph_name`

### 26. `set-node-property`
**功能**: 通过 GUID 设置图节点上的属性。支持 UPROPERTY 成员、内部动画节点结构体属性和引脚默认值。  
**关键参数**: `asset_path` (必需), `node_guid` (必需), `properties` (必需)

### 27. `modify-interface`
**功能**: 在 Blueprint 或 AnimBlueprint 上添加或移除已实现的接口。  
**关键参数**: `asset_path` (必需), `action` (add/remove, 必需), `interface_class` (必需)

### 28. `compile-blueprint`
**功能**: 编译 Blueprint 或 AnimBlueprint 并返回编译结果 (成功、警告、错误)。  
**关键参数**: `asset_path` (必需)

### 29. `add-datatable-row`
**功能**: 向 DataTable 添加新行。  
**关键参数**: `asset_path` (必需), `row_name` (必需), `row_data` (JSON 字符串)

---

## 五、Material 系统

### 30. `query-material`
**功能**: 查询 Material 结构：表达式图和参数。  
**关键参数**: `asset_path` (必需), `include` (graph/parameters/all), `include_positions`, `include_defaults`, `parameter_filter`, `parent_chain`

### 31. `query-mpc`
**功能**: 读取或写入 Material Parameter Collection (MPC) 值。读取返回默认值和运行时值。  
**关键参数**: `asset_path` (必需), `action` (read/write), `parameter_name`, `value`, `world`

---

## 六、Widget / UMG

### 32. `inspect-widget-blueprint`
**功能**: 检查 Widget Blueprint 特有数据，包括 WidgetTree 层级、Slot 信息 (锚点、偏移、尺寸)、可见性设置、命名槽、属性绑定和动画。  
**关键参数**: `asset_path` (必需), `include_defaults`, `depth_limit`, `include_bindings`

### 33. `inspect-runtime-widgets`
**功能**: 在 PIE 会话期间检查运行时 UMG 控件几何体。遍历运行时控件树返回计算后的几何体 (绝对位置、本地尺寸)、Slot 属性和渲染设置。  
**关键参数**: `filter`, `class_filter`, `depth_limit`, `include_slate`, `include_geometry`, `root_widget`

### 34. `add-widget`
**功能**: 向 WidgetBlueprint 树添加控件。  
**支持的控件类**: Button, TextBlock, Image, ProgressBar, CanvasPanel, VerticalBox, HorizontalBox, Border, Overlay  
**关键参数**: `asset_path` (必需), `widget_class` (必需), `widget_name` (必需), `parent_widget`

---

## 七、StateTree 状态树

### 35. `query-statetree`
**功能**: 查询 StateTree 结构：状态、转换、任务、评估器和参数。  
**关键参数**: `asset_path` (必需), `include` (states/transitions/tasks/evaluators/parameters/all)

### 36. `add-statetree-state`
**功能**: 向 StateTree 添加新状态。  
**关键参数**: `asset_path` (必需), `state_name` (必需), `state_type`, `parent_state`, `selection_behavior`

### 37. `add-statetree-task`
**功能**: 向 StateTree 的状态添加任务。  
**关键参数**: `asset_path` (必需), `state_name` (必需), `task_class` (必需), `task_name`

### 38. `add-statetree-transition`
**功能**: 在 StateTree 的状态之间添加转换。  
**关键参数**: `asset_path` (必需), `source_state` (必需), `target_state` (必需), `trigger`, `priority`

### 39. `remove-statetree-state`
**功能**: 从 StateTree 中移除状态。⚠️ 也会移除所有子状态和关联的转换。  
**关键参数**: `asset_path` (必需), `state_name` (必需)

---

## 八、截图与视口

### 40. `capture-viewport`
**功能**: 捕获视口截图。`source='editor'` 捕获关卡编辑器视口，`source='game'` (默认) 捕获 PIE/独立模式。  
**关键参数**: `source` (game/editor), `format` (png/jpeg), `output` (file/base64)

### 41. `capture-screenshot`
**功能**: 捕获编辑器窗口、标签页、区域或游戏视口的截图。  
**模式**:
- `window`: 整个编辑器
- `tab`: 特定编辑器面板
- `region`: 矩形区域 [x, y, width, height]
- `viewport`: PIE 游戏画面

**关键参数**: `mode` (必需), `window_name`, `region`, `format`, `output`

---

## 九、PIE / 游戏控制

### 42. `pie-session`
**功能**: 控制 PIE (Play-In-Editor) 会话。  
**动作**: `start`, `stop`, `pause`, `resume`, `get-state`, `wait-for`  
**关键参数**: `action` (必需), `mode` (viewport/new_window/standalone), `map`, `timeout`, `actor_name`, `property`, `operator`, `expected`

---

## 十、编译与构建

### 43. `trigger-live-coding`
**功能**: 触发 Live Coding 编译以应用 C++ 代码更改。仅 Windows。  
**关键参数**: `wait_for_completion` (bool, 默认 false)

### 44. `build-and-relaunch`
**功能**: 关闭当前编辑器实例，触发完整项目构建，然后重新启动编辑器。  
**关键参数**: `build_config` (Development/Debug/Shipping), `skip_relaunch`

---

## 十一、日志与控制台

### 45. `get-logs`
**功能**: 获取最近的输出日志条目。可按文本或日志类别过滤。  
**关键参数**: `lines` (默认 100), `filter`, `category`

### 46. `get-console-var`
**功能**: 获取控制台变量 (CVar) 的当前值。  
**关键参数**: `name` (必需, 如 `r.ScreenPercentage`)

### 47. `set-console-var`
**功能**: 设置控制台变量 (CVar) 的新值。  
**关键参数**: `name` (必需), `value` (必需)

---

## 十二、性能分析 (Insights)

### 48. `insights-capture`
**功能**: 控制 Unreal Insights 追踪捕获。  
**动作**: `start` (启动, 可选通道), `stop` (停止, 返回文件路径), `status` (状态查询)  
**关键参数**: `action`, `channels` (如 ['cpu','gpu','frame']), `output_file`

### 49. `insights-list-traces`
**功能**: 列出项目 Saved/Profiling 目录中的可用 Insights 追踪文件 (.utrace)。  
**关键参数**: `directory`

### 50. `insights-analyze`
**功能**: 分析 Unreal Insights 追踪文件。当前支持 `basic_info` 分析类型 (文件元数据)。  
**关键参数**: `trace_file`, `analysis_type`

---

## 十三、项目信息

### 51. `get-project-info`
**功能**: 获取项目和插件信息，包括项目名称、路径和插件版本。可选包含项目设置 (输入映射、碰撞、Gameplay Tag、地图)。  
**关键参数**: `section` (input/collision/tags/maps/all)

---

## 十四、依赖与引用

### 52. `find-references`
**功能**: 查找资产、Blueprint 变量或节点的引用。  
**类型**:
- `asset`: 查找哪些资产引用了目标资产
- `property`: 查找 Blueprint 内变量的使用位置
- `node`: 查找节点/函数的使用位置

**关键参数**: `type` (必需), `asset_path` (必需), `variable_name`, `node_class`, `function_name`, `limit`

### 53. `find-dependencies`
**功能**: 查找资产的前向依赖 (X 依赖什么)。与 `find-references` (谁依赖 X) 互补。  
**关键参数**: `asset_path` (必需), `recursive`, `depth`, `class_filter`, `path_filter`, `exclude_engine`

---

## 十五、Python 脚本执行

### 54. `run-python-script`
**功能**: 在 UE 编辑器的 Python 环境中执行 Python 脚本。需要启用 PythonScriptPlugin。  
**关键参数**:
| 参数 | 类型 | 说明 |
|------|------|------|
| `script` | string | 内联 Python 代码 (与 script_path 互斥) |
| `script_path` | string | Python 脚本文件路径 (与 script 互斥) |
| `python_paths` | string[] | 附加 Python sys.path 目录 |
| `arguments` | object | 传递给脚本的参数 (通过 `unreal.get_mcp_args()` 获取) |

---

## 十六、Skill 扩展：动画资产查询工具

> 以下工具通过 `run-python-script` MCP 工具执行，是 `ue-animation-asset-query` Skill 的一部分。  
> **脚本位置**: `Plugins/SoftUEBridge/skills/ue-animation-asset-query/scripts/`

### S1. `query_anim_sequence.py` — AnimSequence / AnimMontage 查询

**功能**: 查询 AnimSequence 或 AnimMontage 的内部二进制数据。  
**返回数据**:
- 基本信息: play_length, num_frames, target_frame_rate, skeleton, rate_scale, enable_root_motion
- Additive 动画: additive_anim_type, ref_pose_type
- 曲线列表: Float 曲线和 Transform 曲线名称
- 通知列表: AnimNotify 和 AnimNotifyState 名称
- Montage 信息: slot_anim_tracks, composite_sections, blend_in/out
- 同步标记: sync_markers

**调用示例**:
```json
{
  "script_path": "Plugins/SoftUEBridge/skills/ue-animation-asset-query/scripts/query_anim_sequence.py",
  "arguments": { "asset_path": "/Game/Path/To/AnimSequence" }
}
```

### S2. `query_blend_space.py` — BlendSpace / BlendSpace1D / AimOffset 查询

**功能**: 查询 BlendSpace 的完整参数和采样点数据。  
**返回数据**:
- 轴参数 (X/Y/Z): display_name, min, max, grid_num
- 采样点: 每个采样点的动画引用、坐标 (x/y/z)、rate_scale
- 插值参数: 每轴的 interpolation_time, interpolation_type, damping_ratio, max_speed
- 全局设置: target_weight_interpolation_speed_per_sec, notify_trigger_mode

**调用示例**:
```json
{
  "script_path": "Plugins/SoftUEBridge/skills/ue-animation-asset-query/scripts/query_blend_space.py",
  "arguments": { "asset_path": "/Game/Path/To/BlendSpace" }
}
```

### S3. `query_control_rig.py` — ControlRig Blueprint 查询

**功能**: 查询 ControlRig Blueprint 的变量、图和骨骼层级。  
**返回数据**:
- 变量列表: name, type, category, is_public
- 图列表: UberGraphPages, FunctionGraphs, RigVM Model 节点
- 骨骼层级: 所有元素 (Bone, Curve, Null, Control 类型)
- 支持 `include` 参数: `variables`, `graphs`, `hierarchy`, `all`

**调用示例**:
```json
{
  "script_path": "Plugins/SoftUEBridge/skills/ue-animation-asset-query/scripts/query_control_rig.py",
  "arguments": { "asset_path": "/Game/Path/To/ControlRig", "include": "all" }
}
```

### S4. `query_bone_weights.py` — BoneWeightsAsset (BWA) 查询

**功能**: 查询 BoneWeightsAsset 的每骨骼混合权重数据。  
**返回数据**:
- 骨架引用: skeleton path
- 权重模式: weight_mode (如 COMBINED_TRANSLATE_ROTATE_SCALE)
- 默认权重: translation_weight, rotation_weight
- 每骨骼权重: bone_name → translation_weight / rotation_weight 映射 (通过 `bone_weight_infos` Map)

**调用示例**:
```json
{
  "script_path": "Plugins/SoftUEBridge/skills/ue-animation-asset-query/scripts/query_bone_weights.py",
  "arguments": { "asset_path": "/Game/Path/To/BWA_Asset" }
}
```

### S5. `query_physics_asset.py` — PhysicsAsset 查询

**功能**: 查询 PhysicsAsset 的约束设置和预览网格体。  
**返回数据**:
- 约束列表: bone1, bone2, angular motions/limits, linear motions/limits
- 预览网格体: preview_skeletal_mesh 路径
- ⚠️ **限制**: SkeletalBodySetups 属性在 UE Python API 中标记为 protected，无法直接读取 body 数据。如需 body 数据，请配合 `inspect-details-panel` MCP 工具使用。

**调用示例**:
```json
{
  "script_path": "Plugins/SoftUEBridge/skills/ue-animation-asset-query/scripts/query_physics_asset.py",
  "arguments": { "asset_path": "/Game/Path/To/PhysicsAsset" }
}
```

---

## 附录：工具分类速查表

| 分类 | 工具名 | 简述 |
|------|--------|------|
| **场景** | query-level | 列出/检查关卡 Actor |
| **场景** | spawn-actor | 生成 Actor |
| **场景** | add-component | 添加组件 |
| **属性** | get-property | 获取 Actor 属性 |
| **属性** | set-property | 设置 Actor 属性 |
| **属性** | call-function | 调用函数 |
| **输入** | trigger-input | 模拟输入 |
| **资产** | query-asset | 搜索/检查资产 |
| **资产** | create-asset | 创建资产 |
| **资产** | delete-asset | 删除资产 |
| **资产** | save-asset | 保存资产 |
| **资产** | open-asset | 打开资产 |
| **资产** | get-asset-preview | 资产预览图 |
| **资产** | get-asset-diff | 资产差异 (SCM) |
| **资产** | set-asset-property | 设置资产属性 |
| **资产** | inspect-details-panel | Details 面板结构 |
| **类型** | get-class-hierarchy | 类继承树 |
| **BP** | query-blueprint | BP 结构查询 |
| **BP** | query-blueprint-graph | BP 图查询 |
| **BP** | add-graph-node | 添加图节点 |
| **BP** | remove-graph-node | 移除图节点 |
| **BP** | insert-graph-node | 插入图节点 |
| **BP** | connect-graph-pins | 连接引脚 |
| **BP** | disconnect-graph-pin | 断开引脚 |
| **BP** | set-node-position | 批量设置节点位置 |
| **BP** | set-node-property | 设置节点属性 |
| **BP** | modify-interface | 修改接口 |
| **BP** | compile-blueprint | 编译 BP |
| **BP** | add-datatable-row | DataTable 添加行 |
| **Material** | query-material | Material 查询 |
| **Material** | query-mpc | MPC 读写 |
| **Widget** | inspect-widget-blueprint | Widget BP 结构 |
| **Widget** | inspect-runtime-widgets | 运行时 Widget 几何 |
| **Widget** | add-widget | 添加 Widget |
| **StateTree** | query-statetree | 查询状态树 |
| **StateTree** | add-statetree-state | 添加状态 |
| **StateTree** | add-statetree-task | 添加任务 |
| **StateTree** | add-statetree-transition | 添加转换 |
| **StateTree** | remove-statetree-state | 移除状态 |
| **截图** | capture-viewport | 视口截图 |
| **截图** | capture-screenshot | 编辑器截图 |
| **PIE** | pie-session | PIE 会话控制 |
| **构建** | trigger-live-coding | Live Coding |
| **构建** | build-and-relaunch | 构建并重启 |
| **日志** | get-logs | 获取日志 |
| **控制台** | get-console-var | 获取 CVar |
| **控制台** | set-console-var | 设置 CVar |
| **Insights** | insights-capture | 追踪捕获控制 |
| **Insights** | insights-list-traces | 列出追踪文件 |
| **Insights** | insights-analyze | 分析追踪文件 |
| **项目** | get-project-info | 项目信息 |
| **引用** | find-references | 查找引用 |
| **引用** | find-dependencies | 查找依赖 |
| **脚本** | run-python-script | 执行 Python |
| **Skill** | query_anim_sequence.py | AnimSequence 查询 |
| **Skill** | query_blend_space.py | BlendSpace 查询 |
| **Skill** | query_control_rig.py | ControlRig 查询 |
| **Skill** | query_bone_weights.py | BWA 权重查询 |
| **Skill** | query_physics_asset.py | PhysicsAsset 查询 |
