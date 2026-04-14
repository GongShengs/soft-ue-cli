// Copyright soft-ue-expert. All Rights Reserved.

#include "Tools/Asset/InspectDetailsPanelTool.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/DataAsset.h"
#include "UObject/UnrealType.h"
#include "UObject/EnumProperty.h"
#include "UObject/TextProperty.h"
#include "UObject/Class.h"
#include "UObject/ObjectMacros.h"
#include "UObject/MetaData.h"
#include "UObject/FieldPathProperty.h"
#include "Tools/BridgeToolResult.h"
#include "SoftUEBridgeEditorModule.h"

// Forward declaration for recursive type string helper
static FString InferPropertyTypeStringImpl(FProperty* Property);

FString UInspectDetailsPanelTool::GetToolDescription() const
{
	return TEXT("Inspect the Details panel structure of a DataAsset or any UObject. "
		"Returns the complete panel layout with categories (rollouts), properties, "
		"CallInEditor buttons, metadata, widget types, and nesting. "
		"Resolves default categories for properties without explicit Category. "
		"Use 'asset_path' to specify the target asset.");
}

TMap<FString, FBridgeSchemaProperty> UInspectDetailsPanelTool::GetInputSchema() const
{
	TMap<FString, FBridgeSchemaProperty> Schema;

	FBridgeSchemaProperty AssetPath;
	AssetPath.Type = TEXT("string");
	AssetPath.Description = TEXT("Asset path to inspect (e.g., /Game/Pioneer/Items/...)");
	AssetPath.bRequired = true;
	Schema.Add(TEXT("asset_path"), AssetPath);

	FBridgeSchemaProperty IncludeValues;
	IncludeValues.Type = TEXT("boolean");
	IncludeValues.Description = TEXT("Include current property values (default: true)");
	IncludeValues.bRequired = false;
	Schema.Add(TEXT("include_values"), IncludeValues);

	FBridgeSchemaProperty Depth;
	Depth.Type = TEXT("integer");
	Depth.Description = TEXT("Struct expansion depth for sub-categories (default: 2, max: 4)");
	Depth.bRequired = false;
	Schema.Add(TEXT("depth"), Depth);

	return Schema;
}

TArray<FString> UInspectDetailsPanelTool::GetRequiredParams() const
{
	return { TEXT("asset_path") };
}

FBridgeToolResult UInspectDetailsPanelTool::Execute(
	const TSharedPtr<FJsonObject>& Arguments,
	const FBridgeToolContext& Context)
{
	FString AssetPath;
	if (!GetStringArg(Arguments, TEXT("asset_path"), AssetPath))
	{
		return FBridgeToolResult::Error(TEXT("Missing required parameter: asset_path"));
	}

	bool bIncludeValues = GetBoolArgOrDefault(Arguments, TEXT("include_values"), true);
	int32 MaxDepth = FMath::Clamp(GetIntArgOrDefault(Arguments, TEXT("depth"), 2), 1, 4);

	// Load asset
	UObject* Object = LoadObject<UObject>(nullptr, *AssetPath);
	if (!Object)
	{
		return FBridgeToolResult::Error(FString::Printf(TEXT("Failed to load asset: %s"), *AssetPath));
	}

	// Collect all items
	TArray<FDetailsPanelItem> Items;
	CollectClassMembers(Object->GetClass(), Object, Items, MaxDepth, bIncludeValues);

	// Build output
	TSharedPtr<FJsonObject> Result = BuildOutput(Object, Items);
	return FBridgeToolResult::Json(Result);
}

