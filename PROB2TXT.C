/*Probe Software (GB/GBC) to MIDI converter*/
/*By Will Trowbridge*/
/*Portions based on code by ValleyBell*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>

#define bankSize 16384

FILE* rom, * txt;
long bank;
long offset;
long tablePtrLoc;
long tableOffset;
int i, j;
char outfile[1000000];
int songNum;
long songPtr;
int chanMask;
long bankAmt;
int foundTable = 0;
long firstPtr = 0;
int curInst = 0;
int curVol = 0;
int stopCvt = 0;

unsigned static char* romData;

const char MagicBytes[6] = { 0xF1, 0x87, 0x26, 0x00, 0x6F, 0x11 };

/*Function prototypes*/
unsigned short ReadLE16(unsigned char* Data);
static void Write8B(unsigned char* buffer, unsigned int value);
static void WriteBE32(unsigned char* buffer, unsigned long value);
static void WriteBE24(unsigned char* buffer, unsigned long value);
static void WriteBE16(unsigned char* buffer, unsigned int value);
void song2txt(int songNum, long ptr);

/*Convert little-endian pointer to big-endian*/
unsigned short ReadLE16(unsigned char* Data)
{
	return (Data[0] << 0) | (Data[1] << 8);
}

static void Write8B(unsigned char* buffer, unsigned int value)
{
	buffer[0x00] = value;
}

static void WriteBE32(unsigned char* buffer, unsigned long value)
{
	buffer[0x00] = (value & 0xFF000000) >> 24;
	buffer[0x01] = (value & 0x00FF0000) >> 16;
	buffer[0x02] = (value & 0x0000FF00) >> 8;
	buffer[0x03] = (value & 0x000000FF) >> 0;

	return;
}

static void WriteBE24(unsigned char* buffer, unsigned long value)
{
	buffer[0x00] = (value & 0xFF0000) >> 16;
	buffer[0x01] = (value & 0x00FF00) >> 8;
	buffer[0x02] = (value & 0x0000FF) >> 0;

	return;
}

static void WriteBE16(unsigned char* buffer, unsigned int value)
{
	buffer[0x00] = (value & 0xFF00) >> 8;
	buffer[0x01] = (value & 0x00FF) >> 0;

	return;
}

int main(int args, char* argv[])
{
	printf("Probe Software (GB/GBC) to TXT converter\n");
	if (args != 3)
	{
		printf("Usage: PROB2TXT <rom> <bank>\n");
		return -1;
	}
	else
	{
		if ((rom = fopen(argv[1], "rb")) == NULL)
		{
			printf("ERROR: Unable to open file %s!\n", argv[1]);
			exit(1);
		}
		else
		{
			bank = strtol(argv[2], NULL, 16);
			if (bank != 1)
			{
				bankAmt = bankSize;
			}
			else
			{
				bankAmt = 0;
			}
			fseek(rom, ((bank - 1) * bankSize), SEEK_SET);
			romData = (unsigned char*)malloc(bankSize);
			fread(romData, 1, bankSize, rom);
			fclose(rom);

			/*Try to search the bank for song table loader*/
			for (i = 0; i < bankSize; i++)
			{
				if ((!memcmp(&romData[i], MagicBytes, 6)) && foundTable != 1)
				{
					tablePtrLoc = bankAmt + i + 6;
					printf("Found pointer to song table at address 0x%04x!\n", tablePtrLoc);
					tableOffset = ReadLE16(&romData[tablePtrLoc - bankAmt]);
					printf("Song table starts at 0x%04x...\n", tableOffset);
					foundTable = 1;
				}
			}

			if (foundTable == 1)
			{
				i = tableOffset - bankAmt;
				songNum = 1;
				while (ReadLE16(&romData[i]) >= bankAmt && ReadLE16(&romData[i]) < (bankAmt * 2) && stopCvt == 0)
				{
					songPtr = ReadLE16(&romData[i]);
					printf("Song %i: 0x%04X\n", songNum, songPtr);
					song2txt(songNum, songPtr);
					i += 2;
					songNum++;
				}
			}
			else
			{
				printf("ERROR: Magic bytes not found!\n");
				exit(1);
			}

			printf("The operation was successfully completed!\n");
			exit(0);
		}
	}
}

