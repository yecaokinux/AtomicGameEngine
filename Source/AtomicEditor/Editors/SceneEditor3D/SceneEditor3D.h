//
// Copyright (c) 2014-2015, THUNDERBEAST GAMES LLC All rights reserved
// LICENSE: Atomic Game Engine Editor and Tools EULA
// Please see LICENSE_ATOMIC_EDITOR_AND_TOOLS.md in repository root for
// license information: https://github.com/AtomicGameEngine/AtomicGameEngine
//

#pragma once

#include <TurboBadger/tb_widgets_common.h>

#include "../ResourceEditor.h"
#include "SceneView3D.h"
#include "Gizmo3D.h"

using namespace Atomic;
using namespace tb;

namespace Atomic
{
class Scene;
class Node;
class View3D;
class Camera;
class DebugRenderer;
class Octree;

}

namespace ToolCore
{
    class ProjectUserPrefs;
}

using namespace ToolCore;

namespace AtomicEditor
{

class SceneSelection;
class SceneEditHistory;

class SceneEditor3D: public ResourceEditor
{
    OBJECT(SceneEditor3D);

public:

    SceneEditor3D(Context* context, const String& fullpath, UITabContainer* container);

    virtual ~SceneEditor3D();

    bool OnEvent(const TBWidgetEvent &ev);

    SceneSelection* GetSelection() { return selection_; }
    SceneEditHistory* GetEditHistory() { return editHistory_; }
    SceneView3D* GetSceneView3D() { return sceneView_; }

    void RegisterNode(Node* node);
    void RegisterNodes(const PODVector<Node*>& nodes);

    void ReparentNode(Node* node, Node* newParent);

    Scene* GetScene() { return scene_; }
    Gizmo3D* GetGizmo() { return gizmo3D_; }

    void SetFocus();

    virtual bool RequiresInspector() { return true; }

    void Close(bool navigateToAvailableResource = true);
    bool Save();

    void Undo();
    void Redo();
    void Cut();
    void Copy();
    void Paste();

    ProjectUserPrefs* GetUserPrefs();

    void InvokeShortcut(const String& shortcut);

    static SceneEditor3D* GetSceneEditor(Scene* scene);

private:

    void HandleUpdate(StringHash eventType, VariantMap& eventData);
    void HandlePlayStarted(StringHash eventType, VariantMap& eventData);
    void HandlePlayStopped(StringHash eventType, VariantMap& eventData);
    void HandleGizmoEditModeChanged(StringHash eventType, VariantMap& eventData);
    void HandleGizmoAxisModeChanged(StringHash eventType, VariantMap& eventData);

    void HandleUserPrefSaved(StringHash eventType, VariantMap& eventData);

    void HandleSceneEditSceneModified(StringHash eventType, VariantMap& eventData);
    void HandleSceneEditNodeCreated(StringHash eventType, VariantMap& eventData);

    void HandleCubemapRenderBegin(StringHash eventType, VariantMap& eventData);
    void HandleCubemapRenderEnd(StringHash eventType, VariantMap& eventData);

    void UpdateGizmoSnapSettings();

    SharedPtr<Scene> scene_;

    // TODO: multiple views
    SharedPtr<SceneView3D> sceneView_;

    SharedPtr<Gizmo3D> gizmo3D_;

    SharedPtr<SceneSelection> selection_;
    SharedPtr<SceneEditHistory> editHistory_;

    SharedPtr<Node> clipboardNode_;

    WeakPtr<ProjectUserPrefs> userPrefs_;

    void RegisterSceneEditor();

    static Vector<WeakPtr<SceneEditor3D>> sceneEditors_;

    int cubemapRenderCount_;

};

}
