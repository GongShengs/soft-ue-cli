// Copyright soft-ue-expert. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Tools/BridgeToolBase.h"
#include "InspectDetailsPanelTool.generated.h"

/**
 * Inspects the Details panel structure of a DataAsset or UObject.
 * Returns the complete panel layout including:
 * - Category (rollout) hierarchy with correct ordering
 * - Properties with full metadata (Category, meta tags, default values, widget types)
 * - CallInEditor buttons
 * - Sub-category nesting (struct expansion)
 * - EditorOnly markers
 * - Default category resolution (class name when no explicit Category)
 */
UCLASS()
class SOFTUEBRIDGEEDITOR_API UInspectDetailsPanelTool : public UBridgeToolBase
{
	GENERATED_BODY()

public:
	virtual FString GetToolName() const override { return TEXT("inspect-details-panel"); }
	virtual FString GetToolDescription() const override;
	virtual TMap<FString, FBridgeSchemaProperty> GetInputSchema() const override;
	virtual TArray<FString> GetRequiredParams() const override;

	virtual FBridgeToolResult Execute(
		const TSharedPtr<FJsonObject>& Arguments,
		const FBridgeToolContext& Context) override;

private:
	/** Represents a single item in the details panel (property or button) */
	struct FDetailsPanelItem
	{
		enum class EType { Property, Button };
		EType Type;
		FString Name;
		FString DisplayName;
		FString Category;
		FString SourceClass;
		bool bIsDefaultCategory = false;
		bool bIsEditorOnly = false;

		// Property-specific
		FString PropertyType;
		FString WidgetType;
		FString Value;
		FString DefaultValue;
		TMap<FString, FString> Metadata; // All meta tags

		// Struct children (recursive expansion)
		TArray<FDetailsPanelItem> Children;

		// Button-specific
		FString FunctionName;
		FString Tooltip;
	};

	/** Collect all properties and buttons from the class hierarchy */
	void CollectClassMembers(UClass* Class, UObject* Object, TArray<FDetailsPanelItem>& OutItems,
		int32 MaxDepth, bool bIncludeValues) const;

	/** Process a single FProperty into a FDetailsPanelItem (with recursive struct expansion) */
	FDetailsPanelItem ProcessProperty(FProperty* Property, void* Container, UObject* Owner,
		UClass* OwnerClass, bool bIsEditorOnly, bool bIncludeValues,
		int32 CurrentDepth = 0, int32 MaxDepth = 2) const;

	/** Resolve the display name for a property: prefer UPROPERTY(DisplayName=...) meta, fallback to CamelToDisplayName */
	static FString ResolveDisplayName(FProperty* Property);

	/** Process a single UFunction (CallInEditor) into a FDetailsPanelItem */
	FDetailsPanelItem ProcessButton(UFunction* Function, UClass* OwnerClass) const;

	/** Resolve the default category name for a class */
	static FString GetDefaultCategoryName(UClass* Class);

	/** Convert CamelCase to display name */
	static FString CamelToDisplayName(const FString& Name);

	/** Infer widget type from property type */
	static FString InferWidgetType(FProperty* Property);

	/** Get all metadata from a property as key-value pairs */
	static TMap<FString, FString> GetAllMetadata(FProperty* Property);

	/** Get all metadata from a function */
	static TMap<FString, FString> GetFunctionMetadata(UFunction* Function);

	/** Build the category-grouped JSON output */
	TSharedPtr<FJsonObject> BuildOutput(UObject* Object, const TArray<FDetailsPanelItem>& Items) const;

	/** Recursively build a property JSON node (with children for struct expansion) */
	static TSharedPtr<FJsonObject> BuildPropertyJson(const FDetailsPanelItem& Item, int32& OutPropertyCount);
};
