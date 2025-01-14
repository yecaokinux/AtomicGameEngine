//
// Copyright (c) 2014-2015, THUNDERBEAST GAMES LLC All rights reserved
// LICENSE: Atomic Game Engine Editor and Tools EULA
// Please see LICENSE_ATOMIC_EDITOR_AND_TOOLS.md in repository root for
// license information: https://github.com/AtomicGameEngine/AtomicGameEngine
//

#include <Atomic/Core/StringUtils.h>
#include <Atomic/IO/FileSystem.h>
#include <Atomic/Resource/ResourceCache.h>

#include "../ToolSystem.h"
#include "../ToolEnvironment.h"
#include "../Project/Project.h"

#include "BuildWindows.h"
#include "BuildSystem.h"
#include "BuildEvents.h"

namespace ToolCore
{

BuildWindows::BuildWindows(Context * context, Project *project) : BuildBase(context, project, PLATFORMID_WINDOWS)
{

}

BuildWindows::~BuildWindows()
{

}

void BuildWindows::Initialize()
{
    ToolSystem* tsystem = GetSubsystem<ToolSystem>();

    Project* project = tsystem->GetProject();

    Vector<String> defaultResourcePaths;
    GetDefaultResourcePaths(defaultResourcePaths);
    String projectResources = project->GetResourcePath();

    for (unsigned i = 0; i < defaultResourcePaths.Size(); i++)
    {
        AddResourceDir(defaultResourcePaths[i]);
    }

    // TODO: smart filtering of cache
    AddResourceDir(project->GetProjectPath() + "Cache/");
    AddResourceDir(projectResources);

    BuildResourceEntries();

}

void BuildWindows::BuildAtomicNET()
{
    // AtomicNET

    FileSystem* fileSystem = GetSubsystem<FileSystem>();
    ToolEnvironment* tenv = GetSubsystem<ToolEnvironment>();
    ToolSystem* tsystem = GetSubsystem<ToolSystem>();
    Project* project = tsystem->GetProject();
    String projectResources = project->GetResourcePath();

    String assembliesPath = projectResources + "Assemblies/";

    // if no assemblies path, no need to install AtomicNET
    if (!fileSystem->DirExists(assembliesPath))
        return;

    Vector<String> results;
    fileSystem->ScanDir(results, assembliesPath, "*.dll", SCAN_FILES, true);

    // if no assembiles in Assemblies path, no need to install AtomicNET
    if (!results.Size())
        return;

    BuildLog("Building AtomicNET");

    fileSystem->CreateDir(buildPath_ + "/AtomicPlayer_Resources/AtomicNET");
    fileSystem->CreateDir(buildPath_ + "/AtomicPlayer_Resources/AtomicNET/Atomic");
    fileSystem->CreateDir(buildPath_ + "/AtomicPlayer_Resources/AtomicNET/Atomic/Assemblies");

    fileSystem->CopyDir(tenv->GetNETCoreCLRAbsPath(), buildPath_ + "/AtomicPlayer_Resources/AtomicNET/CoreCLR");
    fileSystem->CopyDir(tenv->GetNETTPAPaths(), buildPath_ + "/AtomicPlayer_Resources/AtomicNET/Atomic/TPA");

    // Atomic Assemblies

    const String& assemblyLoadPaths = tenv->GetNETAssemblyLoadPaths();
    Vector<String> paths = assemblyLoadPaths.Split(';');

    for (unsigned i = 0; i < paths.Size(); i++)
    {
        Vector<String> loadResults;
        fileSystem->ScanDir(loadResults, paths[i], "*.dll", SCAN_FILES, true);

        for (unsigned j = 0; j < loadResults.Size(); j++)
        {
            String pathName, fileName, ext;
            SplitPath(loadResults[j], pathName, fileName, ext);

            if (fileName != "AtomicNETEngine")
                continue;

            fileSystem->Copy(paths[i] + "/" + loadResults[j], ToString("%s/AtomicPlayer_Resources/AtomicNET/Atomic/Assemblies/%s.dll", buildPath_.CString(), fileName.CString()));
        }

    }

    // Project assemblied
    for (unsigned i = 0; i < results.Size(); i++)
    {
        String pathName, fileName, ext;
        SplitPath(results[i], pathName, fileName, ext);
        fileSystem->Copy(assembliesPath + results[i], ToString("%s/AtomicPlayer_Resources/AtomicNET/Atomic/Assemblies/%s.dll", buildPath_.CString(), fileName.CString()));
    }



}

void BuildWindows::Build(const String& buildPath)
{
    ToolEnvironment* tenv = GetSubsystem<ToolEnvironment>();

    buildPath_ = AddTrailingSlash(buildPath) + GetBuildSubfolder();

    BuildLog("Starting Windows Deployment");

    Initialize();

    if (!BuildClean(buildPath_))
        return;

    BuildSystem* buildSystem = GetSubsystem<BuildSystem>();

    FileSystem* fileSystem = GetSubsystem<FileSystem>();

    String rootSourceDir = tenv->GetRootSourceDir();
    String playerBinary = tenv->GetPlayerBinary();
    String d3d9dll = GetPath(playerBinary) + "/D3DCompiler_47.dll";

    if (!BuildCreateDirectory(buildPath_))
        return;

    if (!BuildCreateDirectory(buildPath_ + "/AtomicPlayer_Resources"))
        return;

    String resourcePackagePath = buildPath_ + "/AtomicPlayer_Resources/AtomicResources" + PAK_EXTENSION;
    GenerateResourcePackage(resourcePackagePath);
    if (buildFailed_)
        return;
   
    if (!BuildCopyFile(playerBinary, buildPath_ + "/AtomicPlayer.exe"))
        return;

    if (!BuildCopyFile(d3d9dll, buildPath_ + "/D3DCompiler_47.dll"))
        return;

    BuildAtomicNET();

    BuildLog("Windows Deployment Complete");

    buildSystem->BuildComplete(PLATFORMID_WINDOWS, buildPath_);

}

}
