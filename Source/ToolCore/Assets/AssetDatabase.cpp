//
// Copyright (c) 2014-2015, THUNDERBEAST GAMES LLC All rights reserved
// LICENSE: Atomic Game Engine Editor and Tools EULA
// Please see LICENSE_ATOMIC_EDITOR_AND_TOOLS.md in repository root for
// license information: https://github.com/AtomicGameEngine/AtomicGameEngine
//

#include <Poco/MD5Engine.h>

#include <Atomic/IO/Log.h>
#include <Atomic/IO/File.h>
#include <Atomic/IO/FileSystem.h>
#include <Atomic/Math/Random.h>

#include <Atomic/Resource/ResourceEvents.h>
#include <Atomic/Resource/ResourceCache.h>

#include "../ToolEvents.h"
#include "../ToolSystem.h"
#include "../Project/Project.h"
#include "../Project/ProjectEvents.h"
#include "AssetEvents.h"
#include "AssetDatabase.h"


namespace ToolCore
{

AssetDatabase::AssetDatabase(Context* context) : Object(context)
{
    SubscribeToEvent(E_LOADFAILED, HANDLER(AssetDatabase, HandleResourceLoadFailed));
    SubscribeToEvent(E_PROJECTLOADED, HANDLER(AssetDatabase, HandleProjectLoaded));
    SubscribeToEvent(E_PROJECTUNLOADED, HANDLER(AssetDatabase, HandleProjectUnloaded));
}

AssetDatabase::~AssetDatabase()
{

}

String AssetDatabase::GetCachePath()
{
    if (project_.Null())
        return String::EMPTY;

    return project_->GetProjectPath() + "Cache/";

}

String AssetDatabase::GenerateAssetGUID()
{

    Time* time = GetSubsystem<Time>();

    while (true)
    {
        Poco::MD5Engine md5;
        PODVector<unsigned> data;

        for (unsigned i = 0; i < 16; i++)
        {
            data.Push(time->GetTimeSinceEpoch() + Rand());
        }

        md5.update(&data[0], data.Size() * sizeof(unsigned));

        String guid = Poco::MD5Engine::digestToHex(md5.digest()).c_str();

        if (!usedGUID_.Contains(guid))
        {
            RegisterGUID(guid);
            return guid;
        }
    }

    assert(0);
    return "";
}

void AssetDatabase::RegisterGUID(const String& guid)
{
    if (usedGUID_.Contains(guid))
    {
        assert(0);
    }

    usedGUID_.Push(guid);
}

void AssetDatabase::Import(const String& path)
{
    FileSystem* fs = GetSubsystem<FileSystem>();

    // nothing for now
    if (fs->DirExists(path))
        return;
}

Asset* AssetDatabase::GetAssetByCachePath(const String& cachePath)
{
    List<SharedPtr<Asset>>::ConstIterator itr = assets_.Begin();

    String cacheFilename = GetFileName(cachePath);

    while (itr != assets_.End())
    {
        if ((*itr)->GetCachePath().Contains(cacheFilename))
            return *itr;

        itr++;
    }

    return 0;

}

Asset* AssetDatabase::GetAssetByGUID(const String& guid)
{
    List<SharedPtr<Asset>>::ConstIterator itr = assets_.Begin();

    while (itr != assets_.End())
    {
        if (guid == (*itr)->GetGUID())
            return *itr;

        itr++;
    }

    return 0;

}

Asset* AssetDatabase::GetAssetByPath(const String& path)
{
    List<SharedPtr<Asset>>::ConstIterator itr = assets_.Begin();

    while (itr != assets_.End())
    {
        if (path == (*itr)->GetPath())
            return *itr;

        itr++;
    }

    return 0;

}

void AssetDatabase::PruneOrphanedDotAssetFiles()
{

    if (project_.Null())
    {
        LOGDEBUG("AssetDatabase::PruneOrphanedDotAssetFiles - called without project loaded");
        return;
    }

    FileSystem* fs = GetSubsystem<FileSystem>();

    const String& resourcePath = project_->GetResourcePath();

    Vector<String> allResults;

    fs->ScanDir(allResults, resourcePath, "*.asset", SCAN_FILES, true);

    for (unsigned i = 0; i < allResults.Size(); i++)
    {
        String dotAssetFilename = resourcePath + allResults[i];
        String assetFilename = ReplaceExtension(dotAssetFilename, "");

        // remove orphaned asset files
        if (!fs->FileExists(assetFilename) && !fs->DirExists(assetFilename))
        {

            LOGINFOF("Removing orphaned asset file: %s", dotAssetFilename.CString());
            fs->Delete(dotAssetFilename);
        }

    }
}

String AssetDatabase::GetDotAssetFilename(const String& path)
{
    FileSystem* fs = GetSubsystem<FileSystem>();

    String assetFilename = path + ".asset";

    if (fs->DirExists(path)) {

        assetFilename = RemoveTrailingSlash(path) + ".asset";
    }

    return assetFilename;

}

void AssetDatabase::AddAsset(SharedPtr<Asset>& asset)
{

    assert(asset->GetGUID().Length());

    assert(!GetAssetByGUID(asset->GetGUID()));

    assets_.Push(asset);

    // set to the current timestamp
    asset->UpdateFileTimestamp();

    VariantMap eventData;
    eventData[ResourceAdded::P_GUID] = asset->GetGUID();
    SendEvent(E_RESOURCEADDED, eventData);
}

void AssetDatabase::DeleteAsset(Asset* asset)
{
    SharedPtr<Asset> assetPtr(asset);

    List<SharedPtr<Asset>>::Iterator itr = assets_.Find(assetPtr);

    if (itr == assets_.End())
        return;

    assets_.Erase(itr);

    const String& resourcePath = asset->GetPath();

    FileSystem* fs = GetSubsystem<FileSystem>();

    if (fs->DirExists(resourcePath))
    {
        fs->RemoveDir(resourcePath, true);
    }
    else if (fs->FileExists(resourcePath))
    {
        fs->Delete(resourcePath);
    }

    String dotAsset = resourcePath + ".asset";

    if (fs->FileExists(dotAsset))
    {
        fs->Delete(dotAsset);
    }

    VariantMap eventData;
    eventData[ResourceRemoved::P_GUID] = asset->GetGUID();
    SendEvent(E_RESOURCEREMOVED, eventData);
}

bool AssetDatabase::ImportDirtyAssets()
{

    PODVector<Asset*> assets;
    GetDirtyAssets(assets);

    for (unsigned i = 0; i < assets.Size(); i++)
    {
        assets[i]->Import();
        assets[i]->Save();
        assets[i]->dirty_ = false;
        assets[i]->UpdateFileTimestamp();
    }

    return assets.Size() != 0;

}

void AssetDatabase::PreloadAssets()
{

    List<SharedPtr<Asset>>::ConstIterator itr = assets_.Begin();

    while (itr != assets_.End())
    {
        (*itr)->Preload();
        itr++;
    }

}


void AssetDatabase::Scan()
{
    PruneOrphanedDotAssetFiles();

    FileSystem* fs = GetSubsystem<FileSystem>();
    const String& resourcePath = project_->GetResourcePath();

    Vector<String> allResults;

    fs->ScanDir(allResults, resourcePath, "", SCAN_FILES | SCAN_DIRS, true);

    Vector<String> filteredResults;

    filteredResults.Push(RemoveTrailingSlash(resourcePath));

    for (unsigned i = 0; i < allResults.Size(); i++)
    {
        allResults[i] = resourcePath + allResults[i];

        const String& path = allResults[i];

        if (path.StartsWith(".") || path.EndsWith("."))
            continue;

        String ext = GetExtension(path);

        if (ext == ".asset")
            continue;

        filteredResults.Push(path);
    }

    for (unsigned i = 0; i < filteredResults.Size(); i++)
    {
        const String& path = filteredResults[i];

        String dotAssetFilename = GetDotAssetFilename(path);

        if (!fs->FileExists(dotAssetFilename))
        {
            // new asset
            SharedPtr<Asset> asset(new Asset(context_));

            if (asset->SetPath(path))
                AddAsset(asset);
        }
        else
        {
            SharedPtr<File> file(new File(context_, dotAssetFilename));
            SharedPtr<JSONFile> json(new JSONFile(context_));
            json->Load(*file);
            file->Close();

            JSONValue root = json->GetRoot();

            assert(root.Get("version").GetInt() == ASSET_VERSION);

            String guid = root.Get("guid").GetString();

            if (!GetAssetByGUID(guid))
            {
                SharedPtr<Asset> asset(new Asset(context_));
                asset->SetPath(path);
                AddAsset(asset);
            }

        }

    }

    PreloadAssets();

    if (ImportDirtyAssets())
        Scan();

}

void AssetDatabase::GetFolderAssets(String folder, PODVector<Asset*>& assets) const
{
    if (project_.Null())
        return;

    List<SharedPtr<Asset>>::ConstIterator itr = assets_.Begin();

    if (!folder.Length())
    {
        folder = project_->GetResourcePath();
    }

    folder = AddTrailingSlash(folder);

    while (itr != assets_.End())
    {
        String path = GetPath((*itr)->GetPath());

        if (path == folder)
            assets.Push(*itr);

        itr++;
    }

}

void AssetDatabase::GetAssetsByImporterType(StringHash type, const String &resourceType, PODVector<Asset*>& assets) const
{
    assets.Clear();

    List<SharedPtr<Asset>>::ConstIterator itr = assets_.Begin();

    while (itr != assets_.End())
    {
        Asset* asset = *itr;

        if (asset->GetImporterType() == type)
            assets.Push(asset);

        itr++;
    }

}

void AssetDatabase::GetDirtyAssets(PODVector<Asset*>& assets)
{
    assets.Clear();

    List<SharedPtr<Asset>>::ConstIterator itr = assets_.Begin();

    while (itr != assets_.End())
    {
        if ((*itr)->IsDirty())
            assets.Push(*itr);
        itr++;
    }
}

void AssetDatabase::HandleProjectLoaded(StringHash eventType, VariantMap& eventData)
{
    project_ = GetSubsystem<ToolSystem>()->GetProject();

    FileSystem* fs = GetSubsystem<FileSystem>();

    if (!fs->DirExists(GetCachePath()))
        fs->CreateDir(GetCachePath());

    ResourceCache* cache = GetSubsystem<ResourceCache>();
    cache->AddResourceDir(GetCachePath());

    Scan();

    SubscribeToEvent(E_FILECHANGED, HANDLER(AssetDatabase, HandleFileChanged));
}

void AssetDatabase::HandleProjectUnloaded(StringHash eventType, VariantMap& eventData)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    cache->RemoveResourceDir(GetCachePath());
    assets_.Clear();
    usedGUID_.Clear();
    assetImportErrorTimes_.Clear();
    project_ = 0;

