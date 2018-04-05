#include "wrd.h"

#include <QDir>
#include <QFileInfo>

WrdFile wrd_from_bytes(const QByteArray &bytes, QString in_file)
{
    WrdFile result;
    int pos = 0;

    result.filename = in_file;
    ushort str_count = bytes_to_num<ushort>(bytes, pos);
    ushort label_count = bytes_to_num<ushort>(bytes, pos);
    ushort flag_count = bytes_to_num<ushort>(bytes, pos);
    uint unk_count = bytes_to_num<ushort>(bytes, pos);

    // padding?
    //uint unk = bytes_to_num<uint>(data, pos);
    pos += 4;

    uint unk_ptr = bytes_to_num<uint>(bytes, pos);
    uint label_offsets_ptr = bytes_to_num<uint>(bytes, pos);
    uint label_names_ptr = bytes_to_num<uint>(bytes, pos);
    uint flags_ptr = bytes_to_num<uint>(bytes, pos);
    uint str_ptr = bytes_to_num<uint>(bytes, pos);


    // Read the name for each label.
    pos = label_names_ptr;
    for (ushort i = 0; i < label_count; i++)
    {
        const uchar label_name_len = bytes.at(pos++) + 1;    // Include null terminator
        QString label_name = bytes_to_str(bytes, pos, label_name_len);
        result.labels.append(label_name);
    }


    // Parse the code for each label.
    // NOTE: As far as I can tell, this data can probably just be read
    // sequentially, and then split after every occurrence of 0x7014.
    // But let's do it the more complex way for now.
    const int header_end = 0x20;
    for (int label_num = 0; label_num < label_count; label_num++)
    {
        QList<WrdCmd> label_cmds;

        pos = label_offsets_ptr + (label_num * 2);
        const ushort label_offset = bytes_to_num<ushort>(bytes, pos);
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

            for (WrdCmd known_cmd : known_commands)
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
                const ushort arg = bytes_to_num<ushort>(bytes, pos, true);

                if ((uchar)(arg >> 8) == 0x70)
                {
                    pos -= 2;
                    break;
                }

                cmd.args.append(arg);
            }

            if (cmd.arg_types.count() < cmd.args.count())
                for (int i = cmd.arg_types.count(); i < cmd.args.count(); i++)
                    cmd.arg_types.append(0);

            label_cmds.append(cmd);
        }
        result.code.append(label_cmds);
        label_cmds.clear();
    }


    // Read unknown data
    pos = unk_ptr;
    for (ushort i = 0; i < unk_count; i++)
    {
        result.unk_data.append(bytes_to_num<uint>(bytes, pos, true));
    }


    // Read command/argument names.
    pos = flags_ptr;
    for (ushort i = 0; i < flag_count; i++)
    {
        uchar length = bytes.at(pos++) + 1;  // Include null terminator
        QString value = bytes_to_str(bytes, pos, length);
        result.flags.append(value);
    }


    // Read text strings
    if (str_ptr > 0)    // Text is stored internally.
    {
        pos = str_ptr;
        for (ushort i = 0; i < str_count; i++)
        {
            int str_len = bytes.at(pos++);

            // ┐(´∀｀)┌
            if (str_len >= 0x80)
                str_len += (bytes.at(pos++) - 1) * 0x80;

            QString str = bytes_to_str(bytes, pos, str_len, "UTF-16");
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


WrdCodeModel::WrdCodeModel(QObject *parent, WrdFile *file, const int lbl)
{
    wrd_file = file;
    label = lbl;
}
int WrdCodeModel::rowCount(const QModelIndex &parent) const
{
    return (*wrd_file).code[label].count();
}
int WrdCodeModel::columnCount(const QModelIndex &parent) const
{
    return 3;
}
QVariant WrdCodeModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole || role == Qt::EditRole)
    {
        const WrdCmd cmd = (*wrd_file).code[label].at(index.row());

        if (index.column() == 0)
        {
            QString opString = QString::number(cmd.opcode, 16).toUpper().rightJustified(2, '0');
            return opString;
        }
        else if (index.column() == 1)
        {
            QString argHexString;
            for (ushort arg : cmd.args)
            {
                const QString part = QString::number(arg, 16).toUpper().rightJustified(4, '0');
                argHexString += part;
            }
            return argHexString.simplified();
        }
        else
        {
            QString argParsedString;
            argParsedString += cmd.name;

            if (cmd.args.count() > 0)
                argParsedString += ": ";

            for (int a = 0; a < cmd.args.count(); a++)
            {
                const ushort arg = cmd.args.at(a);

                if (cmd.arg_types.at(a) == 0 && arg < (*wrd_file).flags.count())
                    argParsedString += (*wrd_file).flags.at(arg) + " ";
                else if (cmd.arg_types.at(a) == 1 && arg < (*wrd_file).strings.count())
                    argParsedString += (*wrd_file).strings.at(arg) + " ";
                else
                    argParsedString += QString::number(arg) + " ";
            }

            return argParsedString.simplified();
        }
    }

    return QVariant();
}
QVariant WrdCodeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole)
    {
        if (orientation == Qt::Horizontal) {
            switch (section)
            {
            case 0:
                return QString("Opcode");
            case 1:
                return QString("Args");
            case 2:
                return QString("Parsed Args (Read-Only)");
            }
        }
    }
    return QVariant();
}
bool WrdCodeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role == Qt::EditRole)
    {
        const int row = index.row();
        const int column = index.column();

        // Opcode
        if (column == 0)
        {
            const QString opText = value.toString();
            if (opText.length() != 2)
            {
                //QMessageBox errorMsg(QMessageBox::Warning, "Hex Conversion Error", "Opcode length must be exactly 2 hexadecimal digits (1 byte).", QMessageBox::Ok, this);
                //errorMsg.exec();
                //ui->tableWidget_Code->editItem(item);
                return false;
            }

            bool ok;
            const uchar val = (uchar)opText.toUInt(&ok, 16);
            if (!ok)
            {
                //QMessageBox errorMsg(QMessageBox::Warning, "Hex Conversion Error", "Invalid hexadecimal value.", QMessageBox::Ok, this);
                //errorMsg.exec();
                //ui->tableWidget_Code->editItem(item);
                return false;
            }

            (*wrd_file).code[label][row].opcode = val;
            (*wrd_file).code[label][row].name = "UNKNOWN_CMD";
            (*wrd_file).code[label][row].arg_types.clear();

            for (const WrdCmd known_cmd : known_commands)
            {
                if (val == known_cmd.opcode)
                {
                    (*wrd_file).code[label][row].name = known_cmd.name;
                    (*wrd_file).code[label][row].arg_types = known_cmd.arg_types;
                    break;
                }
            }

            if ((*wrd_file).code[label][row].arg_types.count() < (*wrd_file).code[label][row].args.count())
                for (int i = (*wrd_file).code[label][row].arg_types.count(); i < (*wrd_file).code[label][row].args.count(); i++)
                    (*wrd_file).code[label][row].arg_types.append(0);
        }
        // Args
        else if (column == 1)
        {
            const QString argText = value.toString();

            (*wrd_file).code[label][row].args.clear();
            for (int argNum = 0; argNum < argText.length() / 4; argNum++)
            {
                const QString splitText = argText.mid(argNum * 4, 4);
                if (splitText.length() != 4)
                {
                    //QMessageBox errorMsg(QMessageBox::Warning, "Hex Conversion Error", "Argument length must be a multiple of 4 hexadecimal digits (2 bytes).", QMessageBox::Ok, this);
                    //errorMsg.exec();
                    //ui->tableWidget_Code->editItem(item);
                    return false;
                }

                bool ok;
                const ushort val = splitText.toUShort(&ok, 16);
                if (!ok)
                {
                    //QMessageBox errorMsg(QMessageBox::Warning, "Hex Conversion Error", "Invalid hexadecimal value.", QMessageBox::Ok, this);
                    //errorMsg.exec();
                    //ui->tableWidget_Code->editItem(item);
                    return false;
                }

                (*wrd_file).code[label][row].args.append(val);
            }

            if ((*wrd_file).code[label][row].arg_types.count() < (*wrd_file).code[label][row].args.count())
                for (int i = (*wrd_file).code[label][row].arg_types.count(); i < (*wrd_file).code[label][row].args.count(); i++)
                    (*wrd_file).code[label][row].arg_types.append(0);
        }
    }

    return true;
}
Qt::ItemFlags WrdCodeModel::flags(const QModelIndex &index) const
{
    if (index.column() == 2)
        return QAbstractTableModel::flags(index) & ~Qt::ItemIsEditable;
    else
        return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
}


