/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DirectoryNodeOverview.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "video/VideoDatabase.h"

#include <utility>

using namespace XFILE::VIDEODATABASEDIRECTORY;

Node OverviewChildren[] = {
    {NodeType::MOVIES_OVERVIEW, "movies", 342},
    {NodeType::TVSHOWS_OVERVIEW, "tvshows", 20343},
    {NodeType::MUSICVIDEOS_OVERVIEW, "musicvideos", 20389},
    {NodeType::RECENTLY_ADDED_MOVIES, "recentlyaddedmovies", 20386},
    {NodeType::RECENTLY_ADDED_EPISODES, "recentlyaddedepisodes", 20387},
    {NodeType::RECENTLY_ADDED_MUSICVIDEOS, "recentlyaddedmusicvideos", 20390},
    {NodeType::INPROGRESS_TVSHOWS, "inprogresstvshows", 626},
    {NodeType::COLLECTION_ITEMS, "collections", 40803},
};

CDirectoryNodeOverview::CDirectoryNodeOverview(const std::string& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NodeType::OVERVIEW, strName, pParent)
{

}

NodeType CDirectoryNodeOverview::GetChildType() const
{
  for (const Node& node : OverviewChildren)
    if (GetName() == node.id)
      return node.node;

  return NodeType::NONE;
}

std::string CDirectoryNodeOverview::GetLocalizedName() const
{
  for (const Node& node : OverviewChildren)
    if (GetName() == node.id)
      return CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(node.label);
  return "";
}

bool CDirectoryNodeOverview::GetContent(CFileItemList& items) const
{
  // For the collections overview, list all collections directly so that
  // pressing Back from a collection item list lands on the collections list.
  if (GetName() == "collections")
  {
    CVideoDatabase db;
    if (!db.Open())
      return false;
    std::vector<CVideoDatabase::CCollection> collections;
    if (!db.GetCollections(collections))
      return false;
    const std::string basePath = "videodb://collections/";
    for (const auto& col : collections)
    {
      CFileItemPtr item = std::make_shared<CFileItem>(
          basePath + StringUtils::Format("{}/", col.idCollection), true);
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

  CVideoDatabase database;
  database.Open();
  bool hasMovies = database.HasContent(VideoDbContentType::MOVIES);
  bool hasTvShows = database.HasContent(VideoDbContentType::TVSHOWS);
  bool hasMusicVideos = database.HasContent(VideoDbContentType::MUSICVIDEOS);
  bool hasCollections = database.HasCollections();
  std::vector<std::pair<const char*, int> > vec;
  if (hasMovies)
  {
    if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_MYVIDEOS_FLATTEN))
      vec.emplace_back("movies/titles", 342);
    else
      vec.emplace_back("movies", 342); // Movies
  }
  if (hasTvShows)
  {
    if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_MYVIDEOS_FLATTEN))
      vec.emplace_back("tvshows/titles", 20343);
    else
      vec.emplace_back("tvshows", 20343); // TV Shows
  }
  if (hasMusicVideos)
  {
    if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_MYVIDEOS_FLATTEN))
      vec.emplace_back("musicvideos/titles", 20389);
    else
      vec.emplace_back("musicvideos", 20389); // Music Videos
  }
  {
    if (hasMovies)
      vec.emplace_back("recentlyaddedmovies", 20386); // Recently Added Movies
    if (hasTvShows)
    {
      vec.emplace_back("recentlyaddedepisodes", 20387); // Recently Added Episodes
      vec.emplace_back("inprogresstvshows", 626); // InProgress TvShows
    }
    if (hasMusicVideos)
      vec.emplace_back("recentlyaddedmusicvideos", 20390); // Recently Added Music Videos
  }
  if (hasCollections)
    vec.emplace_back("collections", 40803); // Collections
  std::string path = BuildPath();
  for (unsigned int i = 0; i < vec.size(); ++i)
  {
    CFileItemPtr pItem(new CFileItem(path + vec[i].first + "/", true));
    pItem->SetLabel(
        CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(vec[i].second));
    pItem->SetLabelPreformatted(true);
    pItem->SetCanQueue(false);
    items.Add(pItem);
  }

  return true;
}
