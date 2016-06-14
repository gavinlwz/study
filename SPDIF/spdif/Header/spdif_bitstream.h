#ifndef TCC_SPDIF_BITSTREAM_H              
#define TCC_SPDIF_BITSTREAM_H

#define array_size(array) (sizeof(array) / sizeof(array[0]))

#define int2be32(i) spdif_swab_s32(i)
#define int2le32(i) (i)
#define int2be16(i) spdif_swab_s16(i)
#define int2le16(i) (i)

#define be2int32(i) spdif_swab_s32(i)
#define le2int32(i) (i)
#define be2int16(i) spdif_swab_s16(i)
#define le2int16(i) (i)

#define uint2be32(i) spdif_swab_u32(i)
#define uint2le32(i) (i)
#define uint2be16(i) spdif_swab_u16(i)
#define uint2le16(i) (i)

#define be2uint32(i) spdif_swab_u32(i)
#define le2uint32(i) (i)
#define be2uint16(i) spdif_swab_u16(i)
#define le2uint16(i) (i)



int bs_convert(const unsigned char *in_buf, int size, int in_bs, unsigned char *out_buf, int out_bs);

typedef int (*bs_conv_t)(const unsigned char *in_buf, int size, unsigned char *out_buf);
bs_conv_t bs_conversion(int bs_from, int bs_to);

int bs_conv_copy(const unsigned char *in_buf, int size, unsigned char *out_buf);
int bs_conv_swab16(const unsigned char *in_buf, int size, unsigned char *out_buf);

int bs_conv_8_14be(const unsigned char *in_buf, int size, unsigned char *out_buf);
int bs_conv_8_14le(const unsigned char *in_buf, int size, unsigned char *out_buf);
int bs_conv_14be_8(const unsigned char *in_buf, int size, unsigned char *out_buf);
int bs_conv_14le_8(const unsigned char *in_buf, int size, unsigned char *out_buf);

int bs_conv_16le_14be(const unsigned char *in_buf, int size, unsigned char *out_buf);
int bs_conv_16le_14le(const unsigned char *in_buf, int size, unsigned char *out_buf);
int bs_conv_14be_16le(const unsigned char *in_buf, int size, unsigned char *out_buf);
int bs_conv_14le_16le(const unsigned char *in_buf, int size, unsigned char *out_buf);


#endif /* TCC_SPDIF_BITSTREAM_H */
