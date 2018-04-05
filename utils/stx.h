#ifndef STX_H
#define STX_H

#include "utils_global.h"
#include "binarydata.h"

const QString STX_MAGIC = "STXT";

UTILSSHARED_EXPORT QStringList get_stx_strings(const QByteArray &bytes);
UTILSSHARED_EXPORT QByteArray repack_stx_strings(QStringList strings);

#endif // STX_H
