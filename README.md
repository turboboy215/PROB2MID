# PROB2MID
Probe Software (GB/GBC) to MIDI converter
This tool converts music (and sound effects) from games for various systems using Probe Software's sound engines to MIDI format.

It works with ROM images for all systems, except SNES, which works with SPC files. To use it, you must specify the name of the ROM followed by the number of the bank containing the sound data (in hex). An additional parameter "GB", "GG", "SNES", or "SMD" should be used to specify the system afterwards (default is GB).

For SNES, the bank number is meaningless, so it can always be set to 1. There are three different versions of the driver (2 is the common version), the last of which is very different than the first two, and the program will try to automatically determine the version. However, this doesn't always work, and won't automatically choose the first (early) version, so if you get incorrect MIDIs, an additional parameter "1", "2", or "3" should be used. (e.g. Chuck Rock needs the "1" parameter)

Additionally for SNES SPC files, all the current songs in RAM will be converted. Some songs will be split into intro/loop sequences. For version 3, there are also some false positives that will be converted in addition to the "real" songs, which are either sound effects or invalid data. These can be deleted. This is done so that all the "real" songs in the SPC are present and converted.

Two batch files, "convall.bat" and "convall1.bat" are included to batch-convert SPC files (convall1.bat forces version 1).

For MegaDrive/32X, the bank parameter is instead the address of the start of the song pointer table in the ROM.

Examples:
* PROB2MID "Jelly Boy (U).gb" 3
* PROB2MID "Mortal Kombat II (U) [!].gb" 10
* PROB2MID "Bust-A-Move 2 - Arcade Edition (U).gb" 8
* PROB2MID "Batman Forever (U) [!].gg" 7 GG
* PROB2MID "01 Title Theme.spc" 1 SNES
* PROB2MID "Judge Dredd - The Movie (W) [!].bin" 1E3C24 SMD
This tool was based on my own reverse-engineering of the Game Boy sound engine from Jelly Boy, as well as the official source code for an earlier version of the SNES sound engine.

This tool was based on my own reverse-engineering of the sound engine from Jelly Boy.

Supported games:

SNES:
  * ACME Animation Factory
  * Alien 3
  * Batman Forever
  * Beauty and the Beast
  * Chuck Rock
  * FIFA Soccer '96
  * The Hurricanes
  * Jelly Boy
  * Judge Dredd
  * Kawasaki Superbike Challenge
  * Mick & Mack As the Global Gladiators (prototype)
  * The Pagemaster
  * Power Monger
  * Stargate
  * Super Shadow of the Beast
  * T2: The Arcade Game
  * Total Football (prototype)

Game Boy (Color):
  * Batman Forever
  * Bust-A-Move 2: Arcade Edition
  * Bust-A-Move 3 DX
  * Bust-A-Move 4
  * FIFA International Soccer
  * FIFA Soccer '96
  * Jelly Boy
  * Judge Dredd
  * Maya the Bee & Her Friends (prototype: South Park)
  * NFL Quarterback Club II
  * NFL Quarterback Club '96
  * NHL '96
  * NHL Hockey '95
  * The Pagemaster
  * Primal Rage
  * WWF Attitude
  * WWF War Zone

MegaDrive/32X:
 * Batman Forever
 * Bugs Bunny in Double Trouble
 * Judge Dredd
 * Primal Rage (also on 32X)
 * Stargate

Game Gear:
 * Batman Forever
 * Bugs Bunny in Double Trouble
 * FIFA Soccer '96
 * Judge Dredd
 * Primal Rage
 * Stargate

## To do:
  * SNES ROM file support? (will likely have to deal with decompression)
  * GBS file support
