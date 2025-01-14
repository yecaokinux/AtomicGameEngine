//
// Copyright (c) 2014-2015, THUNDERBEAST GAMES LLC All rights reserved
// LICENSE: Atomic Game Engine Editor and Tools EULA
// Please see LICENSE_ATOMIC_EDITOR_AND_TOOLS.md in repository root for
// license information: https://github.com/AtomicGameEngine/AtomicGameEngine
//

#pragma once

#include <Atomic/Core/Object.h>
#include <Atomic/Container/List.h>
#include "Asset.h"

using namespace Atomic;

namespace ToolCore
{

class Project;

class AssetDatabase : public Object
{
    OBJECT(AssetDatabase);

public:
    /// Construct.
    AssetDatabase(Context* context);
    virtual ~AssetDatabase();

    Asset* GetAssetByGUID(const String& guid);
    Asset* GetAssetByPath(const String& path);
    Asset* GetAssetByCachePath(const String& cachePath);

    String GenerateAssetGUID();
    void RegisterGUID(const String& guid);

    String GetCachePath();

    void DeleteAsset(Asset* asset);

    void Scan();

    void GetFolderAssets(String folder, PODVector<Asset*>& assets) const;

    String GetResourceImporterName(const String& resourceTypeName);

    void GetAssetsByImporterType(StringHash type, const String& resourceType, PODVector<Asset*>& assets) const;

    void GetDirtyAssets(PODVector<Asset*>& assets);

    String GetDotAssetFilename(const String& path);

private:

    void HandleProjectLoaded(StringHash eventType, VariantMap& eventData);
    void HandleProjectUnloaded(StringHash eventType, VariantMap& eventData);
    void HandleFileChanged(StringHash eventType, VariantMap& eventData);
    void HandleResourceLoadFailed(StringHash eventType, VariantMap& eventData);

    void AddAsset(SharedPtr<Asset>& asset);

    void PruneOrphanedDotAssetFiles();

    void Import(const String& path);

    bool ImportDirtyAssets();
    void PreloadAssets();

    SharedPtr<Project> project_;
    List<SharedPtr<Asset>> assets_;

    HashMap<StringHash, String> resourceTypeToImporterType_;

    /// Hash value of times, so we don't spam import errors
    HashMap<StringHash, unsigned> assetImportErrorTimes_;

    Vector<String> usedGUID_;

};

}
