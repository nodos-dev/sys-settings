// Copyright Nodos AS. All Rights Reserved.
#include <Nodos/PluginAPI.h>
#include <nosSettingsSubsystem/nosSettingsSubsystem.h>
#include <nosSettingsSubsystem/Types_generated.h>
#include <unordered_map>
#include <Nodos/Helpers.hpp>
#include "VersionUtils.h"

namespace nos::sys::settings
{
struct SettingsFileManager
{
	typedef std::pair<nos::Name, nos::Buffer> EntryTypeNameBufferPair;
	// EntryName -> {PluginVer, {TypeName, Buffer}} (sorted by plugin version)
	struct ReadEntryList
	{
		std::shared_mutex FileMutex;
		std::unordered_map<nos::Name, std::vector<std::pair<nos::util::SemVer, EntryTypeNameBufferPair>>> Entries;
	};
	struct RegisteredEntry {
		nos::Name TypeName;
		nos::Buffer DefaultValue;
		nosSettingsFileDirectoryFlag SaveFlag;
		bool IsEditableFromEditor;
		nos::fb::TVisualizer Visualizer{};
		nos::Name TargetName;
		nosPfnSettingsEntryUpdate UpdateCallback = nullptr;
	};
	// PluginName -> EntryName -> RegisteredEntry
	std::unordered_map<nos::Name, std::unordered_map<nos::Name, RegisteredEntry>> RegisteredEntries;
	// PluginName -> { PluginVersion, PluginInfo }
	std::unordered_map<nos::Name, std::pair<nos::util::SemVer, nosPluginInfo>> PluginVersions;
	// Entries from files
	// PluginName -> EntryList
	std::unordered_map<nos::Name, ReadEntryList> GlobalEntries, WorkspaceEntries, LocalEntries;
	std::filesystem::path GetSettingsFilePath(nosSettingsFileDirectory dir, nos::Name pluginName) const;
	nosResult ReadSettingsFile(std::filesystem::path filePath, ReadEntryList& readEntries);
	nosResult WriteSettingsFile(std::filesystem::path filePath, const ReadEntryList& entries) const;
	nosResult TryToGetClosestFittingEntry(nos::Name pluginName, nos::Name entryName, RegisteredEntry& entry);
	nosResult UpdateEntry(nos::Name pluginName, nos::util::SemVer pluginVersion, nosSettingsFileDirectoryFlag directories, nos::Name entryName, EntryTypeNameBufferPair entryVal);
};
}