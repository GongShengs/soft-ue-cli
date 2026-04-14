"""
ControlRig 资产查询脚本
========================================
通过 UE Python API 查询 ControlRig Blueprint 的内部数据：
  - 变量列表（名称、类型、默认值）
  - 函数/图表列表
  - 图表节点信息（RigUnit 类型、连接关系）
  - 骨骼/空间层级

用法（通过 MCP run-python-script）：
  script_path: "Plugins/SoftUEBridge/skills/ue-controlrig-query/scripts/query_control_rig.py"
  arguments: { "asset_path": "/Game/Path/To/ControlRig", "include": "all" }

include 参数可选值: "variables", "graphs", "hierarchy", "all"（默认）
"""

import unreal
import json

def get_args():
    """获取 MCP 传入的参数"""
    if hasattr(unreal, 'get_mcp_args'):
        return unreal.get_mcp_args()
    return {}

def query_control_rig(asset_path, include="all"):
    """查询 ControlRig Blueprint 的完整内部数据"""
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

    # 判断是否是 ControlRig Blueprint
    is_control_rig_bp = False
    if hasattr(unreal, 'ControlRigBlueprint'):
        is_control_rig_bp = isinstance(asset, unreal.ControlRigBlueprint)
    elif 'ControlRig' in class_name:
        is_control_rig_bp = True

    if not is_control_rig_bp:
        result["error"] = f"Asset is not a ControlRig Blueprint, got: {class_name}"
        print(json.dumps(result, indent=2, ensure_ascii=False))
        return

    # === 基础信息 ===
    try:
        parent_class = asset.get_editor_property('parent_class')
        if parent_class:
            result["parent_class"] = parent_class.get_name()
    except:
        pass

    # === 变量 ===
    include_variables = include in ("all", "variables")
    include_graphs = include in ("all", "graphs")
    include_hierarchy = include in ("all", "hierarchy")

    if include_variables:
        variables = []
        try:
            # 尝试通过 Blueprint 变量列表
            new_vars = asset.get_editor_property('new_variables')
            if new_vars:
                for var in new_vars:
                    var_info = {}
                    try:
                        var_info["name"] = str(var.get_editor_property('var_name'))
                    except:
                        try:
                            var_info["name"] = str(var.get_editor_property('name'))
                        except:
                            var_info["name"] = "Unknown"
                    try:
                        var_type = var.get_editor_property('var_type')
                        var_info["type"] = str(var_type)
                    except:
                        pass
                    try:
                        var_info["category"] = str(var.get_editor_property('category'))
                    except:
                        pass
                    try:
                        var_info["is_public"] = var.get_editor_property('is_public') if hasattr(var, 'is_public') else None
                    except:
                        pass
                    try:
                        var_info["tooltip"] = str(var.get_editor_property('tool_tip'))
                    except:
                        pass
                    variables.append(var_info)
        except:
            pass

        # 备用：尝试 ControlRig 特有的变量接口
        if not variables:
            try:
                # ControlRig 可能有 Model/Hierarchy 中的变量
                model = asset.get_editor_property('model') if hasattr(asset, 'model') else None
                if model:
                    # 遍历 RigVMModel 图表中的变量
                    try:
                        local_vars = model.get_editor_property('local_variables')
                        if local_vars:
                            for lv in local_vars:
                                variables.append({
                                    "name": str(lv.get_editor_property('name')) if hasattr(lv, 'name') else str(lv),
                                    "source": "RigVM_local"
                                })
                    except:
                        pass
            except:
                pass

        result["variables"] = variables
        result["variable_count"] = len(variables)

    # === 图表 ===
    if include_graphs:
        graphs = []
        try:
            # UBlueprint 的标准图表接口
            all_graphs = asset.get_editor_property('uber_graph_pages')
            if all_graphs:
                for graph in all_graphs:
                    graph_info = extract_graph_info(graph)
                    graphs.append(graph_info)
        except:
            pass

        try:
            func_graphs = asset.get_editor_property('function_graphs')
            if func_graphs:
                for graph in func_graphs:
                    graph_info = extract_graph_info(graph)
                    graph_info["graph_type"] = "Function"
                    graphs.append(graph_info)
        except:
            pass

        # ControlRig 特有的 RigVM 图表
        try:
            rig_vm_graphs = asset.get_editor_property('rig_graph_display_settings') if hasattr(asset, 'rig_graph_display_settings') else None
        except:
            pass

        # 尝试获取 RigVM 模型
        try:
            model = None
            if hasattr(asset, 'get_model'):
                model = asset.get_model()
            elif hasattr(asset, 'model'):
                model = asset.get_editor_property('model')

            if model:
                rig_graph_info = {
                    "name": "RigVM_Model",
                    "graph_type": "RigVM"
                }
                # 尝试获取节点
                try:
                    nodes = model.get_editor_property('nodes') if hasattr(model, 'nodes') else None
                    if nodes:
                        node_list = []
                        for node in nodes:
                            node_info = {}
                            try:
                                node_info["name"] = str(node.get_editor_property('name'))
                            except:
                                node_info["name"] = str(node)
                            try:
                                node_info["node_type"] = node.get_class().get_name()
                            except:
                                pass
                            try:
                                node_info["title"] = str(node.get_editor_property('node_title'))
                            except:
                                pass
                            node_list.append(node_info)
                        rig_graph_info["nodes"] = node_list
                        rig_graph_info["node_count"] = len(node_list)
                except:
                    pass
                graphs.append(rig_graph_info)
        except:
            pass

        # 尝试通过 EdGraphs
        try:
            if hasattr(asset, 'get_all_graphs'):
                all_g = asset.get_all_graphs()
                for g in all_g:
                    found = False
                    for existing in graphs:
                        if existing.get("name") == g.get_name():
                            found = True
                            break
                    if not found:
                        graphs.append(extract_graph_info(g))
        except:
            pass

        result["graphs"] = graphs
        result["graph_count"] = len(graphs)

    # === 骨骼层级 ===
    if include_hierarchy:
        hierarchy = {}
        try:
            # ControlRig 通常有 hierarchy 属性
            rig_hierarchy = None
            if hasattr(asset, 'hierarchy'):
                rig_hierarchy = asset.get_editor_property('hierarchy')

            if rig_hierarchy:
                # 尝试获取所有元素
                elements = []
                try:
                    all_keys = rig_hierarchy.get_all_keys()
                    for key in all_keys:
                        elem_info = {}
                        try:
                            elem_info["name"] = str(key.get_editor_property('name'))
                        except:
                            elem_info["name"] = str(key)
                        try:
                            elem_info["type"] = str(key.get_editor_property('type'))
                        except:
                            pass
                        elements.append(elem_info)
                except:
                    pass
                hierarchy["elements"] = elements
                hierarchy["element_count"] = len(elements)
        except:
            pass

        # 备用：读取引用的 Skeleton
        try:
            skeleton = asset.get_editor_property('target_skeleton') if hasattr(asset, 'target_skeleton') else None
            if skeleton is None:
                skeleton = asset.get_editor_property('skeleton') if hasattr(asset, 'skeleton') else None
            if skeleton:
                hierarchy["skeleton"] = skeleton.get_path_name()
        except:
            pass

        result["hierarchy"] = hierarchy

    # 输出结果
    print(json.dumps(result, indent=2, ensure_ascii=False, default=str))


