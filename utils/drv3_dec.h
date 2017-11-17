#ifndef DRV3_DEC_H
#define DRV3_DEC_H

#include "binarydata.h"

inline uchar bit_reverse(uchar b);
BinaryData spc_dec(BinaryData &data);
BinaryData srd_dec(BinaryData &data);
BinaryData srd_dec_chunk(BinaryData &chunk, QString cmp_mode);

#endif // DRV3_DEC_H
