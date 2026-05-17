/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIWindowVideoCollection.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "guilib/WindowIDs.h"
#include "media/MediaType.h"
#include "video/VideoFileItemClassify.h"
#include "video/VideoLibraryQueue.h"
#include "video/dialogs/GUIDialogVideoInfo.h"

using namespace KODI;

CGUIWindowVideoCollection::CGUIWindowVideoCollection()
  : CGUIWindowVideoBase(WINDOW_VIDEO_COLLECTION, "MyVideoCollection.xml")
{
  m_thumbLoader.SetObserver(this);
}

CGUIWindowVideoCollection::~CGUIWindowVideoCollection() = default;

bool CGUIWindowVideoCollection::GetDirectory(const std::string& strDirectory, CFileItemList& items)
{
  if (!CGUIWindowVideoBase::GetDirectory(strDirectory, items))
    return false;

  // Propagate the collection name (set by DirectoryNodeCollectionItems) to the window property
  // so the skin can display it in the header.
  const std::string collectionName = items.GetProperty("collection.name").asString();
  SetProperty("CollectionName", collectionName);

  return true;
}

bool CGUIWindowVideoCollection::OnSelect(int iItem)
{
  if (iItem < 0 || iItem >= m_vecItems->Size())
    return false;

  const CFileItemPtr item = m_vecItems->Get(iItem);
  if (item && item->GetProperty("collection.isgroupheader").asBoolean())
    return true;

  return CGUIWindowVideoBase::OnSelect(iItem);
}

void CGUIWindowVideoCollection::GetContextButtons(int itemNumber, CContextButtons& buttons)
{
  CGUIWindowVideoBase::GetContextButtons(itemNumber, buttons);

  if (itemNumber < 0 || itemNumber >= m_vecItems->Size())
    return;

  const CFileItemPtr item = m_vecItems->Get(itemNumber);
  if (!item || item->GetProperty("collection.isgroupheader").asBoolean())
    return;

  if (!CVideoLibraryQueue::GetInstance().IsScanningLibrary() &&
      VIDEO::IsVideoDb(*item) && item->HasVideoInfoTag())
  {
    const std::string& type = item->GetVideoInfoTag()->m_type;
    if (type == MediaTypeMovie || type == MediaTypeTvShow || type == MediaTypeSeason ||
        type == MediaTypeEpisode || type == MediaTypeMusicVideo)
    {
      buttons.Add(CONTEXT_BUTTON_EDIT, 16106); // "Manage..."
    }
  }
}

bool CGUIWindowVideoCollection::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  if (button == CONTEXT_BUTTON_EDIT)
  {
    if (itemNumber >= 0 && itemNumber < m_vecItems->Size())
    {
      const CFileItemPtr item = m_vecItems->Get(itemNumber);
      const CONTEXT_BUTTON ret{
          static_cast<CONTEXT_BUTTON>(CGUIDialogVideoInfo::ManageVideoItem(item))};
      if (ret != CONTEXT_BUTTON_CANCELLED)
      {
        Refresh(true);
        if (ret == CONTEXT_BUTTON_DELETE)
        {
          const int select = itemNumber >= m_vecItems->Size() - 1 ? itemNumber - 1 : itemNumber;
          m_viewControl.SetSelectedItem(select);
        }
      }
      return true;
    }
  }

  return CGUIWindowVideoBase::OnContextButton(itemNumber, button);
}
