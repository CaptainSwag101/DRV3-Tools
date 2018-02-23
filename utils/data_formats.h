#ifndef DRV3_DEC_H
#define DRV3_DEC_H

#include <algorithm>
#include <cmath>
#include <iterator>
#include <QHash>
#include <QTextCodec>
#include <QTextStream>
#include "binarydata.h"

const QString SPC_MAGIC = "CPS.";
const QString SPC_TABLE_MAGIC = "Root";
const QString STX_MAGIC = "STXT";

struct SpcSubfile
{
    QString filename;
    QByteArray data;
    ushort cmp_flag;
    ushort unk_flag;
    int cmp_size;
    int dec_size;
    int name_len;
};

struct SpcFile
{
    QString filename;
    QByteArray unk1;
    int unk2;
    QList<SpcSubfile> subfiles;
};

inline uchar bit_reverse(uchar b);
QByteArray spc_dec(const QByteArray &data, int dec_size = -1);
QByteArray spc_cmp(const QByteArray &data);
QByteArray srd_dec(const QByteArray &data);
QByteArray srd_dec_chunk(const QByteArray &chunk, QString cmp_mode);
QStringList get_stx_strings(const QByteArray &data);
QByteArray repack_stx_strings(int table_len, QHash<int, QString> strings);

#endif // DRV3_DEC_H
