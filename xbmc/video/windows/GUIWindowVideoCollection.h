/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GUIWindowVideoBase.h"

class CFileItemList;

class CGUIWindowVideoCollection : public CGUIWindowVideoBase
{
public:
  CGUIWindowVideoCollection();
  ~CGUIWindowVideoCollection() override;

protected:
  bool GetDirectory(const std::string& strDirectory, CFileItemList& items) override;
  bool OnSelect(int iItem) override;
  void GetContextButtons(int itemNumber, CContextButtons& buttons) override;
  bool OnContextButton(int itemNumber, CONTEXT_BUTTON button) override;

private:
};
