---
name: ue-dataasset-analyzer
description: >
  This skill should be used when the user wants to analyze, document, or understand an Unreal Engine DataAsset.
  It generates comprehensive parameter documentation that combines UE Editor Details Panel data with source code analysis.
  Trigger phrases include "分析这个DataAsset", "写一份参数详解", "查看DA的配置", "document this data asset",
  "帮我看看这个DA", or any request involving DataAsset parameter explanation.
  This skill covers: reading panel structure and current/default values from UE Editor via MCP tools,
  finding complete enum/struct/class definitions in AngelScript/C++ source code,
  and producing a structured markdown document with every property fully expanded.
---

# UE DataAsset 参数详解生成器

## 目的

从 UE 编辑器和项目源码两个维度，为指定的 DataAsset 生成一份完整的中文参数详解文档。文档同时包含：
- **面板视角**：Details Panel 的完整布局、控件类型、当前值、默认值
- **代码视角**：枚举全部值、结构体全部字段、类继承关系、源码位置
- **数据视角**：Map/Array/Transform 等复合类型逐项拆开，不允许笼统概括

## 工作流程（严格按顺序执行）

### Phase 1: 数据采集

1. **获取面板数据**：调用 MCP 工具 `inspect-details-panel`，参数：
   - `asset_path`：用户提供的资产路径（如 `/Game/Pioneer/Items/...`）
   - `include_values`: `true`
   - `depth`: `4`（最大深度，确保所有子结构展开）

2. **路径修正**：如果资产路径不对导致查询失败，使用 `query-asset` 搜索资产名称确认正确路径，然后重新查询。

3. **记录元信息**：从返回结果中记录：
   - `asset_class`（类名）
   - `class_hierarchy`（继承链）
   - `property_count`（属性总数）
   - `button_count`（按钮总数）
   - 所有 category 的名称和数量

### Phase 2: 源码分析

对 Phase 1 中获取到的**每一个属性类型**，执行以下分析：

#### 枚举类型（enum）
- 使用 `search_content` 搜索 `enum {枚举名}`，找到源码中的完整定义
- 记录：文件路径、行号、所有枚举值及其注释
- 如果枚举值有代码注释（如 `//TODO: Remove`），必须在文档中标注

#### 结构体类型（struct）
- 使用 `search_content` 搜索 `struct {结构体名}`，找到源码中的完整定义
- 记录：文件路径、行号、所有 UPROPERTY 字段及其类型和默认值
- 如果结构体字段本身是枚举或结构体，递归执行分析

#### 类类型（class）
- 对于引用类型属性（如 `TObjectPtr<XXX>`），查找被引用类的继承关系
- 使用 `get-class-hierarchy` 或 `search_content` 搜索 `class U{类名}` / `class A{类名}`

#### TMap 的 Key 类型
- 如果 Key 是枚举类型，必须找到枚举的完整定义
- 在文档中列出所有枚举值，标注哪些在当前资产的 Map 中有条目、哪些没有

#### 按钮关联函数
- 使用 `search_content` 搜索按钮的 `function_name`
- 简要分析函数做了什么（1~2 句话即可）

### Phase 3: 文档生成

按照 `references/document_template.md` 中的模板格式生成文档。

#### 核心规则（不可违反）

1. **枚举必须完整**：枚举类型必须列出源码中定义的**全部值**，不能只列出当前资产使用的值。每个枚举值都要标注「当前是否使用」。

2. **结构体必须完整**：结构体类型必须列出源码中定义的**全部字段**，即使当前资产中某些字段为默认值也要列出。

3. **Map/Array 必须逐项展开**：TMap 的每个条目、TArray 的每个元素都必须单独列出，不能写"N 条"然后不展开。

4. **Transform 必须拆分量**：FTransform 必须拆开为 Rotation（四元数 XYZW）、Translation（XYZ）、Scale3D（XYZ）。Identity 可简写标注，非 Identity 必须列出完整数值。

5. **当前值 + 默认值双列**：每个属性都必须同时列出当前值和默认值，即使二者相同。

6. **源码位置标注**：枚举和结构体的完整定义必须标注源码文件路径和行号。

7. **EditorOnly 标注**：编辑器专用属性必须在当前值前加 ⚠️ 标记。

8. **EditCondition 说明**：有编辑条件的属性必须说明依赖关系。

### Phase 4: 质量自检

文档生成后，按照 `references/quality_checklist.md` 逐项检查：

1. **完整性**：属性数量是否与 `property_count` 一致，按钮数量是否与 `button_count` 一致
2. **类型展开**：所有枚举/结构体是否都查了源码并列出完整定义
3. **数据准确性**：当前值和默认值是否与 MCP 工具返回的一致
4. **格式规范**：表格是否对齐，路径是否正确

如发现遗漏，立即补充修正。

## 输出位置

文档输出为 Markdown 文件，命名规则：`{资产名}_参数详解.md`，保存在工作区根目录。

## 工具依赖

本 Skill 依赖以下 MCP 工具（来自 `soft-ue-bridge` 服务器）：

| 工具 | 用途 |
|------|------|
| `inspect-details-panel` | 获取 DataAsset 的完整面板结构、属性值 |
| `query-asset` | 按名称搜索资产，确认正确路径 |
| `get-class-hierarchy` | 查询类继承关系 |

以及 CodeBuddy 内置工具：

| 工具 | 用途 |
|------|------|
| `search_content` | 在源码中搜索枚举/结构体/类/函数的完整定义 |
| `read_file` | 读取源码文件获取完整上下文 |
| `write_to_file` | 输出生成的文档 |
