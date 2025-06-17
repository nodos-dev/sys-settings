// Copyright Nodos AS. All Rights Reserved.
#include <Nodos/PluginAPI.h>
#include <nosSettingsSubsystem/nosSettingsSubsystem.h>
#include <nosSettingsSubsystem/EditorEvents_generated.h>
#include <unordered_map>
#include <Nodos/Helpers.hpp>
#include "Main.h"
#include "Globals.h"
NOS_INIT()
NOS_BEGIN_IMPORT_DEPS()
NOS_END_IMPORT_DEPS()

namespace settings = nos::sys::settings;
namespace nos::sys::settings {
std::unique_ptr<EntryManager> GSettingsEntryManager = nullptr;
std::unordered_map<uint32_t, std::unique_ptr<nosSettingsSubsystem>> GExportedAPIVersions;


nos::Buffer GenerateEditorItemsForPlugin(nos::Name pluginName, const std::unordered_map<nos::Name, RegisteredEntry>& entries) {
	editor::TSettingsUpdateFromSubsystem list;
	list.plugin_name = pluginName;
	for (auto& [entryName, entry] : entries) {
		list.settings.push_back(std::make_unique<editor::TSettingsEditorItem>());
		auto& item = list.settings.back();
		item->item_display_name = entry.DisplayName;
		item->visualizer = std::make_unique<nos::fb::TVisualizer>(entry.Visualizer);
		item->entry = std::make_unique<TSettingsEntry>();
		item->entry->type_name = entry.TypeName;
		item->entry->entry_name = entryName;
		item->entry->data = entry.LastValue;
		item->entry->target_name = entry.TargetName;
	}
	return nos::Buffer::From(list);
}

nosResult UpdateEditorEntriesForPlugin(nos::Name pluginName)
{
	auto buf = GenerateEditorItemsForPlugin(pluginName, GSettingsEntryManager->RegisteredEntries[pluginName]);
	nosSendEditorMessageParams params{ .Message = buf,
										.DispatchType = NOS_EDITOR_MESSAGE_DISPATCH_TYPE_BROADCAST };
	nosEngine.SendEditorMessage(&params);
	return NOS_RESULT_SUCCESS;
}

nosResult UnregisterEditorSettings(nos::Name pluginName)
{
	std::shared_lock lock(GSettingsEntryManager->RegisteredEntriesMutex);
	if (auto it = GSettingsEntryManager->RegisteredEntries.find(pluginName); it == GSettingsEntryManager->RegisteredEntries.end())
	{
		nosEngine.LogE("Plugin %s is not registered for editor settings, but called UnregisterEditorSettings().", pluginName.AsCStr());
		return NOS_RESULT_FAILED;
	}

	auto buf = GenerateEditorItemsForPlugin(pluginName, {});
	nosSendEditorMessageParams params{ .Message = buf, .DispatchType = NOS_EDITOR_MESSAGE_DISPATCH_TYPE_BROADCAST };
	nosEngine.SendEditorMessage(&params);
	return NOS_RESULT_SUCCESS;
}

void OnEditorConnected(uint64_t editorId)
{
	std::shared_lock lock(GSettingsEntryManager->RegisteredEntriesMutex);
	for (auto const& [pluginName, entries] : GSettingsEntryManager->RegisteredEntries) {
		auto message = GenerateEditorItemsForPlugin(pluginName, entries);
		nosSendEditorMessageParams params{ .Message = message,
											.DispatchType = NOS_EDITOR_MESSAGE_DISPATCH_TYPE_TO_SELECTED,
											.ToSelected = {editorId} };
		nosEngine.SendEditorMessage(&params);
	}
}

void OnMessageFromEditor(uint64_t editorId, nosBuffer message)
{
	auto msg = flatbuffers::GetMutableRoot<editor::SettingsUpdateFromEditor>(message.Data);
	auto pluginName = msg->plugin_name()->c_str();
	auto it = GSettingsEntryManager->RegisteredEntries.find(nos::Name(pluginName));
	if (it == GSettingsEntryManager->RegisteredEntries.end()) {
		nosEngine.LogW("Plugin %s is not registered for editor settings, but Editor sent a message.", pluginName);
		return;
	}

	auto entryName = msg->entry()->entry_name()->c_str();
	auto entryIt = it->second.find(nos::Name(entryName));
	if (entryIt == it->second.end()) {
		nosEngine.LogW("Plugin %s is not registered %s for editor settings, but Editor sent a message.", pluginName, entryName);
		return;
	}
	auto& entry = entryIt->second;
	auto val = nos::Buffer(msg->entry()->data());
	auto ret = entry.UpdateCallback(nos::Name(entryName), *val.GetInternal());
	if (entry.ReadEntryPluginVer == util::SemVer{}) 
		entry.ReadEntryPluginVer = util::SemVer::ParseFrom(nos::Name(GSettingsEntryManager->PluginVersions[nos::Name(pluginName)].second.Id.Version).AsCStr());
	if (ret == NOS_RESULT_SUCCESS)
		GSettingsEntryManager->UpdateEntry(nos::Name(pluginName), entry.ReadEntryPluginVer, entry.SaveFlag, nos::Name(entryName), { entry.TypeName, val });
	else
		nosEngine.LogE("Failed to update the entry, last value remains");
}

// Returns a vector of file paths in 'directory' whose filenames start with 'prefix'.
// Only regular files are included.
std::vector<std::filesystem::path> GetFilesWithPrefix(
	const std::filesystem::path& directory,
	const std::string& prefix)
{
	std::vector<std::filesystem::path> result;
	if (!std::filesystem::exists(directory) || !std::filesystem::is_directory(directory))
		return result;

	for (const auto& entry : std::filesystem::directory_iterator(directory)) {
		if (!entry.is_regular_file())
			continue;
		const auto& filename = entry.path().filename().string();
		if (filename.rfind(prefix, 0) == 0) { // filename starts with prefix
			result.push_back(entry.path());
		}
	}
	return result;
}

void RegisterPlugin(const nosPluginInfo& pluginInfo) {
	auto pluginName = nos::Name(pluginInfo.Id.Name);
	auto readAllFilesInDir = [&pluginInfo, &pluginName](std::filesystem::path const& dir, ReadEntryList& entryList) {
		auto workspaceFiles = GetFilesWithPrefix(dir, pluginName.AsCStr());
		std::unique_lock lock(entryList.FileMutex);
		for (auto& file : workspaceFiles) {
			ReadEntryList readList;
			GSettingsEntryManager->ReadSettingsFile(file, readList);
			for (auto& entry : readList.Entries)
				entryList.Entries[entry.first] = entry.second;
		}
		};
	readAllFilesInDir(std::filesystem::path(pluginInfo.RootFolderPath), GSettingsEntryManager->LocalEntries[pluginName]);
	readAllFilesInDir(std::filesystem::path(nosEngine.GetEnginePath(NOS_ENGINE_PATH_CONFIG)), GSettingsEntryManager->WorkspaceEntries[pluginName]);
	readAllFilesInDir(std::filesystem::path(nosEngine.GetEnginePath(NOS_ENGINE_PATH_APPDATA)), GSettingsEntryManager->GlobalEntries[pluginName]);

	settings::GSettingsEntryManager->PluginVersions[pluginInfo.Id.Name] = {nos::util::SemVer::ParseFrom(nos::Name(pluginInfo.Id.Version).AsCStr()), pluginInfo};
}

nosResult RegisterEntry(nosSettingsEntryParams* params) {
	if (!params)
		return NOS_RESULT_INVALID_ARGUMENT;

	nosPluginInfo pluginInfo = {};
	if (nosEngine.GetCallingPlugin(&pluginInfo) != NOS_RESULT_SUCCESS) {
		nosEngine.LogE("Failed to get calling plugin info for settings entry registration.");
		return NOS_RESULT_FAILED;
	}
	if (settings::GSettingsEntryManager->PluginVersions.find(pluginInfo.Id.Name) == settings::GSettingsEntryManager->PluginVersions.end())
		RegisterPlugin(pluginInfo);

	auto& entryManager = settings::GSettingsEntryManager;
	std::unique_lock lock(entryManager->RegisteredEntriesMutex);
	auto& entry = entryManager->RegisteredEntries[pluginInfo.Id.Name][nos::Name(params->EntryName)];
	if (params->Buffer)
		entry.LastValue = *params->Buffer;
	entry.IsEditableFromEditor = params->IsEditableFromEditor;
	entry.SaveFlag = params->WriteDirectories;
	entry.TargetName = params->TargetName;
	entry.TypeName = params->TypeName;
	entry.UpdateCallback = params->UpdateCallback;
	entry.DisplayName = params->DisplayName;
	if (params->Visualizer)
		params->Visualizer->UnPackTo(&entry.Visualizer);

	if (entryManager->TryToGetClosestFittingEntry(pluginInfo.Id.Name, nos::Name(params->EntryName), entry) != NOS_RESULT_SUCCESS) {
		nosEngine.LogE("Failed to get closest fitting entry for settings registration.");
		return NOS_RESULT_FAILED;
	}

	return NOS_RESULT_SUCCESS;
}

void UnregisterEntry(nosName entryName) {
	nosPluginInfo pluginInfo = {};
	if (nosEngine.GetCallingPlugin(&pluginInfo) != NOS_RESULT_SUCCESS) {
		nosEngine.LogE("Failed to get calling plugin info for settings entry unregistration.");
		return;
	}

	auto& entryManager = settings::GSettingsEntryManager;
	std::unique_lock lock(entryManager->RegisteredEntriesMutex);
	auto pluginName = pluginInfo.Id.Name;
	auto& entries = entryManager->RegisteredEntries[pluginName];
	entries.erase(entryName);
	if (!entries.size()) {
		entryManager->RegisteredEntries.erase(pluginName);
		entryManager->LocalEntries.erase(pluginName);
	}
}

nosResult UpdateEntryValue(nosName entryName, nosBuffer value) {
	nosPluginInfo pluginInfo = {};
	if (nosEngine.GetCallingPlugin(&pluginInfo) != NOS_RESULT_SUCCESS) {
		nosEngine.LogE("Failed to get calling plugin info for settings entry update.");
		return NOS_RESULT_FAILED;
	}

	nos::Name pluginName = pluginInfo.Id.Name;
	auto& entryManager = settings::GSettingsEntryManager;
	std::shared_lock lock(entryManager->RegisteredEntriesMutex);
	auto entry = entryManager->RegisteredEntries[pluginName].find(entryName);
	if (entry == entryManager->RegisteredEntries[pluginName].end())
		return NOS_RESULT_NOT_FOUND;

	entry->second.LastValue = value;
	GSettingsEntryManager->UpdateEntry(pluginName, entry->second.ReadEntryPluginVer, entry->second.SaveFlag, entryName, { entry->second.TypeName, value });
	auto buf = GenerateEditorItemsForPlugin(pluginName, GSettingsEntryManager->RegisteredEntries[pluginName]);
	nosSendEditorMessageParams params{ .Message = buf,
										.DispatchType = NOS_EDITOR_MESSAGE_DISPATCH_TYPE_BROADCAST };
	nosEngine.SendEditorMessage(&params);
}
}

