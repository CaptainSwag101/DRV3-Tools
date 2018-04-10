#pragma once

#include "utils_global.h"
#include <algorithm>
#include <QByteArray>
#include <QString>

UTILSSHARED_EXPORT QString str_from_bytes(const QByteArray &data, int &pos, const int len = -1, const QString codec = "UTF-8");
UTILSSHARED_EXPORT QByteArray str_to_bytes(const QString &string, const bool utf16 = false);
UTILSSHARED_EXPORT QByteArray get_bytes(const QByteArray &data, int &pos, const int len = -1);

template <typename T> T num_from_bytes(const QByteArray &data, int &pos, const bool big_endian = false)
{
    const int num_size = sizeof(T);
    T result = 0;
    QByteArray byte_array = data.mid(pos, num_size);

    if (big_endian)
    {
        std::reverse_copy(byte_array.begin(),
                          byte_array.end(),
                          reinterpret_cast<char*>(&result));
    }
    else
    {
        std::copy(byte_array.begin(),
                  byte_array.end(),
                  reinterpret_cast<char*>(&result));
    }

    pos += num_size;
    return result;
}

template <typename T> QByteArray num_to_bytes(T num, const bool big_endian = false)
{
    const int num_size = sizeof(T);
    char *result = new char[num_size];
    const char *byte_array = reinterpret_cast<const char[num_size]>(&num);

    if (big_endian)
    {
        std::reverse_copy(reinterpret_cast<const char*>(&byte_array[0]),
                          reinterpret_cast<const char*>(&byte_array[num_size]),
                          result);
    }
    else
    {
        std::copy(reinterpret_cast<const char*>(&byte_array[0]),
                  reinterpret_cast<const char*>(&byte_array[num_size]),
                  result);
    }

    //delete byte_array;
    return QByteArray(result);
}
