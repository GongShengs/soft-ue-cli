# soft-ue-cli Skills

soft-ue-cli 附带了一组 [CodeBuddy](https://www.codebuddy.ai) Skills，用于增强 AI 辅助 UE 开发体验。

## 包含的 Skills

| Skill | 说明 |
|-------|------|
| `ue-animsequence-query` | 查询 AnimSequence / AnimMontage 二进制资产的内部数据（时长、曲线、Notify、Root Motion、Montage Slot/Section 等） |
| `ue-blendspace-query` | 查询 BlendSpace / BlendSpace1D / AimOffset 二进制资产的内部数据（轴参数、采样点、插值配置等） |
| `ue-controlrig-query` | 查询 ControlRig Blueprint 二进制资产的内部数据（变量、图表、骨骼层级等） |
| `ue-boneweights-query` | 查询 BoneWeightsAsset (BWA) / PerBoneBlendWeight 二进制资产的内部数据（骨骼权重、默认权重、权重模式等） |
| `ue-physicsasset-query` | 查询 PhysicsAsset 二进制资产的内部数据（Body 列表、碰撞形状、约束列表等） |
| `ue-dataasset-analyzer` | 分析 DataAsset 并生成完整的中文参数详解文档，结合编辑器面板数据和源码分析 |

## 安装方式

### 方式 1：运行安装脚本（推荐）

脚本会自动检测项目根目录，将所有 Skills 复制到 `.codebuddy/skills/` 下。

**Windows (PowerShell)：**

```powershell
powershell -ExecutionPolicy Bypass -File "Plugins/soft-ue-cli/install-skills.ps1"
```

**macOS / Linux：**

```bash
chmod +x Plugins/soft-ue-cli/install-skills.sh
./Plugins/soft-ue-cli/install-skills.sh
```

### 方式 2：手动复制

将 `Plugins/soft-ue-cli/skills/` 下的各个 Skill 文件夹复制到项目根目录的 `.codebuddy/skills/` 中：

```
Plugins/soft-ue-cli/skills/ue-animsequence-query/
    → .codebuddy/skills/ue-animsequence-query/

Plugins/soft-ue-cli/skills/ue-blendspace-query/
    → .codebuddy/skills/ue-blendspace-query/

Plugins/soft-ue-cli/skills/ue-controlrig-query/
    → .codebuddy/skills/ue-controlrig-query/

Plugins/soft-ue-cli/skills/ue-boneweights-query/
    → .codebuddy/skills/ue-boneweights-query/

Plugins/soft-ue-cli/skills/ue-physicsasset-query/
    → .codebuddy/skills/ue-physicsasset-query/

Plugins/soft-ue-cli/skills/ue-dataasset-analyzer/
    → .codebuddy/skills/ue-dataasset-analyzer/
```

### 方式 3：在 CodeBuddy 中直接说

在 CodeBuddy 对话中输入：

> 帮我安装 soft-ue-cli 的 Skills

AI 会自动执行安装脚本。

## 更新 Skills

重新运行安装脚本即可，已有的同名 Skill 会被覆盖更新。

## 添加新 Skill

1. 在 `Plugins/soft-ue-cli/skills/` 下创建新的子目录
2. 参考现有 Skill 的结构编写 `SKILL.md`
3. 重新运行安装脚本

## 目录结构

```
Plugins/soft-ue-cli/
├── skills/                              ← Skill 源文件（随插件分发）
│   ├── ue-animsequence-query/           ← AnimSequence/AnimMontage 查询
│   │   ├── SKILL.md
│   │   ├── scripts/
│   │   │   └── query_anim_sequence.py
│   │   └── references/
│   │       └── python_api_guide.md
│   ├── ue-blendspace-query/             ← BlendSpace/AimOffset 查询
│   │   ├── SKILL.md
│   │   ├── scripts/
│   │   │   └── query_blend_space.py
│   │   └── references/
│   │       └── python_api_guide.md
│   ├── ue-controlrig-query/             ← ControlRig Blueprint 查询
│   │   ├── SKILL.md
│   │   ├── scripts/
│   │   │   └── query_control_rig.py
│   │   └── references/
│   │       └── python_api_guide.md
│   ├── ue-boneweights-query/            ← BoneWeightsAsset 查询
│   │   ├── SKILL.md
│   │   ├── scripts/
│   │   │   └── query_bone_weights.py
│   │   └── references/
│   │       └── python_api_guide.md
│   ├── ue-physicsasset-query/           ← PhysicsAsset 查询
│   │   ├── SKILL.md
│   │   ├── scripts/
│   │   │   └── query_physics_asset.py
│   │   └── references/
│   │       └── python_api_guide.md
│   └── ue-dataasset-analyzer/           ← DataAsset 分析器
│       ├── SKILL.md
│       └── references/
│           ├── document_template.md
│           └── quality_checklist.md
├── install-skills.ps1                   ← Windows 安装脚本
├── install-skills.sh                    ← macOS/Linux 安装脚本
├── soft_ue_cli/                         ← Python CLI + MCP Server 源码
├── docs/                                ← 架构文档
└── tests/                               ← 测试
```
