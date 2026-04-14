# UE Python API 参考指南 — AnimSequence / AnimMontage

本文档记录了 AnimSequence/AnimMontage 查询脚本使用的核心 UE Python API。

## 通用 API

### 资产加载

```python
import unreal

# 加载资产（返回 UObject 或 None）
asset = unreal.load_asset("/Game/Path/To/Asset")

# 获取类名
class_name = asset.get_class().get_name()

# 类型检查
isinstance(asset, unreal.AnimSequence)
isinstance(asset, unreal.AnimMontage)

# 读取属性（通用反射）
value = asset.get_editor_property('property_name')

# 获取资产路径
path = asset.get_path_name()
```

## AnimSequence API

```python
# 基础信息
length = unreal.AnimationLibrary.get_sequence_length(anim_seq)
num_frames = unreal.AnimationLibrary.get_num_frames(anim_seq)

# 曲线
float_curves = unreal.AnimationLibrary.get_animation_curve_names(
    anim_seq, unreal.RawCurveTrackTypes.RCT_FLOAT)
transform_curves = unreal.AnimationLibrary.get_animation_curve_names(
    anim_seq, unreal.RawCurveTrackTypes.RCT_TRANSFORM)

# 属性
anim_seq.get_editor_property('skeleton')          # USkeleton 引用
anim_seq.get_editor_property('rate_scale')          # float
anim_seq.get_editor_property('enable_root_motion')  # bool
anim_seq.get_editor_property('additive_anim_type')  # EAdditiveAnimationType 枚举
anim_seq.get_editor_property('ref_pose_type')       # EAdditiveBasePoseType 枚举
anim_seq.get_editor_property('ref_pose_seq')        # UAnimSequence 引用（Additive 基准）
anim_seq.get_editor_property('notifies')            # TArray<FAnimNotifyEvent>
```

## AnimMontage 特有 API

```python
montage.get_editor_property('slot_anim_tracks')     # TArray<FSlotAnimTrack>
montage.get_editor_property('composite_sections')    # TArray<FCompositeSection>
montage.get_editor_property('blend_in')              # FAlphaBlend
montage.get_editor_property('blend_out_time')        # float

# SlotAnimTrack
track.get_editor_property('slot_name')               # FName
track.get_editor_property('anim_segments')            # TArray<FAnimSegment>

# AnimSegment
seg.get_editor_property('anim_reference')             # UAnimSequence
seg.get_editor_property('anim_start_time')            # float
seg.get_editor_property('anim_end_time')              # float
seg.get_editor_property('anim_play_rate')             # float
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
4. **大数组**：对于大型数组，脚本会限制输出数量
