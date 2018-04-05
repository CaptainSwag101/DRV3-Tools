#include "dat.h"

DatFile dat_from_bytes(const QByteArray &bytes)
{
    DatFile result;

    int pos = 0;
    const uint struct_count = bytes_to_num<uint>(bytes, pos);
    const uint struct_size = bytes_to_num<uint>(bytes, pos);
    const uint var_count = bytes_to_num<uint>(bytes, pos);

    for (int v = 0; v < var_count; v++)
    {
        const QString var_name = bytes_to_str(bytes, pos);
        const QString var_type = bytes_to_str(bytes, pos);
        pos += 2;   // Skip the 2 "var terminator" bytes?
        result.var_info.append(QPair<QString, QString>(var_name, var_type));
    }

    pos += (0x10 - (pos % 0x10)) % 0x10;

    for (int d = 0; d < struct_count; d++)
    {
        const QByteArray data = get_bytes(bytes, pos, struct_size);
        result.struct_data.append(data);
    }

    const ushort string_count = bytes_to_num<ushort>(bytes, pos);
    const ushort unk_count = bytes_to_num<ushort>(bytes, pos);

    for (int s = 0; s < string_count; s++)
    {
        const QString str = bytes_to_str(bytes, pos);
        result.strings.append(str);
    }

    pos++;

    for (int u = 0; u < unk_count; u++)
    {
        QByteArray unk;

        uchar c1 = bytes.at(pos++);
        uchar c2 = bytes.at(pos++);
        while (c1 != 0 || c2 != 0)
        {
            unk.append(c1);
            unk.append(c2);
            c1 = bytes.at(pos++);
            c2 = bytes.at(pos++);
        }

        result.unk_data.append(unk);
    }

    return result;
}

QByteArray dat_to_bytes(const DatFile &dat)
{
    QByteArray result;

    return result;
}
