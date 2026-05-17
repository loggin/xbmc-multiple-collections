Unified Franchise & Collections System for Kodi
Specification Document – Draft v1

------------------------------------------------
1. Goals and Scope
------------------------------------------------

Primary Goals
- G1: Mixed‑media franchises
  Support "universes" that span:
  - Movies
  - TV shows
  - Seasons
  - Episodes
  - Specials / shorts

- G2: Multi‑collection membership
  Any media item can belong to multiple collections.

- G3: Episode‑level collections
  Support curated episode collections (e.g., Arrowverse crossovers).

- G4: Collection items view
  A new window (GUIWindowVideoCollection) is activated when a collection
  container is selected from within the existing Movies or TV Shows library.
  It renders a flat, mixed-media, ordered, optionally grouped list of all
  items in the collection. The library remains the entry point; the new
  window handles the mixed-media rendering that GUIWindowVideoNav cannot.

- G5: API‑level access
  JSON‑RPC support for collections and collection items.

Out of Scope (v1)
- Online sync of franchise definitions
- Full GUI editor for collections
- Skin‑specific customisation beyond Estuary baseline


------------------------------------------------
2. Data Model
------------------------------------------------

Core Concepts

Collection
- A logical grouping of media items (e.g., MCU, Star Wars, Crisis on Infinite Earths).

Collection Item
- A single entry in a collection, referencing a specific media entity.

Collection Types
- franchise
- crossover
- arc
- timeline
- set (legacy compatibility)

Media Types
- movie
- tvshow
- season
- episode
- special


------------------------------------------------
3. Database Design
------------------------------------------------

3.1 New Tables

3.1.1 collection

```sql
CREATE TABLE collection (
  idCollection    INTEGER PRIMARY KEY,
  name            TEXT NOT NULL,
  type            TEXT NOT NULL DEFAULT 'franchise',
  description     TEXT,
  sortType        TEXT NOT NULL DEFAULT 'custom',
  artwork         TEXT,
  dateAdded       TEXT,
  dateModified    TEXT
);
```

3.1.2 collection_item

```sql
CREATE TABLE collection_item (
  idCollection    INTEGER NOT NULL,
  mediaType       TEXT NOT NULL,
  idMedia         INTEGER NOT NULL,
  sortOrder       INTEGER NOT NULL DEFAULT 0,
  groupName       TEXT,
  PRIMARY KEY (idCollection, mediaType, idMedia),
  FOREIGN KEY (idCollection) REFERENCES collection(idCollection)
);
```

Notes:
- idMedia maps to:
  - movie.idMovie
  - tvshow.idShow
  - seasons.idSeason
  - episode.idEpisode

3.2 Migration of Existing Movie Sets

Current:
- sets(idSet, strSet)
- movie(idMovie, idSet, …)

Migration Strategy:
1) Create collection entries for each sets row:
   - collection.idCollection = sets.idSet
   - collection.name = sets.strSet
   - collection.type = 'set'
2) Create collection_item entries for each movie with idSet:
   - idCollection = movie.idSet
   - mediaType = 'movie'
   - idMedia = movie.idMovie
   - sortOrder = existing set order if available, else 0
3) Keep sets and movie.idSet for backward compatibility in v1.
   - New code should prefer collection / collection_item.

3.3 Schema Versioning
- Increment video DB version (MyVideosNNN → MyVideosNNN+1).
- In CVideoDatabase::UpdateOldVersion():
  - Create collection and collection_item tables.
  - Perform migration from sets / movie.idSet.
  - Ensure migration is idempotent and safe.


------------------------------------------------
4. Metadata Ingestion (Scanner & NFO)
------------------------------------------------

4.1 NFO Extensions

Multiple collections per item (movies, tvshows, episodes):

