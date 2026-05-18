/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GUIWindowVideoBase.h"

class CFileItemList;

enum class SelectFirstUnwatchedItem
{
  NEVER = 0,
  ON_FIRST_ENTRY = 1,
  ALWAYS = 2
};

enum class IncludeAllSeasonsAndSpecials
{
  NEITHER = 0,
  BOTH = 1,
  ALL_SEASONS = 2,
  SPECIALS = 3
};

class CGUIWindowVideoNav : public CGUIWindowVideoBase
{
public:

  CGUIWindowVideoNav(void);
  ~CGUIWindowVideoNav(void) override;

  bool OnAction(const CAction &action) override;
  bool OnMessage(CGUIMessage& message) override;
  bool OnBack(int actionID) override;

protected:
  bool ApplyWatchedFilter(CFileItemList &items);
  bool GetFilteredItems(const std::string &filter, CFileItemList &items) override;

  void OnItemLoaded(CFileItem* pItem) override {};

  // override base class methods
  bool Update(const std::string &strDirectory, bool updateFilterPath = true) override;
  bool GetDirectory(const std::string &strDirectory, CFileItemList &items) override;
  void UpdateButtons() override;
  void DoSearch(const std::string& strSearch, CFileItemList& items) override;
  void OnDeleteItem(const CFileItemPtr& pItem) override;
  void GetContextButtons(int itemNumber, CContextButtons &buttons) override;
  bool OnPopupMenu(int iItem) override;
  bool OnContextButton(int itemNumber, CONTEXT_BUTTON button) override;
  bool OnAddMediaSource() override;
  bool OnClick(int iItem, const std::string &player = "") override;
  std::string GetStartFolder(const std::string &dir) override;
  std::string GetRootPath() override;

  std::vector<CMediaSource> m_shares;

private:
  // When activated from GUIWindowVideoCollection, stores the collection URL so that OnBack
  // can navigate directly back to the collection, bypassing the window history de-duplication
  // in CGUIWindowManager::AddToWindowHistory which would otherwise strip WINDOW_VIDEO_COLLECTION.
  std::string m_collectionReturnUrl;

  virtual SelectFirstUnwatchedItem GetSettingSelectFirstUnwatchedItem();
  virtual IncludeAllSeasonsAndSpecials GetSettingIncludeAllSeasonsAndSpecials();
  virtual int GetFirstUnwatchedItemIndex(bool includeAllSeasons, bool includeSpecials);
  int GetFirstSelectedItemIndex() const;
  void SelectDefaultItem();
};
