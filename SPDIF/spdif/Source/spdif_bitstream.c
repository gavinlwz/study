#include "spdif_defs.h"
#include "spdif_bitstream.h"


// use general inline functions
inline unsigned int spdif_swab_u32(unsigned int i)
{ 
	return(i >> 24) | (i >> 8) & 0xff00 | (i << 8) & 0xff0000 | (i << 24); 
}

inline unsigned int spdif_swab_s32(int i)
{ 
	return (int)spdif_swab_u32((unsigned int)i);
}

inline unsigned short spdif_swab_u16(unsigned short i)
{ 
	return (i << 8) | (i >> 8);
}

inline short spdif_swab_s16(short i)
{ 
	return (short)spdif_swab_u16((unsigned short)i);
}


static const int bs_index[] =
{
  BITSTREAM_8,
  BITSTREAM_16BE,
  BITSTREAM_16LE,
  BITSTREAM_14BE,
  BITSTREAM_14LE
};


static const bs_conv_t conv[7][7] = 
{ 
//        8              16be          16le              14be            14le
  { bs_conv_copy,   bs_conv_copy,   bs_conv_swab16,    bs_conv_8_14be,    bs_conv_8_14le    }, // 8
  { bs_conv_copy,   bs_conv_copy,   bs_conv_swab16,    bs_conv_8_14be,    bs_conv_8_14le    }, // 16be
  { bs_conv_swab16, bs_conv_swab16, bs_conv_copy,      bs_conv_16le_14be, bs_conv_16le_14le }, // 16le
  { bs_conv_14be_8, bs_conv_14be_8, bs_conv_14be_16le, bs_conv_copy,      bs_conv_swab16    }, // 14be
  { bs_conv_14le_8, bs_conv_14le_8, bs_conv_14le_16le, bs_conv_swab16,    bs_conv_copy      }, // 14le
};

bs_conv_t bs_conversion(int bs_from, int bs_to)
{
  int i = 0;
  int ibs_from = -1;
  int ibs_to = -1;

  for (i = 0; i < array_size(bs_index); i++)
    if (bs_index[i] == bs_from)
    {
      ibs_from = i;
      break;
    }

  for (i = 0; i < array_size(bs_index); i++)
    if (bs_index[i] == bs_to)
    {
      ibs_to = i;
      break;
    }

  if (ibs_from == -1 || ibs_to == -1)
    return FALSE;
  else
    return conv[ibs_from][ibs_to];
}

int bs_convert(const unsigned char *in_buf, int size, int in_bs, unsigned char *out_buf, int out_bs)
{
  bs_conv_t conv = bs_conversion(in_bs, out_bs);
  if (conv)
    return (*conv)(in_buf, size, out_buf);
  else
    return 0;
}



int bs_conv_copy(const unsigned char *in_buf, int size, unsigned char *out_buf)
{ 
	memcpy(out_buf, in_buf, size);
	return size; 
}

int bs_conv_swab16(const unsigned char *in_buf, int size, unsigned char *out_buf)
{
  // If input size is odd we add a zero byte to the end.
  // Therefore output buffer size MUST BE LARGER than input buffer.
	unsigned short *in16;
	unsigned short *out16;
	int i;

  if (size & 1)
    out_buf[size++] = 0;

  in16 = (unsigned short *)in_buf;
  out16 = (unsigned short *)out_buf;
  i = size >> 1;
  while (i--)
    out16[i] = spdif_swab_u16(in16[i]);

  return size;
}

///////////////////////////////////////////////////////////////////////////////
//                               byte <-> 14be
///////////////////////////////////////////////////////////////////////////////

