#ifndef TCC_SPDIF_FIFO_H
#define TCC_SPDIF_FIFO_H

#define SPDIF_FIFO_FRAME_NUM	1024

#define	SPDIF_FIFO_EMPTY		1
#define	SPDIF_FIFO_FULL			2
#define	SPDIF_FIFO_NORMAL		3

#define	SPDIF_FIFO_AUDIO_BUFFERSIZE 1024*1024 


typedef struct _SPDIF_FIFO_FRAME_S_
{
	unsigned int 	offset;
	unsigned int	size;
	unsigned int 	mode;	// bitstream or pcm
	unsigned int	channel;
	unsigned int	endian;
	unsigned int	samplerate;
	unsigned int	codec;
	unsigned int 	bitpersample;
}_SPDIF_FIFO_FRAME_t;

typedef struct _SPDIF_FIFO_S_
{
	_SPDIF_FIFO_FRAME_t stFrameInfo[SPDIF_FIFO_FRAME_NUM];
	unsigned char 	*ptrBuffer;
	unsigned int	uiMaxSize;

	unsigned int	ReadBufPtr;
	unsigned int	WriteBufPtr;
	
	unsigned short	ReadFrmIdx;
	unsigned short	WriteFrmIdx;
	unsigned short	IdxNum;

	char 			fState;
}_SPDIF_FIFO_t;

void SPDIF_FIFO_InitBuffer(unsigned int size, unsigned char* ptrBuffer);
int SPDIF_FIFO_GetLength(void);
unsigned char *SPDIF_FIFO_GetOneFrame(int *Len,int* channel,int* samplerate ,int* mode, int* endian, int* codec, int* bitpersample);
int SPDIF_FIFO_PopFrame(void);
int SPDIF_FIFO_Push(void);
int SPDIF_FIFO_GetOneFifo(int ReqSize,int channel, int samplerate, int mode ,int endian,int codec,int bitpersample);
int SPDIF_FIFO_WriteFifo(unsigned char *ptrData, unsigned int ReqSize);

#endif /* TCC_SPDIF_FIFO_H */
