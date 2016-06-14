#include "spdif_defs.h"
#include "spdif_bitstream.h"
#include "dts_header.h"

int dts_header_parse(unsigned char* hdr,spdif_header_info_s *hinfo)
{
  int bs_type;
  int nblks, amode, sfreq, lff;
  int sample_rate;
  unsigned short *hdr16 = (unsigned short *)hdr;

  // 16 bits big endian bitstream
  if(hdr[0] == 0x7f && hdr[1] == 0xfe &&
     hdr[2] == 0x80 && hdr[3] == 0x01)
  {
    bs_type = BITSTREAM_16BE;
    nblks = (be2uint16(hdr16[2]) >> 2)  & 0x7f;
    amode = (be2uint16(hdr16[3]) << 2)  & 0x3c |
            (be2uint16(hdr16[4]) >> 14) & 0x03;
    sfreq = (be2uint16(hdr16[4]) >> 10) & 0x0f;
    lff   = (be2uint16(hdr16[5]) >> 9)  & 0x03;
    nblks++;
  }
  // 16 bits low endian bitstream
  else if (hdr[0] == 0xfe && hdr[1] == 0x7f &&
           hdr[2] == 0x01 && hdr[3] == 0x80)
  {
    bs_type = BITSTREAM_16LE;
    nblks = (le2uint16(hdr16[2]) >> 2)  & 0x7f;
    amode = (le2uint16(hdr16[3]) << 2)  & 0x3c |
            (le2uint16(hdr16[4]) >> 14) & 0x03;
    sfreq = (le2uint16(hdr16[4]) >> 10) & 0x0f;
    lff   = (le2uint16(hdr16[5]) >> 9)  & 0x03;
    nblks++;
  }
  // 14 bits big endian bitstream
  else if (hdr[0] == 0x1f && hdr[1] == 0xff &&
           hdr[2] == 0xe8 && hdr[3] == 0x00 &&
           hdr[4] == 0x07 && (hdr[5] & 0xf0) == 0xf0)
  {
    bs_type = BITSTREAM_14BE;
    nblks = (be2uint16(hdr16[2]) << 4)  & 0x70 |
            (be2uint16(hdr16[3]) >> 10) & 0x0f;
    amode = (be2uint16(hdr16[4]) >> 4)  & 0x3f;
    sfreq = (be2uint16(hdr16[4]) >> 0)  & 0x0f;
    lff   = (be2uint16(hdr16[6]) >> 11) & 0x03;
    nblks++;
  }
  // 14 bits low endian bitstream
  else if (hdr[0] == 0xff && hdr[1] == 0x1f &&
           hdr[2] == 0x00 && hdr[3] == 0xe8 &&
          (hdr[4] & 0xf0) == 0xf0 && hdr[5] == 0x07)
  {
    bs_type = BITSTREAM_14LE;
    nblks = (le2uint16(hdr16[2]) << 4)  & 0x70 |
            (le2uint16(hdr16[3]) >> 10) & 0x0f;
    amode = (le2uint16(hdr16[4]) >> 4)  & 0x3f;
    sfreq = (le2uint16(hdr16[4]) >> 0)  & 0x0f;
    lff   = (le2uint16(hdr16[6]) >> 11) & 0x03;
    nblks++;
  }
  // no sync
  else
    return FALSE;

  /////////////////////////////////////////////////////////
  // Constraints

  if (nblks < 6) return FALSE;            // constraint
  if (amode > 0xc) return FALSE;          // we don't work with more than 6 channels
  if (dts_sample_rates[sfreq] == 0)       // constraint
    return FALSE; 
  if (lff == 3) return FALSE;             // constraint

  /////////////////////////////////////////////////////////
  // Fill HeaderInfo

  if (!hinfo)
    return TRUE;

  sample_rate = dts_sample_rates[sfreq];

  hinfo->frame_size = 0; // do not rely on the frame size specified at the header!!!
  hinfo->scan_size = 16384; // always scan up to maximum DTS frame size
  hinfo->nsamples = nblks * 32;
  hinfo->bs_type = bs_type;

  switch (hinfo->nsamples)
  {
    case 512:  hinfo->spdif_type = 11; break;
    case 1024: hinfo->spdif_type = 12; break;
    case 2048: hinfo->spdif_type = 13; break;
    default:   hinfo->spdif_type = 0;  break; // cannot do SPDIF passthrough
  }

  return TRUE;
}

