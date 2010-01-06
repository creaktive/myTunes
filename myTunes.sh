#!/bin/sh
MEDIA="/private/var/mobile/Media"
LIBFILE="$MEDIA/iTunes_Control/iTunes/iTunes Library.itlp/Library.itdb"
LOCFILE="$MEDIA/iTunes_Control/iTunes/iTunes Library.itlp/Locations.itdb"
ITFILES="$MEDIA/iTunes_Control/Music"
MYTUNES="$MEDIA/myTunes"

rm -Rf "$MYTUNES"
mkdir -p "$MYTUNES"
cd "$MYTUNES"

(
sqlite3 -list "$LIBFILE" << SQLITE_END | tr ':?*' '___'
	ATTACH '$LOCFILE' AS loc;
	SELECT '$ITFILES/' || location, title || SUBSTR(location, 9) FROM item LEFT JOIN loc.location ON (pid = item_pid) ORDER BY location ASC;
	DETACH DATABASE loc;
SQLITE_END
) | while read line
do
	file=$(echo $line | cut -d '|' -f1)
	name=$(echo $line | cut -d '|' -f2)
	$(ln "$file" "$name")
done
