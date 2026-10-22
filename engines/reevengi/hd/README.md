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

## TEX version 157

`decodeTexture` decodes mip zero of a single-image 2D texture to RGBA32:

* Formats 19/20: BC1, including the transparent palette entry.
* Formats 24/31/43: BC3, including both alpha palette modes.

Normal-map channels and GUI texture channels are retained without shader
color conversion or normal reconstruction. Partial edge blocks are cropped.
The header, offset-table length, mip-zero span and next-mip boundary are
checked before allocation. Other mip payloads are not decoded or validated.
The output is capped at 16 million pixels; failure frees the output surface.
The caller owns the decoded surface and must free it after use.

`decodeDepthTexture` reads the observed single-mip format-14 R32F layout.
Negative infinity is converted to zero and counted as an uncovered sentinel.
Finite nonnegative values, including values above one, are retained; samples
above one are counted separately. NaN, positive infinity and other negative
values are rejected. The payload must end at the end of the input buffer.
Failure resets all output fields.

The ARC and TEX layouts were cross-checked with Albam's
[ARC schema](https://github.com/Brachi/albam/blob/d1fda41beeae129fbb26128157511afdee4002ea/albam/engines/mtfw/structs/arc.ksy),
[TEX schema](https://github.com/Brachi/albam/blob/d1fda41beeae129fbb26128157511afdee4002ea/albam/engines/mtfw/structs/tex-157.ksy)
and [texture format map](https://github.com/Brachi/albam/blob/d1fda41beeae129fbb26128157511afdee4002ea/albam/engines/mtfw/texture.py).
No Blender dependency is required.

## Tests

The CxxTest suites in `test/engines/reevengi/arc.h` and `texture.h` construct
synthetic resources in memory; no retail assets or executable bytes are
included or needed. Configure Reevengi as a static engine and run `make test`.
