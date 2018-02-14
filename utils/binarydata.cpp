#include "binarydata.h"

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

    // Don't increment "pos" because bytes_to_num() does that already
    return result;
}

QByteArray get_bytes(const QByteArray &data, int &pos, int len)
{
    QByteArray result = data.mid(pos, len);
    pos += result.size();
    return result;
}
