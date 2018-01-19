#ifndef DRV3_DEC_H
#define DRV3_DEC_H

#include <cmath>
#include <QMultiMap>
#include <QTextCodec>
#include <QTextStream>
#include "binarydata.h"

const QString SPC_MAGIC = "CPS.";
const QString SPC_TABLE_MAGIC = "Root";
const QString STX_MAGIC = "STXT";

struct SpcSubfile
{
    QString filename;
    BinaryData data;
    ushort cmp_flag;
    ushort unk_flag;
    uint cmp_size;
    uint dec_size;
    uint name_len;
};

struct SpcFile
{
    QString filename;
    QByteArray unk1;
    uint unk2;
    QList<SpcSubfile> subfiles;
};

struct StxFile
{

};

inline uchar bit_reverse(uchar b);
BinaryData spc_dec(BinaryData &data);
BinaryData spc_cmp(BinaryData &data);
BinaryData srd_dec(BinaryData &data);
BinaryData srd_dec_chunk(BinaryData &chunk, QString cmp_mode);
QStringList get_stx_strings(BinaryData &data);
BinaryData repack_stx_strings(int table_len, QMap<int, QString> strings);

#endif // DRV3_DEC_H
