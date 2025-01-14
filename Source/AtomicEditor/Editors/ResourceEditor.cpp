//
// Copyright (c) 2014-2015, THUNDERBEAST GAMES LLC All rights reserved
// LICENSE: Atomic Game Engine Editor and Tools EULA
// Please see LICENSE_ATOMIC_EDITOR_AND_TOOLS.md in repository root for
// license information: https://github.com/AtomicGameEngine/AtomicGameEngine
//

#include <TurboBadger/tb_tab_container.h>

#include <Atomic/Core/StringUtils.h>
#include <Atomic/IO/FileSystem.h>
#include <Atomic/Resource/ResourceEvents.h>

#include <TurboBadger/tb_message_window.h>

#include "ResourceEditorEvents.h"

#include "ResourceEditor.h"

//#include "../UI/UIMainFrame.h"
//#include "../UI/UIResourceFrame.h"

namespace AtomicEditor
{

class EditorTabLayout: public TBLayout
{
public:

    ResourceEditor* editor_;
    TBButton* button_;
    TBButton* close_;
    TBTabContainer* container_;

    void SetValue(int value)
    {
        button_->SetValue(value);
    }

    bool RequestClose()
    {
        if (editor_->HasUnsavedModifications())
        {
            TBMessageWindow *msg_win = new TBMessageWindow(this, TBIDC("unsaved_modifications_dialog"));
            TBMessageWindowSettings settings(TB_MSG_NONE, TBID(uint32(0)));
            settings.dimmer = true;
            settings.styling = true;
            String windowString = Atomic::ToString("%s has unsaved modifications.\nDo you wish to discard them and close?", GetFileNameAndExtension(editor_->GetFullPath()).CString());
            msg_win->Show("Unsaved Modifications",  windowString.CString(), &settings, 640, 360);
            msg_win->AddButtonLeft("dont_save", false);
            msg_win->AddButton("TBMessageWindow.cancel", false);
            msg_win->AddButton("save", true);
            return false;
        }
        else
        {
            editor_->Close(container_->GetNumPages()>1);
            return true;
        }
    }

    bool OnEvent(const TBWidgetEvent &ev)
    {
        if (ev.type == EVENT_TYPE_CLICK || ev.type == EVENT_TYPE_POINTER_DOWN)
        {
            if (ev.target->GetID() == TBIDC("unsaved_modifications_dialog"))
            {
                if (ev.ref_id == TBIDC("dont_save"))
                {
                    container_->OnEvent(ev);
                    editor_->Close(container_->GetNumPages()>1);
                }
                else if (ev.ref_id == TBIDC("cancel"))
                {
                    editor_->SendEvent(E_EDITORRESOURCECLOSECANCELED);
                    SetFocus(WIDGET_FOCUS_REASON_UNKNOWN);
                }
                else if (ev.ref_id == TBIDC("save"))
                {
                    editor_->Save();
                    container_->OnEvent(ev);
                    editor_->Close(container_->GetNumPages()>1);
                }
                return true;
            }
            if (ev.target->GetID() == TBIDC("tabclose"))
            {
                if (RequestClose())
                {
                    container_->OnEvent(ev);
                    return true;
                }
            }
            else 
            {
                TBWidgetEvent nevent = ev;
                nevent.target = this;
                container_->OnEvent(nevent);
            }
        }

        return false;
    }
};

ResourceEditor::ResourceEditor(Context* context, const String& fullpath, UITabContainer *container):
    Object(context), fullpath_(fullpath), container_(container),
    editorTabLayout_(0), rootContentWidget_(0), button_(0), modified_(false)
{

    String filename = GetFileNameAndExtension(fullpath_);

    editorTabLayout_ = new EditorTabLayout();
    editorTabLayout_->SetID(TBIDC("tab"));

    button_ = new UIButton(context_);
    button_->SetText(filename.CString());
    button_->SetSqueezable(true);
    button_->SetSkinBg("TBButton.flat");
    button_->SetValue(1);
    editorTabLayout_->AddChild(button_->GetInternalWidget());

    TBButton* closebutton = new TBButton();
    editorTabLayout_->AddChild(closebutton);
    closebutton->SetSkinBg(TBIDC("TBWindow.close"));
    closebutton->SetIsFocusable(false);
    closebutton->SetID(TBIDC("tabclose"));

    editorTabLayout_->editor_ = this;
    editorTabLayout_->button_ = (TBButton*) button_->GetInternalWidget();
    editorTabLayout_->close_ = closebutton;
    editorTabLayout_->container_ = (TBTabContainer*) container->GetInternalWidget();

    ((TBTabContainer*)container_->GetInternalWidget())->GetTabLayout()->AddChild(editorTabLayout_);

    rootContentWidget_ = new UIWidget(context_);
    rootContentWidget_->SetGravity(UI_GRAVITY_ALL);
    container_->GetContentRoot()->AddChild(rootContentWidget_);

    SubscribeToEvent(E_FILECHANGED, HANDLER(ResourceEditor, HandleFileChanged));
}

ResourceEditor::~ResourceEditor()
{

}

void ResourceEditor::HandleFileChanged(StringHash eventType, VariantMap& eventData)
{
    /*
    using namespace FileChanged;
    const String& fileName = eventData[P_FILENAME].GetString();
    const String& resourceName = eventData[P_RESOURCENAME].GetString();

    if (fullpath_ == fileName)
    {
        FileSystem* fs = GetSubsystem<FileSystem>();
        if (!fs->FileExists(fullpath_))
            Close();
    }
    */
}

void ResourceEditor::RequestClose()
{
    editorTabLayout_->RequestClose();
}

void ResourceEditor::Close(bool navigateToAvailableResource)
{
    // keep us alive through the close
    SharedPtr<ResourceEditor> keepalive(this);

    ((TBTabContainer*)container_->GetInternalWidget())->GetTabLayout()->RemoveChild(editorTabLayout_);

    VariantMap data;
    data[EditorResourceClose::P_EDITOR] = this;
    data[EditorResourceClose::P_NAVIGATE] = navigateToAvailableResource;
    SendEvent(E_EDITORRESOURCECLOSE, data);
}

void ResourceEditor::InvokeShortcut(const String& shortcut)
{
    TBWidgetEvent ev(EVENT_TYPE_SHORTCUT);
    ev.ref_id = TBIDC(shortcut.CString());
    OnEvent(ev);
}

void ResourceEditor::SetModified(bool modified)
{
    modified_ = modified;
    if (modified)
    {
        String filename = GetFileNameAndExtension(fullpath_);
        filename += "*";
        button_->SetText(filename.CString());
    }
    else
    {
        String filename = GetFileNameAndExtension(fullpath_);
        button_->SetText(filename.CString());
    }
}

}
