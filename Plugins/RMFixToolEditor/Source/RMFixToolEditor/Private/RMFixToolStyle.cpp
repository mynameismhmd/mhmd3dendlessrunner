// Copyright 2023 Attaku under EULA https://www.unrealengine.com/en-US/eula/unreal

#include "RMFixToolStyle.h"
#include "RMFixToolEditor.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/SlateStyleRegistry.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleMacros.h"

#define RootToContentDir Style->RootToContentDir

TSharedPtr<FSlateStyleSet> FRMFixToolStyle::StyleInstance = nullptr;

void FRMFixToolStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FRMFixToolStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FRMFixToolStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("RMFixToolStyle"));
	return StyleSetName;
}

const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon20x20(20.0f, 20.0f);
const FVector2D Icon160x137(160.0f, 137.0f);

TSharedRef< FSlateStyleSet > FRMFixToolStyle::Create()
{
	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet("RMFixToolStyle"));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("RMFixTool")->GetBaseDir() / TEXT("Resources"));

	Style->Set("FixRMIssues", new IMAGE_BRUSH_SVG(TEXT("PlaceholderButtonIcon"), Icon20x20));

	Style->Set("FixStartLocationOffset", new IMAGE_BRUSH(TEXT("FixStartLocationOffset"), Icon160x137));
	Style->Set("TransferAnimationBetweenBones", new IMAGE_BRUSH(TEXT("TransferAnimationBetweenBones"), Icon160x137));
	Style->Set("FixRootMotionDirection", new IMAGE_BRUSH(TEXT("FixRootMotionDirection"), Icon160x137));
	Style->Set("SnapIcon", new IMAGE_BRUSH(TEXT("SnapIcon"), Icon160x137));

	return Style;
}

void FRMFixToolStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

const ISlateStyle& FRMFixToolStyle::Get()
{
	return *StyleInstance;
}