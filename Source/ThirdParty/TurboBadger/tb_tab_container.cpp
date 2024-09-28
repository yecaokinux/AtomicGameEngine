// ================================================================================
// ==      This file is a part of Turbo Badger. (C) 2011-2014, Emil Segerås      ==
// ==                     See tb_core.h for more information.                    ==
// ================================================================================

#include "tb_tab_container.h"
#include <assert.h>

// ATOMIC BEGIN
#include "tb_node_tree.h"
#include "tb_widgets_reader.h"
#include "tb_atomic_widgets.h"
// ATOMIC END

namespace tb {

// == TBTabLayout =======================================================================

void TBTabLayout::OnChildAdded(TBWidget *child)
{
    if (TBButton *button = TBSafeCast<TBButton>(child))
    {
        button->SetSqueezable(true);
        button->SetSkinBg(TBIDC("TBTabContainer.tab"));
        button->SetID(TBIDC("tab"));
    }
}

void TBTabLayout::OnChildRemove(TBWidget *child)
{
    int index = GetIndexFromChild(child);
    if (index < tabContainer_->m_current_page)
        --tabContainer_->m_current_page;
}

PreferredSize TBTabLayout::OnCalculatePreferredContentSize(const SizeConstraints &constraints)
{
    PreferredSize ps = TBLayout::OnCalculatePreferredContentSize(constraints);
    // Make sure the number of tabs doesn't grow parents.
    // It is only the content that should do that. The tabs
    // will scroll anyway.
    if (GetAxis() == AXIS_X)
        ps.min_w = MIN(ps.min_w, 1);
    else
        ps.min_h = MIN(ps.min_h, 1);
    return ps;
}

// == TBTabContainer ====================================================================

TBTabContainer::TBTabContainer()
    : m_need_page_update(true)
    , m_current_page(-1)
    , m_align(TB_ALIGN_TOP)
    , m_tab_layout(this)
{
    AddChild(&m_root_layout);
    // Put the tab layout on top of the content in Z order so their skin can make
    // a seamless overlap over the border. Control which side they are layouted
    // to by calling SetLayoutOrder.
    m_root_layout.AddChild(&m_content_root);
    m_root_layout.AddChild(&m_tab_layout);
    m_root_layout.SetAxis(AXIS_Y);
    m_root_layout.SetGravity(WIDGET_GRAVITY_ALL);
    m_root_layout.SetLayoutDistribution(LAYOUT_DISTRIBUTION_AVAILABLE);
    m_root_layout.SetLayoutOrder(LAYOUT_ORDER_TOP_TO_BOTTOM);
    m_root_layout.SetSkinBg(TBIDC("TBTabContainer.rootlayout"));
    m_root_layout.SetID("root_layout");

    m_tab_layout.SetLayoutDistributionPosition(LAYOUT_DISTRIBUTION_POSITION_CENTER);
    m_tab_layout.SetSkinBg(TBIDC("TBTabContainer.tablayout_x"));
    m_tab_layout.SetLayoutPosition(LAYOUT_POSITION_RIGHT_BOTTOM);
    m_tab_layout.SetID("tab_layout");

    m_content_root.SetGravity(WIDGET_GRAVITY_ALL);
    m_content_root.SetSkinBg(TBIDC("TBTabContainer.container"));
    m_content_root.SetID("content_root");
}

TBTabContainer::~TBTabContainer()
{
    m_root_layout.RemoveChild(&m_content_root);
    m_root_layout.RemoveChild(&m_tab_layout);
    RemoveChild(&m_root_layout);
}

void TBTabContainer::SetAxis(AXIS axis)
{
    m_root_layout.SetAxis(axis);
    m_tab_layout.SetAxis(axis == AXIS_X ? AXIS_Y : AXIS_X);
    m_tab_layout.SetSkinBg(axis == AXIS_X ? TBIDC("TBTabContainer.tablayout_y") :
                                            TBIDC("TBTabContainer.tablayout_x"));
}

void TBTabContainer::SetValue(int index)
{
    if (index == m_current_page || !GetNumPages())
        return;

    m_current_page = index;

    // Update the pages visibility and tabs pressed value.
    index = 0;
    TBWidget *page = m_content_root.GetFirstChild();
    TBWidget *tab = m_tab_layout.GetFirstChild();
    for (   ; page && tab; page = page->GetNext(), tab = tab->GetNext(), index++)
    {
        bool active = index == m_current_page;
        page->SetVisibilility(active ? WIDGET_VISIBILITY_VISIBLE : WIDGET_VISIBILITY_INVISIBLE);
        tab->SetValue(active ? 1 : 0);

        if (active)
        {
            if (page->GetVisibility() == WIDGET_VISIBILITY_GONE)
                continue;

            TBRect contentRect = m_content_root.GetRect();            
            TBSkinElement* skin = m_content_root.GetSkinBgElement();
            contentRect = contentRect.Shrink(skin->padding_left, skin->padding_top, skin->padding_right, skin->padding_bottom);

            TBRect pageRect = page->GetRect();

            if (contentRect.w != pageRect.w || contentRect.h != pageRect.h)
            {
                contentRect.x = skin->padding_left;
                contentRect.y = skin->padding_right;
                page->SetRect(contentRect);
            }
        }
    }

    TBWidgetEvent ev(EVENT_TYPE_TAB_CHANGED);
    InvokeEvent(ev);
}

int TBTabContainer::GetNumPages()
{
    int count = 0;
    for (TBWidget *tab = m_tab_layout.GetFirstChild(); tab; tab = tab->GetNext())
        count++;

    if (!count)
        m_current_page = -1;

    return count;
}

TBWidget *TBTabContainer::GetCurrentPageWidget() const
{
    if (m_current_page == -1)
        return nullptr;

    return m_content_root.GetChildFromIndex(m_current_page);
}

void TBTabContainer::SetAlignment(TB_ALIGN align)
{
    bool horizontal = (align == TB_ALIGN_TOP || align == TB_ALIGN_BOTTOM);
    bool reverse = (align == TB_ALIGN_TOP || align == TB_ALIGN_LEFT);
    SetAxis(horizontal ? AXIS_Y : AXIS_X);
    m_root_layout.SetLayoutOrder(reverse ? LAYOUT_ORDER_TOP_TO_BOTTOM : LAYOUT_ORDER_BOTTOM_TO_TOP);
    m_tab_layout.SetLayoutPosition(reverse ? LAYOUT_POSITION_RIGHT_BOTTOM : LAYOUT_POSITION_LEFT_TOP);
    m_align = align;
}

bool TBTabContainer::OnEvent(const TBWidgetEvent &ev)
{
    if ((ev.type == EVENT_TYPE_CLICK || ev.type == EVENT_TYPE_POINTER_DOWN) &&
            ev.target->GetID() == TBIDC("tab") &&
            ev.target->GetParent() == &m_tab_layout)
    {
        int clicked_index = m_tab_layout.GetIndexFromChild(ev.target);
        SetValue(clicked_index);
        return true;
    }
    return false;
}

void TBTabContainer::OnProcess()
{
    if (m_need_page_update)
    {
        m_need_page_update = false;
        // Force update value
        int current_page = m_current_page;
        m_current_page = -1;
        SetValue(current_page);
    }
}

// ATOMIC BEGIN

/// takes the contents of a DockWindow into a tab in a tabcontainer 
bool TBTabContainer::DockFromWindow ( TBStr windowTitle )
{
    int nn = 0;
    TBLinkListOf<TBWidget>::Iterator mx = GetParentRoot(true)->GetIteratorForward();
    while (TBWidget *mxw = mx.GetAndStep())
    {
        if ( mxw->GetText().Equals(windowTitle) )
        {
            TBDockWindow *dw1 = TBSafeCast<TBDockWindow>( mxw );
            if (dw1) 
                dw1->Dock ( this );
            return true;
        }
        nn++;
    }
    return false;
}

/// undocks the page into a window with the tab name, and removes the tab
void TBTabContainer::UndockPage ( int page )
{
    TBWidget *mytab = GetTabLayout()->GetChildFromIndex(page);  // find the offending tab 
    TBWidget *mypage = GetContentRoot()->GetChildFromIndex(page);   // find the offending page (layout)

    if (mytab == NULL || mypage == NULL ) return; // nobody home

    TBStr tabstr = mytab->GetText(); //get the name/text of the tab
    GetTabLayout()->RemoveChild(mytab); // remove, delete the tab[page] button
    mytab->Die();  // and get rid of it
    GetContentRoot()->RemoveChild(mypage);  // remove the pagewidget[page] from the content root (without deleting it, hopefully)
    TBDockWindow *mywindow = new TBDockWindow(tabstr, mypage); // create an undock window.
    mywindow->SetDockOrigin( GetID() );  //tell it to redock here
    mywindow->Show(GetParentRoot(true)); 

    Invalidate(); // and tell everyone the party is over.

}

// ATOMIC END

}; // namespace tb