Example:
```xml
<collections>
  <collection>
    <name>Marvel Cinematic Universe</name>
    <type>franchise</type>
    <order>10</order>
    <group>Phase 1</group>
  </collection>
  <collection>
    <name>Infinity Saga</name>
    <type>arc</type>
    <order>2</order>
  </collection>
</collections>
```

Minimal shorthand (single collection):
```xml
<collection>Marvel Cinematic Universe</collection>
```

Episode collections (crossover events):

Example:
```xml
<collections>
  <collection>
    <name>Crisis on Infinite Earths</name>
    <type>crossover</type>
    <order>3</order>
  </collection>
</collections>
```

4.2 Scanner Behaviour

Changes in:
- CVideoInfoScanner
- CVideoInfoTag

Rules:
1) When scanning an item:
   - Parse <collections> block if present.
   - For each <collection>:
     - Find or create collection by name (and optional type).
     - Insert/replace collection_item with:
       - idCollection
       - mediaType (derived from context: movie/tvshow/episode)
       - idMedia
       - sortOrder (from <order> or default 0)
       - groupName (from <group> if present)

2) If only legacy <set> is present:
   - Map to collection of type 'set' for compatibility.

4.3 Scraper Integration (Optional v1)
- Extend scraper documentation to allow emitting <collections> blocks.
- For v1, NFO-based workflows are primary; scraper support can be added later.


------------------------------------------------
5. Core Logic and APIs
------------------------------------------------

5.1 CVideoDatabase Additions

New methods (signatures indicative):

```cpp
bool GetCollections(std::vector<CCollection>& outCollections,
                    const std::string& typeFilter = "",
                    const std::string& where = "");

bool GetCollectionItems(int idCollection,
                        std::vector<CCollectionItem>& outItems,
                        const std::string& orderBy = "sortOrder");

bool GetCollectionsForMedia(const std::string& mediaType,
                            int idMedia,
                            std::vector<CCollection>& outCollections);

bool AddOrUpdateCollection(const CCollection& collection);

bool AddOrUpdateCollectionItem(const CCollectionItem& item);

bool RemoveCollectionItem(int idCollection,
                          const std::string& mediaType,
                          int idMedia);
```

New structs:

```cpp
struct CCollection
{
  int         idCollection;
  std::string name;
  std::string type;
  std::string description;
  std::string sortType;
  std::string artwork;
};

struct CCollectionItem
{
  int         idCollection;
  std::string mediaType;
  int         idMedia;
  int         sortOrder;
  std::string groupName;
};
```

5.2 Backward Compatibility Helpers
- Existing set-related methods (e.g., GetMoviesBySet) should internally:
  - Use collection / collection_item where type = 'set'.
  - Fall back to legacy sets if needed for older DBs.


------------------------------------------------
6. JSON-RPC Extensions
------------------------------------------------

6.1 New Types in SchemaDefinition.json

Collection:
- idcollection (integer)
- name (string)
- type (string)
- description (string)
- sorttype (string)

CollectionItem:
- idcollection (integer)
- mediatype (string)
- idmedia (integer)
- sortorder (integer)
- groupname (string)

6.2 New Methods

VideoLibrary.GetCollections
- Params:
  - type (optional): filter by collection type (franchise, crossover, arc, timeline, set).
  - properties (optional): list of fields to return.
- Returns:
  - Array of Collection objects.

VideoLibrary.GetCollectionItems
- Params:
  - idcollection (required)
  - properties (optional): standard video properties + groupname, sortorder.
- Returns:
  - Array of mixed media items, each including:
    - mediatype
    - idmedia
    - standard movie/tvshow/episode fields
    - groupname
    - sortorder

VideoLibrary.GetCollectionsForItem
- Params:
  - mediatype
  - idmedia
- Returns:
  - Array of Collection objects.

(Write operations like SetCollection / SetCollectionItems can be deferred to a later version.)


------------------------------------------------
7. UI / UX Design
------------------------------------------------

7.1 Design Principle

