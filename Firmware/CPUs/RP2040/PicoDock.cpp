
#if 0

/* Structure definition */
#define uchar unsigned char
#define ushort unsigned short
typedef struct {
	char devName[256];
	int speed;
	} NewtDevInfo;

/* Constants definition */
#define MaxHeadLen 256
#define MaxInfoLen 256
extern uchar LRFrame[];
extern uchar LDFrame[];

/* Function prototype */
int InitNewtDev(NewtDevInfo *newtDevInfo);
void FCSCalc(ushort *fcsWord, uchar octet);
void SendFrame(int newtFd, uchar *head, uchar *info, int infoLen);
void SendLTFrame(int newtFd, uchar seqNo, uchar *info, int infoLen);
void SendLAFrame(int newtFd, uchar seqNo);
int RecvFrame(int newtFd, uchar *frame);
int WaitLAFrame(int newtFd, uchar seqNo);
int WaitLDFrame(int newtFd);
void ErrHandler(char *errMesg);
void SigInt(int sigNo);

/* UnixNPI */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include "newtmnp.h"

#define VERSION "1.1.5"

#define TimeOut 30

/* Function prototype */
void SigAlrm(int sigNo);

#define NUM_STARS 32

static char* stars(int number) {
	static char string[NUM_STARS+1];
	int i;
	for(i=0; i<NUM_STARS; i++) {
		if(i<number) string[i]='*';
		else string[i]=' ';
	}
	string[NUM_STARS]='\0';
	return string;
}

