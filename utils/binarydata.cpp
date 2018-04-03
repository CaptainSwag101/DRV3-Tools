#include "binarydata.h"
#include <QTextEncoder>

QString bytes_to_str(const QByteArray &data, int &pos, const int len, const bool utf16)
{
    QString result;
    result.reserve(len);

    const int orig_pos = pos;
    if (utf16)
    {
        while (len < 0 || (pos - orig_pos) < len)
        {
            const QChar c = QChar(bytes_to_num<ushort>(data, pos));

            if (c == QChar(0))
                break;

            result.append(c);
        }
    }
    else
    {
        while (len < 0 || (pos - orig_pos) < len)
        {
            const QChar c = QChar(data.at(pos++));

            if (c == QChar(0))
                break;

            result.append(c);
        }
    }

    // Don't increment "pos" because bytes_to_num() does that already
    return result;
}

QByteArray str_to_bytes(const QString &string, const bool utf16)
{
    QByteArray result;

    if (utf16)
    {
        QByteArray str_data;
        for (int i = 0; i < string.size(); i++)
        {
            str_data.append(num_to_bytes(string.at(i).unicode()));
        }
        result.append((uchar)str_data.size());
        result.append(str_data);
        result.append(num_to_bytes((ushort)0x00));   // Null terminator
    }
    else
    {
        QByteArray str_data;
        str_data = string.toUtf8();
        result.append((uchar)str_data.size());
        result.append(str_data);
        result.append(num_to_bytes((uchar)0x00));    // Null terminator
    }

    return result;
}

QByteArray get_bytes(const QByteArray &data, int &pos, const int len)
{
    QByteArray result = data.mid(pos, len);
    pos += result.size();
    return result;
}