int bs_conv_8_14be(const unsigned char *in_buf, int size, unsigned char *out_buf)
{
  // We expand the buffer size so output buffer size MUST BE LARGER than
  // input size specified.

  // We can do inplace conversion. You can specify the same input and output
  // buffer pointers.

  // We convert each 7 bytes into 4 16bit words with 14 data bits each.
  // If input frame size is not multiply of 7 we add zeros to the end of the
  // frame to form integral number of 16bit (14bit) words. So output frame
  // size is always even. But in this case exact backward conversion is not
  // possible!

  static const int inc[7] = { 0, 2, 4, 4, 6, 6, 8 };
  unsigned char *dst;
  unsigned char *src;
  int i;	
  unsigned int w1,w2;
  
  int n = size / 7;
  int r = size % 7;
  src = in_buf + n * 7;
  dst = out_buf + n * 8;

  size = n * 8 + inc[r];

  if (r)
  {
    // copy frame's tail and zero the rest
    i = 0;
    for (; i < r; i++) dst[i] = src[i];
    for (; i < 7; i++) dst[i] = 0;

    // convert frame's tail
    w1 = be2int32(*(unsigned int *)(dst + 0));
    w2 = be2int32(*(unsigned int *)(dst + 3));
    w1 = ((w1 >> 2) & 0x3fff0000) | ((w1 >> 4) & 0x00003fff);
    w2 = ((w2 << 2) & 0x3fff0000) | (w2 & 0x00003fff);
    (*(unsigned int *)(dst + 0)) = int2be32(w1);
    (*(unsigned int *)(dst + 4)) = int2be32(w2);
  }

  src -= 7;
  dst -= 8;
  while (n--)
  {
    w1 = be2int32(*(unsigned int *)(src + 0));
    w2 = be2int32(*(unsigned int *)(src + 3));
    w1 = ((w1 >> 2) & 0x3fff0000) | ((w1 >> 4) & 0x00003fff);
    w2 = ((w2 << 2) & 0x3fff0000) | (w2 & 0x00003fff);
    (*(unsigned int *)(dst + 0)) = int2be32(w1);
    (*(unsigned int *)(dst + 4)) = int2be32(w2);

    src -= 7;
    dst -= 8;
  }

  return size;
}

int bs_conv_14be_8(const unsigned char *in_buf, int size, unsigned char *out_buf)
{
  // Input frame size MUST BE EVEN!!!

  // We convert each 4 16bit words with 14 data bits each into 7 bytes.
  // If input frame size is not multiply of 8 we add zeros to the end of the
  // frame to form integral number bytes. But in this case exact backward
  // conversion is not possible!

  int n = size / 8;
  int r = size % 8;
  const unsigned char *src = in_buf;
  unsigned char *dst = out_buf;
  unsigned int w1,w2,i;	

  size = n * 7 + r;

  while (n--)
  {
    w1 = be2int32(*(unsigned int *)(src + 0));
    w2 = be2int32(*(unsigned int *)(src + 4));
    w2 = ((w2 & 0x3fff0000) >> 2) | (w2 & 0x00003fff) | (w1 << 28);
    w1 = ((w1 & 0x3fff0000) << 2) | ((w1 & 0x00003fff) << 4);
    (*(unsigned int *)(dst + 0)) = int2be32(w1);
    (*(unsigned int *)(dst + 3)) = int2be32(w2);

    src += 8;
    dst += 7;
  }

  if (r)
  {
    i = 0;
    for (; i < r; i++) dst[i] = src[i];
    for (; i < 8; i++) dst[i] = 0;

    w1 = be2int32(*(unsigned int *)(dst + 0));
    w2 = be2int32(*(unsigned int *)(dst + 4));
    w2 = ((w2 & 0x3fff0000) >> 2) | (w2 & 0x00003fff) | (w1 << 28);
    w1 = ((w1 & 0x3fff0000) << 2) | ((w1 & 0x00003fff) << 4);
    (*(unsigned int *)(dst + 0)) = int2be32(w1);
    (*(unsigned int *)(dst + 3)) = int2be32(w2);
  }

  return size;
}

///////////////////////////////////////////////////////////////////////////////
//                               byte <-> 14le
///////////////////////////////////////////////////////////////////////////////


int bs_conv_8_14le(const unsigned char *in_buf, int size, unsigned char *out_buf)
{
  // We expand the buffer size so output buffer size MUST BE LARGER than
  // input size specified.

  // We can do inplace conversion. You can specify the same input and output
  // buffer pointers.

  // We convert each 7 bytes into 4 16bit words with 14 data bits each.
  // If input frame size is not multiply of 7 we add zeros to the end of the
  // frame to form integral number of 16bit (14bit) words. So output frame
  // size is always even. But in this case exact backward conversion is not
  // possible!

  static const int inc[7] = { 0, 2, 4, 4, 6, 6, 8 };
  int n = size / 7;
  int r = size % 7;
  const unsigned char *src = in_buf + n * 7;
  unsigned char *dst = out_buf + n * 8;
  unsigned int i,w1,w2;	


  size = n * 8 + inc[r];

  if (r)
  {
    // copy frame's tail and zero the rest
    i = 0;
    for (; i < r; i++) dst[i] = src[i];
    for (; i < 7; i++) dst[i] = 0;

    // convert frame's tail
    w1 = be2int32(*(unsigned int *)(dst + 0));
    w2 = be2int32(*(unsigned int *)(dst + 3));
    w1 = ((w1 >> 2) & 0x3fff0000) | ((w1 >> 4) & 0x00003fff);
    w2 = ((w2 << 2) & 0x3fff0000) | (w2 & 0x00003fff);
    w1 = ((w1 & 0xff00ff00) >> 8) | ((w1 & 0x00ff00ff) << 8);
    w2 = ((w2 & 0xff00ff00) >> 8) | ((w2 & 0x00ff00ff) << 8);
    (*(unsigned int *)(dst + 0)) = int2be32(w1);
    (*(unsigned int *)(dst + 4)) = int2be32(w2);
  }

  src -= 7;
  dst -= 8;
  while (n--)
  {
    w1 = be2int32(*(unsigned int *)(src + 0));
    w2 = be2int32(*(unsigned int *)(src + 3));
    w1 = ((w1 >> 2) & 0x3fff0000) | ((w1 >> 4) & 0x00003fff);
    w2 = ((w2 << 2) & 0x3fff0000) | (w2 & 0x00003fff);
    w1 = ((w1 & 0xff00ff00) >> 8) | ((w1 & 0x00ff00ff) << 8);
    w2 = ((w2 & 0xff00ff00) >> 8) | ((w2 & 0x00ff00ff) << 8);
    (*(unsigned int *)(dst + 0)) = int2be32(w1);
    (*(unsigned int *)(dst + 4)) = int2be32(w2);
    src -= 7;
    dst -= 8;
  }

  return size;
}

