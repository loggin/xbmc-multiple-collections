/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogCollectionTimeline.h"

#include "FileItemList.h"
#include "ServiceBroker.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/WindowIDs.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"

CGUIDialogCollectionTimeline::CGUIDialogCollectionTimeline()
  : CGUIDialog(WINDOW_DIALOG_COLLECTION_TIMELINE, "DialogCollectionTimeline.xml")
{
}

void CGUIDialogCollectionTimeline::SetCollection(int idCollection, const CFileItemList& items)
{
  m_idCollection = idCollection;
  m_orderedItems.clear();
  m_confirmed = false;
  for (int i = 0; i < items.Size(); ++i)
    m_orderedItems.push_back(items.Get(i));
}

void CGUIDialogCollectionTimeline::OnInitWindow()
{
  CGUIDialog::OnInitWindow();
  SET_CONTROL_LABEL(2, CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(40805));
  RefreshList(0);
  SET_CONTROL_FOCUS(CONTROL_LIST, 0);
}

bool CGUIDialogCollectionTimeline::OnMessage(CGUIMessage& message)
{
  if (message.GetMessage() == GUI_MSG_CLICKED)
  {
    const int ctrl = message.GetSenderId();
    if (ctrl == CONTROL_MOVE_UP)
    {
      MoveItem(true);
      return true;
    }
    if (ctrl == CONTROL_MOVE_DOWN)
    {
      MoveItem(false);
      return true;
    }
    if (ctrl == CONTROL_DONE)
    {
      SaveOrder();
      m_confirmed = true;
      Close();
      return true;
    }
  }
  return CGUIDialog::OnMessage(message);
}

bool CGUIDialogCollectionTimeline::OnAction(const CAction& action)
{
  // Back/escape cancels without saving
  if (action.GetID() == ACTION_PREVIOUS_MENU || action.GetID() == ACTION_NAV_BACK)
  {
    m_confirmed = false;
    Close();
    return true;
  }
  return CGUIDialog::OnAction(action);
}

void CGUIDialogCollectionTimeline::RefreshList(int keepFocus)
{
  CFileItemList list;
  for (const auto& item : m_orderedItems)
    list.Add(item);

  CGUIMessage msg(GUI_MSG_LABEL_BIND, GetID(), CONTROL_LIST, 0, 0, &list);
  OnMessage(msg);

  if (keepFocus >= 0 && keepFocus < static_cast<int>(m_orderedItems.size()))
  {
    CGUIMessage msgSel(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_LIST, keepFocus);
    OnMessage(msgSel);
  }
}

int CGUIDialogCollectionTimeline::GetFocusedItem() const
{
  CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_LIST);
  const_cast<CGUIDialogCollectionTimeline*>(this)->OnMessage(msg);
  return static_cast<int>(msg.GetParam1());
}

void CGUIDialogCollectionTimeline::MoveItem(bool up)
{
  const int idx = GetFocusedItem();
  const int swapWith = up ? idx - 1 : idx + 1;

  if (swapWith < 0 || swapWith >= static_cast<int>(m_orderedItems.size()))
    return;

  std::swap(m_orderedItems[idx], m_orderedItems[swapWith]);
  RefreshList(swapWith);
}

void CGUIDialogCollectionTimeline::SaveOrder()
{
  if (m_idCollection < 0)
    return;

  CVideoDatabase db;
  if (!db.Open())
    return;

  for (int i = 0; i < static_cast<int>(m_orderedItems.size()); ++i)
  {
    const auto& item = m_orderedItems[i];
    if (!item || !item->HasVideoInfoTag())
      continue;

    const int idMedia = item->GetVideoInfoTag()->m_iDbId;
    const std::string mediaType = item->GetProperty("collection.mediatype").asString();
    const std::string groupName = item->GetProperty("collection.groupname").asString();

    if (idMedia <= 0 || mediaType.empty())
    {
      CLog::LogF(LOGWARNING, "Skipping item {} with missing mediaType or idMedia", i);
      continue;
    }

    CVideoDatabase::CCollectionItem ci;
    ci.idCollection = m_idCollection;
    ci.mediaType = mediaType;
    ci.idMedia = idMedia;
    ci.sortOrder = i;
    ci.groupName = groupName;
    db.AddOrUpdateCollectionItem(ci);
  }
}