int main(int argc, char *argv[])
{
	FILE *inFile;
	long inFileLen;
	long tmpFileLen;
	uchar sendBuf[MaxHeadLen + MaxInfoLen];
	uchar recvBuf[MaxHeadLen + MaxInfoLen];
	NewtDevInfo newtDevInfo;
	int newtFd;
	uchar ltSeqNo;
	int i, j, k;

	k = 1;

	/* Initialization */
	fprintf(stdout, "\n");
	fprintf(stdout, "UnixNPI - a Newton Package Installer for Unix platforms\n");
	fprintf(stdout, "Version " VERSION " by Richard C. L. Li, Victor Rehorst, Chayam Kirshen\n");
	fprintf(stdout, "patches by Phil <phil@squack.COM>, Heinrik Lipka\n");
	fprintf(stdout, "This program is distributed under the terms of the GNU GPL: see the file COPYING\n");

	strcpy(newtDevInfo.devName, "/dev/newton");

	/* Install time out function */
	if(signal(SIGALRM, SigAlrm) == SIG_ERR)
		ErrHandler("Error in setting up timeout function!!");

	/* Check arguments */
	if(argc < 2)
		ErrHandler("Usage: unixnpi [-s speed] [-d device] PkgFiles...");
	else
	{
		if (strcmp(argv[1],"-s") == 0)
		{
			if (argc < 4)
				ErrHandler("Usage: unixnpi [-s speed] [-d device] PkgFiles...");
			newtDevInfo.speed = atoi(argv[2]);
			k = 3;
		}
		else
		{
			newtDevInfo.speed = 38400;
			k = 1;
		}
		if (strcmp(argv[k],"-d")==0)
		{
			if (argc<(k+1))
				ErrHandler("Usage: unixnpi [-s speed] [-d device] PkgFiles...");
			strcpy(newtDevInfo.devName, argv[k+1]);
			k+=2;
		}
	}

	/* Initialize Newton device */
	if((newtFd = InitNewtDev(&newtDevInfo)) < 0)
		ErrHandler("Error in opening Newton device!!\nDo you have a symlink to /dev/newton?");
	ltSeqNo = 0;

	/* Waiting to connect */
	fprintf(stdout, "\nWaiting to connect\n");
	do {
		while(RecvFrame(newtFd, recvBuf) < 0);
	} while(recvBuf[1] != '\x01');
	fprintf(stdout, "Connected\n");
	fprintf(stdout, "Handshaking");
	fflush(stdout);

	/* Send LR frame */
	alarm(TimeOut);
	do {
		SendFrame(newtFd, LRFrame, NULL, 0);
		} while(WaitLAFrame(newtFd, ltSeqNo) < 0);
	ltSeqNo++;
	fprintf(stdout, ".");
	fflush(stdout);

	/* Wait LT frame newtdockrtdk */
	while(RecvFrame(newtFd, recvBuf) < 0 || recvBuf[1] != '\x04');
	SendLAFrame(newtFd, recvBuf[2]);
	fprintf(stdout, ".");
	fflush(stdout);

	/* Send LT frame newtdockdock */
	alarm(TimeOut);
	do {
		SendLTFrame(newtFd, ltSeqNo, "newtdockdock\0\0\0\4\0\0\0\4", 20);
	} while(WaitLAFrame(newtFd, ltSeqNo) < 0);
	ltSeqNo++;
	fprintf(stdout, ".");
	fflush(stdout);

	/* Wait LT frame newtdockname */
	alarm(TimeOut);
	while(RecvFrame(newtFd, recvBuf) < 0 || recvBuf[1] != '\x04');
	SendLAFrame(newtFd, recvBuf[2]);
	fprintf(stdout, ".");
	fflush(stdout);

	/* Get owner name */
	i = recvBuf[19] * 256 * 256 * 256 + recvBuf[20] * 256 * 256 + recvBuf[21] *
		256 + recvBuf[22];
	i += 24;
	j = 0;
	while(recvBuf[i] != '\0') {
		sendBuf[j] = recvBuf[i];
		j++;
		i += 2;
		}
	sendBuf[j] = '\0';

	/* Send LT frame newtdockstim */
	alarm(TimeOut);
	do {
		SendLTFrame(newtFd, ltSeqNo, "newtdockstim\0\0\0\4\0\0\0\x1e", 20);
	} while(WaitLAFrame(newtFd, ltSeqNo) < 0);
	ltSeqNo++;
	fprintf(stdout, ".");
	fflush(stdout);

	/* Wait LT frame newtdockdres */
	alarm(TimeOut);
	while(RecvFrame(newtFd, recvBuf) < 0 || recvBuf[1] != '\x04');
	SendLAFrame(newtFd, recvBuf[2]);
	fprintf(stdout, ".\n");
	fflush(stdout);

	/* batch install all of the files */
	for (k; k < argc; k++)
	{
		/* load the file */
		if((inFile = fopen(argv[k], "rb")) == NULL)
			ErrHandler("Error in opening package file!!");
		fseek(inFile, 0, SEEK_END);
		inFileLen = ftell(inFile);
		rewind(inFile);
		/* printf("File is '%s'\n", argv[k]); */

		/* Send LT frame newtdocklpkg */
		alarm(TimeOut);
		strcpy(sendBuf, "newtdocklpkg");
		tmpFileLen = inFileLen;
		for(i = 15; i >= 12; i--) {
			sendBuf[i] = tmpFileLen % 256;
			tmpFileLen /= 256;
			}
		do {
			SendLTFrame(newtFd, ltSeqNo, sendBuf, 16);
		} while(WaitLAFrame(newtFd, ltSeqNo) < 0);
		ltSeqNo++;
		/* fprintf(stdout, ".\n"); */

		/* fprintf(stdout, "Sending %d / %d\r", 0, inFileLen); */
		fprintf(stdout, "%20s %3d%% |%s| %d\r",
			argv[k], 0, stars(0), 0);
		fflush(stdout);

		/* Send package data */
		while(!feof(inFile)) {
                        int bytes;
			alarm(TimeOut);
			i = fread(sendBuf, sizeof(uchar), MaxInfoLen, inFile);
			while(i % 4 != 0)
				sendBuf[i++] = '\0';
			do {
				SendLTFrame(newtFd, ltSeqNo, sendBuf, i);
			} while(WaitLAFrame(newtFd, ltSeqNo) < 0);
			ltSeqNo++;
			if(ltSeqNo % 4 == 0) {
				/* fprintf(stdout, "Sending %d / %d\r", ftell(inFile), inFileLen); */
				bytes=ftell(inFile);
				fprintf(stdout, "%20s %3d%% |%s| %d\r",
					argv[k],
					(int)(((float)bytes/(float)inFileLen)*100),
					stars(((float)bytes/(float)inFileLen)*NUM_STARS),
					bytes);
				fflush(stdout);
			}
		}
		/* fprintf(stdout, "Sending %d / %d\n", inFileLen, inFileLen); */
		fprintf(stdout, "\n");

		/* Wait LT frame newtdockdres */
		alarm(TimeOut);
		while(RecvFrame(newtFd, recvBuf) < 0 || recvBuf[1] != '\x04');
		SendLAFrame(newtFd, recvBuf[2]);

		fclose (inFile);

	} /* END OF FOR LOOP */

	/* Send LT frame newtdockdisc */
	alarm(TimeOut);
	do {
		SendLTFrame(newtFd, ltSeqNo, "newtdockdisc\0\0\0\0", 16);
	} while(WaitLAFrame(newtFd, ltSeqNo) < 0);

	/* Wait disconnect */
	alarm(0);
	WaitLDFrame(newtFd);
	fprintf(stdout, "Finished!!\n\n");

	/* fclose(inFile); */
	close(newtFd);
	return 0;
}

