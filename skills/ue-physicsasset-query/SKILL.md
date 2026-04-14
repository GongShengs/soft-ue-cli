---
name: ue-physicsasset-query
description: >
  This skill should be used when the user wants to query, inspect, or analyze internal data of Unreal Engine
  PhysicsAsset binary assets that cannot be accessed through standard MCP tools.
  Queryable data includes: SkeletalBodySetup list (bone name, collision shapes, physics type),
  ConstraintSetup list (bone pairs, angular/linear limits), and preview skeletal mesh.
  Trigger phrases include "查看物理资产", "分析PhysicsAsset", "查询物理约束", "physics asset的body列表",
  "analyze physics asset bodies", "query physics constraints", "inspect PhysicsAsset",
  or any request involving PhysicsAsset internal data.
---

# UE PhysicsAsset 深度查询工具

## 目的

通过 UE Python API（`run-python-script` MCP 工具）穿透查询 PhysicsAsset 二进制资产（.uasset）的内部数据，补充标准 MCP 工具无法覆盖的深层信息。

## 可查询的数据

| 数据类别 | 说明 |
|----------|------|
| **Body 列表** | 每个 SkeletalBodySetup 的骨骼名称、物理类型（Simulated/Kinematic）、碰撞标志、碰撞响应 |
| **碰撞形状** | 每个 Body 的聚合几何体：球体（半径/中心）、胶囊体（半径/长度/中心）、盒体（尺寸/中心）、凸包数量 |
| **约束列表** | ConstraintSetup 的骨骼对（Bone1/Bone2）、角度限制（Swing1/Swing2/Twist Motion + 限制角度）、线性限制 |
| **预览网格** | 预览骨骼网格体路径 |

## 使用方式

### 调用模式

通过 MCP `run-python-script` 工具调用：

```
工具: run-python-script
参数:
  script_path: "Plugins/SoftUEBridge/skills/ue-physicsasset-query/scripts/query_physics_asset.py"
  arguments: { "asset_path": "/Game/路径/到/PhysicsAsset" }
```

### 调用示例

```json
{
  "script_path": "Plugins/SoftUEBridge/skills/ue-physicsasset-query/scripts/query_physics_asset.py",
  "arguments": { "asset_path": "/Game/Pioneer/Characters/PA_Avatar" }
}
```

### 返回数据结构

```json
{
  "asset_path": "...",
  "asset_type": "PhysicsAsset",
  "bodies": [
    {
      "index": 0,
      "bone_name": "pelvis",
      "physics_type": "PhysicsType.Kinematic",
      "collision_trace_flag": "...",
      "shapes": {
        "capsules": 1,
        "capsule_details": [{ "radius": 15.0, "length": 10.0, "center": {"x": 0, "y": 0, "z": 0} }]
      }
    }
  ],
  "body_count": 25,
  "constraints": [
    {
      "index": 0,
      "bone1": "pelvis",
      "bone2": "spine_01",
      "angular_swing1_motion": "ACM_Limited",
      "angular_swing2_motion": "ACM_Limited",
      "angular_twist_motion": "ACM_Limited",
      "swing1_limit_angle": 30.0,
      "swing2_limit_angle": 30.0,
      "twist_limit_angle": 15.0
    }
  ],
  "constraint_count": 24,
  "preview_mesh": "/Game/Pioneer/Characters/SK_Avatar.SK_Avatar"
}
```

**注意**：UE Python API 中 `SkeletalBodySetups` 属性通常标记为 protected，脚本使用多种备用策略（属性名变体、call_method）来读取。如果所有策略均失败，结果中会包含 `body_note` 字段说明替代方案。

## 重要注意事项

1. **资产路径格式**：必须使用 `/Game/...` 格式的 UE 资产路径，不是文件系统路径
2. **PythonScriptPlugin**：UE 编辑器必须启用 PythonScriptPlugin 才能运行脚本
3. **输出格式**：脚本输出 JSON 格式，通过 `print()` 返回到 MCP
4. **错误处理**：脚本包含健壮的 try/except 保护，不存在的属性会被跳过而非报错
5. **Protected 属性**：Body 数据读取可能受限于 UE Python API 权限，脚本会尝试多种策略
6. **路径查找**：如果不确定资产路径，先用 MCP `query-asset` 搜索资产名称确认正确路径

## 工具依赖

本 Skill 依赖以下 MCP 工具：

| 工具 | 用途 |
|------|------|
| `run-python-script` | 执行 UE Python 脚本（核心） |
| `query-asset` | 搜索资产路径（辅助） |
| `inspect-details-panel` | Body 数据的备用读取方式 |
| `get-property` | Body 数据的备用读取方式 |
