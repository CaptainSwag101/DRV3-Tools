#ifndef DAT_H
#define DAT_H

#include "utils_global.h"
#include "binarydata.h"

struct UTILS_EXPORT DatFile
{
    QString filename;
    QStringList data_names;
    QStringList data_types; // var_name, var_type, 0x0100 (var count + null terminator?)
    QVector<QVector<QByteArray>> data;
    QStringList labels;
    QStringList refs;
};

UTILS_EXPORT DatFile dat_from_bytes(const QByteArray &bytes);
UTILS_EXPORT QByteArray dat_to_bytes(const DatFile &dat);

#endif // DAT_H