void SigAlrm(int sigNo)
{
	ErrHandler("Timeout error, connection stopped!!");
}
  


#include <stdio.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include "newtmnp.h"

/* Constants definition */
uchar FrameStart[] = "\x16\x10\x02";
uchar FrameEnd[] = "\x10\x03";
uchar LRFrame[] = {
	'\x17', /* Length of header */
	'\x01', /* Type indication LR frame */
	'\x02', /* Constant parameter 1 */
	'\x01', '\x06', '\x01', '\x00', '\x00', '\x00', '\x00', '\xff',
		/* Constant parameter 2 */
	'\x02', '\x01', '\x02', /* Octet-oriented framing mode */
	'\x03', '\x01', '\x01', /* k = 1 */
	'\x04', '\x02', '\x40', '\x00', /* N401 = 64 */
	'\x08', '\x01', '\x03' /* N401 = 256 & fixed LT, LA frames */
	};
uchar LDFrame[] = {
	'\x04', /* Length of header */
	'\x02', /* Type indication LD frame */
	'\x01', '\x01', '\xff'
	};

int intNewtFd;

int InitNewtDev(NewtDevInfo *newtDevInfo)
{
	int newtFd;
	struct termios newtTty;

	/*  Install Ctrl-C function */
	intNewtFd = -1;
	if(signal(SIGINT, SigInt) == SIG_ERR)
		ErrHandler("Error in setting up interrupt function!!");

	/* Open the Newton device */
	if((newtFd = open(newtDevInfo->devName, O_RDWR)) == -1)
		return -1;

	/* Get the current device settings */
	tcgetattr(newtFd, &newtTty);

	/* Change the device settings */
	newtTty.c_iflag = IGNBRK | INPCK;
	newtTty.c_oflag = 0;
	newtTty.c_cflag = CREAD | CLOCAL | CS8 & ~PARENB & ~PARODD & ~CSTOPB;
	newtTty.c_lflag = 0;
	newtTty.c_cc[VMIN] = 1;
	newtTty.c_cc[VTIME] = 0;

	/* Select the communication speed */
	switch(newtDevInfo->speed) {
		#ifdef B2400
		case 2400:
			cfsetospeed(&newtTty, B2400);
			cfsetispeed(&newtTty, B2400);
			break;
		#endif
		#ifdef B4800
		case 4800:
			cfsetospeed(&newtTty, B4800);
			cfsetispeed(&newtTty, B4800);
			break;
		#endif
		#ifdef B9600
		case 9600:
			cfsetospeed(&newtTty, B9600);
			cfsetispeed(&newtTty, B9600);
			break;
		#endif
		#ifdef B19200
		case 19200:
			cfsetospeed(&newtTty, B19200);
			cfsetispeed(&newtTty, B19200);
			break;
		#endif
		#ifdef B38400
		case 38400:
			cfsetospeed(&newtTty, B38400);
			cfsetispeed(&newtTty, B38400);
			break;
		#endif
		#ifdef B57600
		case 57600:
			cfsetospeed(&newtTty, B57600);
			cfsetispeed(&newtTty, B57600);
			break;
		#endif
		#ifdef B115200
		case 115200:
			cfsetospeed(&newtTty, B115200);
			cfsetispeed(&newtTty, B115200);
			break;
		#endif
		#ifdef B230400
		case 230400:
			cfsetospeed(&newtTty, B230400);
			cfsetispeed(&newtTty, B230400);
			break;
		#endif
		default:
			close(newtFd);
			return -1;
		}

	/* Flush the device and restarts input and output */
	tcflush(newtFd, TCIOFLUSH);
	tcflow(newtFd, TCOON);

	/* Update the new device settings */
	tcsetattr(newtFd, TCSANOW, &newtTty);

	/* Return with file descriptor */
	intNewtFd = newtFd;
	return newtFd;
}

