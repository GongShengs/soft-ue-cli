// Copyright softdaddy-o 2024. All Rights Reserved.

#include "Tools/References/FindDependenciesTool.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "SoftUEBridgeEditorModule.h"

FString UFindDependenciesTool::GetToolDescription() const
{
	return TEXT("Find forward dependencies of an asset (what does X depend on / reference). "
		"This is the complement to find-references (which finds who depends on X). "
		"Use recursive=true to build a full dependency tree. "
		"Use class_filter to narrow results to specific asset types (e.g., 'AnimBlueprint', 'Blueprint', 'Texture2D'). "
		"Use path_filter to only show dependencies under a specific content path. "
		"By default, engine/script dependencies are excluded (set exclude_engine=false to include them).");
}

TMap<FString, FBridgeSchemaProperty> UFindDependenciesTool::GetInputSchema() const
{
	TMap<FString, FBridgeSchemaProperty> Schema;

	FBridgeSchemaProperty AssetPath;
	AssetPath.Type = TEXT("string");
	AssetPath.Description = TEXT("Asset path to find dependencies for (e.g., /Game/Pioneer/Characters/Avatar/ABP_Master)");
	AssetPath.bRequired = true;
	Schema.Add(TEXT("asset_path"), AssetPath);

	FBridgeSchemaProperty Recursive;
	Recursive.Type = TEXT("boolean");
	Recursive.Description = TEXT("Whether to recursively follow dependencies to build a full tree (default: false)");
	Recursive.bRequired = false;
	Schema.Add(TEXT("recursive"), Recursive);

	FBridgeSchemaProperty Depth;
	Depth.Type = TEXT("integer");
	Depth.Description = TEXT("Maximum recursion depth when recursive=true (default: 3, max: 10)");
	Depth.bRequired = false;
	Schema.Add(TEXT("depth"), Depth);

	FBridgeSchemaProperty ClassFilter;
	ClassFilter.Type = TEXT("string");
	ClassFilter.Description = TEXT("Filter dependencies by asset class name (e.g., 'AnimBlueprint', 'Blueprint', 'Texture2D', 'SkeletalMesh'). Supports partial/substring match.");
	ClassFilter.bRequired = false;
	Schema.Add(TEXT("class_filter"), ClassFilter);

	FBridgeSchemaProperty PathFilter;
	PathFilter.Type = TEXT("string");
	PathFilter.Description = TEXT("Filter dependencies to only those under this path prefix (e.g., '/Game/Pioneer')");
	PathFilter.bRequired = false;
	Schema.Add(TEXT("path_filter"), PathFilter);

	FBridgeSchemaProperty Limit;
	Limit.Type = TEXT("integer");
	Limit.Description = TEXT("Maximum number of dependency results to return (default: 200)");
	Limit.bRequired = false;
	Schema.Add(TEXT("limit"), Limit);

	FBridgeSchemaProperty ExcludeEngine;
	ExcludeEngine.Type = TEXT("boolean");
	ExcludeEngine.Description = TEXT("Whether to exclude engine/script dependencies like /Engine/*, /Script/* (default: true)");
	ExcludeEngine.bRequired = false;
	Schema.Add(TEXT("exclude_engine"), ExcludeEngine);

	return Schema;
}

TArray<FString> UFindDependenciesTool::GetRequiredParams() const
{
	return { TEXT("asset_path") };
}

FBridgeToolResult UFindDependenciesTool::Execute(
	const TSharedPtr<FJsonObject>& Arguments,
	const FBridgeToolContext& Context)
{
	FString AssetPath;
	if (!GetStringArg(Arguments, TEXT("asset_path"), AssetPath))
	{
		return FBridgeToolResult::Error(TEXT("Missing required parameter: asset_path"));
	}

	bool bRecursive = GetBoolArgOrDefault(Arguments, TEXT("recursive"), false);
	int32 MaxDepth = FMath::Clamp(GetIntArgOrDefault(Arguments, TEXT("depth"), 3), 1, 10);
	int32 Limit = GetIntArgOrDefault(Arguments, TEXT("limit"), 200);
	FString ClassFilter = GetStringArgOrDefault(Arguments, TEXT("class_filter"), TEXT(""));
	FString PathFilter = GetStringArgOrDefault(Arguments, TEXT("path_filter"), TEXT(""));
	bool bExcludeEngine = GetBoolArgOrDefault(Arguments, TEXT("exclude_engine"), true);

	UE_LOG(LogSoftUEBridgeEditor, Log, TEXT("find-dependencies: path='%s', recursive=%d, depth=%d, limit=%d, class='%s', pathFilter='%s', excludeEngine=%d"),
		*AssetPath, bRecursive, MaxDepth, Limit, *ClassFilter, *PathFilter, bExcludeEngine);

	if (bRecursive)
	{
		return FindRecursiveDependencies(AssetPath, MaxDepth, Limit, ClassFilter, PathFilter, bExcludeEngine);
	}
	else
	{
		return FindDirectDependencies(AssetPath, Limit, ClassFilter, PathFilter, bExcludeEngine);
	}
}

