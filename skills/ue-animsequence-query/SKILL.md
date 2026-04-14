---
name: ue-animsequence-query
description: >
  This skill should be used when the user wants to query, inspect, or analyze internal data of Unreal Engine
  AnimSequence or AnimMontage binary assets that cannot be accessed through standard MCP tools.
  Queryable data includes: play length, frame rate, curve tracks, AnimNotify list, Root Motion config,
  Additive animation settings, Montage Slot/Section info.
  Trigger phrases include "查询动画序列", "查看AnimSequence", "分析AnimMontage", "这个动画有哪些曲线",
  "这个Montage有哪些Slot", "query AnimSequence curves", "inspect AnimMontage sections",
  "what notifies does this animation have", or any request involving AnimSequence/AnimMontage internal data.
---

# UE AnimSequence / AnimMontage 深度查询工具

## 目的

通过 UE Python API（`run-python-script` MCP 工具）穿透查询 AnimSequence 或 AnimMontage 二进制资产（.uasset）的内部数据，补充标准 MCP 工具无法覆盖的深层信息。

## 支持的资产类型

| 资产类型 | 可查询的数据 |
|----------|------------|
| **AnimSequence** | 时长、帧率、帧数、骨骼、曲线轨道列表（名称+类型）、AnimNotify/AnimNotifyState 列表（触发时间+类名）、Root Motion 配置、Additive 动画配置（类型、基准 Pose）、Rate Scale、插值模式、Sync Markers |
| **AnimMontage** | 以上全部 + SlotGroup/Slot 信息、AnimSegment 列表（引用动画+起止时间+播放速率）、CompositeSection 列表（Section 名称+起始时间+下一 Section）、Blend In/Out 时间 |

## 使用方式

### 调用模式

通过 MCP `run-python-script` 工具调用：

```
工具: run-python-script
参数:
  script_path: "Plugins/SoftUEBridge/skills/ue-animsequence-query/scripts/query_anim_sequence.py"
  arguments: { "asset_path": "/Game/路径/到/AnimSequence或AnimMontage" }
```

### 调用示例

```json
{
  "script_path": "Plugins/SoftUEBridge/skills/ue-animsequence-query/scripts/query_anim_sequence.py",
  "arguments": { "asset_path": "/Game/Pioneer/Animation/Rifle/AS_Rifle_Idle" }
}
```

### 返回数据结构

```json
{
  "asset_path": "...",
  "asset_type": "AnimSequence",
  "basic_info": {
    "play_length": 2.5,
    "num_frames": 75,
    "target_frame_rate": "30/1",
    "skeleton": "/Game/Pioneer/Characters/SK_Avatar.SK_Avatar",
    "enable_root_motion": false,
    "additive_anim_type": "AAT_None",
    "rate_scale": 1.0
  },
  "curves": [
    { "name": "AO_Yaw", "type": "Float" },
    { "name": "IsMoving", "type": "Float" }
  ],
  "curve_count": 2,
  "notifies": [
    { "notify_class": "AnimNotify_PlaySound", "trigger_time": 0.5 }
  ],
  "notify_count": 1
}
```

**AnimMontage 额外返回**：

```json
{
  "montage_info": {
    "slots": [
      {
        "slot_name": "DefaultGroup.DefaultSlot",
        "segments": [
          { "anim_reference": "/Game/.../AS_Fire", "anim_start_time": 0.0, "anim_end_time": 1.5, "anim_play_rate": 1.0 }
        ]
      }
    ],
    "sections": [
      { "section_name": "Default", "start_time": 0.0, "next_section": "None" }
    ],
    "blend_in_time": 0.25,
    "blend_out_time": 0.25
  }
}
```

## 重要注意事项

1. **资产路径格式**：必须使用 `/Game/...` 格式的 UE 资产路径，不是文件系统路径
2. **PythonScriptPlugin**：UE 编辑器必须启用 PythonScriptPlugin 才能运行脚本
3. **输出格式**：脚本输出 JSON 格式，通过 `print()` 返回到 MCP
4. **错误处理**：脚本包含健壮的 try/except 保护，不存在的属性会被跳过而非报错
5. **路径查找**：如果不确定资产路径，先用 MCP `query-asset` 搜索资产名称确认正确路径

## 工具依赖

本 Skill 依赖以下 MCP 工具：

| 工具 | 用途 |
|------|------|
| `run-python-script` | 执行 UE Python 脚本（核心） |
| `query-asset` | 搜索资产路径（辅助） |