void UInspectDetailsPanelTool::CollectClassMembers(UClass* Class, UObject* Object,
	TArray<FDetailsPanelItem>& OutItems, int32 MaxDepth, bool bIncludeValues) const
{
	if (!Class || !Object)
	{
		return;
	}

	// Build class hierarchy (child first)
	TArray<UClass*> ClassHierarchy;
	for (UClass* C = Class; C && C != UObject::StaticClass(); C = C->GetSuperClass())
	{
		ClassHierarchy.Add(C);
	}

	// Process each class in hierarchy (child classes first for correct Details panel ordering)
	for (UClass* CurrentClass : ClassHierarchy)
	{
		FString DefaultCategory = GetDefaultCategoryName(CurrentClass);

		// Collect properties declared in this class only
		for (TFieldIterator<FProperty> PropIt(CurrentClass, EFieldIteratorFlags::ExcludeSuper); PropIt; ++PropIt)
		{
			FProperty* Property = *PropIt;
			if (!Property) continue;

			// Skip deprecated, transient-only hidden properties
			if (Property->HasAnyPropertyFlags(CPF_Deprecated))
			{
				continue;
			}

			bool bIsEditorOnly = Property->HasAnyPropertyFlags(CPF_EditorOnly);
			void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Object);

			FDetailsPanelItem Item = ProcessProperty(Property, ValuePtr, Object,
				CurrentClass, bIsEditorOnly, bIncludeValues, 0, MaxDepth);

			// Resolve default category
			if (Item.Category.IsEmpty())
			{
				Item.Category = DefaultCategory;
				Item.bIsDefaultCategory = true;
			}

			OutItems.Add(MoveTemp(Item));
		}

		// Collect CallInEditor functions declared in this class only
		for (TFieldIterator<UFunction> FuncIt(CurrentClass, EFieldIteratorFlags::ExcludeSuper); FuncIt; ++FuncIt)
		{
			UFunction* Function = *FuncIt;
			if (!Function) continue;

			// CallInEditor is stored as metadata in this engine, not as a function flag
			if (Function->HasMetaData(TEXT("CallInEditor")))
			{
				FDetailsPanelItem Item = ProcessButton(Function, CurrentClass);
				if (Item.Category.IsEmpty())
				{
					Item.Category = DefaultCategory;
					Item.bIsDefaultCategory = true;
				}
				OutItems.Add(MoveTemp(Item));
			}
		}
	}
}

