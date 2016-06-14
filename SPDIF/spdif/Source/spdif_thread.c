
#include <OMX_Types.h>
#include "OMX_TCC_Index.h"
#include "spdif_thread.h"
#include "spdif_parse.h"
#include "spdif_fifo.h"

Spdif_Thread_t* Spdif_Thread_S;

int spdif_thread_init(void)
{
	// FIFO BUFFER INIT..
	Spdif_Thread_S = TCC_malloc(sizeof(Spdif_Thread_t));

	Spdif_Thread_S->channel = 2;
	Spdif_Thread_S->mode = ALSA_OPEN_SRC;	// pcm mode.
	Spdif_Thread_S->samplerate = 44100;
	Spdif_Thread_S->endian = 1;	// little endian..
	Spdif_Thread_S->codec = 0;
	Spdif_Thread_S->bitpersample = 16;
	Spdif_Thread_S->spdif_thread_running = 1;
	
	Spdif_Thread_S->spdif_fifo_bufptr = (unsigned char*)TCC_calloc(1,SPDIF_FIFO_AUDIO_BUFFERSIZE);

	SPDIF_FIFO_InitBuffer(SPDIF_FIFO_AUDIO_BUFFERSIZE,Spdif_Thread_S->spdif_fifo_bufptr);

	// alsa init
	Spdif_Thread_S->alsa_open = FALSE;
	Spdif_Thread_S->hw_params = NULL;
	Spdif_Thread_S->playback_handle = NULL;
	Spdif_Thread_S->alsa_configure_request = FALSE;

	Spdif_Thread_S->temp_buf = TCC_malloc(sizeof(char)*1024*10);

    Spdif_Thread_S->WaitSema = TCC_calloc(1,sizeof(tsem_t));
	tsem_init(Spdif_Thread_S->WaitSema,0);

	Spdif_Thread_S->SpdifThreadID = pthread_create(&Spdif_Thread_S->SpdifThread,NULL,spdif_thread,NULL);

	if(Spdif_Thread_S->SpdifThreadID < 0)
	{
		printf("spdif thread create fail!!! \n");
		return -1;
	}

	printf("spdif thread init OK!!!!\n");
	return 1;
}

int spdif_thread_deinit(void)
{

	spdif_parse_deinit();

	tsem_deinit(Spdif_Thread_S->WaitSema);
    TCC_free(Spdif_Thread_S->WaitSema);

	if(Spdif_Thread_S->hw_params) {
		snd_pcm_hw_params_free (Spdif_Thread_S->hw_params);
	}
	if(Spdif_Thread_S->playback_handle) {
		snd_pcm_close(Spdif_Thread_S->playback_handle);
	}

	if(Spdif_Thread_S->temp_buf)
	{
		TCC_free(Spdif_Thread_S->temp_buf);
		Spdif_Thread_S->temp_buf = NULL;
	}

	if(Spdif_Thread_S->spdif_fifo_bufptr)
	{
		TCC_free(Spdif_Thread_S->spdif_fifo_bufptr);
		Spdif_Thread_S->spdif_fifo_bufptr = NULL;
	}

	if(Spdif_Thread_S)
	{
		TCC_free(Spdif_Thread_S);
	}
}


