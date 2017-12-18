#ifndef DRV3_DEC_H
#define DRV3_DEC_H

#include <cmath>
#include <QTextCodec>
#include <QTextStream>
#include "binarydata.h"

inline uchar bit_reverse(uchar b);
BinaryData spc_dec(BinaryData &data);
BinaryData spc_cmp(BinaryData &data);
BinaryData srd_dec(BinaryData &data);
BinaryData srd_dec_chunk(BinaryData &chunk, QString cmp_mode);
QStringList get_stx_strings(BinaryData &data);
BinaryData repack_stx_strings(int table_len, QMap<int, QString> strings);

#endif // DRV3_DEC_H
