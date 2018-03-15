#include "wrd.h"

#include <QDir>
#include <QFileInfo>

WrdFile wrd_from_data(const QByteArray &data, QString in_file)
{
    WrdFile result;
    int pos = 0;

    result.filename = in_file;
    ushort str_count = bytes_to_num<ushort>(data, pos);
    ushort label_count = bytes_to_num<ushort>(data, pos);
    ushort flag_count = bytes_to_num<ushort>(data, pos);
    uint unk_count = bytes_to_num<ushort>(data, pos);

    // padding?
    //uint unk = bytes_to_num<uint>(data, pos);
    pos += 4;

    uint unk_ptr = bytes_to_num<uint>(data, pos);
    uint label_offsets_ptr = bytes_to_num<uint>(data, pos);
    uint label_names_ptr = bytes_to_num<uint>(data, pos);
    uint flags_ptr = bytes_to_num<uint>(data, pos);
    uint str_ptr = bytes_to_num<uint>(data, pos);


    // Read the name for each label.
    pos = label_names_ptr;
    for (ushort i = 0; i < label_count; i++)
    {
        const uchar label_name_len = data.at(pos++) + 1;    // Include null terminator
        QString label_name = bytes_to_str(data, pos, label_name_len);
        result.labels.append(label_name);
    }


    // Parse the code for each label.
    // NOTE: As far as I can tell, this data can probably just be read
    // sequentially, and then split after every occurrence of 0x7014.
    // But let's do it the more complex way for now.
    const int header_end = 0x20;
    pos = header_end;
    QList<WrdCmd> cur_label_cmds;

    // We need at least 2 bytes for a command
    while (pos < unk_ptr - 1)
    {
        const ushort op = bytes_to_num<ushort>(data, pos, true);
        if ((op >> 8) != 0x70)
            continue;

        WrdCmd cmd;
        cmd.opcode = op;

        // If we've reached the end of the script/label
        if (op == 0x7011)
        {
            cur_label_cmds.append(cmd);
            result.code.append(cur_label_cmds);
            cur_label_cmds.clear();
            continue;
        }
        // If we've reached the start of a new label
        else if (op == 0x7014 && cur_label_cmds.count() > 0)
        {
            result.code.append(cur_label_cmds);
            cur_label_cmds.clear();
            pos -= 2;
            continue;
        }

        while (pos < unk_ptr - 1)
        {
            const ushort arg = bytes_to_num<ushort>(data, pos, true);

            if ((uchar)(arg >> 8) == 0x70)
            {
                pos -= 2;
                break;
            }

            cmd.args.append(arg);
        }

        cur_label_cmds.append(cmd);
    }
    result.code.append(cur_label_cmds);


    // Read unknown data
    pos = unk_ptr;
    for (ushort i = 0; i < unk_count; i++)
    {
        result.unk_data.append(bytes_to_num<uint>(data, pos, true));
    }


    // Read command/argument names.
    pos = flags_ptr;
    for (ushort i = 0; i < flag_count; i++)
    {
        uchar length = data.at(pos++) + 1;  // Include null terminator
        QString value = bytes_to_str(data, pos, length);
        result.flags.append(value);
    }


    // Read text strings
    if (str_ptr > 0)    // Text is stored internally.
    {
        pos = str_ptr;
        for (ushort i = 0; i < str_count; i++)
        {
            int str_len = data.at(pos++);

            // ┐(´∀｀)┌
            if (str_len >= 0x80)
                str_len += (data.at(pos++) - 1) * 0x80;

            QString str = bytes_to_str(data, pos, str_len, true);
            result.strings.append(str);
        }

        result.external_strings = false;
    }
    else                // Text is stored externally.
    {
        // Strings are stored in the "(current spc name)_text_(region).spc" file,
        // within an STX file with the same name as the current WRD file.
        QString stx_file = QFileInfo(in_file).absolutePath();
        if (stx_file.endsWith(".SPC", Qt::CaseInsensitive))
            stx_file.chop(4);
        if (stx_file.endsWith("_US"))
            stx_file.chop(3);
        stx_file.append("_text_US.SPC");
        stx_file.append(QDir::separator());
        stx_file.append(QFileInfo(in_file).fileName());
        stx_file.replace(".wrd", ".stx");

        if (QFileInfo(stx_file).exists())
        {
            QFile f(stx_file);
            f.open(QFile::ReadOnly);
            const QByteArray stx_data = f.readAll();
            f.close();
            result.strings = get_stx_strings(stx_data);
        }

        result.external_strings = true;
    }

    return result;
}

QByteArray wrd_to_data(const WrdFile &wrd_file)
{
    QByteArray result;

    result.append(num_to_bytes((ushort)wrd_file.strings.count()));
    result.append(num_to_bytes((ushort)wrd_file.labels.count()));
    result.append(num_to_bytes((ushort)wrd_file.flags.count()));
    result.append(num_to_bytes((ushort)wrd_file.unk_data.count()));
    result.append(QByteArray(4, 0x00));

    QByteArray code_data;
    QByteArray code_offsets;
    QByteArray label_names_data;
    for (int i = 0; i < wrd_file.labels.count(); i++)
    {
        label_names_data.append(str_to_bytes(wrd_file.labels.at(i)));

        code_offsets.append(num_to_bytes((ushort)code_data.size()));

        const QList<WrdCmd> cur_label_cmds = wrd_file.code.at(i);
        for (const WrdCmd cmd : cur_label_cmds)
        {
            code_data.append(num_to_bytes(cmd.opcode, true));
            for (const ushort arg : cmd.args)
            {
                code_data.append(num_to_bytes(arg, true));
            }
        }
    }

    QByteArray unk_data_bytes;
    for (const uint unk : wrd_file.unk_data)
    {
        unk_data_bytes.append(num_to_bytes(unk, true));
    }

    QByteArray flags_data;
    for (const QString flag : wrd_file.flags)
    {
        flags_data.append(str_to_bytes(flag));
    }


    const uint header_end = 0x20;
    const uint unk_ptr = header_end + code_data.size();
    result.append(num_to_bytes(unk_ptr));               // unk_ptr
    const uint code_offsets_ptr = unk_ptr + unk_data_bytes.size();
    result.append(num_to_bytes(code_offsets_ptr));      // code_offsets_ptr
    const uint label_names_ptr = code_offsets_ptr + code_offsets.size();
    result.append(num_to_bytes(label_names_ptr));       // label_names_ptr
    const uint flags_ptr = label_names_ptr + label_names_data.size();
    result.append(num_to_bytes(flags_ptr));             // flags_ptr

    uint str_ptr = 0;
    if (!wrd_file.external_strings)
        str_ptr = flags_ptr + flags_data.size();
    result.append(num_to_bytes(str_ptr));



    // Now that the offsets/ptrs to the data have been calculated, append the actual data
    result.append(code_data);
    result.append(unk_data_bytes);
    result.append(code_offsets);
    result.append(label_names_data);
    result.append(flags_data);
    if (!wrd_file.external_strings)
    {
        for (const QString str : wrd_file.strings)
        {
            result.append(str_to_bytes(str, true));
        }
    }

    return result;
}