UInspectDetailsPanelTool::FDetailsPanelItem UInspectDetailsPanelTool::ProcessProperty(
	FProperty* Property, void* Container, UObject* Owner,
	UClass* OwnerClass, bool bIsEditorOnly, bool bIncludeValues,
	int32 CurrentDepth, int32 MaxDepth) const
{
	FDetailsPanelItem Item;
	Item.Type = FDetailsPanelItem::EType::Property;
	Item.Name = Property->GetName();
	Item.DisplayName = ResolveDisplayName(Property);
	Item.SourceClass = OwnerClass->GetName();
	Item.bIsEditorOnly = bIsEditorOnly;

	// Category
	Item.Category = Property->GetMetaData(TEXT("Category"));

	// Property type
	Item.PropertyType = InferPropertyTypeStringImpl(Property);
	Item.WidgetType = InferWidgetType(Property);

	// All metadata
	Item.Metadata = GetAllMetadata(Property);

	// Value
	if (bIncludeValues && Container)
	{
		Property->ExportText_Direct(Item.Value, Container, Container, Owner, PPF_None);
	}

	// Default value from CDO (only for top-level class properties, not nested struct fields)
	UObject* CDO = nullptr;
	if (CurrentDepth == 0 && OwnerClass)
	{
		CDO = OwnerClass->GetDefaultObject();
		if (CDO)
		{
			void* DefaultContainer = Property->ContainerPtrToValuePtr<void>(CDO);
			if (DefaultContainer)
			{
				FString DefaultVal;
				Property->ExportText_Direct(DefaultVal, DefaultContainer, DefaultContainer, CDO, PPF_None);
				Item.DefaultValue = DefaultVal;
			}
		}
	}

	// Recursive struct expansion: expand struct children if depth allows
	if (CurrentDepth < MaxDepth)
	{
		FStructProperty* StructProp = CastField<FStructProperty>(Property);
		if (StructProp && StructProp->Struct && Container)
		{
			// Iterate over struct's inner properties
			for (TFieldIterator<FProperty> InnerIt(StructProp->Struct); InnerIt; ++InnerIt)
			{
				FProperty* InnerProperty = *InnerIt;
				if (!InnerProperty) continue;
				if (InnerProperty->HasAnyPropertyFlags(CPF_Deprecated)) continue;

				void* InnerValuePtr = InnerProperty->ContainerPtrToValuePtr<void>(Container);

				FDetailsPanelItem ChildItem;
				ChildItem.Type = FDetailsPanelItem::EType::Property;
				ChildItem.Name = InnerProperty->GetName();
				ChildItem.DisplayName = ResolveDisplayName(InnerProperty);
				ChildItem.SourceClass = StructProp->Struct->GetName();
				ChildItem.bIsEditorOnly = bIsEditorOnly;
				ChildItem.PropertyType = InferPropertyTypeStringImpl(InnerProperty);
				ChildItem.WidgetType = InferWidgetType(InnerProperty);
				ChildItem.Metadata = GetAllMetadata(InnerProperty);

				// Inner value
				if (bIncludeValues && InnerValuePtr)
				{
					InnerProperty->ExportText_Direct(ChildItem.Value, InnerValuePtr, InnerValuePtr, Owner, PPF_None);
				}

				// Inner default value from CDO struct (only when CDO is available, i.e. top-level property)
				if (CDO && CurrentDepth == 0)
				{
					void* CDOStructPtr = Property->ContainerPtrToValuePtr<void>(CDO);
					if (CDOStructPtr)
					{
						void* InnerDefaultPtr = InnerProperty->ContainerPtrToValuePtr<void>(CDOStructPtr);
						if (InnerDefaultPtr)
						{
							FString InnerDefaultVal;
							InnerProperty->ExportText_Direct(InnerDefaultVal, InnerDefaultPtr, InnerDefaultPtr, CDO, PPF_None);
							ChildItem.DefaultValue = InnerDefaultVal;
						}
					}
				}

				// Recurse deeper for nested structs
				FStructProperty* InnerStructProp = CastField<FStructProperty>(InnerProperty);
				if (InnerStructProp && InnerStructProp->Struct && InnerValuePtr && (CurrentDepth + 1) < MaxDepth)
				{
					// Recursively expand nested struct children
					FDetailsPanelItem TempItem = ProcessProperty(InnerProperty, InnerValuePtr, Owner,
						OwnerClass, bIsEditorOnly, bIncludeValues,
						CurrentDepth + 1, MaxDepth);
					ChildItem.Children = MoveTemp(TempItem.Children);
				}

				Item.Children.Add(MoveTemp(ChildItem));
			}
		}
	}

	return Item;
}

UInspectDetailsPanelTool::FDetailsPanelItem UInspectDetailsPanelTool::ProcessButton(
	UFunction* Function, UClass* OwnerClass) const
{
	FDetailsPanelItem Item;
	Item.Type = FDetailsPanelItem::EType::Button;
	Item.FunctionName = Function->GetName();
	Item.DisplayName = CamelToDisplayName(Function->GetName());
	Item.SourceClass = OwnerClass->GetName();
	Item.bIsEditorOnly = true; // CallInEditor is always editor-only

	// Category from function metadata
	if (Function->HasMetaData(TEXT("Category")))
	{
		Item.Category = Function->GetMetaData(TEXT("Category"));
	}

	// Tooltip
	if (Function->HasMetaData(TEXT("ToolTip")))
	{
		Item.Tooltip = Function->GetMetaData(TEXT("ToolTip"));
	}

	Item.Name = Item.DisplayName;
	return Item;
}

