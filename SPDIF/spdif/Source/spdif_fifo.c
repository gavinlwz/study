
#include "spdif_fifo.h"

static _SPDIF_FIFO_t pstFifoHandle;

/**************************************************************************
*  FUNCTION NAME : 
*      void SPDIF_FIFO_InitBuffer(unsigned int size, unsigned char* ptrBuffer );
*  
*  DESCRIPTION : 
*  INPUT:
*  OUTPUT:	void - Return Type
*  REMARK  :	
**************************************************************************/
void SPDIF_FIFO_InitBuffer(unsigned int size, unsigned char* ptrBuffer )
{
	int i;
	
 	pstFifoHandle.ptrBuffer = ptrBuffer; 
	pstFifoHandle.uiMaxSize = size; 

	pstFifoHandle.ReadBufPtr = 0; 
	pstFifoHandle.WriteBufPtr = 0; 
	pstFifoHandle.ReadFrmIdx = 0; 
	pstFifoHandle.WriteFrmIdx = 0; 

	pstFifoHandle.IdxNum = SPDIF_FIFO_FRAME_NUM; 
 	pstFifoHandle.fState = SPDIF_FIFO_EMPTY;

	memset(ptrBuffer, 0, size);
	for( i=0 ; i < SPDIF_FIFO_FRAME_NUM ; i++ )
		memset(&pstFifoHandle.stFrameInfo[i], 0, sizeof(_SPDIF_FIFO_FRAME_t) );
}

/**********************************************************
*
*	Function of	SPDIF_FIFO_GetLength
*
*	Input	: 
*	Output	: 
*	Return	: 
*
*	Description	: data가 존재 하는 FIFO의 수를 리턴 함.
**********************************************************/
int SPDIF_FIFO_GetLength(void)
{
	int lRemainSize = 0;
	unsigned int	uiTail, uiHead;

	uiTail = pstFifoHandle.ReadFrmIdx;
	uiHead = pstFifoHandle.WriteFrmIdx;
	if(pstFifoHandle.fState != SPDIF_FIFO_EMPTY)
	{
		if( uiTail < uiHead )	//	Tail  < 	Head
		{
			 lRemainSize = uiHead - uiTail;
		}
		else
		{
			lRemainSize = (pstFifoHandle.IdxNum - uiTail) +  uiHead;
		}
		return lRemainSize;
	}
/*	else
		lRemainSize = pstFifoHandle[nHandle].IdxNum;*/
	
	return lRemainSize;
}

/**********************************************************
*
*	Function of	SPDIF_FIFO_GetOneFrame
*
*	Input	: 
*	Output	: 
*	Return	: 
*
*	Description	: data가 복사한후 버퍼 업데이트 
**********************************************************/
unsigned char *SPDIF_FIFO_GetOneFrame(int *Len,int* channel,int* samplerate ,int* mode, int* endian, int* codec, int* bitpersample) 
{
	unsigned int	uiTail;
	unsigned char	*ptrData;

	if(pstFifoHandle.fState == SPDIF_FIFO_EMPTY)
	{
		*Len = 0;
		return 0;
	}
		
	uiTail = pstFifoHandle.ReadFrmIdx;

	ptrData = pstFifoHandle.ptrBuffer + pstFifoHandle.stFrameInfo[uiTail].offset;

	*Len = pstFifoHandle.stFrameInfo[uiTail].size;
	*channel = pstFifoHandle.stFrameInfo[uiTail].channel;
	*samplerate = pstFifoHandle.stFrameInfo[uiTail].samplerate;
	*mode = pstFifoHandle.stFrameInfo[uiTail].mode;
	*endian = pstFifoHandle.stFrameInfo[uiTail].endian;
	*codec = pstFifoHandle.stFrameInfo[uiTail].codec;
	*bitpersample = pstFifoHandle.stFrameInfo[uiTail].bitpersample;
	
	return ptrData; 
}

/**********************************************************
*
*	Function of	SPDIF_FIFO_PopFrame
*
*	Input	: 
*	Output	: 
*	Return	: 
*
*	Description	: data가 복사한후 버퍼 업데이트 
**********************************************************/
int SPDIF_FIFO_PopFrame(void)
{
	unsigned int	uiTail, uiHead;
	unsigned int	ReadBufPtr;

	if(pstFifoHandle.fState == SPDIF_FIFO_EMPTY)
		return 0;
	
	uiTail = pstFifoHandle.ReadFrmIdx;
	uiHead = pstFifoHandle.WriteFrmIdx;

	ReadBufPtr = pstFifoHandle.stFrameInfo[uiTail].offset + pstFifoHandle.stFrameInfo[uiTail].size;
	
	uiTail++;
	if( uiTail == pstFifoHandle.IdxNum )
		uiTail = 0;

	if(uiTail  == uiHead)
		pstFifoHandle.fState = SPDIF_FIFO_EMPTY;
	else
		pstFifoHandle.fState = SPDIF_FIFO_NORMAL;

	pstFifoHandle.ReadFrmIdx = uiTail;
	pstFifoHandle.ReadBufPtr = ReadBufPtr;
	return 1;
}