nosResult NOSAPI_CALL OnPreUnloadPlugin()
{
	settings::GSettingsEntryManager = nullptr;
	settings::GExportedAPIVersions.clear();
	return NOS_RESULT_SUCCESS;
}


extern "C"
{
	nosResult OnRequestAPI(uint32_t minor, void** outPluginAPI)
	{
		if (auto it = settings::GExportedAPIVersions.find(minor); it != settings::GExportedAPIVersions.end())
		{
			*outPluginAPI = it->second.get();
			return NOS_RESULT_SUCCESS;
		}
		nosSettingsSubsystem& subsystem = *(settings::GExportedAPIVersions[minor] = std::make_unique<nosSettingsSubsystem>());
		subsystem.RegisterEntry = settings::RegisterEntry;
		subsystem.UnregisterEntry = settings::UnregisterEntry;
		subsystem.UpdateEntryValue = settings::UpdateEntryValue;
		*outPluginAPI = &subsystem;
		return NOS_RESULT_SUCCESS;
	}

	NOSAPI_ATTR nosResult NOSAPI_CALL nosExportPlugin(nosPluginFunctions* pluginFunctions)
	{
		pluginFunctions->OnRequestAPI = OnRequestAPI;
		pluginFunctions->OnPreUnloadPlugin = OnPreUnloadPlugin;
		pluginFunctions->OnEditorConnected = settings::OnEditorConnected;
		pluginFunctions->OnMessageFromEditor = settings::OnMessageFromEditor;
		settings::GSettingsEntryManager.reset(new settings::EntryManager());

		return NOS_RESULT_SUCCESS;
	}
}