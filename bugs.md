## Navigating back from collection view takes you to an empty node view

There's back panel/button within the collection view, clicking/selecting this takes you to an empty node view, selecting the back again in this view finally takes you back to the main list view (myvideonav ?)#

| Date\Time | Description | Author |
|-----------|-------------|--------|
| 2026-05-18 00:30 | Still navigates to an empty parent node | User |
| 2026-05-26 | HideParentDirItems() added to CGUIViewStateWindowVideoCollection — `..` item suppressed in collection window; CGUIWindowVideoNav::GetRootPath() returns m_startDirectory when set so `..` is also suppressed in sub-windows opened with "return" flag | GitHub Copilot |
| 2026-05-18 10:00 | This is currently not hidden, typical back action (i.e. right click) does navigate back to the main myvideonav list | User |


Status: Resolved

## Movie set clickable button takes you to a myvideonav view of the set

If you select information from teh context menu for an item you get more detail about the item, within this view there is a movie set button and selecting this takes you to a view of teh set which I think is the legacy view, this needs to go back out video collection view instead.

| Date\Time | Description | Author |
|-----------|-------------|--------|
| 2026-05-18 00:30 | Still navigates to legacy view | User |

Status: Resolved

## Adding a new movie set removes the current item from the set it is currently associated with

When managing an items set, choosing to add a new set removes the item from the set it is currently in, this is old behaviour and needs to be changed to maintain existing associations.


| Date\Time | Description | Author |
|-----------|-------------|--------|
| 2026-05-18 00:30 | Can't comment as the ability to add via this mechanism has disapeared for now due to the changes made for the following item | User |

Status: resolved

## Managing an items movie set only allows for a single selection

You can only select one movie set within the interface, this needs to be changed to a multiple selection process, the ability to do exists within the actual manage movie set where you can select multiple films to be included within the set so implement the tolling how it is done within that dialog.


| Date\Time | Description | Author |
|-----------|-------------|--------|
| 2026-05-18 00:30 | This change is working but we have lost the add new set button on the right hand pane of the dialog, can you re add the button to this view before the OK, Cancel buttons | User |
| 2026-05-26 | Switched from EnableButton/IsButtonPressed to EnableButton2/IsButton2Pressed — in multi-select mode OnInitWindow() calls EnableButton(186) which overwrites button 5; button2 (id 8) is unaffected | GitHub Copilot |
| 2026-05-26 10:00 | Still no option to add a new set when selecting existing sets within the dialog | User |

Status: resolved

## View type is not persisting

The selected view type is not being persisted within the video collection view (currently only widelist and iconwall views are available but it always reverts back to iconwall).

Status: Resolved

## Selecting a tv show in video collection view opens a folder and not a tv show view that lists seasons/episodes

I'm seeing the additional metadata files and folders within the tv show folders, this should list the seasons/episodes like a tv show does, seelcting back should take us back to our video collection view.


| Date\Time | Description | Author |
|-----------|-------------|--------|
| 2026-05-18 00:30 | Still happening | User |
| 2026-05-18 00:30 | It now opens a tv show view, but the in list back item (like bug #1) takes you back to the main tv show list rather than the colleciton view | User |
| 2026-05-26 | ActivateWindow now passes {navPath, "return"} for both tvshow and season; CGUIWindowVideoNav::GetRootPath() returns m_startDirectory so `..` is suppressed and Back pops to collection | GitHub Copilot |
| 2026-05-18 10:00 | Still happening, also typical back action (i.e. right click) takes you "back" to the main tv show list view | User |
| 2026-05-18 11:00 | now hidden but the  typical back action (i.e. right click) takes you "back" to the home screen now | User |
| 2026-05-18 11:30 | back action (i.e. right click) on episode list once you drill down through seasons takes you "back" to the home screen now | User |
| 2026-05-18 | Root cause identified: CGUIWindowManager::AddToWindowHistory de-duplicates window IDs — activating WINDOW_VIDEO_NAV again for tvshow stripped WINDOW_VIDEO_COLLECTION from history. Fix: GUIWindowVideoCollection passes 'collectionreturn=<url>' param; GUIWindowVideoNav::OnBack intercepts when at m_startDirectory and activates WINDOW_VIDEO_COLLECTION directly. Back from episodes (deeper than m_startDirectory) falls through to GoParentFolder() correctly. | GitHub Copilot |
| 2026-05-18 13:48 | Still happening and going back to the home screen from the episode list, needs to go back up to the season list | User |
| 2026-05-18 | Root cause: CGUIMediaWindow::OnBack only calls GoParentFolder() for ACTION_NAV_BACK. ACTION_PREVIOUS_MENU (Escape key) bypasses GoParentFolder and falls through to CGUIWindow::OnBack → PreviousWindow() → Home. Fix: CGUIWindowVideoNav::OnBack now intercepts both actions at ALL levels when m_collectionReturnUrl is set — at m_startDirectory it returns to the collection, at deeper levels it calls GoParentFolder() explicitly. | GitHub Copilot |

Status: Resolved

## importing is not detecting collection imagery

Root cause: `DirectoryNodeCollections` sets `m_type = "videocollection"` on collection file items.
`FillLibraryArt` called `GetArtForItem(idCollection, "videocollection")` but legacy set art is stored
under `media_type = "set"` (MediaTypeVideoCollection) in the `art` table — both share the same ID
since `AddSet` uses the same integer for both `sets` and `collection` rows.

Fix: Added fallback in `FillLibraryArt` (VideoThumbLoader.cpp) — when `m_type == "videocollection"`
and no art found, retry with `MediaTypeVideoCollection` ("set"). This surfaces art imported via
movie NFO `<set><thumb>` blocks in the collection window without changing any storage or import code.

| Date\Time | Description | Author |
|-----------|-------------|--------|
| 2026-05-27 | Diagnosed and fixed display-side art lookup mismatch | GitHub Copilot |
| 2026-05-18 13:40 | Doesn't resolve the documented requested changes, also needs an addendum to the file name of teh nfo as in some situations the third party media manager tools are creating set.nfo instead of collection.nfo | GitHub Copilot |
| 2026-05-18 | SetTagLoaderNFO now tries collection.nfo first, falls back to set.nfo. VideoInfoScanner also searches for `<title>-poster/fanart.(jpg\|png)` per spec 12.2 art filename patterns. | GitHub Copilot |

Status: Resolved


## repo has become polluted with "build-" and "*.vcxproj" files and directories

The project and repo have become polluted with what appear to be incorrectly placed files, excluding our own build script