A new window class, GUIWindowVideoCollection, handles the display of
collection contents. It is not a standalone top-level destination; it is
activated when the user selects a collection container item from within
the existing Movies or TV Shows library windows (GUIWindowVideoNav).

This separation is necessary because GUIWindowVideoNav operates in a
single-media-type context (movies or TV shows) and cannot natively render
a mixed list of movies, TV shows, seasons, episodes, and specials in one
container. GUIWindowVideoCollection owns that responsibility.

The library remains the entry point for all collection access. The user
browses Movies or TV Shows as normal, encounters a collection container
item, selects it, and GUIWindowVideoCollection is pushed onto the
navigation stack. Pressing Back dismisses it and returns the user to the
library at the same position.

7.2 GUIWindowVideoCollection

Class:
- GUIWindowVideoCollection
- Location: xbmc/video/windows/GUIWindowVideoCollection.*

Responsibilities:
- Accept an idCollection on activation.
- Call GetCollectionItems for that idCollection.
- Render a flat, ordered, mixed-media list of the collection's contents.
- Handle item-type-specific selection behaviour (see 7.2.2).
- Return to the calling library window on Back.

7.2.1 Activation

GUIWindowVideoCollection is activated by GUIWindowVideoNav when the
selected item is a collection container. The calling window passes:
- idCollection: the collection to display.
- returnUrl: the library URL to restore when Back is pressed.

It can also be activated from the VideoInfo dialog when the user selects
a collection name from the "Part of" list (see 7.3.3), passing the same
parameters.

7.2.2 Collection Items View

GUIWindowVideoCollection renders a flat ordered list. Each item is
displayed according to its mediaType:

Movie:
- Rendered as a standard movie item (poster, title, year, rating).
- Selecting opens the movie's VideoInfo dialog or begins playback.

TV Show (mediaType = 'tvshow', series level):
- Rendered as a TV show container item (show poster, title, episode
  count, watched state).
- Selecting pushes GUIWindowVideoNav in TV show context onto the stack,
  opening the show's season list.
- The full series is a single item in the collection list.

Season (mediaType = 'season'):
- Rendered as a season container item (season poster or show poster,
  season label, episode count, watched state).
- Selecting pushes GUIWindowVideoNav in season context, opening that
  season's episode list.
- Used when a collection references a specific season rather than a
  full series.

Episode (mediaType = 'episode'):
- Rendered as an individual episode item (episode thumbnail, show title,
  season/episode number, title, watched state).
- Selecting opens the episode's VideoInfo dialog or begins playback.
- Used for curated episode collections such as crossover events where
  specific episodes from multiple shows are interleaved in watch order.

Special (mediaType = 'special'):
- Rendered as an episode-style item with a "Special" label in place of
  a season/episode number.
- Selecting opens VideoInfo or begins playback.

All items additionally display:
- A media type indicator icon (movie reel, TV, season, episode, special).
- Per-item watched state from the active profile.
- Optional group header separator rows derived from groupName
  (e.g. "Phase 1", "Crisis on Earth-X"), rendered as non-selectable
  rows above the first item in each group.
- Sort indicator in the view header (Chronological / Release / Custom).

7.2.3 Navigation and Back Behaviour

Selecting a collection container in Movies or TV Shows library:
- GUIWindowVideoCollection is pushed onto the navigation stack.
- URL path reflects the collection context:
  videodb://collections/42/items

Pressing Back from GUIWindowVideoCollection:
- Pops back to the calling GUIWindowVideoNav at the position of the
  collection container item that was selected.

Selecting a movie or episode item within the collection:
- Opens VideoInfo dialog or begins playback.
- Back from VideoInfo returns to GUIWindowVideoCollection.

