// Copyright 2023 Attaku under EULA https://www.unrealengine.com/en-US/eula/unreal

#pragma once

#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogRMFixToolEditor, Log, All);

class UAnimSequence;

class IRMFixToolInstantiator
{
public:

	virtual void ExtendContentBrowserSelectionMenu() = 0;

private:

	virtual void CreateRMFixToolCommands(TArray<UAnimSequence*> AnimSequencesToEdit) = 0;
};

class FRMFixToolEditorModule : public IModuleInterface
{
public:

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:

	void RegisterContentBrowserExtensions(IRMFixToolInstantiator* Instantiator);

private:

	TSharedPtr<IRMFixToolInstantiator> RMFixToolInstantiator = nullptr;
};