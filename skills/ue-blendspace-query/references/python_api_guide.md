# UE Python API 参考指南 — BlendSpace / AimOffset

本文档记录了 BlendSpace 查询脚本使用的核心 UE Python API。

## 通用 API

### 资产加载

```python
import unreal

# 加载资产（返回 UObject 或 None）
asset = unreal.load_asset("/Game/Path/To/Asset")

# 获取类名
class_name = asset.get_class().get_name()

# 类型检查
isinstance(asset, unreal.BlendSpace)
isinstance(asset, unreal.BlendSpace1D)
isinstance(asset, unreal.AimOffsetBlendSpace)

# 读取属性（通用反射）
value = asset.get_editor_property('property_name')

# 获取资产路径
path = asset.get_path_name()
```

## BlendSpace API

```python
# 轴参数（BlendParameter 结构体）
bs.get_editor_property('blend_parameters')     # 可能是数组
# 单独的轴：
param.get_editor_property('display_name')      # FString
param.get_editor_property('min')               # float
param.get_editor_property('max')               # float
param.get_editor_property('grid_num')          # int

# 采样点
bs.get_editor_property('sample_data')          # TArray<FBlendSample>
# 或
bs.get_editor_property('blend_samples')
# 每个采样点：
sample.get_editor_property('animation')        # UAnimSequence 引用
sample.get_editor_property('sample_value')     # FVector（x, y, z 坐标）
sample.get_editor_property('rate_scale')       # float
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

1. **属性不存在**：不同 UE 版本的属性名可能不同，脚本中用 `try/except` 保护
2. **枚举值**：UE Python 中枚举转字符串用 `str(enum_value)`
3. **路径格式**：必须是 `/Game/...` 格式，不能是文件系统路径
4. **AimOffset**：AimOffset 是 BlendSpace 的子类，脚本会自动识别