void FCSCalc(ushort *fcsWord, unsigned char octet)
{
	int i;
	uchar pow;

	pow = 1;
	for(i = 0; i < 8; i++) {
		if((((*fcsWord % 256) & 0x01) == 0x01) ^ ((octet & pow) == pow))
			*fcsWord = (*fcsWord / 2) ^ 0xa001;
		else
			*fcsWord /= 2;
		pow *= 2;
		}
}

void SendFrame(int newtFd, uchar *head, uchar *info, int infoLen)
{
	char errMesg[] = "Error in writing to Newton device, connection stopped!!";
	int i;
	ushort fcsWord;
	uchar buf;

	/* Initialize */
	fcsWord = 0;

	/* Send frame start */
	if(write(newtFd, FrameStart, 3) < 0)
		ErrHandler(errMesg);

	/* Send frame head */
	for(i = 0; i <= head[0]; i++) {
		FCSCalc(&fcsWord, head[i]);
		if(write(newtFd, &head[i], 1) < 0)
			ErrHandler(errMesg);
		if(head[i] == FrameEnd[0]) {
			if(write(newtFd, &head[i], 1) < 0)
				ErrHandler(errMesg);
			}
		}

	/* Send frame information */
	if(info != NULL) {
		for(i = 0; i < infoLen; i++) {
			FCSCalc(&fcsWord, info[i]);
			if(write(newtFd, &info[i], 1) < 0)
				ErrHandler(errMesg);
			if(info[i] == FrameEnd[0]) {
				if(write(newtFd, &info[i], 1) < 0)
					ErrHandler(errMesg);
				}
			}
		}

	/* Send frame end */
	if(write(newtFd, FrameEnd, 2) < 0)
		ErrHandler(errMesg);
	FCSCalc(&fcsWord, FrameEnd[1]);

	/* Send FCS */
	buf = fcsWord % 256;
	if(write(newtFd, &buf, 1) < 0)
		ErrHandler(errMesg);
	buf = fcsWord / 256;
	if(write(newtFd, &buf, 1) < 0)
		ErrHandler(errMesg);

	return;
}

void SendLTFrame(int newtFd, uchar seqNo, uchar *info, int infoLen)
{
	uchar ltFrameHead[3] = {
		'\x02', /* Length of header */
		'\x04', /* Type indication LT frame */
		};

	ltFrameHead[2] = seqNo;
	SendFrame(newtFd, ltFrameHead, info, infoLen);

	return;
}

void SendLAFrame(int newtFd, uchar seqNo)
{
	uchar laFrameHead[4] = {
		'\x03', /* Length of header */
		'\x05', /* Type indication LA frame */
		'\x00', /* Sequence number */
		'\x01' /* N(k) = 1 */
		};

	laFrameHead[2] = seqNo;
	SendFrame(newtFd, laFrameHead, NULL, 0);

	return;
}