int SPDIF_FIFO_Push(void)
{
	unsigned int	uiTail, uiHead;

	if(pstFifoHandle.fState == SPDIF_FIFO_FULL)
		return 0;
	
	uiTail = pstFifoHandle.ReadFrmIdx;
	uiHead = pstFifoHandle.WriteFrmIdx;
	uiHead++;
	if( uiHead == pstFifoHandle.IdxNum )
		uiHead = 0;

	if(uiTail  == uiHead)
		pstFifoHandle.fState = SPDIF_FIFO_FULL;
	else
		pstFifoHandle.fState = SPDIF_FIFO_NORMAL;

	pstFifoHandle.WriteFrmIdx = uiHead;

	return 1;
}

/**************************************************************************
*  FUNCTION NAME : 
*      int SPDIF_FIFO_GetOneFifo(int ReqSize,int channel, int samplerate, int mode ,int endian)
*  
*  DESCRIPTION : 
*  INPUT:
*			iFlags	= 
*			nHandle	= 
*			ptrPESInfo	= 
*			ReqSize	= 
*  
*  OUTPUT:	int - Return Type
*  			= 
*  REMARK  :	
**************************************************************************/
int SPDIF_FIFO_GetOneFifo(int ReqSize,int channel, int samplerate, int mode ,int endian,int codec,int bitpersample)
{
	unsigned int	uiHead;
	unsigned int	ReadBufPtr, WriteBufPtr;
	unsigned int 	lRemainSize = 0;

	if(pstFifoHandle.fState == SPDIF_FIFO_FULL)
		return 0;

	ReadBufPtr = pstFifoHandle.ReadBufPtr;
	WriteBufPtr = pstFifoHandle.WriteBufPtr;
	
	uiHead = pstFifoHandle.WriteFrmIdx;
	
	if( ReqSize <= 0 ) /* [DMP:0213] */
		ReqSize = (32*1024);
	
	if( ReadBufPtr > WriteBufPtr )	//	Tail  < 	Head
	{
		lRemainSize = ReadBufPtr -  WriteBufPtr;

		if( lRemainSize <= ReqSize )
		{
			pstFifoHandle.fState = SPDIF_FIFO_FULL;
			return 0;
		}
	}
	else
	{
		 lRemainSize = pstFifoHandle.uiMaxSize - WriteBufPtr;

		 if( ReqSize >= lRemainSize )
		 {
		 	WriteBufPtr = 0;
		 	lRemainSize = ReadBufPtr;
			if( lRemainSize <= ReqSize )
			{
				pstFifoHandle.fState = SPDIF_FIFO_FULL;
				return 0;
			}
		 }
	}
	
	pstFifoHandle.stFrameInfo[uiHead].offset = WriteBufPtr;
	pstFifoHandle.stFrameInfo[uiHead].size = ReqSize;
	
	pstFifoHandle.stFrameInfo[uiHead].channel = channel;
	pstFifoHandle.stFrameInfo[uiHead].samplerate = samplerate;
	
	pstFifoHandle.stFrameInfo[uiHead].mode = mode;
	pstFifoHandle.stFrameInfo[uiHead].endian = endian;
	pstFifoHandle.stFrameInfo[uiHead].codec = codec;
	pstFifoHandle.stFrameInfo[uiHead].bitpersample = bitpersample;
	
	pstFifoHandle.WriteBufPtr = WriteBufPtr;

	return 1;
}

/**************************************************************************
*  FUNCTION NAME : 
*      int SPDIF_FIFO_WriteFifo(unsigned char *ptrData, unsigned int ReqSize) 
*  
*  DESCRIPTION : 
*  INPUT:
*			nHandle	= 
*			ptrData	= 
*			ReqSize	= 
*  
*  OUTPUT:	int - Return Type
*  			= 
*  REMARK  :	
**************************************************************************/
int SPDIF_FIFO_WriteFifo(unsigned char *ptrData, unsigned int ReqSize) 
{
	unsigned int	WriteBufPtr;
	unsigned char 	*ptrWrite;
	unsigned int	MaxSize;
	int	lRemainSize;

	if(pstFifoHandle.fState == SPDIF_FIFO_FULL)
		return 0;
	
	MaxSize =  pstFifoHandle.uiMaxSize;
	WriteBufPtr = pstFifoHandle.WriteBufPtr;
	lRemainSize = MaxSize - WriteBufPtr;

	 if( lRemainSize < 0 )
	 {
	 	printf("Buffer OverFlow\n");
	 	return 0;
	 }	
	else
	{
		ptrWrite = pstFifoHandle.ptrBuffer + WriteBufPtr;
		 if( ReqSize >= lRemainSize )
		 {
	 		memcpy( ptrWrite, ptrData, lRemainSize );
			ptrWrite = pstFifoHandle.ptrBuffer;
			ReqSize -= lRemainSize;
			ptrData += lRemainSize;
			WriteBufPtr = 0;
		 }
	}	 	
	memcpy( ptrWrite, ptrData, ReqSize );	

	pstFifoHandle.WriteBufPtr = WriteBufPtr + ReqSize;

	return ReqSize;	
}



