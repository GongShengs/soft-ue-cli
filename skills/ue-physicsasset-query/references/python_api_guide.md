# UE Python API 参考指南 — PhysicsAsset

本文档记录了 PhysicsAsset 查询脚本使用的核心 UE Python API。

## 通用 API

### 资产加载

```python
import unreal

# 加载资产（返回 UObject 或 None）
asset = unreal.load_asset("/Game/Path/To/Asset")

# 获取类名
class_name = asset.get_class().get_name()

# 类型检查
isinstance(asset, unreal.PhysicsAsset)

# 读取属性（通用反射）
value = asset.get_editor_property('property_name')

# 获取资产路径
path = asset.get_path_name()
```

## PhysicsAsset API

```python
phys_asset.get_editor_property('skeletal_body_setups')  # TArray<USkeletalBodySetup>
phys_asset.get_editor_property('constraint_setups')      # TArray<UPhysicsConstraintTemplate>

# SkeletalBodySetup
body.get_editor_property('bone_name')         # FName
body.get_editor_property('physics_type')      # EPhysicsType 枚举
body.get_editor_property('agg_geom')          # FKAggregateGeom

# FKAggregateGeom（碰撞形状）
geom.get_editor_property('sphere_elems')         # TArray<FKSphereElem>
geom.get_editor_property('sphyl_elems')          # TArray<FKSphylElem>（胶囊）
geom.get_editor_property('box_elems')            # TArray<FKBoxElem>
geom.get_editor_property('convex_elems')         # TArray<FKConvexElem>

# PhysicsConstraintTemplate
constraint.get_editor_property('default_instance')  # FConstraintInstance
instance.get_editor_property('constraint_bone1')    # FName
instance.get_editor_property('constraint_bone2')    # FName
instance.get_editor_property('angular_swing1_motion')  # EAngularConstraintMotion
```

## Body 读取策略

由于 `SkeletalBodySetups` 在 UE Python API 中通常标记为 protected，脚本使用以下备用策略：

```python
# 策略 1: snake_case 属性名
asset.get_editor_property('skeletal_body_setups')

# 策略 2: 常见别名
asset.get_editor_property('body_setup')
asset.get_editor_property('body_setups')
asset.get_editor_property('bodies')

# 策略 3: call_method
asset.call_method('GetBodySetups')
asset.call_method('GetSkeletalBodySetups')
asset.call_method('GetBodies')
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
4. **Protected 属性**：Body 数据读取可能受限，如果 Python API 无法读取，可使用 MCP `inspect-details-panel` 或 `get-property` 工具作为备用方案
