#ifndef DRV3_DEC_H
#define DRV3_DEC_H

#include <cmath>
#include <QException>
#include <QHash>
#include <QTextCodec>
#include <QTextStream>
#include <QVector>
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
    uint unk2;
    QList<SpcSubfile> subfiles;
};

struct WrdFile
{
    QString filename;
    QStringList labels;
    QList<QByteArray> code;
    QStringList cmds;
    QStringList strings;
};

inline uchar bit_reverse(uchar b);
SpcFile spc_from_data(const QByteArray &data);
QByteArray spc_to_data(const SpcFile &spc);
QByteArray spc_dec(const QByteArray &data, int dec_size = -1);
QByteArray spc_cmp(const QByteArray &data);
QByteArray srd_dec(const QByteArray &data);
QByteArray srd_dec_chunk(const QByteArray &chunk, QString cmp_mode);
QStringList get_stx_strings(const QByteArray &data);
QByteArray repack_stx_strings(int table_len, QHash<int, QString> strings);
WrdFile wrd_from_data(const QByteArray &data);
QByteArray wrd_to_data(const WrdFile &wrd);

#endif // DRV3_DEC_H