FBridgeToolResult UFindDependenciesTool::FindDirectDependencies(
	const FString& AssetPath, int32 Limit,
	const FString& ClassFilter, const FString& PathFilter,
	bool bExcludeEngine)
{
	FString PackagePath = FPackageName::ObjectPathToPackageName(AssetPath);
	FName PackageName = FName(*PackagePath);

	TArray<FDependencyEntry> Entries = GetPackageDependencies(PackageName, 0, ClassFilter, PathFilter, bExcludeEngine);

	// Build result
	TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
	Result->SetStringField(TEXT("asset_path"), AssetPath);
	Result->SetBoolField(TEXT("recursive"), false);

	TArray<TSharedPtr<FJsonValue>> DepsArray;
	int32 Count = 0;
	for (const FDependencyEntry& Entry : Entries)
	{
		if (Count >= Limit)
		{
			break;
		}
		DepsArray.Add(MakeShareable(new FJsonValueObject(DependencyToJson(Entry))));
		Count++;
	}

	Result->SetArrayField(TEXT("dependencies"), DepsArray);
	Result->SetNumberField(TEXT("count"), DepsArray.Num());
	Result->SetNumberField(TEXT("total_found"), Entries.Num());
	Result->SetBoolField(TEXT("truncated"), Entries.Num() > Limit);

	return FBridgeToolResult::Json(Result);
}

FBridgeToolResult UFindDependenciesTool::FindRecursiveDependencies(
	const FString& AssetPath, int32 MaxDepth, int32 Limit,
	const FString& ClassFilter, const FString& PathFilter,
	bool bExcludeEngine)
{
	FString PackagePath = FPackageName::ObjectPathToPackageName(AssetPath);
	FName RootPackageName = FName(*PackagePath);

	// BFS traversal to build dependency tree
	TSet<FName> Visited;
	TArray<FDependencyEntry> AllEntries;

	struct FQueueItem
	{
		FName PackageName;
		int32 Depth;
	};

	TArray<FQueueItem> Queue;
	Queue.Add({ RootPackageName, 0 });
	Visited.Add(RootPackageName);

	while (Queue.Num() > 0 && AllEntries.Num() < Limit)
	{
		FQueueItem Current = Queue[0];
		Queue.RemoveAt(0);

		if (Current.Depth >= MaxDepth)
		{
			continue;
		}

		TArray<FDependencyEntry> DirectDeps = GetPackageDependencies(
			Current.PackageName, Current.Depth + 1, ClassFilter, PathFilter, bExcludeEngine);

		for (const FDependencyEntry& Entry : DirectDeps)
		{
			if (AllEntries.Num() >= Limit)
			{
				break;
			}

			FName EntryPackageName = FName(*Entry.Package);

			// Always add to results (even if visited, for tree representation)
			AllEntries.Add(Entry);

			// Only recurse into unvisited packages
			if (!Visited.Contains(EntryPackageName))
			{
				Visited.Add(EntryPackageName);

				if (Current.Depth + 1 < MaxDepth)
				{
					Queue.Add({ EntryPackageName, Current.Depth + 1 });
				}
			}
		}
	}

	// Build result
	TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
	Result->SetStringField(TEXT("asset_path"), AssetPath);
	Result->SetBoolField(TEXT("recursive"), true);
	Result->SetNumberField(TEXT("max_depth"), MaxDepth);

	TArray<TSharedPtr<FJsonValue>> DepsArray;
	for (const FDependencyEntry& Entry : AllEntries)
	{
		DepsArray.Add(MakeShareable(new FJsonValueObject(DependencyToJson(Entry))));
	}

	Result->SetArrayField(TEXT("dependencies"), DepsArray);
	Result->SetNumberField(TEXT("count"), DepsArray.Num());
	Result->SetNumberField(TEXT("unique_packages_visited"), Visited.Num());
	Result->SetBoolField(TEXT("truncated"), AllEntries.Num() >= Limit);

	return FBridgeToolResult::Json(Result);
}

