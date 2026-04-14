"""
AnimSequence / AnimMontage 资产查询脚本
========================================
通过 UE Python API 查询 AnimSequence 或 AnimMontage 的内部数据：
  - 基础信息（时长、帧率、帧数、骨骼）
  - 曲线轨道列表（名称、类型）
  - AnimNotify / AnimNotifyState 列表（名称、触发时间）
  - Root Motion 配置
  - Additive 动画配置（类型、基准 Pose）
  - Montage 特有：SlotGroup / Section 信息

用法（通过 MCP run-python-script）：
  script_path: "Plugins/SoftUEBridge/skills/ue-animsequence-query/scripts/query_anim_sequence.py"
  arguments: { "asset_path": "/Game/Path/To/AnimSequence" }
"""

import unreal
import json

def get_args():
    """获取 MCP 传入的参数"""
    if hasattr(unreal, 'get_mcp_args'):
        return unreal.get_mcp_args()
    return {}

def query_anim_sequence(asset_path):
    """查询 AnimSequence 或 AnimMontage 的完整内部数据"""
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

    # 判断类型
    is_montage = isinstance(asset, unreal.AnimMontage)
    is_sequence = isinstance(asset, unreal.AnimSequenceBase)

    if not is_sequence:
        result["error"] = f"Asset is not an AnimSequence/AnimMontage, got: {class_name}"
        print(json.dumps(result, indent=2, ensure_ascii=False))
        return

    # === 基础信息 ===
    basic_info = {}

    # 时长
    try:
        basic_info["play_length"] = asset.sequence_length if hasattr(asset, 'sequence_length') else 0.0
    except:
        pass

    try:
        length = unreal.AnimationLibrary.get_sequence_length(asset)
        basic_info["play_length"] = length
    except:
        pass

    # 帧率和帧数
    try:
        basic_info["num_frames"] = unreal.AnimationLibrary.get_num_frames(asset)
    except:
        try:
            basic_info["num_frames"] = asset.get_editor_property('number_of_frames') if hasattr(asset, 'number_of_frames') else 0
        except:
            pass

    try:
        rate_struct = asset.get_editor_property('target_frame_rate')
        if rate_struct:
            basic_info["target_frame_rate"] = f"{rate_struct.numerator}/{rate_struct.denominator}"
    except:
        pass

    # 骨骼
    try:
        skeleton = asset.get_editor_property('skeleton')
        if skeleton:
            basic_info["skeleton"] = skeleton.get_path_name()
    except:
        pass

    # Rate Scale
    try:
        basic_info["rate_scale"] = asset.get_editor_property('rate_scale')
    except:
        pass

    # Root Motion
    try:
        basic_info["enable_root_motion"] = asset.get_editor_property('enable_root_motion')
    except:
        basic_info["enable_root_motion"] = False

    try:
        basic_info["force_root_lock"] = asset.get_editor_property('force_root_lock')
    except:
        pass

    # Additive 配置
    try:
        additive_type = asset.get_editor_property('additive_anim_type')
        basic_info["additive_anim_type"] = str(additive_type)
    except:
        pass

    try:
        ref_pose_type = asset.get_editor_property('ref_pose_type')
        basic_info["ref_pose_type"] = str(ref_pose_type)
    except:
        pass

    try:
        ref_pose_seq = asset.get_editor_property('ref_pose_seq')
        if ref_pose_seq:
            basic_info["ref_pose_seq"] = ref_pose_seq.get_path_name()
    except:
        pass

    try:
        basic_info["ref_frame_index"] = asset.get_editor_property('ref_frame_index')
    except:
        pass

    # Interpolation
    try:
        interp = asset.get_editor_property('interpolation')
        basic_info["interpolation"] = str(interp)
    except:
        pass

    result["basic_info"] = basic_info

    # === 曲线轨道 ===
    curves = []
    try:
        # 使用 AnimationLibrary 获取曲线名称
        curve_names = unreal.AnimationLibrary.get_animation_curve_names(asset, unreal.RawCurveTrackTypes.RCT_FLOAT)
        for name in curve_names:
            curves.append({"name": str(name), "type": "Float"})
    except:
        pass

    try:
        transform_curves = unreal.AnimationLibrary.get_animation_curve_names(asset, unreal.RawCurveTrackTypes.RCT_TRANSFORM)
        for name in transform_curves:
            curves.append({"name": str(name), "type": "Transform"})
    except:
        pass

    # 备用方式：直接通过属性
    if not curves:
        try:
            raw_curve_data = asset.get_editor_property('raw_curve_data')
            if raw_curve_data:
                float_curves = raw_curve_data.get_editor_property('float_curves')
                if float_curves:
                    for curve in float_curves:
                        try:
                            cname = curve.get_editor_property('name')
                            curves.append({"name": str(cname.display_name) if hasattr(cname, 'display_name') else str(cname), "type": "Float"})
                        except:
                            curves.append({"name": "Unknown", "type": "Float"})
        except:
            pass

    result["curves"] = curves
    result["curve_count"] = len(curves)

    # === AnimNotify 列表 ===
    notifies = []
    try:
        notify_list = asset.get_editor_property('notifies')
        if notify_list:
            for notify_event in notify_list:
                notify_info = {}
                try:
                    notify_info["trigger_time"] = notify_event.get_editor_property('trigger_time_offset')
                except:
                    pass
                try:
                    link = notify_event.get_editor_property('link')
                    if link:
                        notify_info["link_time"] = link.get_editor_property('link_value')
                except:
                    pass
                try:
                    notify_info["notify_name"] = notify_event.get_editor_property('notify_name')
                except:
                    pass
                try:
                    notify_obj = notify_event.get_editor_property('notify')
                    if notify_obj:
                        notify_info["notify_class"] = notify_obj.get_class().get_name()
                    else:
                        notify_info["notify_class"] = "None"
                except:
                    pass
                try:
                    state_obj = notify_event.get_editor_property('notify_state_class')
                    if state_obj:
                        notify_info["notify_state_class"] = state_obj.get_class().get_name()
                        try:
                            notify_info["duration"] = notify_event.get_editor_property('duration')
                        except:
                            pass
                except:
                    pass
                try:
                    notify_info["trigger_time"] = notify_event.get_editor_property('trigger_time_offset')
                except:
                    pass
                notifies.append(notify_info)
    except:
        pass

    # 备用方式
    if not notifies:
        try:
            anim_notify_tracks = unreal.AnimationLibrary.get_animation_notify_events(asset)
            if anim_notify_tracks:
                for evt in anim_notify_tracks:
                    notify_info = {"notify_name": str(evt)}
                    notifies.append(notify_info)
        except:
            pass

    result["notifies"] = notifies
    result["notify_count"] = len(notifies)

    # === Montage 特有信息 ===
    if is_montage:
        montage_info = {}

        # Slot 信息
        try:
            slot_anim_tracks = asset.get_editor_property('slot_anim_tracks')
            slots = []
            if slot_anim_tracks:
                for track in slot_anim_tracks:
                    slot_info = {}
                    try:
                        slot_info["slot_name"] = str(track.get_editor_property('slot_name'))
                    except:
                        pass
                    try:
                        anim_segments = track.get_editor_property('anim_segments')
                        if anim_segments:
                            segs = []
                            for seg in anim_segments:
                                seg_info = {}
                                try:
                                    anim_ref = seg.get_editor_property('anim_reference')
                                    seg_info["anim_reference"] = anim_ref.get_path_name() if anim_ref else "None"
                                except:
                                    pass
                                try:
                                    seg_info["anim_start_time"] = seg.get_editor_property('anim_start_time')
                                except:
                                    pass
                                try:
                                    seg_info["anim_end_time"] = seg.get_editor_property('anim_end_time')
                                except:
                                    pass
                                try:
                                    seg_info["anim_play_rate"] = seg.get_editor_property('anim_play_rate')
                                except:
                                    pass
                                segs.append(seg_info)
                            slot_info["segments"] = segs
                    except:
                        pass
                    slots.append(slot_info)
            montage_info["slots"] = slots
        except:
            pass

        # Section 信息
        try:
            sections = asset.get_editor_property('composite_sections')
            section_list = []
            if sections:
                for section in sections:
                    sec_info = {}
                    try:
                        sec_info["section_name"] = str(section.get_editor_property('section_name'))
                    except:
                        pass
                    try:
                        sec_info["start_time"] = section.get_editor_property('start_time')
                    except:
                        pass
                    try:
                        next_sec = section.get_editor_property('next_section_name')
                        sec_info["next_section"] = str(next_sec)
                    except:
                        pass
                    section_list.append(sec_info)
            montage_info["sections"] = section_list
        except:
            pass

        # Blend In/Out
        try:
            blend_in = asset.get_editor_property('blend_in')
            if blend_in:
                montage_info["blend_in_time"] = blend_in.get_editor_property('blend_time')
        except:
            pass
        try:
            montage_info["blend_out_time"] = asset.get_editor_property('blend_out_time')
        except:
            pass

        result["montage_info"] = montage_info

    # === Sync Markers ===
    try:
        sync_markers = asset.get_editor_property('unique_marker_names')
        if sync_markers:
            result["sync_markers"] = [str(m) for m in sync_markers]
    except:
        pass

    # 输出结果
    print(json.dumps(result, indent=2, ensure_ascii=False, default=str))


# === 入口 ===
args = get_args()
asset_path = args.get("asset_path", "")

if not asset_path:
    print(json.dumps({"error": "Missing required argument: asset_path"}, indent=2))
else:
    query_anim_sequence(asset_path)