FString UInspectDetailsPanelTool::GetDefaultCategoryName(UClass* Class)
{
	if (!Class) return FString(TEXT("Default"));

	FString ClassName = Class->GetName();
	// Remove common prefixes
	if (ClassName.StartsWith(TEXT("U")) || ClassName.StartsWith(TEXT("A")))
	{
		ClassName = ClassName.Mid(1);
	}
	return CamelToDisplayName(ClassName);
}

FString UInspectDetailsPanelTool::ResolveDisplayName(FProperty* Property)
{
	if (!Property) return FString();

	// Priority 1: Use UPROPERTY(DisplayName="...") metadata if present
	if (Property->HasMetaData(TEXT("DisplayName")))
	{
		FString MetaDisplayName = Property->GetMetaData(TEXT("DisplayName"));
		if (!MetaDisplayName.IsEmpty())
		{
			return MetaDisplayName;
		}
	}

	// Priority 2: Fallback to CamelCase-to-display-name conversion
	return CamelToDisplayName(Property->GetName());
}

FString UInspectDetailsPanelTool::CamelToDisplayName(const FString& Name)
{
	if (Name.IsEmpty()) return Name;

	FString Input = Name;

	// Handle bool prefix
	if (Input.Len() > 1 && Input[0] == 'b' && FChar::IsUpper(Input[1]))
	{
		Input = Input.Mid(1);
	}

	FString Result;
	for (int32 i = 0; i < Input.Len(); ++i)
	{
		TCHAR Curr = Input[i];
		if (i > 0 && FChar::IsUpper(Curr))
		{
			TCHAR Prev = Input[i - 1];
			bool bNextIsLower = (i + 1 < Input.Len()) && FChar::IsLower(Input[i + 1]);

			// Insert space before uppercase if preceded by lowercase/digit
			// Or if this is the start of a new word in an acronym (e.g., IKs→To)
			if (FChar::IsLower(Prev) || FChar::IsDigit(Prev) ||
				(FChar::IsUpper(Prev) && bNextIsLower))
			{
				Result += ' ';
			}
		}
		Result += Curr;
	}
	return Result;
}

FString UInspectDetailsPanelTool::InferWidgetType(FProperty* Property)
{
	if (!Property) return FString(TEXT("unknown"));

	if (Property->IsA<FBoolProperty>()) return FString(TEXT("checkbox"));
	if (Property->IsA<FFloatProperty>() || Property->IsA<FDoubleProperty>()) return FString(TEXT("numeric_slider"));
	if (Property->IsA<FIntProperty>() || Property->IsA<FInt64Property>()) return FString(TEXT("numeric_input"));
	if (Property->IsA<FByteProperty>() || Property->IsA<FEnumProperty>()) return FString(TEXT("dropdown"));
	if (Property->IsA<FStrProperty>() || Property->IsA<FNameProperty>() || Property->IsA<FTextProperty>()) return FString(TEXT("text_input"));

	if (FStructProperty* StructProp = CastField<FStructProperty>(Property))
	{
		FString StructName = StructProp->Struct->GetName();
		if (StructName == TEXT("Transform")) return FString(TEXT("transform_editor"));
		if (StructName == TEXT("Vector")) return FString(TEXT("vector_input"));
		if (StructName == TEXT("Rotator")) return FString(TEXT("rotator_input"));
		if (StructName == TEXT("Color") || StructName == TEXT("LinearColor")) return FString(TEXT("color_picker"));
		if (StructName == TEXT("GameplayTag")) return FString(TEXT("gameplay_tag_selector"));
		if (StructName == TEXT("GameplayTagContainer")) return FString(TEXT("gameplay_tag_container"));
		return FString(TEXT("struct_expandable"));
	}

	if (Property->IsA<FObjectPropertyBase>())
	{
		if (Property->IsA<FSoftObjectProperty>()) return FString(TEXT("soft_object_reference"));
		return FString(TEXT("object_reference"));
	}
	if (Property->IsA<FMapProperty>()) return FString(TEXT("map_editor"));
	if (Property->IsA<FArrayProperty>()) return FString(TEXT("array_editor"));
	if (Property->IsA<FSetProperty>()) return FString(TEXT("set_editor"));

	return FString(TEXT("unknown"));
}