    UnsubscribeFromEvent(E_FILECHANGED);
}

void AssetDatabase::HandleResourceLoadFailed(StringHash eventType, VariantMap& eventData)
{

    if (project_.Null())
        return;

    String path = eventData[LoadFailed::P_RESOURCENAME].GetString();

    Asset* asset = GetAssetByPath(path);

    if (!asset)
        asset = GetAssetByPath(project_->GetResourcePath() + path);

    if (!asset)
        return;

    Time* time = GetSubsystem<Time>();

    unsigned ctime = time->GetSystemTime();

    // if less than 5 seconds since last report, stifle report
    if (assetImportErrorTimes_.Contains(asset->guid_))
        if (ctime - assetImportErrorTimes_[asset->guid_] < 5000)
            return;

    assetImportErrorTimes_[asset->guid_] = ctime;

    VariantMap evData;
    evData[AssetImportError::P_PATH] = asset->path_;
    evData[AssetImportError::P_GUID] = asset->guid_;
    evData[AssetImportError::P_ERROR] = ToString("Asset %s Failed to Load", asset->path_.CString());
    SendEvent(E_ASSETIMPORTERROR, evData);


}

void AssetDatabase::HandleFileChanged(StringHash eventType, VariantMap& eventData)
{
    using namespace FileChanged;
    const String& fullPath = eventData[P_FILENAME].GetString();

    FileSystem* fs = GetSubsystem<FileSystem>();

    String pathName, fileName, ext;

    SplitPath(fullPath, pathName, fileName, ext);

    // ignore changes in the Cache resource dir
    if (fullPath == GetCachePath() || pathName.StartsWith(GetCachePath()))
        return;

    // don't care about directories and asset file changes
    if (fs->DirExists(fullPath) || ext == ".asset")
        return;

    Asset* asset = GetAssetByPath(fullPath);

    if (!asset && fs->FileExists(fullPath))
    {
        Scan();
        return;
    }

    if (asset)
    {
        if(!fs->Exists(fullPath))
        {
            DeleteAsset(asset);
        }
        else
        {
            if (asset->GetFileTimestamp() != fs->GetLastModifiedTime(asset->GetPath()))
            {
                asset->SetDirty(true);
                Scan();
            }
        }
    }
}

