/*Probe Software (GB/GBC) to MIDI converter*/
/*By Will Trowbridge*/
/*Portions based on code by ValleyBell*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>

#define bankSize 16384

FILE* rom, * mid;
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
unsigned static char* midData;
unsigned static char* ctrlMidData;
long midLength;

const char MagicBytes[6] = { 0xF1, 0x87, 0x26, 0x00, 0x6F, 0x11 };

/*Function prototypes*/
unsigned short ReadLE16(unsigned char* Data);
unsigned short ReadBE16(unsigned char* Data);
static void Write8B(unsigned char* buffer, unsigned int value);
static void WriteBE32(unsigned char* buffer, unsigned long value);
static void WriteBE24(unsigned char* buffer, unsigned long value);
static void WriteBE16(unsigned char* buffer, unsigned int value);
unsigned int WriteNoteEvent(unsigned static char* buffer, unsigned int pos, unsigned int note, int length, int delay, int firstNote, int curChan, int inst);
int WriteDeltaTime(unsigned static char* buffer, unsigned int pos, unsigned int value);
void song2mid(int songNum, long ptr);

/*Convert little-endian pointer to big-endian*/
unsigned short ReadLE16(unsigned char* Data)
{
	return (Data[0] << 0) | (Data[1] << 8);
}

