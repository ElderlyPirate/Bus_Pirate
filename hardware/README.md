# Bus Pirate hardware design files

This folder holds the original Bus Pirate v3/v4 board design sources and the
3D models that used to live under `Documentation/art`.  The Eagle schematics and
boards are the originals from the Dangerous Prototypes hardware repository;
the SketchUp models are the board/case models, not circuit sources.

## Revisions

| Folder | Content |
|---|---|
| `v3/` | v3a and v3b Eagle sources + PNG previews; the microBusPirate variant |
| `v3.5/` | v3.5, v3.5a–v3.5e, SOIC and SSOP Eagle sources + previews |
| `v3.6/` | v3.6 and v3.6a Eagle sources; v3.5e was renamed v3.6 |
| `v4/` | v4, v4a–v4e Eagle sources + previews |
| `lcd-adapter/v3a/` | the v3a LCD adapter 3D model and source render |

Revision notes: v3.5c replaced the 4066 control trace; v3.5e moved to the
DP6037 footprint and was renamed v3.6 to avoid confusion with the cases; v4.0
is the original/pre-production v4 board and v4c is a later v4 revision.

## Files

- `.sch` / `.brd` — Eagle schematic and board sources.
- `.sch.png` / `.brd.png` — previews for quick viewing.
- `art/*.skp` — SketchUp 3D models (board or case).  `.skb` backups were
  dropped.
- `lcd-adapter/v3a/BP LCD v3a.tif` — source render of the LCD adapter.

Gerber archives (ZIP/RAR) are intentionally not stored here; generate them
from the Eagle sources when needed.  No compressed archives are kept in the
repository.

## License

The hardware design files are the original Dangerous Prototypes work and are
distributed under **CC BY-SA 3.0**; see [COPYING.TXT](COPYING.TXT) and
[BusPirate-history.txt](BusPirate-history.txt).  This is separate from the
firmware license.
