"""
BoneWeightsAsset (BWA) / PerBoneBlendWeight 查询脚本
========================================
通过 UE Python API 查询 BoneWeightsAsset 或相关混合权重资产的内部数据：
  - 骨骼名称列表及每骨骼的 TranslationWeight / RotationWeight
  - 默认权重信息
  - 权重模式（WeightMode）
  - 来源骨架引用

用法（通过 MCP run-python-script）：
  script_path: "Plugins/SoftUEBridge/skills/ue-boneweights-query/scripts/query_bone_weights.py"
  arguments: { "asset_path": "/Game/Path/To/BWA_Asset" }
"""

import unreal
import json

def get_args():
    """获取 MCP 传入的参数"""
    if hasattr(unreal, 'get_mcp_args'):
        return unreal.get_mcp_args()
    return {}

def serialize_value(val):
    """将 UE 对象值序列化为 JSON 兼容格式"""
    if val is None:
        return None
    if isinstance(val, (bool, int, float, str)):
        return val
    if isinstance(val, (list, tuple)):
        return [serialize_value(v) for v in val[:200]]
    if isinstance(val, dict):
        return {str(k): serialize_value(v) for k, v in list(val.items())[:200]}
    try:
        if hasattr(val, 'get_path_name'):
            return val.get_path_name()
    except:
        pass
    try:
        if hasattr(val, 'get_name'):
            return val.get_name()
    except:
        pass
    return str(val)

def query_bone_weights(asset_path):
    """查询 BoneWeightsAsset 的完整内部数据"""
    result = {
        "asset_path": asset_path,
        "asset_type": "",
        "error": None
    }

    # 加载资产
    asset = unreal.load_asset(asset_path)
    if asset is None:
        result["error"] = f"Failed to load asset: {asset_path}"
        print(json.dumps(result, indent=2, ensure_ascii=False))
        return

    class_name = asset.get_class().get_name()
    result["asset_type"] = class_name

    # === 骨架引用 ===
    try:
        skel = asset.get_editor_property('skeleton')
        if skel:
            result["skeleton"] = skel.get_path_name()
    except:
        pass

    # === 权重模式 ===
    try:
        result["weight_mode"] = str(asset.get_editor_property('weight_mode'))
    except:
        pass

    # === 默认权重 ===
    try:
        dwi = asset.get_editor_property('default_weight_info')
        if dwi:
            default_info = {}
            for attr in ['translation_weight', 'rotation_weight']:
                try:
                    default_info[attr] = float(dwi.get_editor_property(attr))
                except:
                    pass
            result["default_weight_info"] = default_info
    except:
        pass

    # === bone_weight_infos Map<BoneName, BoneWeightInfo> ===
    bone_weights = []
    try:
        bwi = asset.get_editor_property('bone_weight_infos')
        if bwi is not None and hasattr(bwi, 'items'):
            for bone_name, weight_info in bwi.items():
                entry = {"bone_name": str(bone_name)}
                for attr in ['translation_weight', 'rotation_weight']:
                    try:
                        entry[attr] = float(weight_info.get_editor_property(attr))
                    except:
                        pass
                bone_weights.append(entry)
    except Exception as e:
        result["bone_weight_infos_error"] = str(e)

    # === 备用：尝试其他常见属性名 ===
    if not bone_weights:
        fallback_names = [
            'bone_weights', 'blend_weights', 'per_bone_blend_weights',
            'weights', 'bone_blend_weights'
        ]
        for prop_name in fallback_names:
            try:
                val = asset.get_editor_property(prop_name)
                if val is not None:
                    if isinstance(val, (list, tuple)):
                        for i, item in enumerate(val):
                            entry = {"index": i}
                            if isinstance(item, (int, float)):
                                entry["weight"] = item
                            else:
                                for attr in ['bone_name', 'blend_weight', 'weight', 'bone_index']:
                                    try:
                                        entry[attr] = serialize_value(item.get_editor_property(attr))
                                    except:
                                        pass
                            bone_weights.append(entry)
                        result["source_property"] = prop_name
                        break
                    elif hasattr(val, 'items'):
                        for k, v in val.items():
                            bone_weights.append({"bone_name": str(k), "weight": serialize_value(v)})
                        result["source_property"] = prop_name
                        break
            except:
                continue

    # === 备用：通过 dir() 反射查找权重相关属性 ===
    if not bone_weights:
        extra_props = {}
        for attr_name in dir(asset):
            if attr_name.startswith('_'):
                continue
            lower = attr_name.lower()
            if 'bone' in lower or 'weight' in lower or 'blend' in lower:
                try:
                    val = getattr(asset, attr_name)
                    if not callable(val):
                        extra_props[attr_name] = serialize_value(val)
                except:
                    pass
        if extra_props:
            result["reflected_properties"] = extra_props

    result["bone_weights"] = bone_weights
    result["bone_count"] = len(bone_weights)

    # 输出结果
    print(json.dumps(result, indent=2, ensure_ascii=False, default=str))


# === 入口 ===
args = get_args()
asset_path = args.get("asset_path", "")

if not asset_path:
    print(json.dumps({"error": "Missing required argument: asset_path"}, indent=2))
else:
    query_bone_weights(asset_path)