WrdStringsModel::WrdStringsModel(QObject *parent, WrdFile *file)
{
    wrd_file = file;
}
int WrdStringsModel::rowCount(const QModelIndex &parent) const
{
    return (*wrd_file).strings.count();
}
int WrdStringsModel::columnCount(const QModelIndex &parent) const
{
    return 2;
}
QVariant WrdStringsModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole || role == Qt::EditRole)
    {
        if (index.column() == 0)
        {
            // Display hex index headers
            return QString::number(index.row(), 16).toUpper().rightJustified(4, '0');
        }
        else
        {
            QString str = (*wrd_file).strings.at(index.row());
            str.replace("\n", "\\n");
            return str;
        }
    }

    return QVariant();
}
bool WrdStringsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (index.column() == 0)
        return true;

    QString str = value.toString();
    str.replace("\\n", "\n");
    (*wrd_file).strings[index.row()] = str;

    return true;
}
Qt::ItemFlags WrdStringsModel::flags(const QModelIndex &index) const
{
    if (index.column() == 0)
        return QAbstractTableModel::flags(index) & ~Qt::ItemIsEditable;
    else
        return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
}


WrdFlagsModel::WrdFlagsModel(QObject *parent, WrdFile *file)
{
    wrd_file = file;
}
int WrdFlagsModel::rowCount(const QModelIndex &parent) const
{
    return (*wrd_file).flags.count();
}
int WrdFlagsModel::columnCount(const QModelIndex &parent) const
{
    return 2;
}
QVariant WrdFlagsModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole || role == Qt::EditRole)
    {
        if (index.column() == 0)
        {
            // Display hex index headers
            return QString::number(index.row(), 16).toUpper().rightJustified(4, '0');
        }
        else
        {
            return (*wrd_file).flags.at(index.row());
        }
    }

    return QVariant();
}
bool WrdFlagsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (index.column() == 0)
        return true;

    QString flag = value.toString();
    (*wrd_file).flags[index.row()] = flag;

    return true;
}
Qt::ItemFlags WrdFlagsModel::flags(const QModelIndex &index) const
{
    if (index.column() == 0)
        return QAbstractTableModel::flags(index) & ~Qt::ItemIsEditable;
    else
        return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
}
