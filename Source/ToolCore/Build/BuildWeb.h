//
// Copyright (c) 2014-2015, THUNDERBEAST GAMES LLC All rights reserved
// LICENSE: Atomic Game Engine Editor and Tools EULA
// Please see LICENSE_ATOMIC_EDITOR_AND_TOOLS.md in repository root for
// license information: https://github.com/AtomicGameEngine/AtomicGameEngine
//

#pragma once

#include "BuildBase.h"

namespace ToolCore
{

class Project;

class BuildWeb : public BuildBase
{
    OBJECT(BuildWeb);

public:

    BuildWeb(Context* context, Project* project);
    virtual ~BuildWeb();

    void Build(const String& buildPath);

    String GetBuildSubfolder() { return "Web-Build"; }

protected:

    void Initialize();


};

}
