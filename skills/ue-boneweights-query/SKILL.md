---
name: ue-boneweights-query
description: >
  This skill should be used when the user wants to query, inspect, or analyze internal data of Unreal Engine
  BoneWeightsAsset (BWA) or PerBoneBlendWeight binary assets that cannot be accessed through standard MCP tools.
  Queryable data includes: bone names with translation/rotation weights, default weight info, weight mode, and skeleton reference.
  Trigger phrases include "查看骨骼权重", "BWA骨骼权重", "查询BoneWeightsAsset", "PerBoneBlendWeight",
  "这个BWA有哪些骨骼", "query bone weights", "inspect BWA asset",
  or any request involving BoneWeightsAsset/PerBoneBlendWeight internal data.
---

# UE BoneWeightsAsset (BWA) 深度查询工具

## 目的

通过 UE Python API（`run-python-script` MCP 工具）穿透查询 BoneWeightsAsset (BWA) / PerBoneBlendWeight 二进制资产（.uasset）的内部数据，补充标准 MCP 工具无法覆盖的深层信息。

## 可查询的数据

| 数据类别 | 说明 |
|----------|------|
| **骨骼权重列表** | 每根骨骼的名称、TranslationWeight、RotationWeight |
| **默认权重** | 未单独指定骨骼的默认 Translation/Rotation Weight |
| **权重模式** | WeightMode 设置 |
| **骨架引用** | 关联的 Skeleton 资产路径 |

## 使用方式

### 调用模式

通过 MCP `run-python-script` 工具调用：

```
工具: run-python-script
参数:
  script_path: "Plugins/SoftUEBridge/skills/ue-boneweights-query/scripts/query_bone_weights.py"
  arguments: { "asset_path": "/Game/路径/到/BWA_Asset" }
```

### 调用示例

```json
{
  "script_path": "Plugins/SoftUEBridge/skills/ue-boneweights-query/scripts/query_bone_weights.py",
  "arguments": { "asset_path": "/Game/Pioneer/Animation/BWA_HM_UpperBody" }
}
```

### 返回数据结构

```json
{
  "asset_path": "...",
  "asset_type": "BoneWeightsAsset",
  "skeleton": "/Game/Pioneer/Characters/SK_Avatar.SK_Avatar",
  "weight_mode": "...",
  "default_weight_info": {
    "translation_weight": 0.0,
    "rotation_weight": 0.0
  },
  "bone_weights": [
    { "bone_name": "spine_01", "translation_weight": 1.0, "rotation_weight": 1.0 },
    { "bone_name": "spine_02", "translation_weight": 1.0, "rotation_weight": 1.0 },
    { "bone_name": "upperarm_l", "translation_weight": 0.5, "rotation_weight": 0.5 }
  ],
  "bone_count": 15
}
```

**备注**：对于非标准 UE 类型（如项目自定义的 BWA DataAsset），脚本会尝试通过多种备用策略（属性名变体、反射遍历）来读取骨骼权重数据。

## 重要注意事项

1. **资产路径格式**：必须使用 `/Game/...` 格式的 UE 资产路径，不是文件系统路径
2. **PythonScriptPlugin**：UE 编辑器必须启用 PythonScriptPlugin 才能运行脚本
3. **输出格式**：脚本输出 JSON 格式，通过 `print()` 返回到 MCP
4. **错误处理**：脚本包含健壮的 try/except 保护，不存在的属性会被跳过而非报错
5. **自定义资产兼容**：脚本支持多种属性名变体和反射查找，可适配不同项目的 BWA 实现
6. **路径查找**：如果不确定资产路径，先用 MCP `query-asset` 搜索资产名称确认正确路径

## 工具依赖

本 Skill 依赖以下 MCP 工具：

| 工具 | 用途 |
|------|------|
| `run-python-script` | 执行 UE Python 脚本（核心） |
| `query-asset` | 搜索资产路径（辅助） |