Selecting a TV show container item within the collection:
- Pushes GUIWindowVideoNav (TV show context) showing the season list.
- Back from the season list returns to GUIWindowVideoCollection.
- Back from an episode list returns to the season list.
- Back from VideoInfo or playback returns to the episode list.
- Full stack:
  GUIWindowVideoNav (library) → GUIWindowVideoCollection →
  GUIWindowVideoNav (seasons) → GUIWindowVideoNav (episodes) →
  VideoInfo / playback

Selecting a season container item within the collection:
- Pushes GUIWindowVideoNav (season context) showing the episode list.
- Back from the episode list returns to GUIWindowVideoCollection.
- Full stack:
  GUIWindowVideoNav (library) → GUIWindowVideoCollection →
  GUIWindowVideoNav (episodes) → VideoInfo / playback

7.3 Estuary Integration

7.3.1 Collection Container Item Skin Support

Changes to existing Estuary list/grid view XML to support collection
container items within GUIWindowVideoNav:
- Expose isCollection boolean property so skins can apply collection-
  specific presentation (e.g. type badge overlay, item count).
- Expose collectionType string property for type badge label
  (Franchise, Crossover, Arc, Timeline, Set).

7.3.2 GUIWindowVideoCollection Layout

New Estuary skin file:
- addons/skin.estuary/xml/MyVideoCollection.xml

Features:
- List and thumbnail view modes for the mixed-media item list.
- Group header rows for groupName separators.
- Media type icon per item.
- Sort indicator in the view header (Chronological / Release / Custom).
- Collection title and type badge in the window header.

7.3.3 Video Info Integration

In DialogVideoInfo.xml:
- Show a list of collections the item belongs to, e.g.:
  - "Part of: Marvel Cinematic Universe, Infinity Saga, Phase 3"
- Each collection name is selectable and activates
  GUIWindowVideoCollection for that collection, passing the current
  VideoInfo context as the return point.


------------------------------------------------
8. Configuration and Behaviour
------------------------------------------------

8.1 Optional Advanced Settings (advancedsettings.xml)

Example:
```xml
<video>
  <collections>
    <enable>true</enable>
    <showsetsascollections>true</showsetsascollections>
    <defaulttype>franchise</defaulttype>
  </collections>
</video>
```

8.2 Sorting Behaviour

- If collection.sortType = 'custom':
  - Use collection_item.sortOrder.
- If 'release':
  - Sort by release date of underlying media.
- If 'chronological':
  - Use sortOrder if present; otherwise fall back to release date (v1 behaviour).


------------------------------------------------
9. Performance and Indexing
------------------------------------------------

Recommended indices:

```sql
CREATE INDEX idx_collection_type
  ON collection(type);

CREATE INDEX idx_collection_item_collection
  ON collection_item(idCollection);

CREATE INDEX idx_collection_item_media
  ON collection_item(mediaType, idMedia);
```

Goal:
- Ensure GetCollectionsForMedia and GetCollectionItems are efficient for large libraries.


------------------------------------------------
10. Backward Compatibility and Rollout
------------------------------------------------

- Existing users:
  - Movie sets continue to work as before.
  - New system is additive; no UI breakage.
- New features:
  - Only visible if:
    - DB migration succeeded.
    - New menu entry / feature is enabled.
- Skins:
  - Estuary updated in v1.
  - Other skins can opt-in by using the new window ID and JSON-RPC methods.

------------------------------------------------
11. Profiles and Permissions
------------------------------------------------

11.1 Design Principle

Collections do not have their own independent permission model. Instead, a
collection's effective permissions are an amalgamation of the permissions of
the media items it contains. The collection definition itself (name, type,
sort order) follows the profile's existing media info sharing mode.

11.2 Item-Level Permission Inheritance

When a collection contains items drawn from sources with differing profile
permissions, the following rules apply:

- Visibility: an item is shown in the franchise view only if the active
  profile has read access to the media source that item belongs to. Items
  the profile cannot access are excluded from the rendered list.

