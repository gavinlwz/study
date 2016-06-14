#ifndef TCC_SPDIF_THREAD_H
#define TCC_SPDIF_THREAD_H

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include "alsa/asoundlib.h"
#include "TCCMemory.h"
#include <tsemaphore.h>

#define SPDIF_CODEC_AC3	3
#define SPDIF_CODEC_PCM	4
#define SPDIF_CODEC_DTS	6

typedef enum{
	SPDIF_PCM = 0,
	SPDIF_BITSTREAM,
};

typedef struct 
{
	unsigned int 	mode;	// bitstream or pcm
	unsigned int	channel;
	unsigned int	endian;
	unsigned int	samplerate;
	unsigned int 	codec;
	unsigned int 	bitpersample;
	unsigned char*  buffre_ptr;
	unsigned int	data_size;
}Spdif_InfoType_t;

typedef struct Spdif_Thread_Struct
{
	char* spdif_fifo_bufptr;
	unsigned int 	mode;	// bitstream or pcm
	unsigned int	channel;
	unsigned int	endian;
	unsigned int	samplerate;
	unsigned int 	codec;
	unsigned int 	bitpersample;

	// thread
	int SpdifThreadID;
	pthread_t SpdifThread;
	int spdif_thread_running;

  	tsem_t* WaitSema;

	// alsa
	snd_pcm_t*	playback_handle;
	snd_pcm_hw_params_t*	hw_params;
	int 		alsa_open;
	int			alsa_configure_request;
	char* 		temp_buf;
}Spdif_Thread_t,*pSpdif_Thread_t;


int spdif_thread_init(void);
int spdif_thread_deinit(void);
int spdif_dynamic_alsa_setting(void);
void spdif_alsa_write(char* buffer, int length);
void spdif_thread(void);
void spdif_thread_sema_up(void);
void spdif_thread_stop_request(void);


#endif /* TCC_SPDIF_THREAD_H */
