
// Portions Copyright (c) 2008-2015 the Urho3D project.

//
// Copyright (c) 2014-2015, THUNDERBEAST GAMES LLC All rights reserved
// LICENSE: Atomic Game Engine Editor and Tools EULA
// Please see LICENSE_ATOMIC_EDITOR_AND_TOOLS.md in repository root for
// license information: https://github.com/AtomicGameEngine/AtomicGameEngine
//

#pragma once

#include <Atomic/Core/Object.h>
#include <Atomic/Math/MathDefs.h>
#include <Atomic/Math/Ray.h>
#include <Atomic/Scene/Scene.h>

#include <Atomic/Atomic3D/StaticModel.h>
#include <Atomic/Graphics/Camera.h>


#include "SceneView3D.h"

using namespace Atomic;

namespace Atomic
{
    class Node;
}

namespace AtomicEditor
{

class SceneSelection;

class Gizmo3DAxis
{
public:

    Ray axisRay_;
    bool selected_;
    bool lastSelected_;
    float t_;
    float d_;
    float lastT_;
    float lastD_;

    Gizmo3DAxis()
    {
        selected_ = false;
        lastSelected_ = false;
        t_ = 0.0;
        d_ = 0.0;
        lastT_ = 0.0;
        lastD_ = 0.0;
    }

    void Update(Ray cameraRay, float scale, bool drag, Node* cameraNode)
    {
        const float axisMaxD = 0.1f;
        const float axisMaxT = 1.0f;

        Vector3 closest = cameraRay.ClosestPoint(axisRay_);
        Vector3 projected = axisRay_.Project(closest);
        d_ = axisRay_.Distance(closest);
        t_ = (projected - axisRay_.origin_).DotProduct(axisRay_.direction_);

        // Determine the sign of d from a plane that goes through the camera position to the axis
        Plane axisPlane(cameraNode->GetPosition(), axisRay_.origin_, axisRay_.origin_ + axisRay_.direction_);
        if (axisPlane.Distance(closest) < 0.0)
            d_ = -d_;

        // Update selected status only when not dragging
        if (!drag)
        {
            selected_ = Abs(d_) < axisMaxD * scale && t_ >= -axisMaxD * scale && t_ <= axisMaxT * scale;
            lastT_ = t_;
            lastD_ = d_;
        }
    }

    void Moved()
    {
        lastT_ = t_;
        lastD_ = d_;
    }
};

enum EditMode
{
    EDIT_SELECT,
    EDIT_MOVE,
    EDIT_ROTATE,
    EDIT_SCALE
};

enum AxisMode
{
    AXIS_WORLD = 0,
    AXIS_LOCAL
};

class Gizmo3D: public Object
{
    OBJECT(Gizmo3D);

public:

    Gizmo3D(Context* context);
    virtual ~Gizmo3D();

    void SetView(SceneView3D* view3D);

    void SetAxisMode(AxisMode mode);
    AxisMode GetAxisMode() const { return axisMode_; }

    void SetEditMode(EditMode mode);

    bool Selected()
    {
        return gizmoAxisX_.selected_ || gizmoAxisY_.selected_ || gizmoAxisZ_.selected_;
    }

    void Show();
    void Hide();
    void Update();

    Node* GetGizmoNode() { return gizmoNode_; }

    float GetSnapTranslationX() const;
    float GetSnapTranslationY() const;
    float GetSnapTranslationZ() const;
    float GetSnapRotation() const;
    float GetSnapScale() const;

    void SetSnapTranslationX(float value);
    void SetSnapTranslationY(float value);
    void SetSnapTranslationZ(float value);
    void SetSnapRotation(float value);
    void SetSnapScale(float value);

private:

    void Position();
    void Use();
    void Drag();
    void Moved();
    void CalculateGizmoAxes();

    bool MoveEditNodes(Vector3 adjust);
    bool RotateEditNodes(Vector3 adjust);
    bool ScaleEditNodes(Vector3 adjust);

    SharedPtr<Node> gizmoNode_;

    WeakPtr<SceneView3D> view3D_;
    WeakPtr<Scene> scene_;
    WeakPtr<Camera> camera_;
    WeakPtr<StaticModel> gizmo_;
    WeakPtr<SceneSelection> selection_;

    Gizmo3DAxis gizmoAxisX_;
    Gizmo3DAxis gizmoAxisY_;
    Gizmo3DAxis gizmoAxisZ_;

    EditMode editMode_;
    EditMode lastEditMode_;

    AxisMode axisMode_;

    bool dragging_;

    // snap settings
    float snapTranslationX_;
    float snapTranslationY_;
    float snapTranslationZ_;
    float snapRotation_;
    float snapScale_;

};

}
