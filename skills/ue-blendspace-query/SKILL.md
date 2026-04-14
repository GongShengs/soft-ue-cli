---
name: ue-blendspace-query
description: >
  This skill should be used when the user wants to query, inspect, or analyze internal data of Unreal Engine
  BlendSpace, BlendSpace1D, or AimOffset binary assets that cannot be accessed through standard MCP tools.
  Queryable data includes: axis parameters (name, range, grid divisions), sample points (coordinates + animation references),
  interpolation config, and target weight settings.
  Trigger phrases include "查看BlendSpace", "查询混合空间", "混合空间的采样点", "AimOffset的轴参数",
  "inspect BlendSpace samples", "query BlendSpace axes", "what animations are in this BlendSpace",
  or any request involving BlendSpace/BlendSpace1D/AimOffset internal data.
---

# UE BlendSpace 深度查询工具

## 目的

通过 UE Python API（`run-python-script` MCP 工具）穿透查询 BlendSpace / BlendSpace1D / AimOffset 二进制资产（.uasset）的内部数据，补充标准 MCP 工具无法覆盖的深层信息。

## 支持的资产类型

| 资产类型 | 可查询的数据 |
|----------|------------|
| **BlendSpace** | 轴参数（显示名称、最小/最大值、网格数）、采样点列表（XY 坐标+对应 AnimSequence+Rate Scale）、插值参数、目标权重插值速度、Notify 触发模式 |
| **BlendSpace1D** | 同上，但只有 X 轴 |
| **AimOffset** | 同 BlendSpace，额外标记 `is_aim_offset: true` |

## 使用方式

### 调用模式

通过 MCP `run-python-script` 工具调用：

```
工具: run-python-script
参数:
  script_path: "Plugins/SoftUEBridge/skills/ue-blendspace-query/scripts/query_blend_space.py"
  arguments: { "asset_path": "/Game/路径/到/BlendSpace" }
```

### 调用示例

```json
{
  "script_path": "Plugins/SoftUEBridge/skills/ue-blendspace-query/scripts/query_blend_space.py",
  "arguments": { "asset_path": "/Game/Pioneer/Animation/Locomotion/BS_Walk" }
}
```

### 返回数据结构

```json
{
  "asset_path": "...",
  "asset_type": "BlendSpace",
  "skeleton": "/Game/Pioneer/Characters/SK_Avatar.SK_Avatar",
  "is_1d": false,
  "axes": {
    "axis_X": { "index": 0, "display_name": "Direction", "min": -180, "max": 180, "grid_num": 4 },
    "axis_Y": { "index": 1, "display_name": "Speed", "min": 0, "max": 600, "grid_num": 4 }
  },
  "samples": [
    { "index": 0, "animation": "/Game/.../AS_Walk_F", "animation_name": "AS_Walk_F", "x": 0, "y": 300, "rate_scale": 1.0 },
    { "index": 1, "animation": "/Game/.../AS_Walk_B", "animation_name": "AS_Walk_B", "x": 180, "y": 300, "rate_scale": 1.0 }
  ],
  "sample_count": 9,
  "interpolation_params": [
    { "axis": "X", "interpolation_time": "0.0", "interpolation_type": "..." }
  ],
  "interpolation_config": {
    "target_weight_interpolation_speed_per_sec": 5.0,
    "notify_trigger_mode": "..."
  }
}
```

## 重要注意事项

1. **资产路径格式**：必须使用 `/Game/...` 格式的 UE 资产路径，不是文件系统路径
2. **PythonScriptPlugin**：UE 编辑器必须启用 PythonScriptPlugin 才能运行脚本
3. **输出格式**：脚本输出 JSON 格式，通过 `print()` 返回到 MCP
4. **错误处理**：脚本包含健壮的 try/except 保护，不存在的属性会被跳过而非报错
5. **AimOffset 识别**：脚本会自动识别 AimOffset（BlendSpace 的子类），并在结果中标记
6. **路径查找**：如果不确定资产路径，先用 MCP `query-asset` 搜索资产名称确认正确路径

## 工具依赖

本 Skill 依赖以下 MCP 工具：

| 工具 | 用途 |
|------|------|
| `run-python-script` | 执行 UE Python 脚本（核心） |
| `query-asset` | 搜索资产路径（辅助） |
