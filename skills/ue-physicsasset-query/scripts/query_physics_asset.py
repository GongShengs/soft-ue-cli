"""
PhysicsAsset 查询脚本
========================================
通过 UE Python API 查询 PhysicsAsset 的内部数据：
  - SkeletalBodySetup 列表（骨骼名、碰撞形状、物理类型）
  - ConstraintSetup 列表（骨骼对、约束类型、限制范围）
  - 关联骨架

注意：PhysicsAsset 的 SkeletalBodySetups 属性在 Python API 中为 protected，
因此对 body 数据使用多种备用策略读取。

用法（通过 MCP run-python-script）：
  script_path: "Plugins/SoftUEBridge/skills/ue-physicsasset-query/scripts/query_physics_asset.py"
  arguments: { "asset_path": "/Game/Path/To/PhysicsAsset" }
"""

import unreal
import json

def get_args():
    """获取 MCP 传入的参数"""
    if hasattr(unreal, 'get_mcp_args'):
        return unreal.get_mcp_args()
    return {}

def safe_get(obj, prop_name):
    """安全获取 editor property"""
    try:
        return obj.get_editor_property(prop_name)
    except:
        return None

def extract_body_setup(body_setup, index):
    """从 BodySetup 提取数据"""
    body_info = {"index": index}

    # 骨骼名
    val = safe_get(body_setup, 'bone_name')
    if val is not None:
        body_info["bone_name"] = str(val)

    # 物理类型
    val = safe_get(body_setup, 'physics_type')
    if val is not None:
        body_info["physics_type"] = str(val)

    # 碰撞追踪标志
    val = safe_get(body_setup, 'collision_trace_flag')
    if val is not None:
        body_info["collision_trace_flag"] = str(val)

    # 碰撞响应
    val = safe_get(body_setup, 'collision_response')
    if val is not None:
        body_info["collision_response"] = str(val)

    # 聚合几何体（碰撞形状）
    agg_geom = safe_get(body_setup, 'agg_geom')
    if agg_geom:
        shapes = {}

        # 球体
        try:
            spheres = agg_geom.get_editor_property('sphere_elems')
            if spheres and len(spheres) > 0:
                shapes["spheres"] = len(spheres)
                sphere_list = []
                for s in spheres:
                    si = {}
                    r = safe_get(s, 'radius')
                    if r is not None:
                        si["radius"] = r
                    c = safe_get(s, 'center')
                    if c:
                        si["center"] = {"x": c.x, "y": c.y, "z": c.z}
                    sphere_list.append(si)
                shapes["sphere_details"] = sphere_list
        except:
            pass

        # 胶囊体 (sphyl)
        try:
            capsules = agg_geom.get_editor_property('sphyl_elems')
            if capsules and len(capsules) > 0:
                shapes["capsules"] = len(capsules)
                capsule_list = []
                for c in capsules:
                    ci = {}
                    r = safe_get(c, 'radius')
                    if r is not None:
                        ci["radius"] = r
                    l = safe_get(c, 'length')
                    if l is not None:
                        ci["length"] = l
                    ct = safe_get(c, 'center')
                    if ct:
                        ci["center"] = {"x": ct.x, "y": ct.y, "z": ct.z}
                    capsule_list.append(ci)
                shapes["capsule_details"] = capsule_list
        except:
            pass

        # 盒体
        try:
            boxes = agg_geom.get_editor_property('box_elems')
            if boxes and len(boxes) > 0:
                shapes["boxes"] = len(boxes)
                box_list = []
                for b in boxes:
                    bi = {}
                    for dim in ['x', 'y', 'z']:
                        v = safe_get(b, dim)
                        if v is not None:
                            bi[dim] = v
                    ct = safe_get(b, 'center')
                    if ct:
                        bi["center"] = {"x": ct.x, "y": ct.y, "z": ct.z}
                    box_list.append(bi)
                shapes["box_details"] = box_list
        except:
            pass

        # 凸包
        try:
            convex = agg_geom.get_editor_property('convex_elems')
            if convex and len(convex) > 0:
                shapes["convex_hulls"] = len(convex)
        except:
            pass

        # Tapered Capsule
        try:
            tapered = agg_geom.get_editor_property('tapered_capsule_elems')
            if tapered and len(tapered) > 0:
                shapes["tapered_capsules"] = len(tapered)
        except:
            pass

        body_info["shapes"] = shapes

    # 质量、阻尼
    for attr in ['linear_damping', 'angular_damping', 'consider_for_bounds']:
        val = safe_get(body_setup, attr)
        if val is not None:
            body_info[attr] = val

    return body_info


