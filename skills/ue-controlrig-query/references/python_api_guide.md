# UE Python API 参考指南 — ControlRig Blueprint

本文档记录了 ControlRig 查询脚本使用的核心 UE Python API。

## 通用 API

### 资产加载

```python
import unreal

# 加载资产（返回 UObject 或 None）
asset = unreal.load_asset("/Game/Path/To/Asset")

# 获取类名
class_name = asset.get_class().get_name()

# 类型检查 — ControlRig 通常是 ControlRigBlueprint
# isinstance(asset, unreal.ControlRigBlueprint)

# 读取属性（通用反射）
value = asset.get_editor_property('property_name')

# 获取资产路径
path = asset.get_path_name()
```

## ControlRig Blueprint API

```python
# 变量列表
variables = asset.get_editor_property('new_variables')  # TArray<FBPVariableDescription>
# 每个变量：
var.get_editor_property('var_name')       # FName
var.get_editor_property('var_type')       # FEdGraphPinType
var.get_editor_property('category')       # FText
var.get_editor_property('property_flags') # 包含 CPF_BlueprintVisible 等标志
var.get_editor_property('default_value')  # FString

# 父类
parent = asset.get_editor_property('parent_class')

# 图表
graphs = asset.get_editor_property('ubergraph_pages')    # TArray<UEdGraph>
function_graphs = asset.get_editor_property('function_graphs')

# RigVM 模型
model = asset.get_editor_property('rig_vm_model')  # 或其他访问方式
```

## ControlRig Hierarchy

```python
# 层级（骨骼、Control、Space 等）
hierarchy = asset.get_editor_property('hierarchy')
# 遍历层级元素
```

## MCP 参数传递

```python
# MCP run-python-script 通过 unreal.get_mcp_args() 传递参数
args = unreal.get_mcp_args()
asset_path = args.get("asset_path", "")
include = args.get("include", "all")  # "variables", "graphs", "hierarchy", "all"

# 结果通过 print() 输出 JSON
import json
print(json.dumps(result, indent=2, ensure_ascii=False, default=str))
```

## 常见问题

1. **属性不存在**：不同 UE 版本的属性名可能不同，脚本中用 `try/except` 保护
2. **枚举值**：UE Python 中枚举转字符串用 `str(enum_value)`
3. **路径格式**：必须是 `/Game/...` 格式，不能是文件系统路径
4. **include 参数**：支持 `"variables"`, `"graphs"`, `"hierarchy"`, `"all"` 四种选项
