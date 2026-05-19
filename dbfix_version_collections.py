import sqlite3

db = r'kodi-build.x64\Release\portable_data\userdata\Database\MyVideos145.db'
con = sqlite3.connect(db)
cur = con.cursor()

# Find collection_item rows where idMedia is a non-primary version file ID
cur.execute("""
  SELECT ci.idCollection, ci.idMedia, vv.idMedia as primaryMovieId
  FROM collection_item ci
  JOIN videoversion vv ON vv.idFile = ci.idMedia AND vv.media_type = 'movie'
  WHERE ci.mediaType = 'movie' AND vv.idMedia != ci.idMedia
""")
rows = cur.fetchall()
print('Spurious rows:', rows)

for idCol, idMedia, primaryId in rows:
    cur.execute('DELETE FROM collection_item WHERE idCollection=? AND mediaType=? AND idMedia=?', (idCol, 'movie', idMedia))
    print(f'Deleted (col={idCol}, idMedia={idMedia}), primary was {primaryId}')
    cur.execute(
        'INSERT OR IGNORE INTO collection_item(idCollection, mediaType, idMedia, sortOrder, groupName) VALUES (?,?,?,0,"")',
        (idCol, 'movie', primaryId)
    )
    print(f'Ensured correct row (col={idCol}, idMedia={primaryId})')

con.commit()
con.close()
print('Done')
