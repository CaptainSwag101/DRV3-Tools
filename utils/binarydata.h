#pragma once

#include "utils_global.h"
#include <QByteArray>
#include <QString>

UTILSSHARED_EXPORT QString str_from_bytes(const QByteArray &data, int &pos, const int len = -1, const QString codec = "UTF-8");
UTILSSHARED_EXPORT QByteArray str_to_bytes(const QString &string, const bool utf16 = false);
UTILSSHARED_EXPORT QByteArray get_bytes(const QByteArray &data, int &pos, const int len = -1);

template <typename T> UTILSSHARED_EXPORT inline T num_from_bytes(const QByteArray &data, int &pos, const bool big_endian = false)
{
    const int num_size = sizeof(T);
    T result = 0;
    char *byte_array = new char[num_size - 1];

    if (big_endian)
    {
        for (int i = 0; i < num_size; i++)
        {
            byte_array[i] = data.at(pos + num_size - i - 1);
        }
    }
    else
    {
        for (int i = 0; i < num_size; i++)
        {
            byte_array[i] = data.at(pos + i);
        }
    }

    std::copy(reinterpret_cast<const char*>(&byte_array[0]),
              reinterpret_cast<const char*>(&byte_array[num_size]),
              reinterpret_cast<char*>(&result));
    pos += num_size;
    delete byte_array;
    return result;
}

template <typename T> UTILSSHARED_EXPORT inline QByteArray num_to_bytes(T num, const bool big_endian = false)
{
    const int num_size = sizeof(T);

    QByteArray result;
    result.reserve(num_size);

    if (big_endian)
    {
        for (int i = 0; i != num_size; i++)
        {
            result.prepend((char)(num >> (i * 8)));
        }
    }
    else
    {
        for (int i = 0; i != num_size; i++)
        {
            result.append((char)(num >> (i * 8)));
        }
    }

    return result;
}
