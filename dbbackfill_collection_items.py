#!/usr/bin/env python3
"""
Backfill collection_item rows from movie.idSet for existing databases.

Movies imported before the scanner fix only have movie.idSet populated.
This script copies those associations into collection_item so they are
discoverable via videodb://collections/{id}/ without a full re-import.

Usage:
    python dbbackfill_collection_items.py
"""

import sqlite3
import os
import glob

BASE = r"f:\work\Kodi\Multi Collections\kodi-build.x64\Release"

# Find the DB - it may be under portable_data or directly under userdata
candidates = glob.glob(os.path.join(BASE, "**", "MyVideos*.db"), recursive=True)
if not candidates:
    print(f"ERROR: No MyVideos*.db found under {BASE}")
    exit(1)

DB_PATH = candidates[0]
print(f"Using DB: {DB_PATH}")

db = sqlite3.connect(DB_PATH)
db.row_factory = sqlite3.Row
c = db.cursor()

# Count before
c.execute("SELECT COUNT(*) FROM collection_item")
before = c.fetchone()[0]
print(f"collection_item rows before: {before}")

# Find all movies with a valid idSet that are not yet in collection_item
c.execute("""
    SELECT m.idMovie, m.idSet
    FROM movie m
    WHERE m.idSet IS NOT NULL
      AND m.idSet > 0
      AND NOT EXISTS (
          SELECT 1 FROM collection_item ci
          WHERE ci.idCollection = m.idSet
            AND ci.mediaType = 'movie'
            AND ci.idMedia = m.idMovie
      )
""")
rows = c.fetchall()
print(f"Movies needing collection_item rows: {len(rows)}")

inserted = 0
skipped = 0
for row in rows:
    id_movie = row["idMovie"]
    id_set = row["idSet"]
    # Verify the collection exists (should always be true after AddSet)
    c.execute("SELECT 1 FROM collection WHERE idCollection=?", (id_set,))
    if not c.fetchone():
        print(f"  SKIP: no collection row for idSet={id_set} (movie {id_movie})")
        skipped += 1
        continue
    c.execute(
        "INSERT INTO collection_item (idCollection, mediaType, idMedia, sortOrder, groupName) "
        "VALUES (?, 'movie', ?, 0, '')",
        (id_set, id_movie)
    )
    inserted += 1

db.commit()

# Count after
c.execute("SELECT COUNT(*) FROM collection_item")
after = c.fetchone()[0]
print(f"collection_item rows after: {after}")
print(f"Inserted: {inserted}, Skipped: {skipped}")
db.close()
print("Done.")
