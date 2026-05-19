/*
 *  Copyright (C) 2005-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DirectoryNode.h"

namespace XFILE
{
  namespace VIDEODATABASEDIRECTORY
  {
    class CDirectoryNodeCollectionItems : public CDirectoryNode
    {
    public:
      CDirectoryNodeCollectionItems(const std::string& strName, CDirectoryNode* pParent);

    protected:
      NodeType GetChildType() const override;
      bool GetChilds(CFileItemList& items) override;
      bool GetContent(CFileItemList& items) const override;
    };
  }
}