String AssetDatabase::GetResourceImporterName(const String& resourceTypeName)
{
    // TODO: have resource type register themselves
    if (resourceTypeToImporterType_.Empty())
    {
        resourceTypeToImporterType_["Sound"] = "AudioImporter";
        resourceTypeToImporterType_["Model"] = "ModelImporter";
        resourceTypeToImporterType_["Material"] = "MaterialImporter";
        resourceTypeToImporterType_["Texture2D"] = "TextureImporter";
        resourceTypeToImporterType_["Sprite2D"] = "TextureImporter";
        resourceTypeToImporterType_["AnimatedSprite2D"] = "SpriterImporter";
        resourceTypeToImporterType_["JSComponentFile"] = "JavascriptImporter";        
        resourceTypeToImporterType_["JSONFile"] = "JSONImporter";
        resourceTypeToImporterType_["ParticleEffect2D"] = "PEXImporter";

        resourceTypeToImporterType_["Animation"] = "ModelImporter";


#ifdef ATOMIC_DOTNET
        resourceTypeToImporterType_["CSComponentAssembly"] = "NETAssemblyImporter";
#endif

    }

    if (!resourceTypeToImporterType_.Contains(resourceTypeName))
        return String::EMPTY;

    return resourceTypeToImporterType_[resourceTypeName];

}


}
