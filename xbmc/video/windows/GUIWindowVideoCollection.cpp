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
#include "filesystem/VideoDatabaseFile.h"
#include "guilib/WindowIDs.h"
#include "media/MediaType.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "video/VideoDatabase.h"
#include "video/VideoFileItemClassify.h"
#include "video/VideoLibraryQueue.h"
#include "video/dialogs/GUIDialogVideoInfo.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <cstdlib>

using namespace KODI;

namespace
{
std::string NormalizeCollectionMediaType(const std::string& mediaType)
{
  std::string normalizedType = mediaType;
  StringUtils::ToLower(normalizedType);
  if (normalizedType == "special")
    return MediaTypeEpisode;

  return normalizedType;
}

std::string BuildCollectionItemLabel(const std::string& mediaType, const CVideoInfoTag* tag)
{
  if (tag == nullptr)
    return "";

  const auto pickNonNumeric = [](std::string candidate) -> std::string {
    StringUtils::Trim(candidate);
    if (candidate.empty())
      return "";

    const bool isNumeric = std::all_of(candidate.begin(), candidate.end(),
                                       [](unsigned char c) { return std::isdigit(c) != 0; });
    if (isNumeric)
      return "";
    return candidate;
  };

  if (const std::string title = pickNonNumeric(tag->m_strTitle); !title.empty())
    return title;

  if (const std::string originalTitle = pickNonNumeric(tag->m_strOriginalTitle);
      !originalTitle.empty())
  {
    return originalTitle;
  }

  if (const std::string showTitle = pickNonNumeric(tag->m_strShowTitle); !showTitle.empty())
    return showTitle;

  if (const std::string sortTitle = pickNonNumeric(tag->m_strSortTitle); !sortTitle.empty())
    return sortTitle;

  if (mediaType == "season")
  {
    if (tag->m_iSeason == 0)
      return "Specials";

    if (tag->m_iSeason > 0)
      return StringUtils::Format("Season {}", tag->m_iSeason);
  }

  if ((mediaType == "episode" || mediaType == "special") && tag->m_iEpisode > 0)
  {
    return StringUtils::Format("Episode {}", tag->m_iEpisode);
  }

  return "";
}

bool IsNumericLabel(const std::string& label)
{
  if (label.empty())
    return false;

  return std::all_of(label.begin(), label.end(),
                     [](unsigned char c) { return std::isdigit(c) != 0; });
}

std::string BuildFinalFallbackLabel(const std::string& mediaType)
{
  if (mediaType == "season")
    return "Season";

  if (mediaType == "episode" || mediaType == "special")
    return "Episode";

  if (mediaType == "tvshow")
    return "TV Show";

  if (mediaType == "movie")
    return "Movie";

  return "Collection Item";
}
} // namespace

CGUIWindowVideoCollection::CGUIWindowVideoCollection()
  : CGUIWindowVideoBase(WINDOW_VIDEO_COLLECTION, "MyVideoCollection.xml")
{
  m_thumbLoader.SetObserver(this);
}

CGUIWindowVideoCollection::~CGUIWindowVideoCollection() = default;

int CGUIWindowVideoCollection::GetCollectionIdFromPath(const std::string& path)
{
  static constexpr const char* prefix = "videodb://collections/";

  if (!StringUtils::StartsWithNoCase(path, prefix))
    return -1;

  std::string idPart = path.substr(strlen(prefix));
  const std::size_t slashPos = idPart.find('/');
  if (slashPos != std::string::npos)
    idPart = idPart.substr(0, slashPos);

  if (idPart.empty())
    return -1;

  if (!std::all_of(idPart.begin(), idPart.end(), [](unsigned char c) { return std::isdigit(c) != 0; }))
    return -1;

  return std::atoi(idPart.c_str());
}

