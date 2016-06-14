#include "spdif_parse.h"
#include "spdif_bitstream.h"

#include "dts_header.h"
#include "ac3_header.h"
#include "TCCMemory.h"

static spdif_parse_type_s* spdif_parse_t;

void spdif_parse_init(void)
{
	spdif_parse_t = TCC_malloc(sizeof(spdif_parse_type_s));
	spdif_parse_t->buf = TCC_malloc(sizeof(char)*MAX_SPDIF_FRAME_SIZE);
}

void spdif_parse_deinit(void)
{
	if(spdif_parse_t->buf)
	{
		TCC_free(spdif_parse_t->buf);
	}

	if(spdif_parse_t)
	{
		TCC_free(spdif_parse_t);
	}
}

inline int is_14bit(int bs_type)
{
  return (bs_type == BITSTREAM_14LE) || (bs_type == BITSTREAM_14BE);
}

int spdif_make_header(unsigned long param, unsigned short type, unsigned short len)
{
	spdif_header_type_s *header;

	header = (spdif_header_type_s*)param;

	header->zero1 = 0x00;
	header->zero2 = 0x00;
	header->zero3 = 0x00;
	header->zero4 = 0x00;

	header->sync1 = 0xf872;
	header->sync2 = 0x4e1f;

	header->type = type;
	header->len  = len;
	
	return 1;
}