int bs_conv_14le_8(const unsigned char *in_buf, int size, unsigned char *out_buf)
{

  // We convert each 4 16bit words with 14 data bits each into 7 bytes.
  // If input frame size is not multiply of 8 we add zeros to the end of the
  // frame to form integral number bytes. But in this case exact backward
  // conversion is not possible!

  int n = size / 8;
  int r = size % 8;
  const unsigned char *src = in_buf;
  unsigned char *dst = out_buf;
  unsigned int i,w1,w2;	

  size = n * 7 + r;

  while (n--)
  {
    w1 = be2int32(*(unsigned int *)(src + 0));
    w2 = be2int32(*(unsigned int *)(src + 4));
    w2 = ((w2 & 0xff00ff00) >> 8) | ((w2 & 0x00ff00ff) << 8);
    w1 = ((w1 & 0xff00ff00) >> 8) | ((w1 & 0x00ff00ff) << 8);
    w2 = ((w2 & 0x3fff0000) >> 2) | (w2 & 0x00003fff) | (w1 << 28);
    w1 = ((w1 & 0x3fff0000) << 2) | ((w1 & 0x00003fff) << 4);
    (*(unsigned int *)(dst + 0)) = int2be32(w1);
    (*(unsigned int *)(dst + 3)) = int2be32(w2);

    src += 8;
    dst += 7;
  }

  if (r)
  {
    i = 0;
    for (; i < r; i++) dst[i] = src[i];
    for (; i < 8; i++) dst[i] = 0;

    w1 = be2int32(*(unsigned int *)(dst + 0));
    w2 = be2int32(*(unsigned int *)(dst + 4));
    w2 = ((w2 & 0xff00ff00) >> 8) | ((w2 & 0x00ff00ff) << 8);
    w1 = ((w1 & 0xff00ff00) >> 8) | ((w1 & 0x00ff00ff) << 8);
    w2 = ((w2 & 0x3fff0000) >> 2) | (w2 & 0x00003fff) | (w1 << 28);
    w1 = ((w1 & 0x3fff0000) << 2) | ((w1 & 0x00003fff) << 4);
    (*(unsigned int *)(dst + 0)) = int2be32(w1);
    (*(unsigned int *)(dst + 3)) = int2be32(w2);
  }

  return size;
}

///////////////////////////////////////////////////////////////////////////////
//                              16bit <-> 14bit
///////////////////////////////////////////////////////////////////////////////

int bs_conv_16le_14be(const unsigned char *in_buf, int size, unsigned char *out_buf)
{
  size = bs_conv_swab16(in_buf, size, out_buf);
  size = bs_conv_8_14be(out_buf, size, out_buf);
  return size; 
}

int bs_conv_16le_14le(const unsigned char *in_buf, int size, unsigned char *out_buf)
{
  size = bs_conv_swab16(in_buf, size, out_buf);
  size = bs_conv_8_14le(out_buf, size, out_buf);
  return size; 
}

int bs_conv_14be_16le(const unsigned char *in_buf, int size, unsigned char *out_buf)
{
  size = bs_conv_14be_8(in_buf, size, out_buf);
  size = bs_conv_swab16(out_buf, size, out_buf);
  return size; 
}

int bs_conv_14le_16le(const unsigned char *in_buf, int size, unsigned char *out_buf)
{
  size = bs_conv_14le_8(in_buf, size, out_buf);
  size = bs_conv_swab16(out_buf, size, out_buf);
  return size; 
}

