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
#include "media/MediaType.h"
#include "utils/StringUtils.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"
#include "video/VideoManagerTypes.h"

#include <set>

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
  std::string collectionDescription;
  std::string collectionArtwork;
  {
    std::vector<CVideoDatabase::CCollection> cols;
    if (db.GetCollections(cols, "", db.PrepareSQL("idCollection=%i", idCollection)) &&
        !cols.empty())
    {
      collectionName = cols.front().name;
      collectionDescription = cols.front().description;
      collectionArtwork = cols.front().artwork;
    }
  }
  if (collectionName.empty())
    collectionName = db.GetSetById(idCollection);

  if (collectionArtwork.empty())
  {
    collectionArtwork = db.GetArtForItem(idCollection, MediaTypeVideoCollection, "poster");
    if (collectionArtwork.empty())
      collectionArtwork = db.GetArtForItem(idCollection, MediaTypeVideoCollection, "thumb");
    if (collectionArtwork.empty())
      collectionArtwork = db.GetArtForItem(idCollection, MediaTypeVideoCollection, "icon");
  }

  std::string previousGroup;
  std::set<std::string> distinctMediaTypes;
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
      if (!collectionName.empty())
      {
        groupHeader->SetProperty("collection.name", collectionName);
        groupHeader->SetLabel(collectionName);
      }
      if (!collectionDescription.empty())
        groupHeader->SetProperty("plot", collectionDescription);
      if (!collectionArtwork.empty())
      {
        groupHeader->SetArt("poster", collectionArtwork);
        groupHeader->SetArt("thumb", collectionArtwork);
        groupHeader->SetArt("icon", collectionArtwork);
        groupHeader->SetProperty("collection.artwork", collectionArtwork);
      }
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
        if (details.HasVideoVersions() || details.HasVideoExtras())
        {
          item->SetPath(StringUtils::Format(
              "videodb://movies/titles/{}/{}/", details.m_iDbId,
              static_cast<int>(VideoAssetType::VERSIONSANDEXTRASFOLDER)));
          item->SetFolder(true);
          item->SetProperty("IsHybridFolder", true);
        }
        else
        {
          item->SetPath(StringUtils::Format("videodb://movies/titles/{}/", details.m_iDbId));
        }
      }
    }
    else if (mediaType == "tvshow")
    {
      CVideoInfoTag details;
      CFileItem dbItem;
      if (db.GetTvShowInfo("", details, ci.idMedia, &dbItem, VideoDbDetailsAll) && details.m_iDbId > 0)
      {
        // Use the same pattern as movies/episodes: create from tag, then override
        // the path to a videodb:// URL so that VIDEO::IsVideoDb() returns true
        // (SetFromVideoInfoTag would otherwise set the filesystem path).
        item = std::make_shared<CFileItem>(details);
        item->SetPath(StringUtils::Format("videodb://tvshows/titles/{}/", details.m_iDbId));
        item->SetFolder(true);
      }
    }
    else if (mediaType == "season")
    {
      CVideoInfoTag details;
      if (db.GetSeasonInfo(ci.idMedia, details) && details.m_iDbId > 0 && details.m_iIdShow > 0)
      {
        item = std::make_shared<CFileItem>(details);
        item->SetPath(StringUtils::Format("videodb://tvshows/titles/{}/{}/",
                                          details.m_iIdShow, details.m_iSeason));
        item->SetFolder(true);
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
    if (!collectionDescription.empty())
      item->SetProperty("collection.description", collectionDescription);
    if (!collectionArtwork.empty())
      item->SetProperty("collection.artwork", collectionArtwork);
    if (!ci.groupName.empty())
      item->SetProperty("collection.groupname", ci.groupName);
    item->SetLabelPreformatted(true);
    items.Add(item);
    distinctMediaTypes.insert(mediaType);
  }

  // Most collections contain only one media type (e.g. all movies). Use the real Kodi
  // content string in that case so the skin's normal, well-tested per-type item layouts
  // apply across every view mode — only fall back to "mixed" when the collection genuinely
  // blends media types, since skins generally only provide a single generic layout for that.
  std::string content = "mixed";
  if (distinctMediaTypes.size() == 1)
  {
    const std::string& onlyType = *distinctMediaTypes.begin();
    if (onlyType == "movie")
      content = "movies";
    else if (onlyType == "tvshow")
      content = "tvshows";
    else if (onlyType == "season")
      content = "seasons";
    else if (onlyType == "episode" || onlyType == "special")
      content = "episodes";
  }
  items.SetContent(content);
  items.SetProperty("collection.name", collectionName);
  items.SetProperty("collection.description", collectionDescription);
  items.SetProperty("collection.artwork", collectionArtwork);
  return true;
}
