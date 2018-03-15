#ifndef UTILS_GLOBAL_H
#define UTILS_GLOBAL_H

#include <QtCore/qglobal.h>
#include <QTextStream>

#if defined(UTILS_LIBRARY)
#  define UTILSSHARED_EXPORT Q_DECL_EXPORT
#else
#  define UTILSSHARED_EXPORT Q_DECL_IMPORT
#endif

static QTextStream cout(stdout);

// From https://graphics.stanford.edu/~seander/bithacks.html#ReverseByteWith64BitsDiv
UTILSSHARED_EXPORT inline uchar bit_reverse(uchar b)
{
    return (b * 0x0202020202 & 0x010884422010) % 1023;
}

#endif // UTILS_GLOBAL_H
