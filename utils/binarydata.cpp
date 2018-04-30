#include "binarydata.h"

QString str_from_bytes(const QByteArray &data, int &pos, const int len, const QString codec)
{
    const int orig_pos = pos;

    if (codec.startsWith("UTF16", Qt::CaseInsensitive))
    {
        QString result;

        while (len < 0 || (pos - orig_pos) < len)
        {
            const ushort c = num_from_bytes<ushort>(data, pos);

            if (c == 0)
                break;

            result.append(QChar(c));
        }

        return result;
    }
    else
    {
        QByteArray result;

        while (pos < data.size() && (len < 0 || (pos - orig_pos) < len))
        {
            const char c = data.at(pos++);

            if (c == 0)
                break;

            result.append(c);
        }

        return QString::fromUtf8(result);
    }
}

QByteArray str_to_bytes(const QString &string, const bool utf16)
{
    QByteArray result;
    if (utf16)
    {
        for (int i = 0; i < string.size(); i++)
        {
            result.append(num_to_bytes(string.at(i).unicode()));
        }
        result.append(num_to_bytes((ushort)0x00));    // Null terminator
    }
    else
    {
        result = string.toUtf8();
        result.append(num_to_bytes((uchar)0x00));     // Null terminator
    }

    return result;
}

QByteArray get_bytes(const QByteArray &data, int &pos, const int len)
{
    const QByteArray result = data.mid(pos, len);
    pos += result.size();
    return result;
}

QString num_to_hex(const ulong num, const uchar pad_len)
{
    return QString::number(num, 16).toUpper().rightJustified(pad_len, '0');
}
