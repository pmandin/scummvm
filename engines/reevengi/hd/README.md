# HD Remaster resource readers

These read-only helpers provide a first step toward Resident Evil HD Remaster
research in Reevengi. They do not add game detection or playable HD support.
They use ScummVM's existing stream, compression and surface APIs.

## ARC version 7

`ArcIndex::load` reads the archive index from a caller-owned seekable stream.
Each 80-byte entry contains a 64-byte NUL-terminated resource path, resource
type, compressed size, unpacked size/flags and absolute payload offset.
The low 29 bits of the size/flags word hold the unpacked size; the remaining
three bits are retained as metadata. Names and payload bounds are checked
before the index is published. A failed load clears the previous index.

`ArcIndex::readEntry` reads and inflates one zlib-compressed entry using the
same archive stream. It checks the exact unpacked size. Compressed and
unpacked buffers are each capped at 64 MiB, and empty entries are rejected.
This is a reader resource limit rather than a format limit. Failed reads
clear the output buffer.

The layout was cross-checked with Albam's
[ARC schema](https://github.com/Brachi/albam/blob/d1fda41beeae129fbb26128157511afdee4002ea/albam/engines/mtfw/structs/arc.ksy).

## Tests

The CxxTest suite in `test/engines/reevengi/arc.h` constructs synthetic resources
in memory; no retail assets or executable bytes are included or needed.
Configure Reevengi as a static engine and run `make test`.