TArray<UFindDependenciesTool::FDependencyEntry> UFindDependenciesTool::GetPackageDependencies(
	FName PackageName, int32 Depth,
	const FString& ClassFilter, const FString& PathFilter,
	bool bExcludeEngine) const
{
	TArray<FDependencyEntry> Entries;

	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();

	// Get dependencies for this package
	TArray<FAssetIdentifier> Dependencies;
	AssetRegistry.GetDependencies(PackageName, Dependencies);

	for (const FAssetIdentifier& Dep : Dependencies)
	{
		// Only process package dependencies
		if (Dep.PackageName == NAME_None)
		{
			continue;
		}

		FString DepPackageStr = Dep.PackageName.ToString();

		// Exclude engine paths if requested
		if (bExcludeEngine && ShouldExcludePath(DepPackageStr))
		{
			continue;
		}

		// Apply path filter
		if (!PathFilter.IsEmpty() && !DepPackageStr.StartsWith(PathFilter))
		{
			continue;
		}

		// Get assets in this dependency package
		TArray<FAssetData> Assets;
		AssetRegistry.GetAssetsByPackageName(Dep.PackageName, Assets);

		if (Assets.Num() == 0)
		{
			// Package exists but has no indexed assets (could be a code/script package)
			// Still add it if no class filter is active
			if (ClassFilter.IsEmpty())
			{
				FDependencyEntry Entry;
				Entry.Name = FPackageName::GetShortName(DepPackageStr);
				Entry.Path = DepPackageStr;
				Entry.Package = DepPackageStr;
				Entry.Class = TEXT("Unknown");
				Entry.Depth = Depth;
				Entries.Add(Entry);
			}
			continue;
		}

		for (const FAssetData& Asset : Assets)
		{
			// Apply class filter
			if (!ClassFilter.IsEmpty() && !MatchesClassFilter(Asset, ClassFilter))
			{
				continue;
			}

			FDependencyEntry Entry;
			Entry.Name = Asset.AssetName.ToString();
			Entry.Path = Asset.GetObjectPathString();
			Entry.Package = Asset.PackageName.ToString();
			Entry.Class = Asset.AssetClassPath.GetAssetName().ToString();
			Entry.Depth = Depth;
			Entries.Add(Entry);
		}
	}

	return Entries;
}

TSharedPtr<FJsonObject> UFindDependenciesTool::DependencyToJson(const FDependencyEntry& Entry) const
{
	TSharedPtr<FJsonObject> Obj = MakeShareable(new FJsonObject);
	Obj->SetStringField(TEXT("name"), Entry.Name);
	Obj->SetStringField(TEXT("path"), Entry.Path);
	Obj->SetStringField(TEXT("package"), Entry.Package);
	Obj->SetStringField(TEXT("class"), Entry.Class);
	Obj->SetNumberField(TEXT("depth"), Entry.Depth);
	return Obj;
}

bool UFindDependenciesTool::ShouldExcludePath(const FString& Path)
{
	return Path.StartsWith(TEXT("/Engine/"))
		|| Path.StartsWith(TEXT("/Script/"))
		|| Path.StartsWith(TEXT("/Paper2D/"))
		|| Path.StartsWith(TEXT("/Niagara/"))
		|| Path == TEXT("/Engine")
		|| Path == TEXT("/Script");
}

bool UFindDependenciesTool::MatchesClassFilter(const FAssetData& AssetData, const FString& ClassFilter)
{
	if (ClassFilter.IsEmpty())
	{
		return true;
	}

	FString ClassName = AssetData.AssetClassPath.GetAssetName().ToString();

	// Exact match
	if (ClassName.Equals(ClassFilter, ESearchCase::IgnoreCase))
	{
		return true;
	}

	// Substring match
	if (ClassName.Contains(ClassFilter))
	{
		return true;
	}

	return false;
}