- Mutability: an item within a collection can only be acted upon (played,
  marked watched, edited) if the active profile has write access to that
  item's source. Read-only profile members may view but not mutate items
  from shared read-only sources, consistent with Kodi's existing behaviour.

- No permission is inferred or escalated from the collection itself. A
  collection row never grants access to an item that the profile could not
  access by navigating to it directly.

11.3 Collection Definition Permissions

The collection and collection_item table rows are metadata, not media
sources, and follow the profile's media info sharing setting:

- Separate profile: the profile has its own collection table; full read/write.
- Shares with Default: the profile reads from the master's collection table;
  write operations are permitted.
- Shares with Default (Read Only): the profile can view collections defined
  by the master but cannot create, rename, reorder, or delete collections or
  collection items.
- Separate (Locked): own collection table, changes require master mode.

11.4 Partially Visible Collections

When a collection contains items that are inaccessible to the active profile,
the franchise view must handle this gracefully rather than silently omitting
entries with no explanation. The behaviour is configurable:

- hide (default): inaccessible items are silently excluded. The item count
  shown for the collection reflects only accessible items.
- locked: inaccessible items are shown as placeholder entries with a lock
  icon and no actionable controls.

This is exposed via advancedsettings.xml:

```xml
<video>
  <collections>
    <inaccessibleitems>hide</inaccessibleitems>
    <!-- or: locked -->
  </collections>
</video>
```

11.5 Watched State

Watched status and resume points are already stored per-profile in Kodi's
existing viewstate tables. The franchise view inherits this behaviour
automatically; no additional handling is required. Progress indicators at
the collection level (e.g. "12 of 30 watched") are derived from the active
profile's viewstate and are therefore naturally profile-scoped.

------------------------------------------------
12. Collection Export and Import
------------------------------------------------

12.1 Overview

Collections can be exported to and imported from the filesystem using NFO
files and accompanying artwork. Two storage layouts are supported:

- Dedicated collection folder: a user-configured root path where each
  collection gets its own subfolder.
- Inline folder: a collection.nfo found within the existing media folder
  hierarchy, co-located with the media it describes.

Both layouts use the same file naming convention and are scanned and exported
using the same rules. The inline layout takes precedence for import path
resolution and is preferred as the export target when it exists.

12.2 File Layout

12.2.1 Dedicated collection folder layout

A user-configured root path (see 12.5) contains one subfolder per collection:

```
<collectionRoot>/
  <Collection Name>/
    collection.nfo
    poster.jpg      (or poster.png)
    fanart.jpg      (or fanart.png)
```

Example:
```
D:\CollectionArt\
  Marvel Cinematic Universe\
    collection.nfo
    poster.jpg
    fanart.jpg
  Star Wars\
    collection.nfo
    poster.png
    fanart.jpg
```

12.2.2 Inline folder layout

A collection.nfo may exist anywhere within the media folder tree, at any
level above individual media files. The scanner treats any collection.nfo
it encounters as belonging to the collection named within the file, and
associates the folder it resides in as that collection's home path.

Example:
```
V:\Movies\A\Aliens\
  collection.nfo
  poster.jpg
  fanart.jpg
  Alien (The Directors Cut)\
    Alien (The Directors Cut).mkv
    Alien (The Directors Cut).nfo
  Aliens (Directors Cut)\
    Aliens (Directors Cut).mkv
    Aliens (Directors Cut).nfo
```

The inline folder does not need to contain media directly; it only needs to
be an ancestor of the media items that belong to the collection.

12.3 collection.nfo Format

The collection.nfo file describes the collection definition. It uses the
same structure as the <collections> block in item NFOs (section 4.1) but
describes a single collection from its own perspective:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<collection>
  <name>Alien Collection</name>
  <type>franchise</type>
  <description>The Alien film series.</description>
  <sorttype>chronological</sorttype>
