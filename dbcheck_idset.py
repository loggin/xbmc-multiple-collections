import sqlite3
db = r'kodi-build.x64\Release\portable_data\userdata\Database\MyVideos145.db'
con = sqlite3.connect(db)
cur = con.cursor()

# Show current idSet for Ant-Man movies in movie_view
cur.execute("SELECT idMovie, c00, idSet FROM movie_view WHERE c00 LIKE '%Ant-Man%' AND isDefaultVersion=1")
print("Current idSet for Ant-Man movies:")
for r in cur.fetchall():
    print(f"  idMovie={r[0]}, title={r[1]}, idSet={r[2]}")

# Show collection_item for the affected movies
print()
cur.execute("SELECT ci.idCollection, col.name, ci.idMedia FROM collection_item ci JOIN collection col ON col.idCollection=ci.idCollection WHERE ci.idMedia IN (697,699,700) AND ci.mediaType='movie'")
print("collection_item for Ant-Man movies:")
for r in cur.fetchall():
    print(f"  idCollection={r[0]}, name={r[1]}, idMedia={r[2]}")

con.close()
