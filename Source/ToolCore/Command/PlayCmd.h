//
// Copyright (c) 2014-2015, THUNDERBEAST GAMES LLC All rights reserved
// LICENSE: Atomic Game Engine Editor and Tools EULA
// Please see LICENSE_ATOMIC_EDITOR_AND_TOOLS.md in repository root for
// license information: https://github.com/AtomicGameEngine/AtomicGameEngine
//

#pragma once

#include "Command.h"

using namespace Atomic;

namespace ToolCore
{

class PlayCmd: public Command
{
    OBJECT(PlayCmd);

public:

    PlayCmd(Context* context);
    virtual ~PlayCmd();

    bool Parse(const Vector<String>& arguments, unsigned startIndex, String& errorMsg);

    void Run();

private:

    bool LaunchPlayerProcess(const String& command, const Vector<String>& args, const String& initialDirectory);

};

}
