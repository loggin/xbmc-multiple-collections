/*
 *  Copyright (C) 2005-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DirectoryNodeCollectionItems.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "QueryParams.h"
#include "utils/StringUtils.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"

using namespace XFILE::VIDEODATABASEDIRECTORY;

CDirectoryNodeCollectionItems::CDirectoryNodeCollectionItems(const std::string& strName,
                                                             CDirectoryNode* pParent)
  : CDirectoryNode(NodeType::COLLECTION_ITEMS, strName, pParent)
{
}

NodeType CDirectoryNodeCollectionItems::GetChildType() const
{
  return NodeType::NONE;
}

// The base GetChilds() creates a child node with GetChildType() and calls GetContent on that.
// Since our child type is NONE (we are the leaf), we call GetContent on ourselves directly.
bool CDirectoryNodeCollectionItems::GetChilds(CFileItemList& items)
{
  return GetContent(items);
}

bool CDirectoryNodeCollectionItems::GetContent(CFileItemList& items) const
{
  CQueryParams params;
  CollectQueryParams(params);
  const int idCollection = static_cast<int>(params.GetCollectionId());
  if (idCollection <= 0)
    return false;

  CVideoDatabase db;
  if (!db.Open())
    return false;

  CLog::LogF(LOGDEBUG, "Fetching items for collection {}", idCollection);
  std::vector<CVideoDatabase::CCollectionItem> collectionItems;
  if (!db.GetCollectionItems(idCollection, collectionItems))
    return false;
  CLog::LogF(LOGDEBUG, "Collection {} has {} items", idCollection, collectionItems.size());

  std::string collectionName;
  {
    std::vector<CVideoDatabase::CCollection> cols;
    if (db.GetCollections(cols, "", db.PrepareSQL("idCollection=%i", idCollection)) &&
        !cols.empty())
      collectionName = cols.front().name;
  }
  if (collectionName.empty())
    collectionName = db.GetSetById(idCollection);

  std::string previousGroup;
  for (const auto& ci : collectionItems)
  {
    std::string mediaType = ci.mediaType;
    StringUtils::ToLower(mediaType);

    if (!ci.groupName.empty() && ci.groupName != previousGroup)
    {
      auto groupHeader = std::make_shared<CFileItem>();
      groupHeader->SetLabel(ci.groupName);
      groupHeader->SetPath(StringUtils::Format("collection://{}/group/{}", idCollection, items.Size()));
      groupHeader->SetProperty("collection.id", idCollection);
      groupHeader->SetProperty("collection.groupname", ci.groupName);
      groupHeader->SetProperty("collection.isgroupheader", true);
      groupHeader->SetProperty("collection.mediatype", "group");
      groupHeader->SetProperty("mediatype", "group");
      items.Add(groupHeader);
      previousGroup = ci.groupName;
    }
    else if (ci.groupName.empty())
    {
      previousGroup.clear();
    }

    CFileItemPtr item;
    if (mediaType == "movie")
    {
      CVideoInfoTag details;
      if (db.GetMovieInfo("", details, ci.idMedia, -1, -1, VideoDbDetailsAll) && details.m_iDbId > 0)
      {
        item = std::make_shared<CFileItem>(details);
        item->SetPath(StringUtils::Format("videodb://movies/titles/{}/", details.m_iDbId));
      }
    }
    else if (mediaType == "tvshow")
    {
      CVideoInfoTag details;
      CFileItem dbItem;
      if (db.GetTvShowInfo("", details, ci.idMedia, &dbItem, VideoDbDetailsAll) && details.m_iDbId > 0)
      {
        item = std::make_shared<CFileItem>(
            StringUtils::Format("videodb://tvshows/titles/{}/", details.m_iDbId), true);
        item->SetFromVideoInfoTag(details);
      }
    }
    else if (mediaType == "season")
    {
      CVideoInfoTag details;
      if (db.GetSeasonInfo(ci.idMedia, details) && details.m_iDbId > 0 && details.m_iIdShow > 0)
      {
        item = std::make_shared<CFileItem>(
            StringUtils::Format("videodb://tvshows/titles/{}/{}/", details.m_iIdShow,
                                details.m_iSeason),
            true);
        item->SetFromVideoInfoTag(details);
      }
    }
    else if (mediaType == "episode" || mediaType == "special")
    {
      CVideoInfoTag details;
      if (db.GetEpisodeInfo("", details, ci.idMedia, VideoDbDetailsAll) && details.m_iDbId > 0 &&
          details.m_iIdShow > 0)
      {
        item = std::make_shared<CFileItem>(details);
        item->SetPath(StringUtils::Format("videodb://tvshows/titles/{}/{}/{}", details.m_iIdShow,
                                          details.m_iSeason, details.m_iDbId));
      }
    }

    if (!item)
    {
      CLog::LogF(LOGDEBUG, "DirectoryNodeCollectionItems: no item for mediaType='{}' idMedia={}",
                 mediaType, ci.idMedia);
      continue;
    }

    item->SetProperty("collection.id", idCollection);
    item->SetProperty("collection.mediatype", mediaType);
    item->SetProperty("collection.sortorder", ci.sortOrder);
    item->SetProperty("collection.isspecial", mediaType == "special");
    item->SetProperty("mediatype", mediaType);
    if (!collectionName.empty())
      item->SetProperty("collection.name", collectionName);
    if (!ci.groupName.empty())
      item->SetProperty("collection.groupname", ci.groupName);
    item->SetLabelPreformatted(true);
    items.Add(item);
  }

  items.SetProperty("collection.name", collectionName);
  return true;
}