int spdif_dynamic_alsa_setting(void)
{
  	char *pcm_name;
  	int err = 0;

  	if(Spdif_Thread_S->alsa_open)
  	{
		if(Spdif_Thread_S->playback_handle)
			snd_pcm_reset(Spdif_Thread_S->playback_handle);  
  	
		if(Spdif_Thread_S->hw_params) {
			snd_pcm_hw_params_free (Spdif_Thread_S->hw_params);
		}
		if(Spdif_Thread_S->playback_handle) {
			snd_pcm_close(Spdif_Thread_S->playback_handle);
		}

		if(Spdif_Thread_S->mode == ALSA_OPEN_NOTSRC)
		{
			pcm_name = strdup("hw:0,1");
			printf("hw:0,1\n");
		}else if(Spdif_Thread_S->mode == ALSA_OPEN_PCM_SRC){
			pcm_name = strdup("plug:tcc_spdif");
			printf("alsa open plug:tcc_spdif\n");
		}else{
			printf("alsa mode invalid %d\n",Spdif_Thread_S->mode);
		}

		/* Allocate the playback handle and the hardware parameter structure */
		if ((err = snd_pcm_open (&Spdif_Thread_S->playback_handle, pcm_name, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
			printf("snd pcm open error!!!!! \n");
			return -1;
		}

		if (snd_pcm_hw_params_malloc(&Spdif_Thread_S->hw_params) < 0) {
			printf("snd_pcm_hw_params_malloc error!!!!! \n");
			return -1;
		}

		Spdif_Thread_S->alsa_open = FALSE;	
  	}

	// alsa setting 관련.
	if(Spdif_Thread_S->alsa_configure_request)
	{
		if ((err = snd_pcm_hw_params_any (Spdif_Thread_S->playback_handle, Spdif_Thread_S->hw_params)) < 0)
		{
			printf("snd_pcm_hw_params_any error!!!!! \n");
			return -1;
		}

		if ((err = snd_pcm_hw_params_set_access(Spdif_Thread_S->playback_handle, Spdif_Thread_S->hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)
		{
			printf("snd_pcm_hw_params_set_access error!!!!! \n");
			return -1;
		}

		if(Spdif_Thread_S->endian ==  OMX_EndianLittle)
		{
			switch(Spdif_Thread_S->bitpersample)
			{
				case 24:
					if(snd_pcm_hw_params_set_format(Spdif_Thread_S->playback_handle,Spdif_Thread_S->hw_params,SND_PCM_FORMAT_S24_3LE) < 0)
					{
						printf("snd_pcm_hw_params_set_format error!!!!! \n");
					}
					break;
				case 16:
				default:
					if(snd_pcm_hw_params_set_format(Spdif_Thread_S->playback_handle,Spdif_Thread_S->hw_params,SND_PCM_FORMAT_S16_LE) < 0)
					{
						printf("snd_pcm_hw_params_set_format error!!!!! \n");
					}
					break;
			}
		}else{    
			if(snd_pcm_hw_params_set_format(Spdif_Thread_S->playback_handle,Spdif_Thread_S->hw_params,SND_PCM_FORMAT_S16_BE) < 0)
			{
				printf("snd_pcm_hw_params_set_format error!!!!! \n");
		    }
		}

		// sample
	    if(snd_pcm_hw_params_set_rate_near (Spdif_Thread_S->playback_handle,Spdif_Thread_S->hw_params,&Spdif_Thread_S->samplerate,0) < 0)
	    {
			printf("snd_pcm_hw_params_set_rate_near error!!!!! \n");
		}
		// channel

		printf("Spdif_Thread_S->channel %d\n",Spdif_Thread_S->channel);
	    if(snd_pcm_hw_params_set_channels (Spdif_Thread_S->playback_handle,Spdif_Thread_S->hw_params,Spdif_Thread_S->channel) < 0)
	    {
	   		printf("snd_pcm_hw_params_set_channels error!!!!! \n");
	    }

		if ((err = snd_pcm_hw_params (Spdif_Thread_S->playback_handle, Spdif_Thread_S->hw_params)) < 0)
		{
			printf("snd_pcm_hw_params error!!!!! \n");
			return -1;
		}

		if ((err = snd_pcm_prepare (Spdif_Thread_S->playback_handle)) < 0)
		{
			printf("snd_pcm_prepare error!!!!! \n");
			return -1;
		}
		Spdif_Thread_S->alsa_configure_request = 0;
	}
}

void spdif_alsa_write(char* buffer, int length)
{
	int framsize;
	int written;
	int totalbuffer;
	int offsetbuffer;
	int allDatasend;

	framsize = (Spdif_Thread_S->channel * Spdif_Thread_S->bitpersample) >> 3;

	if(length < framsize)
	{
		printf("framsize size error!!!!\n");
		return;
	}
	
	// alsa setting
	spdif_dynamic_alsa_setting();

	allDatasend = FALSE;

	totalbuffer = length/framsize;
	offsetbuffer = 0;

	while (!allDatasend)
	{
		written = snd_pcm_writei(Spdif_Thread_S->playback_handle, buffer + (offsetbuffer * framsize), totalbuffer);
		if (written < 0)
		{
			if(written == -EPIPE)
			{
				//printf("spdif ALSA Underrun.. %d\n",SPDIF_FIFO_GetLength());
				snd_pcm_prepare(Spdif_Thread_S->playback_handle);
				written = 0;
			}else{
				printf("spdif alsa write fail!!!..\n");
				return;
			}
		}

		if(written != totalbuffer)
		{
			totalbuffer = totalbuffer - written;
			offsetbuffer = written;
		} else {
			allDatasend = TRUE;
		}
	}
}

void spdif_thread_sema_up(void)
{
	tsem_up(Spdif_Thread_S->WaitSema);
}


void spdif_thread_stop_request(void)
{
	Spdif_Thread_S->spdif_thread_running = 0;
	tsem_up(Spdif_Thread_S->WaitSema);
}


void spdif_thread(void)
{
	unsigned char* buffptr;
	int length;
	int mode;
	unsigned int channel;
	unsigned int samplerate;
	unsigned int endian;
	unsigned int codec;
	unsigned int bitpersample;
	int spdif_length;
	int codec_sel;
	
	OMX_TICKS measure;
	OMX_TICKS walltime, newtime;
	struct timeval tv;

	Spdif_Thread_S->spdif_thread_running = 1;

	// spdif parser init!!
	spdif_parse_init();

	// 최초 무조건 alsa open 실행.
	Spdif_Thread_S->alsa_open = TRUE;
	
	while(Spdif_Thread_S->spdif_thread_running) 
	{
		// fifo 구조에 data가 존재하는지 체크. 
		tsem_down(Spdif_Thread_S->WaitSema);
		
		if(SPDIF_FIFO_GetLength() > 0)
		{
			// get data
			buffptr = SPDIF_FIFO_GetOneFrame(&length,&channel,&samplerate,&mode,&endian,&codec,&bitpersample);

			memcpy(&Spdif_Thread_S->temp_buf[0],buffptr,length);
			
			SPDIF_FIFO_PopFrame();
			if(mode == ALSA_OPEN_PCM_SRC) // pcm에 대한 처리.
			{
				// alsa setting config....	
				if((Spdif_Thread_S->samplerate != samplerate) || (Spdif_Thread_S->channel != channel) || 
					(Spdif_Thread_S->bitpersample != bitpersample) || (Spdif_Thread_S->endian != endian) || (Spdif_Thread_S->mode != mode))
				{
					printf("Alsa Config Setting \n");
				
					if(Spdif_Thread_S->mode != mode)
					{
						Spdif_Thread_S->alsa_open = TRUE;
						printf("Spdif_Thread_S->alsa_open src!!!\n");
					}
				
					Spdif_Thread_S->alsa_configure_request = TRUE;

					Spdif_Thread_S->channel = channel;
					Spdif_Thread_S->samplerate = samplerate;
					Spdif_Thread_S->endian = endian;
					Spdif_Thread_S->codec = codec;
					Spdif_Thread_S->mode = mode;
					Spdif_Thread_S->bitpersample = bitpersample;
				}
				//printf("spdif pcm write! %d\n",Spdif_Thread_S->WaitSema->semval);
				spdif_alsa_write(&Spdif_Thread_S->temp_buf[0],length);
			}else{ 	// bitstream에 대한 처리.
				if(codec == SPDIF_CODEC_AC3)
				{
					codec_sel = FORMAT_AC3;
				}else{
					codec_sel = FORMAT_DTS;
				}

				if(spdif_parser_frame(&Spdif_Thread_S->temp_buf[0],length,codec_sel,0,0))
				{
					spdif_length = spdif_parse_get_frame_size();
					//Spdif_Thread_S->endian = spdif_parse_get_endian();
					// alsa setting config....	
					if((Spdif_Thread_S->samplerate != samplerate) || (Spdif_Thread_S->channel != channel) ||
						(Spdif_Thread_S->bitpersample != bitpersample) || (Spdif_Thread_S->endian != endian) || (Spdif_Thread_S->mode != mode))
					{
						if(Spdif_Thread_S->mode != mode)
						{
							Spdif_Thread_S->alsa_open = TRUE;
							printf("Spdif_Thread_S->alsa_open non src!!!\n");
						}
						printf("SPDIF_FIFO_GetLength %d\n",SPDIF_FIFO_GetLength());

						Spdif_Thread_S->alsa_configure_request = TRUE;

						//printf("channel %d\n",channel);
						//printf("samplerate %d\n",samplerate);

						Spdif_Thread_S->channel = channel;
						Spdif_Thread_S->samplerate = samplerate;
						Spdif_Thread_S->endian = spdif_parse_get_endian();
						Spdif_Thread_S->codec = codec;
						Spdif_Thread_S->mode = mode;
						Spdif_Thread_S->bitpersample = bitpersample;
					}
					//printf("spdif bitstream write!\n");
					spdif_alsa_write(spdif_parse_get_buf(),spdif_length);
				
				}else{
					printf("spdif parse error!!!\n");
				}
			}
			//SPDIF_FIFO_PopFrame();
		}
	}

	pthread_join(Spdif_Thread_S->SpdifThread,NULL);
	Spdif_Thread_S->SpdifThreadID = -1;
	printf("pthread_join spdif thread\n");

	spdif_thread_deinit();
}