/*Store big-endian pointer*/
unsigned short ReadBE16(unsigned char* Data)
{
	return (Data[0] << 8) | (Data[1] << 0);
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

unsigned int WriteNoteEvent(unsigned static char* buffer, unsigned int pos, unsigned int note, int length, int delay, int firstNote, int curChan, int inst)
{
	int deltaValue;
	deltaValue = WriteDeltaTime(buffer, pos, delay);
	pos += deltaValue;

	if (firstNote == 1)
	{
		Write8B(&buffer[pos], 0xC0 | curChan);

		Write8B(&buffer[pos + 1], inst);
		Write8B(&buffer[pos + 2], 0);

		Write8B(&buffer[pos + 3], 0x90 | curChan);

		pos += 4;
	}

	Write8B(&buffer[pos], note);
	pos++;
	Write8B(&buffer[pos], curVol);
	pos++;

	deltaValue = WriteDeltaTime(buffer, pos, length);
	pos += deltaValue;

	Write8B(&buffer[pos], note);
	pos++;
	Write8B(&buffer[pos], 0);
	pos++;

	return pos;

}

int WriteDeltaTime(unsigned static char* buffer, unsigned int pos, unsigned int value)
{
	unsigned char valSize;
	unsigned char* valData;
	unsigned int tempLen;
	unsigned int curPos;

	valSize = 0;
	tempLen = value;

	while (tempLen != 0)
	{
		tempLen >>= 7;
		valSize++;
	}

	valData = &buffer[pos];
	curPos = valSize;
	tempLen = value;

	while (tempLen != 0)
	{
		curPos--;
		valData[curPos] = 128 | (tempLen & 127);
		tempLen >>= 7;
	}

	valData[valSize - 1] &= 127;

	pos += valSize;

	if (value == 0)
	{
		valSize = 1;
	}
	return valSize;
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
					if (songPtr >= tableOffset)
					{
						stopCvt = 1;
					}
					else
					{
						printf("Song %i: 0x%04X\n", songNum, songPtr);
						song2mid(songNum, songPtr);
						i += 2;
						songNum++;
					}

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

void song2mid(int songNum, long ptr)
{
	int numChan = 0;
	int chanPtr = 0;
	long romPos = 0;
	long seqPos = 0;
	int seqEnd = 0;
	int curNote = 0;
	int curNoteLen = 0;
	unsigned int curDelay = 0;
	unsigned int ctrlDelay = 0;
	unsigned int masterDelay = 0;
	int curTrack = 0;
	int noteSize = 0;
	int restTime = 0;
	int tempo = 0;
	int ticks = 120;
	int noteOn = 0;
	int repeat1Times = 0;
	int repeat1Pos = 0;
	int repeat2Times = 0;
	int repeat2Pos = 0;
	unsigned char command[8];
	unsigned int midPos = 0;
	unsigned int ctrlMidPos = 0;
	long midTrackBase = 0;
	long ctrlMidTrackBase = 0;
	int firstNote = 1;
	int timeVal = 0;
	int holdNote = 0;
	int lastNote = 0;

	int valSize = 0;

	int tempByte = 0;
	long tempPos = 0;
	long trackSize = 0;
	long delayPos = 0;


	midPos = 0;
	ctrlMidPos = 0;

	midLength = 0x10000;
	midData = (unsigned char*)malloc(midLength);

	ctrlMidData = (unsigned char*)malloc(midLength);

	for (j = 0; j < midLength; j++)
	{
		midData[j] = 0;
		ctrlMidData[j] = 0;
	}

	sprintf(outfile, "song%d.mid", songNum);
	if ((mid = fopen(outfile, "wb")) == NULL)
	{
		printf("ERROR: Unable to write to file song%d.mid!\n", songNum);
		exit(2);
	}
	else
	{
		romPos = ptr - bankAmt;
		numChan = romData[romPos];
		romPos++;

		/*Write MIDI header with "MThd"*/
		WriteBE32(&ctrlMidData[ctrlMidPos], 0x4D546864);
		WriteBE32(&ctrlMidData[ctrlMidPos + 4], 0x00000006);
		ctrlMidPos += 8;

		WriteBE16(&ctrlMidData[ctrlMidPos], 0x0001);
		WriteBE16(&ctrlMidData[ctrlMidPos + 2], numChan + 1);
		WriteBE16(&ctrlMidData[ctrlMidPos + 4], ticks);
		ctrlMidPos += 6;

		/*Write initial MIDI information for "control" track*/
		WriteBE32(&ctrlMidData[ctrlMidPos], 0x4D54726B);
		ctrlMidPos += 8;
		ctrlMidTrackBase = ctrlMidPos;

		/*Set channel name (blank)*/
		WriteDeltaTime(ctrlMidData, ctrlMidPos, 0);
		ctrlMidPos++;
		WriteBE16(&ctrlMidData[ctrlMidPos], 0xFF03);
		Write8B(&ctrlMidData[ctrlMidPos + 2], 0);
		ctrlMidPos += 2;

		/*Set initial tempo*/
		WriteDeltaTime(ctrlMidData, ctrlMidPos, 0);
		ctrlMidPos++;
		WriteBE32(&ctrlMidData[ctrlMidPos], 0xFF5103);
		ctrlMidPos += 4;

		WriteBE24(&ctrlMidData[ctrlMidPos], 60000000 / 150);
		ctrlMidPos += 3;

		/*Set time signature*/
		WriteDeltaTime(ctrlMidData, ctrlMidPos, 0);
		ctrlMidPos++;
		WriteBE24(&ctrlMidData[ctrlMidPos], 0xFF5804);
		ctrlMidPos += 3;
		WriteBE32(&ctrlMidData[ctrlMidPos], 0x04021808);
		ctrlMidPos += 4;

		/*Set key signature*/
		WriteDeltaTime(ctrlMidData, ctrlMidPos, 0);
		ctrlMidPos++;
		WriteBE24(&ctrlMidData[ctrlMidPos], 0xFF5902);
		ctrlMidPos += 4;

		for (curTrack == 0; curTrack < numChan; curTrack++)
		{
			firstNote = 1;

			curDelay = 0;
			ctrlDelay = 0;

			curNote = 0;
			lastNote = 0;
			curNoteLen = 0;

			masterDelay = 0;

			repeat1Times = -1;
			repeat1Pos = -1;
			repeat2Times = -1;
			repeat2Pos = -1;
			delayPos = 0;

			/*Add track header*/
			/*Write MIDI chunk header with "MTrk"*/
			WriteBE32(&midData[midPos], 0x4D54726B);
			midPos += 8;
			midTrackBase = midPos;

			/*Calculate MIDI channel size*/
			trackSize = midPos - midTrackBase;
			WriteBE16(&midData[midTrackBase - 2], trackSize);

			chanPtr = ReadLE16(&romData[romPos]);

			seqPos = ptr + chanPtr - bankAmt;
			seqEnd = 0;
			noteOn = 0;
			curInst = 0;
			curVol = 100;
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

				/*Delay OR play note*/
				if (command[0] < 0x80)
				{
					if (noteOn == 0)
					{
						if (seqPos != delayPos)
						{
							curDelay += command[0] * 2;
						}
						seqPos++;
						noteOn = 1;
					}
					else if (noteOn == 1)
					{
						curNote = command[0] - 12;
						curVol = command[1] * 0.6;
						delayPos = seqPos + 2;

						if (command[2] < 0x80)
						{
							curNoteLen = command[2] * 2;
						}
						else
						{
							curNoteLen = (ReadBE16(&romData[seqPos + 2]) - 0x8000) * 2;
						}

						if (curNoteLen >= noteSize)
						{
							tempPos = WriteNoteEvent(midData, midPos, curNote, noteSize, curDelay, firstNote, curTrack, curInst);
							curDelay = curNoteLen - noteSize;
							firstNote = 0;
							midPos = tempPos;
						}

						else
						{
							tempPos = WriteNoteEvent(midData, midPos, curNote, curNoteLen, curDelay, firstNote, curTrack, curInst);
							firstNote = 0;
							curDelay = 0;
							midPos = tempPos;
						}
						seqPos += 2;
						noteOn = 0;
					}
				}

				/*Delay (long)*/
				else if (command[0] >= 0x80 && command[0] < 0x90)
				{
					if (seqPos != delayPos)
					{
						curDelay += (ReadBE16(&romData[seqPos]) - 0x8000) * 2;
					}
					seqPos += 2;
					noteOn = 1;
				}

				/*Set vibrato?*/
				else if (command[0] >= 0x90 && command[0] < 0x9F)
				{
					noteSize = 60;
					seqPos++;
				}

				/*Set note size*/
				else if (command[0] == 0x9F)
				{
					if (command[1] < 0x80)
					{
						noteSize = command[1] * 2;
						seqPos += 2;
					}
					else
					{
						noteSize = (ReadBE16(&romData[seqPos]) - 0x8000) * 2;
						seqPos += 3;
					}
				}

				/*Rest*/
				else if (command[0] > 0x9F && command[0] < 0xF1)
				{
					curDelay = command[0];
					seqPos++;
				}

				/*Set envelope effect*/
				else if (command[0] == 0xF1)
				{
					seqPos += 2;
					noteOn = 0;
				}

				/*Set SFX priority?*/
				else if (command[0] == 0xF2)
				{
					seqPos += 2;
					noteOn = 0;
				}

				/*Set bit 1 of envelope to 1*/
				else if (command[0] == 0xF5)
				{
					seqPos += 2;
					noteOn = 0;
				}

				/*Set bit 1 of envelope to 0*/
				else if (command[0] == 0xF6)
				{
					seqPos += 2;
					noteOn = 0;
				}

				/*Unknown command F7*/
				else if (command[0] == 0xF7)
				{
					seqPos += 2;
					noteOn = 0;
				}

				/*Set SFX priority*/
				else if (command[0] == 0xF8)
				{
					seqPos += 2;
					noteOn = 0;
				}

				/*Set bit 1 of envelope to 1 (v2)*/
				else if (command[0] == 0xF9)
				{
					seqPos += 2;
					noteOn = 0;
				}

				/*Repeat section # of times*/
				else if (command[0] == 0xFA)
				{
					repeat1Times = command[1];
					repeat1Pos = seqPos + 2;
					seqPos += 2;
					noteOn = 0;
				}

				/*End of repeat*/
				else if (command[0] == 0xFB)
				{
					if (repeat1Times > 1)
					{
						seqPos = repeat1Pos;
						repeat1Times--;
						noteOn = 0;
					}
					else
					{
						seqPos += 2;
						noteOn = 0;
						repeat1Times = -1;
					}

				}

				/*Set tempo*/
				else if (command[0] == 0xFC)
				{

					ctrlMidPos++;
					valSize = WriteDeltaTime(ctrlMidData, ctrlMidPos, ctrlDelay);
					ctrlDelay = 0;
					ctrlMidPos += valSize;
					WriteBE24(&ctrlMidData[ctrlMidPos], 0xFF5103);
					ctrlMidPos += 3;
					if (command[1] >= 0x80)
					{
						tempo = ((ReadBE16(&romData[seqPos + 1])) - 0x8000) * 1.66;
						WriteBE24(&ctrlMidData[ctrlMidPos], 60000000 / tempo);
						ctrlMidPos += 2;
						seqPos += 3;
						noteOn = 0;
					}
					else
					{
						tempo = command[1] * 1.66;
						WriteBE24(&ctrlMidData[ctrlMidPos], 60000000 / tempo);
						ctrlMidPos += 2;
						seqPos += 2;
						noteOn = 0;
					}
				}

				/*Skip*/
				else if (command[0] == 0xFD)
				{
					seqPos += 2;
					noteOn = 0;
				}

				/*Set instrument*/
				else if (command[0] == 0xFE)
				{
					curInst = command[1];
					firstNote = 1;
					seqPos += 2;
					noteOn = 0;
				}

				/*End of sequence*/
				else if (command[0] == 0xFF)
				{
					seqEnd = 1;
				}

				else
				{
					seqEnd = 1;
					stopCvt = 1;
				}
			}
			/*End of track*/
			WriteBE32(&midData[midPos], 0xFF2F00);
			midPos += 4;

			/*Calculate MIDI channel size*/
			trackSize = midPos - midTrackBase;
			WriteBE16(&midData[midTrackBase - 2], trackSize);

			romPos += 2;
		}
		/*End of control track*/
		ctrlMidPos++;
		WriteBE32(&ctrlMidData[ctrlMidPos], 0xFF2F00);
		ctrlMidPos += 4;

		/*Calculate MIDI channel size*/
		trackSize = ctrlMidPos - ctrlMidTrackBase;
		WriteBE16(&ctrlMidData[ctrlMidTrackBase - 2], trackSize);

		sprintf(outfile, "song%d.mid", songNum);
		fwrite(ctrlMidData, ctrlMidPos, 1, mid);
		fwrite(midData, midPos, 1, mid);
		fclose(mid);
	}
}