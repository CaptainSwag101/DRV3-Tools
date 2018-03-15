#ifndef SPC_H
#define SPC_H

#include "utils_global.h"
#include "binarydata.h"

const QString SPC_MAGIC = "CPS.";
const QString SPC_TABLE_MAGIC = "Root";

struct UTILSSHARED_EXPORT SpcSubfile
{
    QString filename;
    QByteArray data;
    ushort cmp_flag;
    ushort unk_flag;
    int cmp_size;
    int dec_size;
    int name_len;
};

struct UTILSSHARED_EXPORT SpcFile
{
    QString filename;
    QByteArray unk1;
    uint unk2;
    QList<SpcSubfile> subfiles;
};

UTILSSHARED_EXPORT SpcFile spc_from_data(const QByteArray &data);
UTILSSHARED_EXPORT QByteArray spc_to_data(const SpcFile &spc);
UTILSSHARED_EXPORT QByteArray spc_dec(const QByteArray &data, int dec_size = -1);
UTILSSHARED_EXPORT QByteArray spc_cmp(const QByteArray &data);
UTILSSHARED_EXPORT QByteArray srd_dec(const QByteArray &data);
UTILSSHARED_EXPORT QByteArray srd_dec_chunk(const QByteArray &chunk, QString cmp_mode);

#endif // SPC_H
