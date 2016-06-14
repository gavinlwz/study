#ifndef TCC_SPDIF_DTS_H
#define TCC_SPDIF_DTS_H


static const int dts_sample_rates[] =
{
    0, 8000, 16000, 32000, 0, 0, 11025, 22050, 44100, 0, 0,
    12000, 24000, 48000, 96000, 192000
};

static const int dts_bit_rates[] =
{
    32000, 56000, 64000, 96000, 112000, 128000,
    192000, 224000, 256000, 320000, 384000,
    448000, 512000, 576000, 640000, 768000,
    896000, 1024000, 1152000, 1280000, 1344000,
    1408000, 1411200, 1472000, 1536000, 1920000,
    2048000, 3072000, 3840000, 1/*open*/, 2/*variable*/, 3/*lossless*/
};

static const unsigned char dts_channels[] =
{
    1, 2, 2, 2, 2, 3, 3, 4, 4, 5, 6, 6, 6, 7, 8, 8
};

static const unsigned char dts_bits_per_sample[] =
{
    16, 16, 20, 20, 0, 24, 24
};


int dts_header_parse(unsigned char* hdr,spdif_header_info_s *hinfo);

#endif /* TCC_SPDIF_DTS_H */