def extract_constraint_instance(profile, index):
    """从 ConstraintInstance/ConstraintProfileProperties 提取数据"""
    con_info = {"index": index}

    # 骨骼对
    for attr, key in [('constraint_bone1', 'bone1'), ('constraint_bone2', 'bone2')]:
        val = safe_get(profile, attr)
        if val is not None:
            con_info[key] = str(val)

    # Angular 限制
    for attr in ['angular_swing1_motion', 'angular_swing2_motion', 'angular_twist_motion']:
        val = safe_get(profile, attr)
        if val is not None:
            con_info[attr] = str(val)

    # 角度限制值
    for attr in ['swing1_limit_angle', 'swing2_limit_angle', 'twist_limit_angle']:
        val = safe_get(profile, attr)
        if val is not None:
            con_info[attr] = val

    # Linear 限制
    for attr in ['linear_x_motion', 'linear_y_motion', 'linear_z_motion']:
        val = safe_get(profile, attr)
        if val is not None:
            con_info[attr] = str(val)

    val = safe_get(profile, 'linear_limit_size')
    if val is not None:
        con_info['linear_limit_size'] = val

    return con_info


def query_physics_asset(asset_path):
    """查询 PhysicsAsset 的完整内部数据"""
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

    if not isinstance(asset, unreal.PhysicsAsset):
        result["error"] = f"Asset is not a PhysicsAsset, got: {class_name}"
        print(json.dumps(result, indent=2, ensure_ascii=False))
        return

    # === SkeletalBodySetup 列表 ===
    # 注意：UE Python 中 SkeletalBodySetups 通常标记为 protected
    # 尝试多种策略
    bodies = []
    body_strategy = "none"

    # 策略 1: skeletal_body_setups (snake_case)
    try:
        body_setups = asset.get_editor_property('skeletal_body_setups')
        if body_setups:
            for i, bs in enumerate(body_setups):
                bodies.append(extract_body_setup(bs, i))
            body_strategy = "skeletal_body_setups"
    except:
        pass

    # 策略 2: body_setup (常见别名)
    if not bodies:
        for prop_name in ['body_setup', 'body_setups', 'bodies']:
            try:
                body_setups = asset.get_editor_property(prop_name)
                if body_setups:
                    for i, bs in enumerate(body_setups):
                        bodies.append(extract_body_setup(bs, i))
                    body_strategy = prop_name
                    break
            except:
                continue

    # 策略 3: 使用 call_method 尝试引擎内部函数
    if not bodies:
        for method_name in ['GetBodySetups', 'GetSkeletalBodySetups', 'GetBodies']:
            try:
                body_setups = asset.call_method(method_name)
                if body_setups:
                    for i, bs in enumerate(body_setups):
                        bodies.append(extract_body_setup(bs, i))
                    body_strategy = f"call_method({method_name})"
                    break
            except:
                continue

    if not bodies:
        result["body_note"] = ("SkeletalBodySetups is a protected property in UE Python API and cannot be read directly. "
                               "To get body data, use the MCP 'inspect-details-panel' tool on individual SkeletalBodySetup sub-objects, "
                               "or use the MCP 'get-property' tool with the PhysicsAsset path. "
                               "The SkeletalBodySetups value from inspect-details-panel contains the sub-object paths.")

    result["bodies"] = bodies
    result["body_count"] = len(bodies)
    if body_strategy != "none":
        result["body_strategy"] = body_strategy

    # === ConstraintSetup 列表 ===
    constraints = []

    # 方法 1: 使用 get_constraints API
    try:
        constraint_instances = asset.get_constraints(False)
        if constraint_instances and len(constraint_instances) > 0:
            for i, ci in enumerate(constraint_instances):
                constraints.append(extract_constraint_instance(ci, i))
    except:
        pass

    # 方法 2: 使用 get_constraints(includes_terminated=True)
    if not constraints:
        try:
            constraint_instances = asset.get_constraints(True)
            if constraint_instances and len(constraint_instances) > 0:
                for i, ci in enumerate(constraint_instances):
                    constraints.append(extract_constraint_instance(ci, i))
        except:
            pass

    # 方法 3: editor_property
    if not constraints:
        for prop_name in ['constraint_setups', 'constraints']:
            try:
                constraint_setups = asset.get_editor_property(prop_name)
                if constraint_setups:
                    for i, constraint in enumerate(constraint_setups):
                        profile = safe_get(constraint, 'default_instance')
                        if profile:
                            constraints.append(extract_constraint_instance(profile, i))
                        else:
                            constraints.append(extract_constraint_instance(constraint, i))
                    break
            except:
                continue

    result["constraints"] = constraints
    result["constraint_count"] = len(constraints)

    # === 预览骨架网格体 ===
    try:
        preview_mesh = asset.get_editor_property('preview_skeletal_mesh')
        if preview_mesh:
            result["preview_mesh"] = preview_mesh.get_path_name()
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
    query_physics_asset(asset_path)