bool CGUIWindowVideoCollection::GetDirectory(const std::string& strDirectory, CFileItemList& items)
{
  const int idCollection = GetCollectionIdFromPath(strDirectory);
  if (idCollection <= 0)
    return CGUIWindowVideoBase::GetDirectory(strDirectory, items);

  CVideoDatabase videodb;
  if (!videodb.Open())
    return false;

  std::vector<CVideoDatabase::CCollection> collections;
  std::string collectionName;
  if (videodb.GetCollections(collections, "", 
                             videodb.PrepareSQL("idCollection=%i", idCollection)) &&
      !collections.empty())
  {
    collectionName = collections.front().name;
  }
  if (collectionName.empty())
    collectionName = videodb.GetSetById(idCollection);

  if (!collectionName.empty())
  {
    SetProperty("CollectionName", collectionName);
    items.SetProperty("collection.name", collectionName);
  }
  else
  {
    SetProperty("CollectionName", "");
  }

  std::vector<CVideoDatabase::CCollectionItem> collectionItems;
  if (!videodb.GetCollectionItems(idCollection, collectionItems))
  {
    CLog::LogF(LOGERROR, "GUIWindowVideoCollection: GetCollectionItems failed for id={}", idCollection);
    return false;
  }

  CLog::LogF(LOGDEBUG, "GUIWindowVideoCollection: collection={} '{}' returned {} items",
             idCollection, collectionName, collectionItems.size());

  items.Clear();
  items.SetPath(strDirectory);
  std::string previousGroup;
  for (const auto& collectionItem : collectionItems)
  {
    std::string mediaType = collectionItem.mediaType;
    StringUtils::ToLower(mediaType);

    if (!collectionItem.groupName.empty() && collectionItem.groupName != previousGroup)
    {
      auto groupHeader = std::make_shared<CFileItem>();
      groupHeader->SetLabel(collectionItem.groupName);
      groupHeader->SetLabel2("");
      groupHeader->SetPath(StringUtils::Format("collection://{}/group/{}", idCollection, items.Size()));
      groupHeader->SetProperty("collection.id", idCollection);
      groupHeader->SetProperty("collection.groupname", collectionItem.groupName);
      groupHeader->SetProperty("collection.isgroupheader", true);
      groupHeader->SetProperty("collection.mediatype", "group");
      groupHeader->SetProperty("mediatype", "group");
      items.Add(groupHeader);
      previousGroup = collectionItem.groupName;
    }
    else if (collectionItem.groupName.empty())
    {
      previousGroup.clear();
    }

    CFileItemPtr item;
    if (mediaType == "movie")
    {
      CVideoInfoTag details;
      const bool ok = videodb.GetMovieInfo("", details, collectionItem.idMedia, -1, -1, VideoDbDetailsAll);
      CLog::LogF(LOGDEBUG, "GUIWindowVideoCollection: movie idMedia={} ok={} dbId={} title='{}'",
                 collectionItem.idMedia, ok, details.m_iDbId, details.m_strTitle);
      if (ok && details.m_iDbId > 0)
      {
        item = std::make_shared<CFileItem>(details);
        item->SetPath(StringUtils::Format("videodb://movies/titles/{}/", details.m_iDbId));
      }
    }
    else if (mediaType == "tvshow")
    {
      CVideoInfoTag details;
      CFileItem dbItem;
      const bool ok = videodb.GetTvShowInfo("", details, collectionItem.idMedia, &dbItem, VideoDbDetailsAll);
      CLog::LogF(LOGDEBUG, "GUIWindowVideoCollection: tvshow idMedia={} ok={} dbId={} title='{}'",
                 collectionItem.idMedia, ok, details.m_iDbId, details.m_strTitle);
      if (ok && details.m_iDbId > 0)
      {
        item = std::make_shared<CFileItem>(StringUtils::Format("videodb://tvshows/titles/{}/", details.m_iDbId), true);
        item->SetFromVideoInfoTag(details);
      }
    }
    else if (mediaType == "season")
    {
      CVideoInfoTag details;
      const bool ok = videodb.GetSeasonInfo(collectionItem.idMedia, details);
      CLog::LogF(LOGDEBUG, "GUIWindowVideoCollection: season idMedia={} ok={} dbId={} title='{}' season={}",
                 collectionItem.idMedia, ok, details.m_iDbId, details.m_strTitle, details.m_iSeason);
      if (ok && details.m_iDbId > 0 && details.m_iIdShow > 0)
      {
        item = std::make_shared<CFileItem>(
            StringUtils::Format("videodb://tvshows/titles/{}/{}/", details.m_iIdShow, details.m_iSeason),
            true);
        item->SetFromVideoInfoTag(details);
      }
    }
    else if (mediaType == "episode" || mediaType == "special")
    {
      CVideoInfoTag details;
      const bool ok = videodb.GetEpisodeInfo("", details, collectionItem.idMedia, VideoDbDetailsAll);
      CLog::LogF(LOGDEBUG, "GUIWindowVideoCollection: episode idMedia={} ok={} dbId={} title='{}'",
                 collectionItem.idMedia, ok, details.m_iDbId, details.m_strTitle);
      if (ok && details.m_iDbId > 0 && details.m_iIdShow > 0)
      {
        item = std::make_shared<CFileItem>(details);
        item->SetPath(StringUtils::Format("videodb://tvshows/titles/{}/{}/{}", details.m_iIdShow,
                                          details.m_iSeason, details.m_iDbId));
      }
    }

    if (!item)
    {
      CLog::LogF(LOGDEBUG, "GUIWindowVideoCollection: item is null for mediaType='{}' idMedia={} - skipping",
                 mediaType, collectionItem.idMedia);
      continue;
    }

    std::string resolvedLabel = item->GetLabel();

    if (item->HasVideoInfoTag())
    {
      const CVideoInfoTag* tag = item->GetVideoInfoTag();
      const std::string label = BuildCollectionItemLabel(mediaType, tag);
      if (!label.empty())
      {
        resolvedLabel = label;
        item->SetLabel(resolvedLabel);
        item->SetTitle(resolvedLabel);
      }
    }

    if (resolvedLabel.empty() || IsNumericLabel(resolvedLabel))
    {
      resolvedLabel = BuildFinalFallbackLabel(mediaType);
      item->SetLabel(resolvedLabel);
      item->SetTitle(resolvedLabel);
    }

    // Align list title with the same DB-path tag source used by info dialogs.
    const CVideoInfoTag dbPathTag = XFILE::CVideoDatabaseFile::GetVideoTag(item->GetURL());

    if (!dbPathTag.m_strTitle.empty() && !IsNumericLabel(dbPathTag.m_strTitle))
    {
      resolvedLabel = dbPathTag.m_strTitle;
      item->SetLabel(resolvedLabel);
      item->SetTitle(resolvedLabel);
    }

    if (item->HasVideoInfoTag())
    {
      CVideoInfoTag* mutableTag = item->GetVideoInfoTag();
      if (mutableTag && !resolvedLabel.empty())
      {
        mutableTag->m_strTitle = resolvedLabel;
        if (mutableTag->m_strSortTitle.empty())
          mutableTag->m_strSortTitle = resolvedLabel;
      }
    }

    const std::string normalizedMediaType = NormalizeCollectionMediaType(mediaType);

    item->SetLabel2(CMediaTypes::GetCapitalLocalization(normalizedMediaType));
    item->SetProperty("collection.id", idCollection);
    item->SetProperty("collection.mediatype", mediaType);
    item->SetProperty("collection.mediatype.normalized", normalizedMediaType);
    item->SetProperty("collection.sortorder", collectionItem.sortOrder);
    item->SetProperty("collection.isspecial", mediaType == "special");
    item->SetProperty("mediatype", normalizedMediaType);
    // Prevent CGUIMediaWindow::FormatItemLabels from clearing our label.
    // Without this, an unknown format mask sets label to "", then GetLabel()
    // falls back to CUtil::GetTitleFromPath which returns the numeric DB id.
    item->SetLabelPreformatted(true);
    CLog::LogF(LOGDEBUG, "GUIWindowVideoCollection: final label='{}' for mediaType='{}' idMedia={}",
               resolvedLabel, mediaType, collectionItem.idMedia);
    item->SetProperty("collection.displaylabel", resolvedLabel);
    if (!collectionName.empty())
      item->SetProperty("collection.name", collectionName);
    if (!collectionItem.groupName.empty())
      item->SetProperty("collection.groupname", collectionItem.groupName);

    items.Add(item);
  }

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
