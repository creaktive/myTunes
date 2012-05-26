#!/bin/sh
MEDIA="/private/var/mobile/Media"
LIBFILE="$MEDIA/iTunes_Control/iTunes/MediaLibrary.sqlitedb"
MYTUNES="$MEDIA/myTunes"

rm -Rf "$MYTUNES"
mkdir -p "$MYTUNES"
cd "$MYTUNES"

(
sqlite3 -list "$LIBFILE" << SQLITE_END | tr ':?*' '___'
    SELECT
        '$MEDIA/' || path || '/' || location,
        album_artist || ' - ' || album || ' - ' || title || SUBSTR(location, 5)
    FROM item
    LEFT JOIN base_location
        ON item.base_location_id = base_location.base_location_id
    LEFT JOIN item_extra
        ON item.item_pid = item_extra.item_pid
    LEFT JOIN album_artist
        ON item.album_artist_pid = album_artist.album_artist_pid
    LEFT JOIN album
        ON item.album_pid = album.album_pid;
SQLITE_END
) | while read line
do
	file=$(echo $line | cut -d '|' -f1)
	name=$(echo $line | cut -d '|' -f2)
	#echo "$file" "$name"
	$(ln "$file" "$name")
done