static FString InferPropertyTypeStringImpl(FProperty* Property)
{
	if (!Property) return FString(TEXT("unknown"));

	if (FObjectPropertyBase* ObjProp = CastField<FObjectPropertyBase>(Property))
	{
		FString Wrapper;
		if (Property->IsA<FSoftObjectProperty>())
		{
			Wrapper = TEXT("TSoftObjectPtr");
		}
		else
		{
			Wrapper = TEXT("TObjectPtr");
		}
		return FString::Printf(TEXT("%s<%s>"), *Wrapper, *ObjProp->PropertyClass->GetName());
	}
	if (FStructProperty* StructProp = CastField<FStructProperty>(Property))
	{
		return StructProp->Struct->GetName();
	}
	if (FEnumProperty* EnumProp = CastField<FEnumProperty>(Property))
	{
		if (EnumProp->GetEnum())
		{
			return EnumProp->GetEnum()->GetName();
		}
		return FString(TEXT("EnumProperty"));
	}
	if (FByteProperty* ByteProp = CastField<FByteProperty>(Property))
	{
		if (ByteProp->Enum)
		{
			return ByteProp->Enum->GetName();
		}
	}
	if (FMapProperty* MapProp = CastField<FMapProperty>(Property))
	{
		return FString::Printf(TEXT("TMap<%s, %s>"),
			*InferPropertyTypeStringImpl(MapProp->KeyProp),
			*InferPropertyTypeStringImpl(MapProp->ValueProp));
	}
	if (FArrayProperty* ArrayProp = CastField<FArrayProperty>(Property))
	{
		return FString::Printf(TEXT("TArray<%s>"), *InferPropertyTypeStringImpl(ArrayProp->Inner));
	}

	if (Property->IsA<FBoolProperty>()) return FString(TEXT("bool"));
	if (Property->IsA<FIntProperty>()) return FString(TEXT("int32"));
	if (Property->IsA<FInt64Property>()) return FString(TEXT("int64"));
	if (Property->IsA<FFloatProperty>()) return FString(TEXT("float"));
	if (Property->IsA<FDoubleProperty>()) return FString(TEXT("double"));
	if (Property->IsA<FStrProperty>()) return FString(TEXT("FString"));
	if (Property->IsA<FNameProperty>()) return FString(TEXT("FName"));
	if (Property->IsA<FTextProperty>()) return FString(TEXT("FText"));

	return Property->GetCPPType();
}

TMap<FString, FString> UInspectDetailsPanelTool::GetAllMetadata(FProperty* Property)
{
	TMap<FString, FString> Result;
	if (!Property) return Result;

	// Check common metadata keys using FField::GetMetaData / HasMetaData
	static const FName CommonMetaKeys[] = {
		TEXT("Category"),
		TEXT("DisplayName"),
		TEXT("ToolTip"),
		TEXT("ClampMin"),
		TEXT("ClampMax"),
		TEXT("UIMin"),
		TEXT("UIMax"),
		TEXT("EditCondition"),
		TEXT("EditConditionHides"),
		TEXT("InlineEditConditionToggle"),
		TEXT("ShowOnlyInnerProperties"),
		TEXT("TitleProperty"),
		TEXT("ForceInlineRow"),
		TEXT("Categories"),
		TEXT("AdvancedDisplay"),
		TEXT("MakeStructureDefaultValue"),
		TEXT("AllowedClasses"),
		TEXT("DisallowedClasses"),
		TEXT("ExactClass"),
		TEXT("DisplayPriority"),
		TEXT("DisplayAfter"),
		TEXT("ExposeOnSpawn"),
		TEXT("NativeConstTemplateArg")
	};

	for (const FName& Key : CommonMetaKeys)
	{
		if (Property->HasMetaData(Key))
		{
			Result.Add(Key.ToString(), Property->GetMetaData(Key));
		}
	}

	// Add property flags as readable metadata
	if (Property->HasAnyPropertyFlags(CPF_EditorOnly))
	{
		Result.Add(TEXT("_EditorOnly"), TEXT("true"));
	}
	if (Property->HasAnyPropertyFlags(CPF_AdvancedDisplay))
	{
		Result.Add(TEXT("_AdvancedDisplay"), TEXT("true"));
	}
	if (Property->HasAnyPropertyFlags(CPF_Deprecated))
	{
		Result.Add(TEXT("_Deprecated"), TEXT("true"));
	}
	if (Property->HasAnyPropertyFlags(CPF_EditConst))
	{
		Result.Add(TEXT("_ReadOnly"), TEXT("true"));
	}

	return Result;
}