</collection>
```

Artwork (poster, fanart) is resolved by looking for the following filenames
in the same folder as the collection.nfo, in order of preference:

- poster.jpg, poster.png
- fanart.jpg, fanart.png

12.4 Import / Scan Behaviour

12.4.1 Dedicated folder scan

When a collectionRoot is configured (see 12.5), the scanner walks its
immediate subfolders at startup and on library update. For each subfolder:

1. If collection.nfo is present, parse it and call AddOrUpdateCollection.
2. Resolve artwork from the same folder and store paths.
3. The subfolder path is recorded as the collection's home path for future
   export operations.

12.4.2 Inline folder scan

During a normal video library scan, the scanner checks each folder it
descends into for the presence of collection.nfo before processing media
files within it. If found:

1. Parse and call AddOrUpdateCollection as above.
2. Resolve artwork from the same folder.
3. Record the folder path as the collection's home path, which takes
   precedence over any dedicated folder path for this collection (see 12.6).

12.4.3 Conflict resolution

If the same collection name is encountered in both an inline folder and the
dedicated collection root, the inline folder path wins as the home path.
The collection definition is merged: the inline NFO takes precedence for
all fields it specifies; any fields absent from the inline NFO fall back to
the dedicated folder NFO.

12.5 Configuration

Collection export/import paths are configured in advancedsettings.xml:

```xml
<video>
  <collections>
    <enable>true</enable>
    <collectionroot>D:\CollectionArt</collectionroot>
    <!-- Root folder for dedicated per-collection subfolders.
         Optional; if absent, only inline folder layout is used. -->
  </collections>
