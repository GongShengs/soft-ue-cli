# UE Python API 参考指南 — BoneWeightsAsset (BWA)

本文档记录了 BoneWeightsAsset 查询脚本使用的核心 UE Python API。

## 通用 API

### 资产加载

```python
import unreal

# 加载资产（返回 UObject 或 None）
asset = unreal.load_asset("/Game/Path/To/Asset")

# 获取类名
class_name = asset.get_class().get_name()

# 读取属性（通用反射）
value = asset.get_editor_property('property_name')

# 获取资产路径
path = asset.get_path_name()
```

## BoneWeightsAsset API

```python
# 骨架引用
asset.get_editor_property('skeleton')              # USkeleton 引用

# 权重模式
asset.get_editor_property('weight_mode')           # 枚举

# 默认权重信息
dwi = asset.get_editor_property('default_weight_info')
dwi.get_editor_property('translation_weight')      # float
dwi.get_editor_property('rotation_weight')         # float

# 骨骼权重映射
bwi = asset.get_editor_property('bone_weight_infos')  # Map<FName, BoneWeightInfo>
for bone_name, weight_info in bwi.items():
    weight_info.get_editor_property('translation_weight')  # float
    weight_info.get_editor_property('rotation_weight')     # float
```

## 备用属性名（不同项目实现）

```python
# 脚本会按顺序尝试以下属性名：
'bone_weight_infos'          # 标准
'bone_weights'               # 别名
'blend_weights'              # 别名
'per_bone_blend_weights'     # 别名
'weights'                    # 别名
'bone_blend_weights'         # 别名
```

## MCP 参数传递

```python
# MCP run-python-script 通过 unreal.get_mcp_args() 传递参数
args = unreal.get_mcp_args()
asset_path = args.get("asset_path", "")

# 结果通过 print() 输出 JSON
import json
print(json.dumps(result, indent=2, ensure_ascii=False, default=str))
```

## 常见问题

1. **属性不存在**：不同 UE 版本和项目的属性名可能不同，脚本会尝试多种备用策略
2. **枚举值**：UE Python 中枚举转字符串用 `str(enum_value)`
3. **路径格式**：必须是 `/Game/...` 格式，不能是文件系统路径
4. **反射查找**：如果所有常见属性名都不匹配，脚本会通过 `dir()` 反射查找含 bone/weight/blend 关键词的属性