def extract_graph_info(graph):
    """从 UEdGraph 提取基础信息"""
    info = {}
    try:
        info["name"] = graph.get_name()
    except:
        info["name"] = str(graph)
    try:
        info["graph_class"] = graph.get_class().get_name()
    except:
        pass
    try:
        info["graph_type"] = "EventGraph"  # 默认
        gclass = graph.get_class().get_name()
        if 'Function' in gclass:
            info["graph_type"] = "Function"
        elif 'RigVM' in gclass or 'ControlRig' in gclass:
            info["graph_type"] = "RigVM"
    except:
        pass

    # 节点列表
    try:
        nodes = graph.get_editor_property('nodes') if hasattr(graph, 'nodes') else None
        if nodes is None:
            # 尝试用 Members
            nodes = graph.get_editor_property('members') if hasattr(graph, 'members') else None

        if nodes:
            node_summaries = []
            for node in nodes:
                node_info = {}
                try:
                    node_info["name"] = node.get_name()
                except:
                    node_info["name"] = str(node)
                try:
                    node_info["class"] = node.get_class().get_name()
                except:
                    pass
                try:
                    node_info["title"] = str(node.get_editor_property('node_title'))
                except:
                    pass
                node_summaries.append(node_info)
            info["nodes"] = node_summaries
            info["node_count"] = len(node_summaries)
    except:
        pass

    return info


# === 入口 ===
args = get_args()
asset_path = args.get("asset_path", "")
include = args.get("include", "all")

if not asset_path:
    print(json.dumps({"error": "Missing required argument: asset_path"}, indent=2))
else:
    query_control_rig(asset_path, include)
