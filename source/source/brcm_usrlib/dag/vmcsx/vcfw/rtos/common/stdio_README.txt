
fread.c is a modified replacement for MetaWare's fread.c,
intended to improve read performance on large files,
in the presence of fseek()s to locations which are not sector
aligned, and fread()s larger than the buffer size which are not
a multiple of the sector size.

See bug SW-5158

To avoid requiring a "suprising" checkout of the entire metaware
library, relevant metaware header files are DUPLICATED in
./metaware_internal_headers/*.h

A disadvantage at present is that reading to End-Of-File causes
a double read of the last sector of the file, if the file does not
end on a sector boundary (0 mod 512).
