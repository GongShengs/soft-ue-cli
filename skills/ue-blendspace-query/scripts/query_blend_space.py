"""
BlendSpace / BlendSpace1D 资产查询脚本
========================================
通过 UE Python API 查询 BlendSpace 的内部数据：
  - 轴参数（名称、范围、网格分割数）
  - 采样点列表（坐标、对应 AnimSequence、Rate Scale）
  - 插值配置
  - 目标权重插值设置

用法（通过 MCP run-python-script）：
  script_path: "Plugins/SoftUEBridge/skills/ue-blendspace-query/scripts/query_blend_space.py"
  arguments: { "asset_path": "/Game/Path/To/BlendSpace" }
"""

import unreal
import json

def get_args():
    """获取 MCP 传入的参数"""
    if hasattr(unreal, 'get_mcp_args'):
        return unreal.get_mcp_args()
    return {}

def query_blend_space(asset_path):
    """查询 BlendSpace 或 BlendSpace1D 的完整内部数据"""
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

    is_1d = isinstance(asset, unreal.BlendSpace1D) if hasattr(unreal, 'BlendSpace1D') else ('1D' in class_name)
    is_bs = isinstance(asset, unreal.BlendSpace) if hasattr(unreal, 'BlendSpace') else ('BlendSpace' in class_name)

    if not is_bs and not is_1d:
        # 尝试更宽泛的检查
        if 'BlendSpace' not in class_name and 'AimOffset' not in class_name:
            result["error"] = f"Asset is not a BlendSpace type, got: {class_name}"
            print(json.dumps(result, indent=2, ensure_ascii=False))
            return

    # === 骨骼 ===
    try:
        skeleton = asset.get_editor_property('skeleton')
        if skeleton:
            result["skeleton"] = skeleton.get_path_name()
    except:
        pass

    # === 轴参数 ===
    axes = {}

    # 使用 blend_parameters FixedArray[3] (X, Y, Z)
    try:
        bp = asset.get_editor_property('blend_parameters')
        if bp:
            axis_labels = ["X", "Y", "Z"]
            for i, param in enumerate(bp):
                axis_info = extract_blend_parameter(param, i)
                axes[f"axis_{axis_labels[i]}"] = axis_info
    except:
        pass

    # 如果主方法失败，尝试备用方式
    if not axes:
        axes["axis_X"] = {"index": 0, "display_name": "X"}
        if not is_1d:
            axes["axis_Y"] = {"index": 1, "display_name": "Y"}

    result["axes"] = axes
    result["is_1d"] = is_1d

    # === 采样点 ===
    samples = []
    try:
        sample_data = asset.get_editor_property('sample_data')
        if sample_data:
            for i, sample in enumerate(sample_data):
                sample_info = {"index": i}
                try:
                    anim = sample.get_editor_property('animation')
                    sample_info["animation"] = anim.get_path_name() if anim else "None"
                    if anim:
                        sample_info["animation_name"] = anim.get_name()
                except:
                    sample_info["animation"] = "Unknown"

                try:
                    value = sample.get_editor_property('sample_value')
                    if value:
                        if hasattr(value, 'x'):
                            sample_info["x"] = value.x
                        if hasattr(value, 'y'):
                            sample_info["y"] = value.y
                        if hasattr(value, 'z'):
                            sample_info["z"] = value.z
                except:
                    pass

                try:
                    sample_info["rate_scale"] = float(sample.get_editor_property('rate_scale'))
                except:
                    pass

                samples.append(sample_info)
    except Exception as e:
        result["sample_error"] = str(e)

    result["samples"] = samples
    result["sample_count"] = len(samples)

    # === 插值配置 (interpolation_param FixedArray[3]) ===
    interp_params = []
    try:
        ip = asset.get_editor_property('interpolation_param')
        if ip:
            axis_labels = ["X", "Y", "Z"]
            for i, param in enumerate(ip):
                ip_info = {"axis": axis_labels[i]}
                for attr in ['interpolation_time', 'interpolation_type', 'damping_ratio', 'max_speed']:
                    try:
                        ip_info[attr] = str(param.get_editor_property(attr))
                    except:
                        pass
                interp_params.append(ip_info)
    except:
        pass
    result["interpolation_params"] = interp_params

    # === 全局插值设置 ===
    interp_config = {}
    try:
        interp_config["target_weight_interpolation_speed_per_sec"] = asset.get_editor_property('target_weight_interpolation_speed_per_sec')
    except:
        pass
    try:
        interp_config["notify_trigger_mode"] = str(asset.get_editor_property('notify_trigger_mode'))
    except:
        pass
    result["interpolation_config"] = interp_config

    # === AimOffset 特有 ===
    if 'AimOffset' in class_name:
        result["is_aim_offset"] = True

    # 输出结果
    print(json.dumps(result, indent=2, ensure_ascii=False, default=str))


def extract_blend_parameter(param, index):
    """从 BlendParameter 结构体提取数据"""
    info = {"index": index}
    try:
        dn = param.get_editor_property('display_name')
        info["display_name"] = str(dn) if dn and str(dn) != 'None' else None
    except:
        pass
    try:
        info["min"] = float(param.get_editor_property('min'))
    except:
        pass
    try:
        info["max"] = float(param.get_editor_property('max'))
    except:
        pass
    try:
        info["grid_num"] = int(param.get_editor_property('grid_num'))
    except:
        pass
    return info


# === 入口 ===
args = get_args()
asset_path = args.get("asset_path", "")

if not asset_path:
    print(json.dumps({"error": "Missing required argument: asset_path"}, indent=2))
else:
    query_blend_space(asset_path)
