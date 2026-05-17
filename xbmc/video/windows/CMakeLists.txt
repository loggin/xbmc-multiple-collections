set(SOURCES GUIWindowFullScreen.cpp
            GUIWindowVideoBase.cpp
            GUIWindowVideoCollection.cpp
            GUIWindowVideoNav.cpp
            GUIWindowVideoPlaylist.cpp
            VideoFileItemListModifier.cpp)

set(HEADERS GUIWindowFullScreen.h
            GUIWindowFullScreenDefines.h
            GUIWindowVideoBase.h
            GUIWindowVideoCollection.h
            GUIWindowVideoNav.h
            GUIWindowVideoPlaylist.h
            VideoFileItemListModifier.h)

core_add_library(video_windows)