/*Convert the song data to TXT*/
void song2txt(int songNum, long ptr)
{
	int numChan = 0;
	int chanPtr = 0;
	long romPos = 0;
	long seqPos = 0;
	int seqEnd = 0;
	int curNote = 0;
	int curNoteLen = 0;
	int curDelay = 0;
	int curTrack = 0;
	int noteSize = 0;
	int restTime = 0;
	int tempo = 0;
	int noteOn = 0;
	int repeat1Times = 0;
	int repeat1Pos = 0;
	int repeat2Times = 0;
	int repeat2Pos = 0;
	unsigned char command[8];
	sprintf(outfile, "song%i.txt", songNum);
	if ((txt = fopen(outfile, "wb")) == NULL)
	{
		printf("ERROR: Unable to write to file song%i.txt!\n", songNum);
		exit(2);
	}
	else
	{
		romPos = ptr - bankAmt;
		numChan = romData[romPos];
		fprintf(txt, "Number of channels: %i\n", numChan);
		romPos++;

		for (curTrack == 0; curTrack < numChan; curTrack++)
		{
			chanPtr = ReadLE16(&romData[romPos]);
			fprintf(txt, "Pointer: %01X\n", chanPtr);

			seqPos = ptr + chanPtr - bankAmt;
			seqEnd = 0;
			noteOn = 0;
			while (seqEnd == 0 && seqPos < bankAmt)
			{
				command[0] = romData[seqPos];
				command[1] = romData[seqPos + 1];
				command[2] = romData[seqPos + 2];
				command[3] = romData[seqPos + 3];
				command[4] = romData[seqPos + 4];
				command[5] = romData[seqPos + 5];
				command[6] = romData[seqPos + 6];
				command[7] = romData[seqPos + 7];

				if (command[0] < 0x80)
				{
					if (noteOn == 0)
					{
						curDelay = command[0];
						fprintf(txt, "Delay: %01X\n", curDelay);
						seqPos++;
						noteOn = 1;
					}
					else if (noteOn == 1)
					{
						curNote = command[0];
						curVol = command[1];
						fprintf(txt, "Play note: %01X, volume: %01X\n", curNote, curVol);
						seqPos += 2;
						noteOn = 0;
					}

				}

				else if (command[0] >= 0x80 && command[0] < 0x90)
				{
					curDelay = command[1];
					fprintf(txt, "Delay (long): %01X, %01X\n", command[0], curDelay);
					seqPos += 2;
					noteOn = 1;
				}

				else if (command[0] >= 0x90 && command[0] < 0x9F)
				{
					fprintf(txt, "Set vibrato?: %01X\n", command[0]);
					seqPos++;
				}

				else if (command[0] == 0x9F)
				{
					if (command[1] < 0x80)
					{
						fprintf(txt, "Change note size: %01X\n", command[1]);
						seqPos += 2;
					}
					else
					{
						fprintf(txt, "Change note size (long): %01X, %01X\n", command[1], command[2]);
						seqPos += 3;
					}

				}

				else if (command[0] > 0x9F && command[0] < 0xF1)
				{
					fprintf(txt, "Rest: %01X\n", command[0]);
					seqPos++;
				}

				else if (command[0] == 0xF1)
				{
					fprintf(txt, "Set envelope effect: %01X\n", command[1]);
					seqPos += 2;
					noteOn = 0;
				}

				else if (command[0] == 0xF2)
				{
					fprintf(txt, "Set SFX priority?: %01X\n", command[1]);
					seqPos += 2;
					noteOn = 0;
				}

				else if (command[0] == 0xF5)
				{
					fprintf(txt, "Set bit 1 of envelope to 1: %01X\n", command[1]);
					seqPos += 2;
					noteOn = 0;
				}

				else if (command[0] == 0xF6)
				{
					fprintf(txt, "Set bit 1 of envelope to 0: %01X\n", command[1]);
					seqPos += 2;
					noteOn = 0;
				}

				else if (command[0] == 0xF7)
				{
					fprintf(txt, "Command F7: %01X\n", command[1]);
					seqPos += 2;
					noteOn = 0;
				}

				else if (command[0] == 0xF8)
				{
					fprintf(txt, "Set SFX priority: %01X\n", command[1]);
					seqPos += 2;
					noteOn = 0;
				}

				else if (command[0] == 0xF9)
				{
					fprintf(txt, "Set bit 1 of envelope to 1 (v2): %01X\n", command[1]);
					seqPos += 2;
					noteOn = 0;
				}

				else if (command[0] == 0xFA)
				{
					repeat1Times = command[1];
					repeat1Pos = seqPos + 2;
					fprintf(txt, "Repeat section: %01X times\n", command[1]);
					seqPos += 2;
					noteOn = 0;
				}

				else if (command[0] == 0xFB)
				{
					fprintf(txt, "End of repeat point: %01X\n", command[1]);
					seqPos += 2;
					noteOn = 0;
				}

				else if (command[0] == 0xFC)
				{
					tempo = command[1];
					fprintf(txt, "Set tempo: %01X\n", tempo);
					if (tempo >= 0x80)
					{
						tempo = command[2];
						seqPos++;
					}
					seqPos += 2;
					noteOn = 0;
				}

				else if (command[0] == 0xFD)
				{
					fprintf(txt, "(Skip): %01X\n", command[1]);
					seqPos += 2;
					noteOn = 0;
				}

				else if (command[0] == 0xFE)
				{
					curInst = command[1];
					fprintf(txt, "Set instrument: %01X\n", curInst);
					seqPos += 2;
					noteOn = 0;
				}

				else if (command[0] == 0xFF)
				{
					fprintf(txt, "End of sequence: %01X\n\n", command[1]);
					seqEnd = 1;
				}

				else
				{
					seqEnd = 1;
					stopCvt = 1;
				}

			}
			romPos += 2;

		}
		fclose(txt);
	}
}