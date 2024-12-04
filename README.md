# PROB2MID
Probe Software (GB/GBC) to MIDI converter
This tool converts music (and sound effects) from Game Boy and Game Boy Color games using Probe Software's sound engine to MIDI format.

It works with ROM images. To use it, you must specify the name of the ROM followed by the number of the bank containing the sound data (in hex).

Examples:
* PROB2MID "Jelly Boy (U).gb" 3
* PROB2MID "Mortal Kombat II (U) [!].gb" 10
* PROB2MID "Bust-A-Move 2 - Arcade Edition (U).gb" 8

This tool was based on my own reverse-engineering of the sound engine from Jelly Boy.

Supported games:
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

## To do:
  * Support for other versions of the sound engine (SNES, MegaDrive, Game Gear)
  * GBS file support
