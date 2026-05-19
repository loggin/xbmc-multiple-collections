import sqlite3

db = r'kodi-build.x64\Release\portable_data\userdata\Database\MyVideos145.db'
con = sqlite3.connect(db)
cur = con.cursor()

# Simulate the new ci_primary subquery logic
cur.execute("""
  SELECT DISTINCT ci_outer.idMedia,
    (SELECT ci2.idCollection
     FROM collection_item ci2
     INNER JOIN (
       SELECT idCollection, COUNT(*) AS cnt
       FROM collection_item
       WHERE mediaType = 'movie'
       GROUP BY idCollection
     ) cc ON cc.idCollection = ci2.idCollection
     WHERE ci2.idMedia = ci_outer.idMedia AND ci2.mediaType = 'movie'
     ORDER BY cc.cnt DESC, ci2.idCollection ASC
     LIMIT 1
    ) AS idCollection
  FROM collection_item ci_outer
  WHERE ci_outer.mediaType = 'movie'
  ORDER BY ci_outer.idMedia
""")
rows = cur.fetchall()
print("idMedia -> primary idCollection (most members, tie-break oldest):")
for idMedia, idCol in rows:
    cur2 = con.cursor()
    cur2.execute("SELECT c00 FROM movie WHERE idMovie=?", (idMedia,))
    title_row = cur2.fetchone()
    title = title_row[0] if title_row else "?"
    cur2.execute("SELECT name, (SELECT COUNT(*) FROM collection_item WHERE idCollection=? AND mediaType='movie') FROM collection col WHERE idCollection=?", (idCol, idCol))
    col_row = cur2.fetchone()
    col_name = col_row[0] if col_row else "?"
    col_cnt = col_row[1] if col_row else 0
    print(f"  idMedia={idMedia} ({title}) -> col {idCol} ({col_name}, {col_cnt} members)")

con.close()
