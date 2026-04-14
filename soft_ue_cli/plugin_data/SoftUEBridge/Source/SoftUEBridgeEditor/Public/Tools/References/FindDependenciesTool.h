// Copyright softdaddy-o 2024. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Tools/BridgeToolBase.h"
#include "AssetRegistry/AssetData.h"
#include "FindDependenciesTool.generated.h"

/**
 * Tool for finding forward dependencies of an asset ("what does X depend on").
 *
 * This is the complement to find-references (which finds "who depends on X").
 * Supports recursive dependency traversal to build a complete dependency tree.
 *
 * Parameters:
 * - asset_path: The asset to query dependencies for
 * - recursive: Whether to recursively follow dependencies (default: false)
 * - depth: Maximum recursion depth when recursive=true (default: 3)
 * - class_filter: Optional filter to only show dependencies of a specific class (e.g., "AnimBlueprint")
 * - path_filter: Optional filter to only show dependencies under a specific path (e.g., "/Game/Pioneer")
 * - limit: Maximum number of results (default: 200)
 * - exclude_engine: Whether to exclude /Engine and /Script paths (default: true)
 */
UCLASS()
class SOFTUEBRIDGEEDITOR_API UFindDependenciesTool : public UBridgeToolBase
{
	GENERATED_BODY()

public:
	virtual FString GetToolName() const override { return TEXT("find-dependencies"); }
	virtual FString GetToolDescription() const override;
	virtual TMap<FString, FBridgeSchemaProperty> GetInputSchema() const override;
	virtual TArray<FString> GetRequiredParams() const override;

	virtual FBridgeToolResult Execute(
		const TSharedPtr<FJsonObject>& Arguments,
		const FBridgeToolContext& Context) override;

private:
	/** Result entry for a single dependency */
	struct FDependencyEntry
	{
		FString Name;
		FString Path;
		FString Package;
		FString Class;
		int32 Depth = 0;
	};

	/** Find direct (non-recursive) dependencies */
	FBridgeToolResult FindDirectDependencies(
		const FString& AssetPath, int32 Limit,
		const FString& ClassFilter, const FString& PathFilter,
		bool bExcludeEngine);

	/** Find recursive dependencies up to MaxDepth */
	FBridgeToolResult FindRecursiveDependencies(
		const FString& AssetPath, int32 MaxDepth, int32 Limit,
		const FString& ClassFilter, const FString& PathFilter,
		bool bExcludeEngine);

	/** Get dependencies for a single package name */
	TArray<FDependencyEntry> GetPackageDependencies(
		FName PackageName, int32 Depth,
		const FString& ClassFilter, const FString& PathFilter,
		bool bExcludeEngine) const;

	/** Convert a dependency entry to JSON */
	TSharedPtr<FJsonObject> DependencyToJson(const FDependencyEntry& Entry) const;

	/** Check if a path should be excluded (engine/script paths) */
	static bool ShouldExcludePath(const FString& Path);

	/** Check if an asset matches the class filter */
	static bool MatchesClassFilter(const FAssetData& AssetData, const FString& ClassFilter);
};