</video>
```

12.6 Export Path Resolution

When exporting a collection, the target path is resolved in the following
order of preference:

1. Inline home path — if the collection has a recorded inline folder path
   (i.e. a collection.nfo was previously found or exported within the media
   tree), export to that folder.
2. Dedicated collection folder — if a collectionRoot is configured, export
   to <collectionRoot>/<Collection Name>/, creating the subfolder if needed.
3. No export path available — warn the user and offer to choose a path
   manually or configure a collectionRoot.

This ensures that collections which originated from an inline layout are
exported back to the same location, preserving the co-location of collection
metadata with the media it describes.

12.7 Export Operation

Export writes the following files to the resolved path:

- collection.nfo — full collection definition as per section 12.3.
- poster.jpg / poster.png — if artwork is present in the database.
- fanart.jpg / fanart.png — if artwork is present in the database.

Existing files at the target path are overwritten. Export does not modify
collection_item membership; it only serialises the collection definition
and artwork.

Export can be triggered:
- Per collection, from the franchise view context menu.
- Bulk, via a future VideoLibrary.ExportCollections JSON-RPC method
  (deferred to a later version).

12.8 JSON-RPC (v1 scope)

VideoLibrary.ExportCollections is deferred post-v1. For v1, export is
available only via the UI context menu as described in 12.7.

Import occurs automatically as part of the standard library scan (section
12.4) and requires no dedicated JSON-RPC method.

------------------------------------------------
13. Error Handling
------------------------------------------------

13.1 Principles

- Errors in collection handling must never affect playback or library
  browsing of non-collection media. All collection operations are
  treated as non-critical relative to core library function.
- Errors are logged at appropriate severity levels using Kodi's existing
  CLog infrastructure.
- Where an operation is user-initiated (export, manual scan), errors are
  surfaced via the existing notification/toast system.
- Where an operation is background (scan, migration), errors are logged
  and skipped rather than halted; a summary warning is shown on completion
  if any errors occurred.

13.2 Database Migration Errors

Migration from sets / movie.idSet to collection / collection_item runs
inside CVideoDatabase::UpdateOldVersion() and must be fully transactional.

Rules:
- The entire migration runs inside a single database transaction.
- If any step fails, the transaction is rolled back and the DB version is
  not incremented. Kodi will reattempt migration on the next startup.
- Failures are logged at LOGERROR with enough context to identify the
  offending row (e.g. idSet, strSet).
- If migration fails repeatedly (detected via a migration attempt counter
  stored in the DB settings table), Kodi logs a warning and disables the
  collections feature rather than entering a boot loop. The user is
  notified via a one-time UI warning.

Specific cases:

| Condition                                 | Behaviour                                      |
|-------------------------------------------|------------------------------------------------|
| collection table already exists           | Skip creation; treat as idempotent             |
| Duplicate idCollection during insert      | Log and skip; do not overwrite existing entry  |
| idSet references a non-existent movie     | Log orphan and skip collection_item insert     |
| DB write fails mid-migration              | Roll back full transaction; retry on restart   |

13.3 NFO Parse Errors

During scan, malformed or incomplete <collections> blocks must not abort
scanning of the media item itself.

Rules:
- If the <collections> block is malformed XML, log at LOGWARNING and
  continue scanning the item without collection membership.
- If a <collection> entry is missing a required <name> field, skip that
  entry and log at LOGWARNING.
- If <order> is present but non-numeric, default to 0 and log at LOGDEBUG.
- If <type> is present but not a recognised value, default to 'franchise'
  and log at LOGWARNING.
- If AddOrUpdateCollection fails for a parsed entry, log at LOGERROR and
  continue; do not fail the scan of the item.

13.4 Stale Reference Handling

A collection_item may reference an idMedia that no longer exists (e.g. a
movie was removed from the library without triggering a collection cleanup).

Rules:
- GetCollectionItems filters out any item where the referenced idMedia does
  not exist in its target table. These are not returned to callers.
- Stale items are logged at LOGDEBUG on detection.
- A dedicated cleanup pass — RemoveStaleCollectionItems() — is added to
  CVideoDatabase and called as part of the existing library cleanup
  operation (triggered by "Clean Library"). It deletes collection_item rows
  whose idMedia no longer exists.
- Stale items do not trigger errors in the franchise view UI; the view
  simply renders fewer items than the stored sortOrder sequence would imply.

13.5 Export Errors

Rules:
- If the resolved export path does not exist and cannot be created, notify
  the user and abort the export for that collection.
- If an individual file write fails (e.g. disk full, permissions), log at
  LOGERROR, notify the user, and leave any already-written files in place
  (partial export is acceptable; the operation is idempotent on retry).
- If artwork is missing from the database for a collection, skip artwork
  export silently (not an error condition).

13.6 JSON-RPC Errors

New methods must return standard Kodi JSON-RPC error objects for all failure
cases. Defined error codes:

| Code   | Constant                              | Condition                                      |
|--------|---------------------------------------|------------------------------------------------|
| -32602 | InvalidParams                         | Missing required parameter (e.g. idcollection) |
| 404    | VideoLibrary.Error.CollectionNotFound | idcollection does not exist                    |
| 404    | VideoLibrary.Error.MediaNotFound      | idmedia / mediatype combination not found      |
| 422    | VideoLibrary.Error.InvalidMediaType   | mediatype value not in allowed set             |
| 500    | VideoLibrary.Error.DatabaseError      | Underlying DB operation failed                 |

All list methods (GetCollections, GetCollectionItems, GetCollectionsForItem)
return an empty array rather than an error when the query succeeds but
yields no results, consistent with existing VideoLibrary method behaviour.


------------------------------------------------
14. Testing Strategy
------------------------------------------------

14.1 Scope

Testing covers four layers: database migration, core library logic, JSON-RPC
API, and UI. Each layer has distinct risks and appropriate test approaches.

14.2 Database Migration Tests

Migration is the highest-risk operation as it modifies existing user data.
The following matrix of starting states must all be tested:

| Starting State                              | Expected Outcome                                      |
|---------------------------------------------|-------------------------------------------------------|
| Fresh install (no DB)                       | Tables created; no migration needed                   |
| Existing DB, no sets                        | Tables created; collection/collection_item empty      |
| Existing DB, sets with movies               | Each set becomes a collection; items created correctly|
| Existing DB, sets with orphaned idSet refs  | Orphans logged and skipped; valid sets migrated       |
| Existing DB, collection table already exists| Migration skipped; existing data preserved            |
| Migration interrupted (simulated failure)   | Transaction rolled back; DB version unchanged         |
| Migration reattempted after rollback        | Completes successfully on retry                       |

Tests are implemented as CppUnit / Google Test fixtures that operate on a
temporary copy of a known DB fixture file, never on live user data.

14.3 CVideoDatabase Unit Tests

New methods in CVideoDatabase are tested in isolation against an in-memory
SQLite database seeded with known fixture data.

Coverage required:

- GetCollections: empty DB, type filter, no filter.
- GetCollectionItems: valid idCollection, unknown idCollection, stale items
  present (stale items must be excluded from results).
- GetCollectionsForMedia: media in one collection, media in multiple
  collections, media in no collections.
- AddOrUpdateCollection: insert new, update existing, duplicate name
  handling.
- AddOrUpdateCollectionItem: insert, update sortOrder, update groupName.
- RemoveCollectionItem: existing item, non-existent item (no error).
- RemoveStaleCollectionItems: mixed stale and valid items; only stale removed.

14.4 NFO Parser Tests

Tests for CVideoInfoTag collection parsing cover:

- Full <collections> block with multiple entries.
- Minimal shorthand <collection>Name</collection>.
- Malformed XML in <collections> block (must not throw; item scanned without
  collection membership).
- Missing <name> field (entry skipped).
- Invalid <type> value (defaults to 'franchise').
- Non-numeric <order> value (defaults to 0).
- Legacy <set> present alongside <collections> (both processed correctly).
- Episode NFO with crossover collection entry.

14.5 JSON-RPC Integration Tests

Tests exercise the full JSON-RPC stack against a seeded test database.

Coverage required:

- GetCollections: no filter, type filter, unknown type filter (empty result).
- GetCollectionItems: valid id, unknown id (CollectionNotFound error),
  pagination via limits param.
- GetCollectionsForItem: valid media, unknown mediatype (InvalidMediaType
  error), item in no collections (empty result).
- Error response shape: confirm all error cases return correct code and
  message fields.

14.6 Export / Import Tests

- Export to dedicated folder: verify collection.nfo written, artwork written,
  existing files overwritten correctly.
- Export to inline path: verify inline home path takes precedence over
  collectionRoot.
- Export with no path configured: verify user notification triggered, no
  files written.
- Import via scan (dedicated folder): verify collection created, artwork
  resolved.
- Import via scan (inline folder): verify collection created, inline home
  path recorded.
- Conflict resolution: same collection in both inline and dedicated folder;
  verify inline path wins, fields merged correctly.
- Idempotent re-import: scanning the same collection.nfo twice produces one
  collection row, not two.

14.7 UI / Regression Tests

Manual test cases (automated where Kodi's test infrastructure permits):

- Franchise view renders correctly with zero collections.
- Franchise view renders correctly with a mix of collection types.
- Partially visible collection (profile permissions): hidden and locked modes
  both render without crash.
- Navigation: entering a collection, selecting an item, pressing Back returns
  to the correct level.
- Stale item in collection: view renders remaining items without error.
- Existing movie sets visible and functional after migration (backward
  compatibility regression).
- VideoInfo dialog shows correct collection membership for a movie belonging
  to multiple collections.

14.8 Performance Tests

Given the indexing strategy in section 9, the following must be verified
against a library of at least 5,000 media items:

- GetCollectionsForMedia completes in under 50ms.
- GetCollectionItems for a 100-item collection completes in under 100ms.
- Full library scan with collection.nfo files present adds no more than 10%
  overhead compared to a scan without collection metadata.

------------------------------------------------
End of Document
------------------------------------------------
