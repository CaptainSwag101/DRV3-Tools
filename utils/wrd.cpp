#include "wrd.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>

WrdFile wrd_from_bytes(const QByteArray &bytes, QString in_file)
{
    WrdFile result;
    int pos = 0;

    result.filename = in_file;
    ushort str_count = num_from_bytes<ushort>(bytes, pos);
    ushort label_count = num_from_bytes<ushort>(bytes, pos);
    ushort flag_count = num_from_bytes<ushort>(bytes, pos);
    uint unk_count = num_from_bytes<ushort>(bytes, pos);

    // padding?
    //uint unk = bytes_to_num<uint>(data, pos);
    pos += 4;

    uint unk_ptr = num_from_bytes<uint>(bytes, pos);
    uint label_offsets_ptr = num_from_bytes<uint>(bytes, pos);
    uint label_names_ptr = num_from_bytes<uint>(bytes, pos);
    uint flags_ptr = num_from_bytes<uint>(bytes, pos);
    uint str_ptr = num_from_bytes<uint>(bytes, pos);


    // Read the name for each label.
    pos = label_names_ptr;
    for (ushort i = 0; i < label_count; ++i)
    {
        const uchar label_name_len = bytes.at(pos++) + 1;    // Include null terminator
        QString label_name = str_from_bytes(bytes, pos, label_name_len);
        result.labels.append(label_name);
    }


    // Parse the code for each label.
    // NOTE: As far as I can tell, this data can probably just be read
    // sequentially, and then split after every occurrence of 0x7014.
    // But let's do it the more complex way for now.
    const int header_end = 0x20;
    for (int label_num = 0; label_num < label_count; ++label_num)
    {
        QVector<WrdCmd> label_cmds;

        pos = label_offsets_ptr + (label_num * 2);
        const ushort label_offset = num_from_bytes<ushort>(bytes, pos);
        pos = header_end + label_offset;

        // We need at least 2 bytes for a command
        while (pos + 1 < unk_ptr)
        {
            const uchar b = bytes.at(pos++);
            if (b != 0x70)
                continue;

            const uchar op = bytes.at(pos++);
            WrdCmd cmd;
            cmd.name = "UNKNOWN_CMD";
            cmd.opcode = op;

            for (WrdCmd known_cmd : KNOWN_CMDS)
            {
                if (op == known_cmd.opcode)
                {
                    cmd.name = known_cmd.name;
                    cmd.arg_types = known_cmd.arg_types;
                    break;
                }
            }

            // If we've reached the end of the script/label
            if (op == 0x14 && label_cmds.count() > 0)
            {
                pos -= 2;
                break;
            }

            // We need at least 2 bytes for each arg
            while (pos < unk_ptr - 1)
            {
                const ushort arg = num_from_bytes<ushort>(bytes, pos, true);

                if ((uchar)(arg >> 8) == 0x70)
                {
                    pos -= 2;
                    break;
                }

                cmd.args.append(arg);
            }

            if (cmd.arg_types.count() != cmd.args.count() && cmd.opcode != 0x01 && cmd.opcode != 0x03)  // IFF and IFW have variable-length params
            {
                //cout << in_file << ": Opcode " << num_to_hex(cmd.opcode, 2) << " expected " << cmd.arg_types.count() << " args, but found " << cmd.args.count() << ".";
                //cout.flush();
                qDebug() << in_file << ": Opcode " << num_to_hex(cmd.opcode, 2) << " expected " << cmd.arg_types.count() << " args, but found " << cmd.args.count() << ".";

                /*
                for (int i = cmd.arg_types.count(); i < cmd.args.count(); ++i)
                {
                    cmd.arg_types.append(0);
                }
                */
            }

            label_cmds.append(cmd);
        }
        result.code.append(label_cmds);
        label_cmds.clear();
    }


    // Read unknown data
    pos = unk_ptr;
    for (ushort i = 0; i < unk_count; ++i)
    {
        result.unk_data.append(num_from_bytes<uint>(bytes, pos, true));
    }


    // Read command/argument names.
    pos = flags_ptr;
    for (ushort i = 0; i < flag_count; ++i)
    {
        uchar length = bytes.at(pos++) + 1;  // Include null terminator
        QString value = str_from_bytes(bytes, pos, length);
        result.params.append(value);
    }


    // Read text strings
    if (str_ptr > 0)    // Text is stored internally.
    {
        pos = str_ptr;
        for (ushort i = 0; i < str_count; ++i)
        {
            uint str_len = (uint)bytes.at(pos++);

            // ┐(´∀｀)┌
            if (str_len >= 0x80)
                str_len += (bytes.at(pos++) - 1) * 0x80;

            QString str = str_from_bytes(bytes, pos, -1, "UTF16LE");
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

        QString region = "_US";
        if (stx_file.right(3).startsWith("_"))
        {
            region = stx_file.right(3);
            stx_file.chop(3);
        }

        stx_file.append("_text" + region + ".SPC");
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

QByteArray wrd_to_bytes(const WrdFile &wrd_file)
{
    QByteArray result;

    result.append(num_to_bytes((ushort)wrd_file.strings.count()));
    result.append(num_to_bytes((ushort)wrd_file.labels.count()));
    result.append(num_to_bytes((ushort)wrd_file.params.count()));
    result.append(num_to_bytes((ushort)wrd_file.unk_data.count()));
    result.append(QByteArray(4, 0x00));

    QByteArray code_data;
    QByteArray code_offsets;
    QByteArray label_names_data;
    for (int i = 0; i < wrd_file.labels.count(); ++i)
    {
        const QByteArray lbldata = str_to_bytes(wrd_file.labels.at(i));
        label_names_data.append((uchar)(lbldata.size() - 1));
        label_names_data.append(lbldata);

        code_offsets.append(num_to_bytes((ushort)code_data.size()));

        const QVector<WrdCmd> cur_label_cmds = wrd_file.code.at(i);
        for (const WrdCmd cmd : cur_label_cmds)
        {
            code_data.append((uchar)0x70);
            code_data.append(cmd.opcode);
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
    for (const QString flag : wrd_file.params)
    {
        const QByteArray flagdata = str_to_bytes(flag);
        flags_data.append((uchar)(flagdata.size() - 1));
        flags_data.append(flagdata);
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
            const QByteArray strdata = str_to_bytes(str);
            result.append((uchar)(strdata.size() - 1));
            result.append(str_to_bytes(str, true));
        }
    }

    return result;
}
