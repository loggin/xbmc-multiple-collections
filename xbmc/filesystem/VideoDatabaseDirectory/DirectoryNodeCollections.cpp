/*
 *  Copyright (C) 2005-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DirectoryNodeCollections.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "utils/StringUtils.h"
#include "video/VideoDatabase.h"
#include "video/VideoDbUrl.h"

using namespace XFILE::VIDEODATABASEDIRECTORY;

CDirectoryNodeCollections::CDirectoryNodeCollections(const std::string& strName,
                                                     CDirectoryNode* pParent)
  : CDirectoryNode(NodeType::COLLECTIONS, strName, pParent)
{
}

NodeType CDirectoryNodeCollections::GetChildType() const
{
  return NodeType::COLLECTION_ITEMS;
}

std::string CDirectoryNodeCollections::GetLocalizedName() const
{
  return CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(40803);
}

bool CDirectoryNodeCollections::GetContent(CFileItemList& items) const
{
  CVideoDbUrl videoUrl;
  if (!videoUrl.FromString(BuildPath()))
    return false;

  CVideoDatabase db;
  if (!db.Open())
    return false;

  std::vector<CVideoDatabase::CCollection> collections;
  if (!db.GetCollections(collections))
    return false;

  for (const auto& col : collections)
  {
    CVideoDbUrl itemUrl = videoUrl;
    itemUrl.AppendPath(StringUtils::Format("{}/", col.idCollection));

    CFileItemPtr item = std::make_shared<CFileItem>(itemUrl.ToString(), true);
    item->SetLabel(col.name);
    item->SetCanQueue(false);
    item->GetVideoInfoTag()->m_iDbId = col.idCollection;
    item->GetVideoInfoTag()->m_type = "videocollection";
    item->SetProperty("collection.id", col.idCollection);
    item->SetProperty("collection.name", col.name);
    item->SetProperty("collection.type", col.type);
    items.Add(item);
  }

  return true;
}
