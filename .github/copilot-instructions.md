# Copilot Instructions for Kodi Multi Collections

## Model Policy
- **GPT-4 (all variants: gpt-4, gpt-4o, gpt-4-turbo, o1, o3, o4) MUST NOT be used for any task in this project.**
- Approved models: Claude Sonnet (currently Claude Sonnet 4.6), GPT-5, or another explicitly approved non-GPT-4 model.
- If no approved model is available, stop and notify the user rather than falling back to GPT-4.
- This rule applies to all agents, subagents, inline completions, chat, and any other AI-assisted workflow touching this codebase.

## Project Context
This repository extends Kodi video library behavior with a unified collections system.
The primary source of truth for requirements is [spec.md](../spec.md).

## High-Level Goals
- Support mixed-media collections (movie, tvshow, season, episode, special).
- Support multi-collection membership for any media item.
- Preserve legacy movie set compatibility while adding new collection tables.
- Provide collection read APIs through JSON-RPC.
- Provide mixed-media collection browsing via a dedicated window.

## Implementation Priorities
1. Keep existing Kodi behavior stable for users who do not use collections.
2. Prefer additive changes over disruptive refactors.
3. Maintain backward compatibility with legacy set flows in v1.
4. Ensure DB migrations are idempotent and transactional.
5. Treat collection failures as non-fatal for core playback and library browsing.

## Path-Specific Edit Targets

Database schema and migration:
- Primary files:
  - `xbmc/video/VideoDatabaseDDL.h`
  - `xbmc/video/VideoDatabaseDDL.cpp`
  - `xbmc/video/VideoDatabaseMigration.cpp`
  - `xbmc/video/VideoDatabaseColumns.h`
- Do:
  - Keep all collection table creation and index creation centralized in DDL and migration code.
  - Keep migration fully transactional and safe to rerun.
  - Keep legacy movie set support intact for v1.
- Don't:
  - Do not remove or repurpose existing set columns/tables in v1.
  - Do not increment DB version without a complete upgrade path.

Video database API surface:
- Primary files:
  - `xbmc/video/VideoDatabase.h`
  - `xbmc/video/VideoDatabase.cpp`
- Do:
  - Add collection query and mutation helpers as narrow, testable methods.
  - Filter stale references defensively when materializing collection items.
- Don't:
  - Do not mix collection business rules into unrelated query paths.
  - Do not change method semantics used by existing movie set callers unless compatibility wrappers are in place.

JSON-RPC schema and handlers:
- Primary files:
  - `xbmc/interfaces/json-rpc/schema/types.json`
  - `xbmc/interfaces/json-rpc/schema/methods.json`
  - `xbmc/interfaces/json-rpc/VideoLibrary.h`
  - `xbmc/interfaces/json-rpc/VideoLibrary.cpp`
- Do:
  - Add read methods first and keep response structure stable and explicit.
  - Validate parameters early and return the documented error codes.
- Don't:
  - Do not introduce write RPC methods in v1 unless explicitly requested.
  - Do not return errors for valid empty-result list queries.

Windowing and navigation:
- Primary files:
  - `xbmc/video/windows/GUIWindowVideoNav.h`
  - `xbmc/video/windows/GUIWindowVideoNav.cpp`
  - `xbmc/video/windows/GUIWindowVideoCollection.h` (new)
  - `xbmc/video/windows/GUIWindowVideoCollection.cpp` (new)
- Do:
  - Enter mixed-media collection rendering through the new collection window.
  - Preserve back-stack behavior so Back returns to the caller context.
- Don't:
  - Do not overload `GUIWindowVideoNav` to render mixed-media lists directly.
  - Do not break existing movie-only or tvshow-only navigation flows.

Estuary skin integration:
- Primary files:
  - `addons/skin.estuary/xml/MyVideoNav.xml`
  - `addons/skin.estuary/xml/DialogVideoInfo.xml`
  - `addons/skin.estuary/xml/MyVideoCollection.xml` (new)
- Do:
  - Keep skin changes additive, with graceful fallback for unknown properties.
  - Expose collection badges and mixed-media indicators without regressing existing views.
- Don't:
  - Do not hardcode assumptions that all collection items are movies.
  - Do not require non-Estuary skins to change in v1.

Scanner and metadata parsing:
- Expected ownership area:
  - `xbmc/video/` scanner and video info tag parser classes.
- Do:
  - Parse collections metadata defensively and continue scan on malformed input.
  - Preserve legacy `<set>` handling by mapping to collection type `set`.
- Don't:
  - Do not fail media import solely because collection metadata is invalid.
  - Do not drop valid collection entries because one sibling entry is malformed.

## Pull Request Boundaries
- Prefer PR slices in this order:
  1) DB schema + migration
  2) CVideoDatabase methods
  3) JSON-RPC schema + handlers
  4) New collection window + navigation wiring
  5) Estuary integration
  6) Tests and cleanup
- Each slice should compile independently and preserve current user-visible behavior unless the slice explicitly introduces a new feature gate.

## Database and Migration Rules
- Add and use `collection` and `collection_item` as the new model.
- Migrate legacy `sets` and `movie.idSet` into collection rows (`type = set`).
- Keep legacy tables and fields for compatibility in v1.
- Add indexes for `collection.type`, `collection_item.idCollection`, and `(mediaType, idMedia)`.

## Scanning and Metadata Rules
- Parse both `<collections>` blocks and shorthand `<collection>` tags.
- Accept collection metadata on movies, shows, seasons, and episodes where applicable.
- If `<set>` exists, map to collection type `set`.
- Handle malformed collection metadata defensively: log and continue scanning.

## API Rules
- Add read-focused JSON-RPC methods:
  - `VideoLibrary.GetCollections`
  - `VideoLibrary.GetCollectionItems`
  - `VideoLibrary.GetCollectionsForItem`
- Return empty arrays for valid no-result queries.
- Return explicit JSON-RPC errors for invalid params, missing collection/media, invalid media types, and DB failures.

## UI Rules
- Use `GUIWindowVideoCollection` for mixed-media collection item rendering.
- Enter from existing library windows, and preserve correct back-stack behavior.
- Show media type indicators, watched state, and optional grouping headers.
- Keep Estuary changes scoped and backward compatible.

## Quality and Testing
- Add focused tests for migration, database methods, parser behavior, JSON-RPC, and regressions.
- Validate stale reference filtering and cleanup behavior.
- Preserve performance targets from `spec.md` when implementing queries.

## Coding Style
- Follow existing Kodi code style, naming, and architecture.
- Keep changes minimal, explicit, and localized.
- Avoid speculative features outside the v1 scope in `spec.md`.


## Commit conventions

- **Always commit and push after completing a user request** (or a logical batch of related changes). Never leave modified files uncommitted or unpushed at the end of a turn. Run `git push` immediately after every `git commit`.
- **Commit message format**: first line must be `[GitHub Copilot / Claude Sonnet 4.6] <short summary>`, where `GitHub Copilot` is the agent name and `Claude Sonnet 4.6` is the model currently in use — update this prefix if either changes. Followed by a blank line and a bullet-per-file body describing what changed and why. Example:
  ```
  [GitHub Copilot / Claude Sonnet 4.6] Add collection_item table and model

  xbmc/video/VideoDatabaseDDL.h:
  - Added collection_item table definition with idCollection, mediaType, idMedia

  xbmc/video/VideoDatabaseDDL.cpp:
  - Implemented CreateCollectionItemTable with indexes on idCollection and (mediaType, idMedia)

  xbmc/video/VideoDatabaseMigration.cpp:
  - Added migration step to create collection_item table if not exists
  ```