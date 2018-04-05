#ifndef DAT_H
#define DAT_H

#include "utils_global.h"
#include "binarydata.h"

struct UTILSSHARED_EXPORT DatFile
{
    QString filename;
    QList<QPair<QString, QString>> var_info; // var_name, var_type, 0x00 0x01 0x00 (0x01 = var count? terminator?)
    QList<QByteArray> struct_data;
    QStringList strings;
    QList<QByteArray> unk_data;
};

UTILSSHARED_EXPORT DatFile dat_from_bytes(const QByteArray &bytes);
UTILSSHARED_EXPORT QByteArray dat_to_bytes(const DatFile &dat);

#endif // DAT_H
