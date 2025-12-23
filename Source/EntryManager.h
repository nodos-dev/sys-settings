// Copyright Nodos AS. All Rights Reserved.
#include <Nodos/PluginAPI.h>
#include <nosSysSettings/nosSettingsSubsystem.h>
#include <nosSysSettings/Types_generated.h>
#include <unordered_map>
#include <Nodos/Plugin.hpp>
#include <Nodos/Utils/StringUtils.hpp>
#include <Nodos/Utils/VersionUtils.hpp>

namespace nos::sys::settings
{

struct RegisteredEntry {
	nos::Name TypeName;
	nos::Buffer LastValue;
	editor::SettingsEntryFileDirectory SaveDir;
	nos::fb::TVisualizer Visualizer{};
	std::string UiTargetName;
	std::string DisplayName;
	nosPfnSettingsEntryUpdate UpdateCallback = nullptr;
	// If an entry with the same name from a previous version of this plugin is already read, save according to it
	nos::util::SemVer ReadEntryPluginVer{};
};

typedef std::pair<nos::Name, nos::Buffer> EntryTypeNameBufferPair;
struct ReadEntryList
{
	std::shared_mutex FileMutex;
	// EntryName -> {PluginVer, {TypeName, Buffer}} (sorted by plugin version)
	std::unordered_map<std::string, std::vector<std::pair<nos::util::SemVer, EntryTypeNameBufferPair>>> Entries;
};

nosResult UpdateEditorEntriesForPlugin(nos::Name pluginName);
nosResult UnregisterSettings(nos::Name moduleName);
void OnEditorConnected(uint64_t editorId);
void OnMessageFromEditor(uint64_t editorId, nosBuffer message);

struct EntryManager
{
	// PluginName -> EntryName -> RegisteredEntry
	std::unordered_map<nos::Name, std::unordered_map<std::string, RegisteredEntry>> RegisteredEntries;
	std::shared_mutex RegisteredEntriesMutex;
	// PluginName -> { PluginVersion, PluginInfo }
	std::unordered_map<nos::Name, std::pair<nos::util::SemVer, nosPluginInfo>> PluginVersions;
	// Entries from files
	// PluginName -> EntryList
	std::unordered_map<nos::Name, ReadEntryList> GlobalEntries, WorkspaceEntries, LocalEntries;
	nosResult ReadSettingsFile(std::filesystem::path filePath, ReadEntryList& readEntries);
	nosResult WriteSettingsFile(nos::Name pluginName, util::SemVer pluginVer, editor::SettingsEntryFileDirectory dir, const ReadEntryList& entries) const;
	// Searches through local, workspace and global settings files
	// For each matching entry, calls the UpdateCallback with the entry name and value
	// First one to return NOS_RESULT_SUCCESS will be used
	// If no matching entry is found, NOS_RESULT_FAILED will be returned
	nosResult TryGetOrCreateFromClosestValidEntry(nos::Name pluginName, std::string entryName, RegisteredEntry& entry);
	// If there is a matching entry with the same plugin version, updates it
	// If there is not, creates a new entry
	nosResult UpdateEntry(nos::Name pluginName, nos::util::SemVer pluginVersion, editor::SettingsEntryFileDirectory dir, std::string const& entryName, EntryTypeNameBufferPair entryVal);
};
}