// Copyright 2025, Aquanox.

#pragma once

#include "Modules/ModuleManager.h"

class FEnhancedAsyncActionEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void RegisterCustomSettings();
	void UnRegisterCustomSettings();
};