TMap<FString, FString> UInspectDetailsPanelTool::GetFunctionMetadata(UFunction* Function)
{
	TMap<FString, FString> Result;
	if (!Function) return Result;

	// UFunction inherits from UField. Check common metadata keys.
	static const FName CommonKeys[] = {
		TEXT("Category"), TEXT("ToolTip"), TEXT("DisplayName"),
		TEXT("ScriptName"), TEXT("Keywords"), TEXT("CompactNodeTitle"),
		TEXT("DeprecatedFunction"), TEXT("DeprecationMessage"),
		TEXT("CallInEditor"), TEXT("BlueprintInternalUseOnly")
	};

	for (const FName& Key : CommonKeys)
	{
		if (Function->HasMetaData(Key))
		{
			Result.Add(Key.ToString(), Function->GetMetaData(Key));
		}
	}

	return Result;
}

// Recursively build a property JSON node (with children for struct expansion)
TSharedPtr<FJsonObject> UInspectDetailsPanelTool::BuildPropertyJson(const FDetailsPanelItem& Item, int32& OutPropertyCount)
{
	TSharedPtr<FJsonObject> PropJson = MakeShareable(new FJsonObject);
	PropJson->SetStringField(TEXT("type"), TEXT("property"));
	PropJson->SetStringField(TEXT("name"), Item.Name);
	PropJson->SetStringField(TEXT("display_name"), Item.DisplayName);
	PropJson->SetStringField(TEXT("property_type"), Item.PropertyType);
	PropJson->SetStringField(TEXT("widget_type"), Item.WidgetType);
	PropJson->SetStringField(TEXT("source_class"), Item.SourceClass);
	PropJson->SetBoolField(TEXT("is_editor_only"), Item.bIsEditorOnly);

	if (!Item.Value.IsEmpty())
	{
		PropJson->SetStringField(TEXT("value"), Item.Value);
	}
	if (!Item.DefaultValue.IsEmpty())
	{
		PropJson->SetStringField(TEXT("default_value"), Item.DefaultValue);
	}

	// Metadata as nested object
	if (Item.Metadata.Num() > 0)
	{
		TSharedPtr<FJsonObject> MetaJson = MakeShareable(new FJsonObject);
		for (const auto& Pair : Item.Metadata)
		{
			MetaJson->SetStringField(Pair.Key, Pair.Value);
		}
		PropJson->SetObjectField(TEXT("metadata"), MetaJson);
	}

	// Recursively output struct children
	if (Item.Children.Num() > 0)
	{
		TArray<TSharedPtr<FJsonValue>> ChildArray;
		for (const auto& Child : Item.Children)
		{
			ChildArray.Add(MakeShareable(new FJsonValueObject(BuildPropertyJson(Child, OutPropertyCount))));
			OutPropertyCount++;
		}
		PropJson->SetArrayField(TEXT("children"), ChildArray);
	}

	return PropJson;
}

