#include "spdif_defs.h"
#include "spdif_bitstream.h"
#include "ac3_header.h"

static const int halfrate_tbl[12] = 
{ 
  0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3
};
static const int lfe_mask[] = 
{ 
  16, 16, 4, 4, 4, 1, 4, 1
};
static const int bitrate_tbl[] = 
{ 
  32,  40,  48,  56,  64,  80,  96, 112,
 128, 160, 192, 224, 256, 320, 384, 448,
 512, 576, 640 
};



int ac3_header_parse(unsigned char* hdr,spdif_header_info_s *hinfo)
{
  int fscod;
  int frmsizecod;

  int acmod;
//  int dolby = NO_RELATION;

  int halfrate;
  int bitrate;
  int sample_rate;

  /////////////////////////////////////////////////////////
  // 8 bit or 16 bit big endian stream sync
  if ((hdr[0] == 0x0b) && (hdr[1] == 0x77))
  {
    // constraints
    if (hdr[5] >= 0x60)         return FALSE;   // 'bsid'
    if ((hdr[4] & 0x3f) > 0x25) return FALSE;   // 'frmesizecod'
    if ((hdr[4] & 0xc0) > 0x80) return FALSE;   // 'fscod'
    if (!hinfo) return TRUE;

    fscod      = hdr[4] >> 6;
    frmsizecod = hdr[4] & 0x3f;
    acmod      = hdr[6] >> 5;

//    if (acmod == 2 && (hdr[6] & 0x18) == 0x10)
//      dolby = RELATION_DOLBY;

    if (hdr[6] & lfe_mask[acmod])
      acmod |= 8;

    halfrate   = halfrate_tbl[hdr[5] >> 3];
    bitrate    = bitrate_tbl[frmsizecod >> 1];

    hinfo->bs_type = BITSTREAM_8;
  }
  /////////////////////////////////////////////////////////
  // 16 bit low endian stream sync
  else if ((hdr[1] == 0x0b) && (hdr[0] == 0x77))
  {
    // constraints
    if (hdr[4] >= 0x60)         return FALSE;   // 'bsid'
    if ((hdr[5] & 0x3f) > 0x25) return FALSE;   // 'frmesizecod'
    if ((hdr[5] & 0xc0) > 0x80) return FALSE;   // 'fscod'
    if (!hinfo) return TRUE;

    fscod      = hdr[5] >> 6;
    frmsizecod = hdr[5] & 0x3f;
    acmod      = hdr[7] >> 5;

 //   if (acmod == 2 && (hdr[7] & 0x18) == 0x10)
 //     dolby = RELATION_DOLBY;

    if (hdr[7] & lfe_mask[acmod])
      acmod |= 8;

    halfrate   = halfrate_tbl[hdr[4] >> 3];
    bitrate    = bitrate_tbl[frmsizecod >> 1];

    hinfo->bs_type = BITSTREAM_16LE;
  }
  else
    return FALSE;

  switch (fscod) 
  {
    case 0:    
      hinfo->frame_size = 4 * bitrate;
      sample_rate = 48000 >> halfrate;
      break;

    case 1: 
      hinfo->frame_size = 2 * (320 * bitrate / 147 + (frmsizecod & 1));
      sample_rate = 44100 >> halfrate;
      break;

    case 2: 
      hinfo->frame_size = 6 * bitrate;
      sample_rate = 32000 >> halfrate;

    default:
      return FALSE;
  }

  hinfo->scan_size = 0; // do not scan
  hinfo->nsamples = 1536;
  hinfo->spdif_type = 1; // SPDIF Pc burst-info (data type = AC3) 
  return TRUE;
}


