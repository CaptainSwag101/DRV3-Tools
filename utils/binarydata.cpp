#include "binarydata.h"

template <class T> QByteArray num_to_bytes(T num, bool big_endian)
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

template <class T> T bytes_to_num<T>(const QByteArray &data, int &pos, bool big_endian)
{
    const int num_size = sizeof(T);
    T result = 0;

    for (int i = 0; i != num_size; i++)
    {
        if (big_endian)
            result |= (data[pos + i] & 0xFF) << ((num_size - i - 1) * 8);
        else
            result |= (data[pos + i] & 0xFF) << (i * 8);
    }

    pos += num_size;
    return result;
}

QString bytes_to_str(const QByteArray &data, int &pos, int len, bool utf16)
{
    QString result;
    result.reserve(len);

    int i = 0;
    while (len < 0 || i < len)
    {
        QChar c;
        if (utf16)
            c = QChar(bytes_to_num<ushort>(data, pos));
        else
            c = QChar(bytes_to_num<uchar>(data, pos));

        i++;

        if (len < 0 && c == QChar(0))
            break;

        result.append(c);
    }

    return result;
}

QByteArray get_bytes(const QByteArray &data, int &pos, int len)
{
    QByteArray result = data.mid(pos, len);
    pos += result.size();
    return result;
}