int RecvFrame(int newtFd, unsigned char *frame)
{
	char errMesg[] = "Error in reading from Newton device, connection stopped!!";
	int state;
	unsigned char buf;
	unsigned short fcsWord;
	int i;

	/* Initialize */
	fcsWord = 0;
	i = 0;

	/* Wait for head */
	state = 0;
	while(state < 3) {
		if(read(newtFd, &buf, 1) < 0)
			ErrHandler(errMesg);
    //printf("Received %02x\n", buf);
		switch(state) {
			case 0:
				if(buf == FrameStart[0])
					state++;
				break;
			case 1:
				if(buf == FrameStart[1])
					state++;
				else
					state = 0;
				break;
			case 2:
				if(buf == FrameStart[2])
					state++;
				else
					state = 0;
				break;
			}
		}

	/* Wait for tail */
	state = 0;
	while(state < 2) {
		if(read(newtFd, &buf, 1) < 0)
			ErrHandler(errMesg);
		switch(state) {
			case 0:
				if(buf == '\x10')
					state++;
				else {
					FCSCalc(&fcsWord, buf);
					if(i < MaxHeadLen + MaxInfoLen) {
						frame[i] = buf;
						i++;
						}
					else
						return -1;
					}
				break;
			case 1:
				if(buf == '\x10') {
					FCSCalc(&fcsWord, buf);
					if(i < MaxHeadLen + MaxInfoLen) {
						frame[i] = buf;
						i++;
						}
					else
						return -1;
					state = 0;
					}
				else {
					if(buf == '\x03') {
						FCSCalc(&fcsWord, buf);
						state++;
						}
					else
						return -1;
					}
				break;
			}
		}

	/* Check FCS */
	if(read(newtFd, &buf, 1) < 0)
		ErrHandler(errMesg);
	if(fcsWord % 256 != buf)
		return -1;
	if(read(newtFd, &buf, 1) < 0)
		ErrHandler(errMesg);
	if(fcsWord / 256 != buf)
		return -1;

	if(frame[1] == '\x02')
		ErrHandler("Newton device disconnected, connection stopped!!");
	return 0;
}

int WaitLAFrame(int newtFd, uchar seqNo)
{
	uchar frame[MaxHeadLen + MaxInfoLen];

	do {
		while(RecvFrame(newtFd, frame) < 0);
		if(frame[1] == '\x04')
			SendLAFrame(newtFd, frame[2]);
		} while(frame[1] != '\x05');
	if(frame[2] == seqNo)
		return 0;
	else
		return -1;
}

int WaitLDFrame(int newtFd)
{
	char errMesg[] = "Error in reading from Newton device, connection stopped!!";
	int state;
	unsigned char buf;
	unsigned short fcsWord;
	int i;

	/* Initialize */
	fcsWord = 0;
	i = 0;

	/* Wait for head */
	state = 0;
	while(state < 5) {
		if(read(newtFd, &buf, 1) < 0)
			ErrHandler(errMesg);
		switch(state) {
			case 0:
				if(buf == FrameStart[0])
					state++;
				break;
			case 1:
				if(buf == FrameStart[1])
					state++;
				else
					state = 0;
				break;
			case 2:
				if(buf == FrameStart[2])
					state++;
				else
					state = 0;
				break;
			case 3:
				FCSCalc(&fcsWord, buf);
				state++;
				break;
			case 4:
				if(buf == '\x02') {
					FCSCalc(&fcsWord, buf);
					state++;
					}
				else {
					state = 0;
					fcsWord = 0;
					}
				break;
			}
		}

	/* Wait for tail */
	state = 0;
	while(state < 2) {
		if(read(newtFd, &buf, 1) < 0)
			ErrHandler(errMesg);
		switch(state) {
			case 0:
				if(buf == '\x10')
					state++;
				else
					FCSCalc(&fcsWord, buf);
				break;
			case 1:
				if(buf == '\x10') {
					FCSCalc(&fcsWord, buf);
					state = 0;
					}
				else {
					if(buf == '\x03') {
						FCSCalc(&fcsWord, buf);
						state++;
						}
					else
						return -1;
					}
				break;
			}
		}

	/* Check FCS */
	if(read(newtFd, &buf, 1) < 0)
		ErrHandler(errMesg);
	if(fcsWord % 256 != buf)
		return -1;
	if(read(newtFd, &buf, 1) < 0)
		ErrHandler(errMesg);
	if(fcsWord / 256 != buf)
		return -1;

	return 0;
}

void ErrHandler(char *errMesg)
{
	fprintf(stderr, "\n");
	fprintf(stderr, errMesg);
	fprintf(stderr, "\n\n");
	_exit(0);
}

void SigInt(int sigNo)
{
	if(intNewtFd >= 0) {
		/* Wait for all buffer sent */
		tcdrain(intNewtFd);

		SendFrame(intNewtFd, LDFrame, NULL, 0);
		}
	ErrHandler("User interrupted, connection stopped!!");
}


#endif
