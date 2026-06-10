/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "FileItem.h"
#include "guilib/GUIDialog.h"

#include <memory>
#include <vector>

class CGUIDialogCollectionTimeline : public CGUIDialog
{
public:
  CGUIDialogCollectionTimeline();
  ~CGUIDialogCollectionTimeline() override = default;

  void SetCollection(int idCollection, const CFileItemList& items);
  bool IsConfirmed() const { return m_confirmed; }

protected:
  void OnInitWindow() override;
  bool OnMessage(CGUIMessage& message) override;
  bool OnAction(const CAction& action) override;

private:
  static constexpr int CONTROL_LIST     = 50;
  static constexpr int CONTROL_MOVE_UP  = 5;
  static constexpr int CONTROL_MOVE_DOWN = 6;
  static constexpr int CONTROL_DONE     = 7;

  void RefreshList(int keepFocus = 0);
  int  GetFocusedItem() const;
  void MoveItem(bool up);
  void SaveOrder();

  int m_idCollection{-1};
  std::vector<CFileItemPtr> m_orderedItems;
  bool m_confirmed{false};
};