// 주의 반드시 frame 단위로 입력 되어야 한다.
int spdif_parser_frame(unsigned char* frame, int size,int codec_type,int usr_mode,int dts_mode)
{
	int spdif_frame_size;
	unsigned char *raw_frame;
	int raw_size;
	int payload_size;
    int frame_grows; 
    int frame_shrinks; 
	
	// 1. 입력된 codec type에 맞추어 정보를 추출한다. 
	switch(codec_type)
	{
		case FORMAT_DTS:
			if(!dts_header_parse(frame,&spdif_parse_t->hdr))
			{
				printf("DTS HEADER PARSE eRROR\n");
				return 0;
			}
			break;
		case FORMAT_AC3:
			if(!ac3_header_parse(frame,&spdif_parse_t->hdr))
			{
				printf("ac3 HEADER PARSE eRROR\n");
				return 0;
			}
			break;
		default:
			printf("Error!!! not support!\n");
			return 0;
	}

	raw_frame = frame;
	raw_size = size;
	
	// 2. SPDIF frame size를 계산한다.
	spdif_frame_size = spdif_parse_t->hdr.nsamples*4;

	if(spdif_frame_size > MAX_SPDIF_FRAME_SIZE)
	{
		printf("Error!!! MAX SPDIR FRAME size %d\n",spdif_frame_size);
		return 0;
	}

	spdif_parse_t->spdif_frame_size = spdif_frame_size;

	// 3 codec_type 별로 별도의 작업 진행.
	if(codec_type == FORMAT_DTS)
	{
  		// 14 or 16.
    	// DTS frame may grow if conversion to 14bit stream format is used
   	
		if((usr_mode == DTS_CONV_14BIT) && !is_14bit(spdif_parse_t->hdr.bs_type))
    	{
			frame_grows = 1;
    	}else{
			frame_grows = 0;
    	}

		if((usr_mode == DTS_CONV_16BIT) && is_14bit(spdif_parse_t->hdr.bs_type))
    	{
			frame_shrinks = 1;
    	}else{
			frame_shrinks = 0;
    	}
    	
	    switch (dts_mode)
	    {
		    case DTS_MODE_WRAPPED:
		      spdif_parse_t->use_header = TRUE;
		      if (frame_grows && (raw_size * 8 / 7 <= spdif_frame_size - sizeof(spdif_header_type_s)))
		        spdif_parse_t->spdif_bs = BITSTREAM_14LE;
		      else if (frame_shrinks && (raw_size * 7 / 8 <= spdif_frame_size - sizeof(spdif_header_type_s)))
		        spdif_parse_t->spdif_bs = BITSTREAM_16LE;
		      else if (raw_size <= spdif_frame_size - sizeof(spdif_header_type_s))
		        spdif_parse_t->spdif_bs = is_14bit(spdif_parse_t->hdr.bs_type) ? BITSTREAM_14LE: BITSTREAM_16LE;
		      else
		      {
		        // impossible to wrap
		        printf("impossible to wrap\n");
		        return 0;
		      }
		      break;

		    case DTS_MODE_PADDED:
		      spdif_parse_t->use_header = FALSE;
		      if (frame_grows && (raw_size * 8 / 7 <= spdif_frame_size))
		        spdif_parse_t->spdif_bs = BITSTREAM_14LE;
		      else if (frame_shrinks && (raw_size * 7 / 8 <= spdif_frame_size))
		        spdif_parse_t->spdif_bs = BITSTREAM_16LE;
		      else if (raw_size <= spdif_frame_size)
		        spdif_parse_t->spdif_bs = is_14bit(spdif_parse_t->hdr.bs_type)? BITSTREAM_14LE: BITSTREAM_16LE;
		      else
		      {
		        // impossible to send over spdif
		        // passthrough non-spdifable data
		        printf("impossible to send over spdif\n");
		        return 0;
		      }
		      break;

		    case DTS_MODE_AUTO:
		    default:
		      if (frame_grows && (raw_size * 8 / 7 <= spdif_frame_size - sizeof(spdif_header_type_s)))
		      {
		        spdif_parse_t->use_header = TRUE;
		        spdif_parse_t->spdif_bs = BITSTREAM_14LE;
		      }
		      else if (frame_grows && (raw_size * 8 / 7 <= spdif_frame_size))
		      {
		        spdif_parse_t->use_header = FALSE;
		        spdif_parse_t->spdif_bs = BITSTREAM_14LE;
		      }
		      if (frame_shrinks && (raw_size * 7 / 8 <= spdif_frame_size - sizeof(spdif_header_type_s)))
		      {
		        spdif_parse_t->use_header = TRUE;
		        spdif_parse_t->spdif_bs = BITSTREAM_14LE;
		      }
		      else if (frame_shrinks && (raw_size * 7 / 8 <= spdif_frame_size))
		      {
		        spdif_parse_t->use_header = FALSE;
		        spdif_parse_t->spdif_bs = BITSTREAM_14LE;
		      }
		      else if (raw_size <= spdif_frame_size - sizeof(spdif_header_type_s))
		      {
		        spdif_parse_t->use_header = TRUE;
		        spdif_parse_t->spdif_bs = is_14bit(spdif_parse_t->hdr.bs_type)? BITSTREAM_14LE: BITSTREAM_16LE;
		      }
		      else if (raw_size <= spdif_frame_size)
		      {
		        spdif_parse_t->use_header = FALSE;
		        spdif_parse_t->spdif_bs = is_14bit(spdif_parse_t->hdr.bs_type)? BITSTREAM_14LE: BITSTREAM_16LE;
		      }
		      else
		      {
		        // impossible to send over spdif
		        // passthrough non-spdifable data
		        printf("impossible to send over spdif\n");
		        return 0;
		      }
		      break;
		    }
	}else{
		if (raw_size <= spdif_frame_size - sizeof(spdif_header_type_s))
		{
			spdif_parse_t->use_header = TRUE;
			spdif_parse_t->spdif_bs = BITSTREAM_16LE;
		}
		else
		{
			printf("ERROR!!!\n");
			return 0;
		}
	}

	// 4.
	if(spdif_parse_t->use_header)
	{
	    payload_size = bs_convert(raw_frame, raw_size, spdif_parse_t->hdr.bs_type, &spdif_parse_t->buf[sizeof(spdif_header_type_s)], spdif_parse_t->spdif_bs);
	    memset(&spdif_parse_t->buf[sizeof(spdif_header_type_s) + payload_size], 0, spdif_frame_size - sizeof(spdif_header_type_s) - payload_size);

	    // We must correct DTS synword when converting to 14bit
	    if (spdif_parse_t->spdif_bs == BITSTREAM_14LE)
	    {
	    	spdif_parse_t->buf[sizeof(spdif_header_type_s) + 3] = 0xe8;
		}

		spdif_make_header(&spdif_parse_t->buf[0],spdif_parse_t->hdr.spdif_type,(unsigned short)payload_size*8);
	}else{
	    payload_size = bs_convert(raw_frame, raw_size, spdif_parse_t->hdr.bs_type, &spdif_parse_t->buf[0], spdif_parse_t->spdif_bs);
	    memset(&spdif_parse_t->buf[payload_size], 0, spdif_frame_size - payload_size);

	    // We must correct DTS synword when converting to 14bit
	    if(spdif_parse_t->spdif_bs == BITSTREAM_14LE)
	      spdif_parse_t->buf[3] = 0xe8;
	}

	if (!payload_size)
	{
		printf("Error!!! payload_size %d\n",payload_size);
		return 0;
	}

	// 5. send spdif frame..
	return 1;
}

unsigned char* spdif_parse_get_buf(void)
{
	return &spdif_parse_t->buf[0];
}

unsigned int spdif_parse_get_frame_size(void)
{
	return spdif_parse_t->spdif_frame_size;
}

unsigned int spdif_parse_get_endian(void)
{
	int return_endian;
	
	switch(spdif_parse_t->spdif_bs)
	{
		case BITSTREAM_16BE:
		case BITSTREAM_32BE:
		case BITSTREAM_14BE:
			return_endian = 0;
			break;
		case BITSTREAM_16LE:
		case BITSTREAM_32LE:
		case BITSTREAM_14LE:
			return_endian = 1;
			break;
		default:
			return_endian = 1;
			break;
	}
	return return_endian;
}


