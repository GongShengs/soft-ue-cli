---
name: ue-controlrig-query
description: >
  This skill should be used when the user wants to query, inspect, or analyze internal data of Unreal Engine
  ControlRig Blueprint binary assets that cannot be accessed through standard MCP tools.
  Queryable data includes: variables (name, type, category), graph/function list, RigVM nodes,
  and bone/space hierarchy.
  Trigger phrases include "分析ControlRig", "查询ControlRig变量", "ControlRig有哪些变量",
  "ControlRig的图表", "query ControlRig variables", "inspect ControlRig graphs",
  "what functions does this ControlRig have", or any request involving ControlRig Blueprint internal data.
---

# UE ControlRig 深度查询工具

## 目的

通过 UE Python API（`run-python-script` MCP 工具）穿透查询 ControlRig Blueprint 二进制资产（.uasset）的内部数据，补充标准 MCP 工具无法覆盖的深层信息。

## 可查询的数据

| 数据类别 | include 参数 | 说明 |
|----------|-------------|------|
| **变量列表** | `"variables"` | 变量名称、类型、分类、是否公开、工具提示 |
| **图表/函数** | `"graphs"` | EventGraph、Function 图表、RigVM 模型图表、节点列表 |
| **骨骼层级** | `"hierarchy"` | ControlRig 层级中的所有元素（骨骼、空间、Control 等）、引用骨架 |
| **全部** | `"all"`（默认） | 以上全部 |

## 使用方式

### 调用模式

通过 MCP `run-python-script` 工具调用：

```
工具: run-python-script
参数:
  script_path: "Plugins/SoftUEBridge/skills/ue-controlrig-query/scripts/query_control_rig.py"
  arguments: { "asset_path": "/Game/路径/到/ControlRig", "include": "all" }
```

### 调用示例

```json
{
  "script_path": "Plugins/SoftUEBridge/skills/ue-controlrig-query/scripts/query_control_rig.py",
  "arguments": { "asset_path": "/Game/Pioneer/Animation/ControlRig/Avatar_CtrlRig_Master", "include": "all" }
}
```

`include` 参数可选值：`"variables"`, `"graphs"`, `"hierarchy"`, `"all"`（默认）

### 返回数据结构

```json
{
  "asset_path": "...",
  "asset_type": "ControlRigBlueprint",
  "parent_class": "ControlRig",
  "variables": [
    { "name": "IKAlpha", "type": "float", "category": "IK", "is_public": true }
  ],
  "variable_count": 5,
  "graphs": [
    {
      "name": "RigVM_Model",
      "graph_type": "RigVM",
      "nodes": [
        { "name": "RigUnit_TwoBoneIK_0", "node_type": "RigVMStructNode" }
      ],
      "node_count": 20
    }
  ],
  "graph_count": 3,
  "hierarchy": {
    "elements": [
      { "name": "pelvis", "type": "Bone" },
      { "name": "IK_foot_l", "type": "Control" }
    ],
    "element_count": 100,
    "skeleton": "/Game/Pioneer/Characters/SK_Avatar.SK_Avatar"
  }
}
```

## 重要注意事项

1. **资产路径格式**：必须使用 `/Game/...` 格式的 UE 资产路径，不是文件系统路径
2. **PythonScriptPlugin**：UE 编辑器必须启用 PythonScriptPlugin 才能运行脚本
3. **输出格式**：脚本输出 JSON 格式，通过 `print()` 返回到 MCP
4. **错误处理**：脚本包含健壮的 try/except 保护，不存在的属性会被跳过而非报错
5. **include 参数**：可以只查询需要的部分，减少输出量（默认是 `"all"`）
6. **路径查找**：如果不确定资产路径，先用 MCP `query-asset` 搜索资产名称确认正确路径

## 工具依赖

本 Skill 依赖以下 MCP 工具：

| 工具 | 用途 |
|------|------|
| `run-python-script` | 执行 UE Python 脚本（核心） |
| `query-asset` | 搜索资产路径（辅助） |
