## new set no longer listed in in main lirbrary movies view and lost the ability to add a tv show to a collection

newly created set no longer listed in in main lirbrary movies view, still appears in selectable list for collection when setting for a movie and lost the ability to add a tv show to a collection and episode implementation doesn't exist

| Date\Time | Description | Author |
|-----------|-------------|--------|
| 2026-05-18 | Root cause: currentIds was used both for pre-selection and as the add/remove baseline. When "New set..." was pressed, the new set's ID was inserted into currentIds for pre-selection — which caused the add-delta loop to treat it as already-existing and skip AddOrUpdateCollectionItem and UpdateMovieSetId entirely. Fixed by splitting into originalIds (immutable, true memberships at session open) and preSelectIds (mutable, dialog pre-selection state). All three sub-issues (new set not in library, TV show, episode) resolved by same fix. | GitHub Copilot |
| 2026-05-18 19:32 | The ability to add colection to tv shows has returned but newly created sets/collection are not appearing in the main library lists for navigation (either in movies or tv shows), also still unable to select set collection for a tv show episode | User |

Status: resolved

## movie versions no longer spawn a pop up to select the version of the film you want to play

movie versions no longer spawn a pop up to select the version of the film you want to play, instead it now opens a listing type view, we shouldn't have done anything to change this behaviour as versions are not in our scope

| Date\Time | Description | Author |
|-----------|-------------|--------|
| 2026-05-19 | **Not caused by our commits.** The version-selection popup (`CVideoChooser` via `VideoVersionHelper.cpp`, window id 12015 `SelectVideoVersion`) was **deliberately removed by upstream Kodi v22** in commit `7bc7cc5e03` ("[guilib] Remove Choose Version/Extras Dialog and related functions", Jan 16 2025) and companion commit `a44a7901cd` ("[video] Integrate movie versions and extras in directory nodes and normal navigation"). The new Kodi v22 behavior replaces the popup with folder-based navigation: versioned movies are marked `IsFolder=true` in `GetMoviesByWhere` with path `videodb://movies/titles/{id}/-2/`, and clicking them navigates to the versions listing. None of our commits touched `GUIWindowVideoBase.cpp`, `VideoPlayActionProcessor.cpp`, or any version-navigation code. Confirmed by auditing all 8 of our commits. | GitHub Copilot |

Status: **Not a regression from our commits — this is upstream Kodi v22 behavior change.** The popup was intentionally removed by the Kodi team. To restore popup behavior would require re-implementing `VideoVersionHelper.cpp` and wiring it back in, which is out of scope for our collection work.

## Only one collection is listed in the tv show list so far

Only the "Marvel" collection is listed in the main TV Show list, there should be two others at least at this point. "Marvel earth 616..." and "Arrowverse - Crisis on Infinite Earths" (which will contain just episodes)

Status: Resolved

## Unable to "Manage" TV shows

Now tv shows in collections are being hidden in the root list they only accessible via the colleciton view, in here via the long press context menu they do not have the "Manage" option, can you ensure the context menu here for tv shows aligns with what they normally have in the main tv show list.

Status: resolved