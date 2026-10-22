# HD Remaster camera and cut-screen metadata

`camera.h` provides read-only helpers for the observed room-camera and
cut-screen resource layouts. The data stays in its original coordinate system.
The helpers do not construct projections, select cameras, render effects or
implement widescreen following. They have no dependencies on other HD readers.

## XFS version 15 camera arrays

`decodeCameras` accepts the observed two-class schema, rather than arbitrary
XFS object graphs. The header contains `XFS\0`, version 15, an object count at
byte 8, a class count at byte 16 and a data-section offset at byte 20. Class
offsets begin at byte 24. Class, property-name and data offsets are relative to
byte 24; arithmetic and descriptor spans are checked before dereferencing them.
Each class has an eight-byte header followed by 24-byte property descriptors.
The reader checks property counts, names, type IDs and declared sizes:

| Class | Property | Type | Size |
| --- | --- | --- | --- |
| Root | `mAutoDelete` | 3 | 1 |
| Root | `mpArray` | 0xa002 | 4 |
| Camera | `mCameraPos` | 20 | 16 |
| Camera | `mTargetPos` | 20 | 16 |
| Camera | `mRoll` | 12 | 4 |
| Camera | `mFovy` | 12 | 4 |

Objects start with a 16-bit class tag, a 16-bit sequential object ID and a
32-bit length. The odd class tag is `(class index << 1) | 1`. The length includes
its own four bytes, excluding the class tag and ID. Each serialized property is
preceded by a 32-bit element count. A camera object has length 60 and occupies
64 bytes. Root length, array count, header object count and complete input
length must agree. Only the observed scalar/object-array encodings are accepted.

The reader accepts 1–256 cameras and values within ±100,000,000; field of view
must be strictly between 0 and 180. These are reader acceptance bounds, not
claims about every possible XFS resource. NaN and infinity are rejected. Vector
fourth components and serialized order are retained. Roll and field of view are
returned as stored, without unit conversion. Empty camera arrays, other schemas
and reference graphs are unsupported. Failure clears the output array.

## Version-1 rCutScrParam

`decodeCutScreenParams` reads the observed 44-byte little-endian resource:

| Offset | Field | Representation |
| --- | --- | --- |
| 0 | Version | uint32, 1 |
| 4 / 8 / 12 / 16 / 20 | Movie / depth / shadow / mirror / mirror-mask flags | Five uint32 booleans |
| 24 | `mBGTextureMax` | uint32 layer count |
| 28 | `mIsPlayerScroll` | uint32 boolean |
| 32 | `mOffsetY` | float32 |
| 36 | `mScrollRange` | float32 |
| 40 | `mScrollRangeOffset` | float32 |

All five effect flags and the full unsigned layer count are retained, including
zero-layer records and counts above eight. Reading metadata does not establish
that the corresponding effects or layer composition can be rendered. The reader
does not allocate per layer or impose a renderer's effect-combination limits.

Framing values must be finite, with offset in [-10000, 10000], range in [0, 1)
and range offset in [-10, 10]. Those bounds retain the prototype reader's
acceptance profile. Failure resets the complete output to its declared defaults.
Both readers clear the error string on success and set it on failure.

## Tests

`test/engines/reevengi/hd/camera.h` builds synthetic reflected classes, object
arrays and cut-screen records in memory. No retail camera values or executable
bytes are included or needed. With Reevengi configured as a static engine,
`make test` covers decoded fields, multi-camera order, count bounds, relative
offsets, schema mismatches, object lengths, invalid floats, failure cleanup and
independent screen flags. Widescreen and custom-camera behavior are excluded.
