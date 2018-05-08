#ifndef SPC_H
#define SPC_H

#include "utils_global.h"
#include "binarydata.h"

const QString SPC_MAGIC = "CPS.";
const QString SPC_TABLE_MAGIC = "Root";

struct UTILS_EXPORT SpcSubfile
{
    QString filename;
    QByteArray data;
    ushort cmp_flag;
    ushort unk_flag;
    int cmp_size;
    int dec_size;
    int name_len;
};

struct UTILS_EXPORT SpcFile
{
    QString filename;
    QByteArray unk1;
    uint unk2;
    QList<SpcSubfile> subfiles;
};

UTILS_EXPORT SpcFile spc_from_bytes(const QByteArray &bytes);
UTILS_EXPORT QByteArray spc_to_bytes(const SpcFile &spc);
UTILS_EXPORT QByteArray spc_dec(const QByteArray &bytes, int dec_size = -1);
UTILS_EXPORT QByteArray spc_cmp(const QByteArray &bytes);
UTILS_EXPORT QByteArray srd_dec(const QByteArray &bytes);
UTILS_EXPORT QByteArray srd_dec_chunk(const QByteArray &chunk, QString cmp_mode);

#endif // SPC_H