TSharedPtr<FJsonObject> UInspectDetailsPanelTool::BuildOutput(
	UObject* Object, const TArray<FDetailsPanelItem>& Items) const
{
	TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
	Result->SetStringField(TEXT("asset_path"), Object->GetPathName());
	Result->SetStringField(TEXT("asset_class"), Object->GetClass()->GetName());

	// Class hierarchy
	TArray<TSharedPtr<FJsonValue>> HierarchyArray;
	for (UClass* C = Object->GetClass(); C && C != UObject::StaticClass(); C = C->GetSuperClass())
	{
		HierarchyArray.Add(MakeShareable(new FJsonValueString(C->GetName())));
	}
	Result->SetArrayField(TEXT("class_hierarchy"), HierarchyArray);

	// Group items by category, preserving order
	TArray<FString> CategoryOrder;
	TMap<FString, TArray<const FDetailsPanelItem*>> CategoryItems;

	for (const FDetailsPanelItem& Item : Items)
	{
		if (!CategoryItems.Contains(Item.Category))
		{
			CategoryOrder.Add(Item.Category);
			CategoryItems.Add(Item.Category, {});
		}
		CategoryItems[Item.Category].Add(&Item);
	}

	// Build categories array
	TArray<TSharedPtr<FJsonValue>> CategoriesArray;
	int32 PropertyCount = 0;
	int32 ButtonCount = 0;

	for (const FString& CategoryName : CategoryOrder)
	{
		TSharedPtr<FJsonObject> CatJson = MakeShareable(new FJsonObject);
		CatJson->SetStringField(TEXT("type"), TEXT("category"));
		CatJson->SetStringField(TEXT("name"), CategoryName);

		// Check if this is a default category
		bool bIsDefault = false;
		for (const FDetailsPanelItem* Item : CategoryItems[CategoryName])
		{
			if (Item->bIsDefaultCategory)
			{
				bIsDefault = true;
				break;
			}
		}
		CatJson->SetBoolField(TEXT("is_default_category"), bIsDefault);

		// Build children - buttons first, then properties (UE convention)
		TArray<TSharedPtr<FJsonValue>> ChildrenArray;

		// Buttons first
		for (const FDetailsPanelItem* Item : CategoryItems[CategoryName])
		{
			if (Item->Type == FDetailsPanelItem::EType::Button)
			{
				TSharedPtr<FJsonObject> BtnJson = MakeShareable(new FJsonObject);
				BtnJson->SetStringField(TEXT("type"), TEXT("button"));
				BtnJson->SetStringField(TEXT("name"), Item->DisplayName);
				BtnJson->SetStringField(TEXT("function_name"), Item->FunctionName);
				BtnJson->SetStringField(TEXT("source_class"), Item->SourceClass);
				if (!Item->Tooltip.IsEmpty())
				{
					BtnJson->SetStringField(TEXT("tooltip"), Item->Tooltip);
				}
				ChildrenArray.Add(MakeShareable(new FJsonValueObject(BtnJson)));
				ButtonCount++;
			}
		}

		// Then properties (using recursive helper for struct children)
		for (const FDetailsPanelItem* Item : CategoryItems[CategoryName])
		{
			if (Item->Type == FDetailsPanelItem::EType::Property)
			{
				ChildrenArray.Add(MakeShareable(new FJsonValueObject(BuildPropertyJson(*Item, PropertyCount))));
				PropertyCount++;
			}
		}

		CatJson->SetArrayField(TEXT("children"), ChildrenArray);
		CategoriesArray.Add(MakeShareable(new FJsonValueObject(CatJson)));
	}

	Result->SetArrayField(TEXT("details_panel"), CategoriesArray);
	Result->SetNumberField(TEXT("property_count"), PropertyCount);
	Result->SetNumberField(TEXT("button_count"), ButtonCount);

	return Result;
}
