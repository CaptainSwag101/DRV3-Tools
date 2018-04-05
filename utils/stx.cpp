#include "stx.h"

#include <QTextCodec>

QStringList get_stx_strings(const QByteArray &bytes)
{
    int pos = 0;
    QStringList strings;

    QString magic = bytes_to_str(bytes, pos, 4);
    if (magic != STX_MAGIC)
    {
        //cout << "Invalid STX file.\n";
        return strings;
    }

    QString lang = bytes_to_str(bytes, pos, 4);      // "JPLL" in the JP and US versions
    const uint unk1 = bytes_to_num<uint>(bytes, pos); // Table count?
    const uint table_off  = bytes_to_num<uint>(bytes, pos);
    const uint unk2 = bytes_to_num<uint>(bytes, pos);
    const uint table_len = bytes_to_num<uint>(bytes, pos);

    for (uint i = 0; i < table_len; i++)
    {
        pos = table_off + (8 * i);
        const uint str_id = bytes_to_num<uint>(bytes, pos);
        const uint str_off = bytes_to_num<uint>(bytes, pos);

        pos = str_off;

        QString str = bytes_to_str(bytes, pos, -1, "UTF-16");
        strings.append(str);
    }

    return strings;
}

QByteArray repack_stx_strings(QStringList strings)
{
    QByteArray result;
    uint table_off = 0x20;
    int table_len = strings.count();

    result.append(QString("STXTJPLL").toUtf8());    // header
    result.append(num_to_bytes((uint)0x01));        // table_count?
    result.append(num_to_bytes(table_off));         // table_off
    result.append(num_to_bytes((uint)0x08));        // unk2
    result.append(num_to_bytes(table_len));         // table_len

    int *offsets = new int[table_len];
    int highest_index = 0;
    for (int i = 0; i < table_len; i++)
    {
        result.append((table_off + (i * 8)) - (result.size()), (char)0x00); // padding

        int str_off = 0;
        for (int j = 0; j < i; j++)
        {
            if (strings[i] == strings[j])
            {
                str_off = offsets[j];
            }
        }

        // If there are no previous occurences of this string or it's the first one
        if (i == 0)
        {
            str_off = 0x20 + (8 * table_len);   // 8 bytes per table entry
        }
        else if (str_off == 0)
        {
            str_off = offsets[highest_index] + ((strings[highest_index].size() + 1) * 2);
            highest_index = i;
        }

        offsets[i] = str_off;

        result.append(num_to_bytes(i));         // str_index
        result.append(num_to_bytes(str_off));   // str_off
    }

    QStringList written;
    for (int i = 0; i < table_len; i++)
    {
        if (written.contains(strings[i]))
            continue;

        QTextCodec *codec = QTextCodec::codecForName("UTF-16");
        QTextEncoder *encoder = codec->makeEncoder(QTextCodec::IgnoreHeader);
        const QByteArray bytes = encoder->fromUnicode(strings[i]);

        result.append(bytes);
        result.append(num_to_bytes((ushort)0x00));  // Null terminator
        written.append(strings[i]);
    }
    delete[] offsets;

    return result;
}
