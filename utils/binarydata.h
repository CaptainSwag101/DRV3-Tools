#pragma once

#include "utils_global.h"
#include <cmath>
#include <QByteArray>
#include <QString>

UTILSSHARED_EXPORT QString str_from_bytes(const QByteArray &data, int &pos, const int len = -1, const QString codec = "UTF8");
UTILSSHARED_EXPORT QByteArray str_to_bytes(const QString &string, const bool utf16 = false);
UTILSSHARED_EXPORT QByteArray get_bytes(const QByteArray &data, int &pos, const int len = -1);
UTILSSHARED_EXPORT QString num_to_hex(const ulong num, const uchar pad_len);

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
    QByteArray result(num_size, 0x00);
    const char *byte_array = reinterpret_cast<const char*>(&num);
    const int arr_size = std::min((int)sizeof(byte_array), num_size);

    if (big_endian)
    {
        for (int i = 0; i < arr_size; i++)
        {
            result[i] = byte_array[arr_size - i - 1];
        }
    }
    else
    {
        for (int i = 0; i < arr_size; i++)
        {
            result[i] = byte_array[i];
        }
    }

    //delete byte_array;
    return result;
}
