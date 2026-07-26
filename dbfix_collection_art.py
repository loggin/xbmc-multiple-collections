"""
Repair tool for the Clean-Library / AddSet id-divergence bug diagnosed 2026-07-26.

Two things went wrong on the live DB before the fix in VideoDatabase.cpp/VideoInfoScanner.cpp:

1. CleanDatabase() deleted `sets` rows for any set/collection whose movies were only
   linked via collection_item (not the legacy movie.idSet column). Since the paired
   `collection` row survives (collection_item still references it), the next scan's
   AddSet() re-inserted a NEW `sets` row under a NEW id and wrote fresh artwork there,
   orphaning it relative to the real collection id that actually holds the movies.
   The `delete_set` trigger then deletes that orphaned art the next time cleanup runs,
   which is why most affected collections have nothing left to recover in the DB at
   all -- they need a rescan so the source images get re-attached to the correct id.

2. A smaller number of leftover `art` rows sit under a media_id that matches neither
   the `sets` table nor the `collection` table at all (an older, unrelated mix-up).
   These are still recoverable by matching the art's file path against a collection
   name.

This script reports both categories against a live MyVideosNNN.db and, for anything
in category 2, offers to re-point the art onto the correct collection id -- never
overwriting an art type the target collection already has (same rule AddCollection()
follows during a normal scan).

Usage:
    python dbfix_collection_art.py <path-to-MyVideosNNN.db>            # dry run, report only
    python dbfix_collection_art.py <path-to-MyVideosNNN.db> --apply    # write the repairs
"""

import argparse
import os
import sqlite3
import sys


def main():
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8", errors="replace")

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("db_path", help="Path to MyVideosNNN.db")
    parser.add_argument("--apply", action="store_true", help="Write changes (default: dry run)")
    args = parser.parse_args()

    conn = sqlite3.connect(args.db_path)
    cur = conn.cursor()

    # --- Category 1: collections with real membership but zero artwork -----------------
    cur.execute(
        """
        SELECT col.idCollection, col.name,
               (SELECT COUNT(*) FROM collection_item ci WHERE ci.idCollection = col.idCollection),
               (SELECT COUNT(*) FROM art WHERE media_id = col.idCollection AND media_type = 'set')
        FROM collection col
        WHERE col.type = 'set'
        ORDER BY col.name
        """
    )
    rows = cur.fetchall()
    missing_art = [r for r in rows if r[2] > 0 and r[3] == 0]

    print(f"Set-type collections checked: {len(rows)}")
    print(
        f"Collections with movies but ZERO artwork: {len(missing_art)}  "
        "(nothing recoverable in the DB for these -- the art rows were already deleted; "
        "a library rescan is required to re-attach local/scraper images to the correct id)"
    )
    for idc, name, nitems, _ in missing_art:
        print(f"  [{idc}] {name} ({nitems} item(s))")

    # --- Category 2: orphaned 'set' art rows with a media_id matching no collection ----
    cur.execute(
        """
        SELECT art_id, media_id, type, url FROM art
        WHERE media_type = 'set' AND media_id NOT IN (SELECT idCollection FROM collection)
        """
    )
    orphans = cur.fetchall()
    print(f"\nOrphaned 'set' art rows (media_id matches no collection): {len(orphans)}")

    cur.execute("SELECT idCollection, name FROM collection WHERE type = 'set'")
    collections_by_name = {name.strip().lower(): idc for idc, name in cur.fetchall()}

    applied = 0
    for art_id, media_id, art_type, url in orphans:
        # Guess the intended collection from the last folder segment of the art's path.
        folder = os.path.basename(os.path.dirname(url.replace("\\", "/")))
        target_id = collections_by_name.get(folder.strip().lower())

        if not target_id:
            print(f"  UNMATCHED art_id={art_id} media_id={media_id} type={art_type} url={url}")
            continue

        cur.execute(
            "SELECT 1 FROM art WHERE media_id = ? AND media_type = 'set' AND type = ?",
            (target_id, art_type),
        )
        if cur.fetchone():
            print(
                f"  SKIP art_id={art_id} '{folder}' {art_type}: "
                f"target collection {target_id} already has this art type"
            )
            continue

        action = "APPLY" if args.apply else "DRY-RUN"
        print(
            f"  {action}: art_id={art_id} media_id {media_id} -> {target_id} "
            f"('{folder}', {art_type}) {url}"
        )
        if args.apply:
            cur.execute("UPDATE art SET media_id = ? WHERE art_id = ?", (target_id, art_id))
            applied += 1

    if args.apply:
        conn.commit()
        print(f"\n{applied} row(s) updated and committed.")
    else:
        print("\nDry run only -- pass --apply to write these changes.")

    conn.close()


if __name__ == "__main__":
    main()
