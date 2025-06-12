/*
 * Copyright MediaZ Teknoloji A.S. All Rights Reserved.
 */

#ifndef NOS_SETTINGS_SUBSYSTEM_H_INCLUDED
#define NOS_SETTINGS_SUBSYSTEM_H_INCLUDED
#include "Nodos/Types.h"

typedef enum nosSettingsFileDirectory {
	NOS_SETTINGS_FILE_DIRECTORY_LOCAL, // Module's root folder
	NOS_SETTINGS_FILE_DIRECTORY_WORKSPACE, // Engine's config folder
	NOS_SETTINGS_FILE_DIRECTORY_GLOBAL	// AppData folder
} nosSettingsFileDirectory;

typedef enum nosSettingsEntryType {
	NOS_SETTINGS_ENTRY_TYPE_MODULE, // Module's settings
	NOS_SETTINGS_ENTRY_TYPE_DEVICE // Device's settings
} nosSettingsEntryType;

typedef nosResult(*nosPfnSettingsEntryUpdate)(const char* entryName, nosBuffer itemValue);
typedef uint64_t nosSettingsEntryId;

#if NOS_HAS_CPLUSPLUS_20
typedef nos::fb::Visualizer nosSettingsEditorVisualizer;
#else
typedef void* nosSettingsEditorVisualizer;
#endif

typedef struct nosSettingsEntryParams {
	nosName TypeName;
	nosBuffer Buffer; // Don't forget to free this buffer
	const char* EntryName; // If NULL, "default" will be used
	nosSettingsFileDirectory Directory;
	nosPfnSettingsEntryUpdate UpdateCallback; // Callback to update the entry value
	nosSettingsEntryType Type; // Type of the entry
	bool IsEditableFromEditor; // If true, this entry will be editable from editor
	nosSettingsEditorVisualizer Visualizer; // Visualizer for the entry in editor
} nosSettingsEntryParams;

typedef struct nosSettingsSubsystem
{
	// parameters->directory will be set to found directory
	// if parameters->typeName == 0, it will be set to first typename found in the closest file
	// parameters->directory is ignored because closest one is chosen
	nosResult(NOSAPI_CALL *RegisterSettingsEntry)(nosSettingsEntryParams* parameters, nosSettingsEntryId* entryId);
	void(NOSAPI_CALL* UnregisterSettingsEntry)(nosSettingsEntryId entryId);
} nosSettingsSubsystem;

#pragma region Helper Declarations & Macros
// Make sure these are same with nossys file.
#define NOS_SETTINGS_SUBSYSTEM_NAME "nos.sys.settings"

#define NOS_SETTINGS_SUBSYSTEM_VERSION_MAJOR 1
#define NOS_SETTINGS_SUBSYSTEM_VERSION_MINOR 0

extern struct nosPluginInfo nosSettingsModuleInfo;
extern nosSettingsSubsystem* nosSettings;

#define NOS_SETTINGS_INIT()                                                                                              \
	nosPluginInfo nosSettingsModuleInfo;                                                                                 \
	nosSettingsSubsystem* nosSettings = nullptr;

#define NOS_SETTINGS_IMPORT() NOS_IMPORT_DEP(NOS_SETTINGS_SUBSYSTEM_NAME, nosSettingsModuleInfo, nosSettings)

#pragma endregion


#endif // NOS_SETTINGS_SUBSYSTEM_H_INCLUDED