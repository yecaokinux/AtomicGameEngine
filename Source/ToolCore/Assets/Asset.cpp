//
// Copyright (c) 2014-2015, THUNDERBEAST GAMES LLC All rights reserved
// LICENSE: Atomic Game Engine Editor and Tools EULA
// Please see LICENSE_ATOMIC_EDITOR_AND_TOOLS.md in repository root for
// license information: https://github.com/AtomicGameEngine/AtomicGameEngine
//

#include <Atomic/IO/Log.h>
#include <Atomic/IO/File.h>
#include <Atomic/IO/FileSystem.h>

#include "../ToolSystem.h"
#include "../Project/Project.h"

#include "AssetDatabase.h"
#include "AudioImporter.h"
#include "ModelImporter.h"
#include "FolderImporter.h"
#include "SceneImporter.h"
#include "MaterialImporter.h"
#include "TextureImporter.h"
#include "PrefabImporter.h"
#include "JavascriptImporter.h"
#include "JSONImporter.h"
#include "SpriterImporter.h"
#include "TMXImporter.h"
#include "PEXImporter.h"
#include "TextImporter.h"
#include "NETAssemblyImporter.h"

#include "AssetEvents.h"
#include "Asset.h"

namespace ToolCore
{

Asset::Asset(Context* context) :
    Object(context),
    dirty_(false),
    isFolder_(false),
    fileTimestamp_(0xffffffff)
{

}

Asset::~Asset()
{

}

void Asset::UpdateFileTimestamp()
{
    FileSystem* fs = GetSubsystem<FileSystem>();

    if (fs->FileExists(path_))
    {
        fileTimestamp_ = fs->GetLastModifiedTime(path_);
    }
}

Asset* Asset::GetParent()
{
    AssetDatabase* db = GetSubsystem<AssetDatabase>();

    String pathName;
    String fileName;
    String ext;

    SplitPath(path_, pathName, fileName, ext);

    return db->GetAssetByPath(RemoveTrailingSlash(pathName));

}

String Asset::GetRelativePath()
{
    Project* project =GetSubsystem<ToolSystem>()->GetProject();

    String path = path_;

    path.Replace(project->GetResourcePath(), "", false);

    return path;

}

bool Asset::CheckCacheFile()
{
    if (importer_.Null())
        return true;

    FileSystem* fs = GetSubsystem<FileSystem>();
    AssetDatabase* db = GetSubsystem<AssetDatabase>();
    String cachePath = db->GetCachePath();

    String cacheFile = cachePath + guid_;

    unsigned modifiedTime = fs->GetLastModifiedTime(path_);

    if (importer_->RequiresCacheFile()) {

        if (!fs->FileExists(cacheFile) || fs->GetLastModifiedTime(cacheFile) < modifiedTime)
            return false;
    }

    if (fs->GetLastModifiedTime(GetDotAssetFilename()) < modifiedTime)
    {
        return false;
    }

    return true;
}

bool Asset::Import()
{

    if (importer_.Null())
        return true;

    return importer_->Import();
}

bool Asset::Preload()
{
    if (importer_.Null())
        return true;

    // disabled preload for now, as this is on a background thread and causing init problems
    return true;
    //return importer_->Preload();
}

void Asset::PostImportError(const String& message)
{
    VariantMap eventData;
    eventData[AssetImportError::P_PATH] = path_;
    eventData[AssetImportError::P_GUID] = guid_;
    eventData[AssetImportError::P_ERROR] = message;

    SendEvent(E_ASSETIMPORTERROR, eventData);

}

// load .asset
bool Asset::Load()
{
    FileSystem* fs = GetSubsystem<FileSystem>();
    AssetDatabase* db = GetSubsystem<AssetDatabase>();

    String assetFilename = GetDotAssetFilename();

    SharedPtr<File> file(new File(context_, assetFilename));
    json_ = new JSONFile(context_);
    json_->Load(*file);
    file->Close();

    JSONValue root = json_->GetRoot();

    assert(root.Get("version").GetInt() == ASSET_VERSION);

    guid_ = root.Get("guid").GetString();

    db->RegisterGUID(guid_);

    dirty_ = false;
    if (!CheckCacheFile())
    {
        LOGINFOF("CheckCacheFile:false - %s", path_.CString());
        dirty_ = true;
    }

    // handle import

    if (importer_.NotNull())
        importer_->LoadSettings(root);

    json_ = 0;

    return true;

}

// save .asset
bool Asset::Save()
{
    FileSystem* fs = GetSubsystem<FileSystem>();
    String assetFilename = GetDotAssetFilename();

    json_ = new JSONFile(context_);

    JSONValue& root = json_->GetRoot();

    root.Set("version", JSONValue(ASSET_VERSION));
    root.Set("guid", JSONValue(guid_));

    // handle import

    if (importer_.NotNull())
    {
        importer_->SaveSettings(root);

        SharedPtr<File> file(new File(context_, assetFilename, FILE_WRITE));
        json_->Save(*file);
        file->Close();
    }

    json_ = 0;

    return true;

}

String Asset::GetDotAssetFilename()
{
    assert(path_.Length());

    FileSystem* fs = GetSubsystem<FileSystem>();

    String assetFilename = path_ + ".asset";

    if (fs->DirExists(path_)) {

        assetFilename = RemoveTrailingSlash(path_) + ".asset";

    }

    return assetFilename;

}

bool Asset::CreateImporter()
{
    assert(importer_.Null());

    FileSystem* fs = GetSubsystem<FileSystem>();

    if (fs->DirExists(path_))
    {
        name_ = GetFileName(RemoveTrailingSlash(path_));
        isFolder_ = true;
        importer_ = new FolderImporter(context_, this);
    }
    else
    {
        String ext = Atomic::GetExtension(path_);

        name_ = GetFileName(path_);

        Vector<String> textureFormats;
        textureFormats.Push(".jpg");
        textureFormats.Push(".png");
        textureFormats.Push(".tga");
        textureFormats.Push(".dds");

        // todo, externalize recognizers
        if (ext == ".fbx" || ext == ".blend" || ext == ".dae" || ext == ".mdl")
        {
            importer_ = new ModelImporter(context_, this);
        }
        if (ext == ".ogg" || ext == ".wav")
        {
            importer_ = new AudioImporter(context_, this);
        }
        else if (ext == ".prefab")
        {
            importer_ = new PrefabImporter(context_, this);
        }
        else if (ext == ".js")
        {
            importer_ = new JavascriptImporter(context_, this);
        }
        else if (ext == ".json")
        {
            importer_ = new JSONImporter(context_, this);
        }
        else if (ext == ".scene")
        {
            importer_ = new SceneImporter(context_, this);
        }
        else if (ext == ".material")
        {
            importer_ = new MaterialImporter(context_, this);
        }
        else if (ext == ".scml")
        {
            importer_ = new SpriterImporter(context_, this);
        }
        else if (ext == ".tmx")
        {
            importer_ = new TMXImporter(context_, this);
        }
        else if (ext == ".pex")
        {
            importer_ = new PEXImporter(context_, this);
        }
        else if (ext == ".txt")
        {
            importer_ = new TextImporter(context_, this);
        }
        else if (ext == ".dll")
        {
            // TODO: check for native dll
#ifdef ATOMIC_DOTNET
            importer_ = new NETAssemblyImporter(context_, this);
#endif
        }
        else if (textureFormats.Contains(ext))
        {
            importer_ = new TextureImporter(context_, this);
        }

    }

    if (importer_.Null())
        return false;

    return true;

}

String Asset::GetCachePath() const
{
    AssetDatabase* db = GetSubsystem<AssetDatabase>();
    String cachePath = db->GetCachePath();
    cachePath += guid_;
    return cachePath;
}

String Asset::GetExtension() const
{

    return Atomic::GetExtension(path_);

}

bool Asset::SetPath(const String& path)
{

    assert(!guid_.Length());
    assert(!path_.Length());

    // need to update path, not set, which should only be done on first import
    assert(importer_.Null());

    FileSystem* fs = GetSubsystem<FileSystem>();
    AssetDatabase* db = GetSubsystem<AssetDatabase>();

    path_ = path;

    // create importer based on path
    if (!CreateImporter())
        return false;

    String assetFilename = GetDotAssetFilename();

    if (fs->FileExists(assetFilename))
    {
        // load the json, todo: handle fail
        Load();
    }
    else
    {
        dirty_ = true;
        guid_ = db->GenerateAssetGUID();

        Save();
    }

    // TODO: handle failed

    return true;

}

bool Asset::Move(const String& newPath)
{
    if (importer_.Null())
        return false;

    String oldPath = path_;

    bool result = importer_->Move(newPath);

    if (result)
    {
        VariantMap eventData;
        eventData[AssetMoved::P_ASSET] = this;
        eventData[AssetMoved::P_OLDPATH] = oldPath;
        SendEvent(E_ASSETMOVED, eventData);
    }

    return result;

}

bool Asset::Rename(const String& newName)
{
    if (importer_.Null())
        return false;

    bool result = importer_->Rename(newName);

    if (result)
    {
        VariantMap eventData;
        eventData[AssetRenamed::P_ASSET] = this;
        SendEvent(E_ASSETRENAMED, eventData);
    }

    return result;
}

Resource* Asset::GetResource(const String &typeName)
{
    if (importer_)
        return importer_->GetResource(typeName);

    return 0;
}

Node* Asset::InstantiateNode(Node* parent, const String& name)
{
    if (!parent)
        return 0;

    if (importer_)
        return importer_->InstantiateNode(parent, name);

    return 0;

}

}
