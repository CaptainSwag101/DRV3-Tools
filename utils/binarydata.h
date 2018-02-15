#ifndef BINARYDATA_H
#define BINARYDATA_H

#include <QByteArray>
#include <QString>

QString bytes_to_str(const QByteArray &data, int &pos, int len = -1, bool utf16 = false);
QByteArray get_bytes(const QByteArray &data, int &pos, int len = -1);

template <typename T> QByteArray num_to_bytes(T num, bool big_endian = false)
{
    const int num_size = sizeof(T);

    QByteArray result;
    result.reserve(num_size);

    for (int i = 0; i != num_size; i++)
    {
        if (big_endian)
            result.prepend((char)(num >> (i * 8)));
        else
            result.append((char)(num >> (i * 8)));
    }

    return result;
}

template <typename T> T bytes_to_num(const QByteArray &data, int &pos, bool big_endian = false)
{
    const int num_size = sizeof(T);
    T result = 0;

    for (int i = 0; i != num_size; i++)
    {
        if (big_endian)
            result |= (data.at(pos + i) & 0xFF) << ((num_size - i - 1) * 8);
        else
            result |= (data.at(pos + i) & 0xFF) << (i * 8);
    }

    pos += num_size;
    return result;
}

#endif // BINARYDATA_H
