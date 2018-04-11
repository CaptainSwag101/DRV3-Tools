#include "dat.h"

DatFile dat_from_bytes(const QByteArray &bytes)
{
    DatFile result;

    int pos = 0;
    const int struct_count = num_from_bytes<int>(bytes, pos);
    const int struct_size = num_from_bytes<int>(bytes, pos);
    const int var_count = num_from_bytes<int>(bytes, pos);

    if (struct_count <= 0 || struct_size <= 0 || var_count <= 0)
    {
        cout << "Error: Invalid DAT file.\n";
        cout.flush();
        throw 1;
    }

    for (int v = 0; v < var_count; v++)
    {
        const QString var_name = str_from_bytes(bytes, pos);
        const QString var_type = str_from_bytes(bytes, pos);
        pos += 2;   // Skip the 2 "var terminator" bytes?
        result.data_names.append(var_name);
        result.data_types.append(var_type);
    }

    pos += (0x10 - (pos % 0x10)) % 0x10;

    for (int d = 0; d < struct_count; d++)
    {
        QList<QByteArray> data;
        for (int v = 0; v < result.data_types.count(); v++)
        {
            const QString data_type = result.data_types.at(v);
            QByteArray var;

            if (data_type.startsWith("u") || data_type.startsWith("s") || data_type.startsWith("f"))
            {
                const int size = data_type.right(2).toInt() / 8;
                var = get_bytes(bytes, pos, size);
                data.append(var);
            }
            else if (data_type == "LABEL" || data_type == "REFER" || data_type == "ASCII" || data_type == "UTF16")
            {
                var = get_bytes(bytes, pos, 2);
                data.append(var);
            }
        }
        result.data.append(data);
    }

    const ushort label_count = num_from_bytes<ushort>(bytes, pos);
    const ushort refer_count = num_from_bytes<ushort>(bytes, pos);

    for (ushort s = 0; s < label_count; s++)
    {
        const QString str = str_from_bytes(bytes, pos);
        result.labels.append(str);
    }

    pos += (2 - (pos % 2)) % 2;

    for (ushort r = 0; r < refer_count; r++)
    {
        QByteArray unk;

        uchar c1, c2;
        do
        {
            c1 = bytes.at(pos++);
            c2 = bytes.at(pos++);
            unk.append(c1);
            unk.append(c2);

        } while (c1 != 0 || c2 != 0);

        int str_pos = 0;
        result.refs.append(str_from_bytes(unk, str_pos, -1, "UTF-16"));
    }

    return result;
}

QByteArray dat_to_bytes(const DatFile &dat_file)
{
    QByteArray result;

    int struct_size = 0;
    for (int s = 0; s < dat_file.data.at(0).count(); s++)
        struct_size += dat_file.data.at(0).at(s).size();

    result.append(num_to_bytes<uint>(dat_file.data.count()));       // struct_count
    result.append(num_to_bytes<uint>(struct_size));                 // struct_size
    result.append(num_to_bytes<uint>(dat_file.data_types.count())); // var_count

    for (int v = 0; v < dat_file.data_types.count(); v++)
    {
        result.append(str_to_bytes(dat_file.data_names.at(v)));
        result.append(str_to_bytes(dat_file.data_types.at(v)));
        result.append(num_to_bytes<uchar>(0x01));
        result.append(num_to_bytes<uchar>(0x00));
    }

    result.append((0x10 - (result.size() % 0x10)) % 0x10, 0x00);    // padding

    for (int d = 0; d < dat_file.data.count(); d++)
    {
        for (int v = 0; v < dat_file.data_types.count(); v++)
        {
            result.append(dat_file.data.at(d).at(v));
        }
    }

    result.append(num_to_bytes<ushort>(dat_file.labels.count()));
    result.append(num_to_bytes<ushort>(dat_file.refs.count()));

    for (QString label : dat_file.labels)
    {
        result.append(str_to_bytes(label));
    }

    result.append((2 - (result.size() % 2)) % 2, 0x00);             // padding

    for (QString ref : dat_file.refs)
    {
        result.append(str_to_bytes(ref, true));
    }

    return result;
}
