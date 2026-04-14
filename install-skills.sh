#!/usr/bin/env bash
#
# install-skills.sh — Install SoftUEBridge bundled CodeBuddy skills
#
# Copies all skills from Plugins/soft-ue-cli/skills/ to <ProjectRoot>/.codebuddy/skills/.
# Detects the project root by walking up from the script location looking for a .uproject file.
#
# Usage:
#   ./Plugins/soft-ue-cli/install-skills.sh
#   ./Plugins/soft-ue-cli/install-skills.sh /path/to/project

set -euo pipefail

# ── Colors ────────────────────────────────────────────────────────────────────

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# ── Locate paths ─────────────────────────────────────────────────────────────

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SKILLS_SOURCE_DIR="$SCRIPT_DIR/skills"

if [ ! -d "$SKILLS_SOURCE_DIR" ]; then
    echo -e "${RED}ERROR: Skills source directory not found: $SKILLS_SOURCE_DIR${NC}"
    echo -e "${YELLOW}Make sure this script is in the soft-ue-cli plugin root alongside the 'skills' folder.${NC}"
    exit 1
fi

# Auto-detect project root if not specified
PROJECT_ROOT="${1:-}"

if [ -z "$PROJECT_ROOT" ]; then
    SEARCH_DIR="$SCRIPT_DIR"
    while [ "$SEARCH_DIR" != "/" ] && [ "$SEARCH_DIR" != "." ]; do
        if compgen -G "$SEARCH_DIR/*.uproject" > /dev/null 2>&1; then
            PROJECT_ROOT="$SEARCH_DIR"
            break
        fi
        SEARCH_DIR="$(dirname "$SEARCH_DIR")"
    done

    if [ -z "$PROJECT_ROOT" ]; then
        echo -e "${RED}ERROR: Could not find a .uproject file in any parent directory.${NC}"
        echo -e "${YELLOW}Please specify the project root as an argument.${NC}"
        exit 1
    fi
fi

echo -e "${CYAN}Project root: $PROJECT_ROOT${NC}"

TARGET_DIR="$PROJECT_ROOT/.codebuddy/skills"

# ── Install skills ────────────────────────────────────────────────────────────

SKILL_DIRS=("$SKILLS_SOURCE_DIR"/*)
SKILL_COUNT=0
INSTALLED_COUNT=0

for DIR in "${SKILL_DIRS[@]}"; do
    [ -d "$DIR" ] && SKILL_COUNT=$((SKILL_COUNT + 1))
done

if [ "$SKILL_COUNT" -eq 0 ]; then
    echo -e "${YELLOW}No skills found in $SKILLS_SOURCE_DIR${NC}"
    exit 0
fi

echo ""
echo -e "${GREEN}Installing $SKILL_COUNT skill(s) from soft-ue-cli...${NC}"
echo "  Source: $SKILLS_SOURCE_DIR"
echo "  Target: $TARGET_DIR"
echo ""

for SKILL_DIR in "${SKILL_DIRS[@]}"; do
    [ ! -d "$SKILL_DIR" ] && continue

    SKILL_NAME="$(basename "$SKILL_DIR")"
    DEST_PATH="$TARGET_DIR/$SKILL_NAME"

    # Check if SKILL.md exists
    if [ ! -f "$SKILL_DIR/SKILL.md" ]; then
        echo -e "  ${YELLOW}SKIP: $SKILL_NAME (no SKILL.md found)${NC}"
        continue
    fi

    # Create target and copy
    mkdir -p "$DEST_PATH"
    cp -R "$SKILL_DIR/"* "$DEST_PATH/"

    INSTALLED_COUNT=$((INSTALLED_COUNT + 1))
    if [ -f "$DEST_PATH/SKILL.md" ]; then
        echo -e "  ${GREEN}OK: $SKILL_NAME${NC}"
    else
        echo -e "  ${YELLOW}WARN: $SKILL_NAME (copied but SKILL.md not found in target)${NC}"
    fi
done

echo ""
echo -e "${GREEN}Done! Installed $INSTALLED_COUNT skill(s).${NC}"
echo -e "${CYAN}Skills are now available in CodeBuddy IDE.${NC}"
