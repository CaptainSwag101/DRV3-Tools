#include "wrd_ui_model.h"
#include <QMessageBox>

WrdUiModel::WrdUiModel(QObject * /*parent*/, WrdFile *file, const int mode)
{
    wrd_file = file;
    data_mode = mode;
}

int WrdUiModel::rowCount(const QModelIndex & /*parent*/) const
{
    switch (data_mode)
    {
    case 0:
        return (*wrd_file).code.count();

    case 1:
        return (*wrd_file).params.count();

    case 2:
        return (*wrd_file).strings.count();

    default:
        return 0;
    }
}

int WrdUiModel::columnCount(const QModelIndex & /*parent*/) const
{
    switch (data_mode)
    {
    case 0:
        return 3;

    case 1:
    case 2:
        return 2;

    default:
        return 0;
    }
}

QVariant WrdUiModel::data(const QModelIndex &index, int role) const
{
    if (role != Qt::DisplayRole && role != Qt::EditRole)
        return QVariant();

    const int row = index.row();
    const int col = index.column();

    switch (data_mode)
    {
    case 0:
    {
        const WrdCmd cmd = (*wrd_file).code.at(row);

        if (col == 0)       // Opcode
        {
            return num_to_hex(cmd.opcode, 2);
        }
        else if (col == 1)  // Parameter data
        {
            QString argHexString;
            for (ushort arg : cmd.args)
                argHexString += num_to_hex(arg, 4);
            return argHexString.simplified();
        }
        else                // Parsed parameters
        {
            QString argParsedString;
            argParsedString += cmd.name;

            if (cmd.args.count() > 0 && !cmd.name.endsWith("="))
                argParsedString += ":";

            argParsedString += "    ";

            for (int a = 0; a < cmd.args.count(); a++)
            {
                const ushort arg = cmd.args.at(a);

                if (cmd.arg_types.at(a) == 0 && a < cmd.arg_types.count() && arg < (*wrd_file).params.count())
                    argParsedString += (*wrd_file).params.at(arg) + "    ";
                else if (cmd.arg_types.at(a) == 2 && a < cmd.arg_types.count() && arg < (*wrd_file).strings.count())
                    argParsedString += "\"" + (*wrd_file).strings.at(arg) + "\"    ";
                else if (cmd.arg_types.at(a) == 3 && a < cmd.arg_types.count() && arg < (*wrd_file).labels.count())
                    argParsedString += "\"" + (*wrd_file).labels.at(arg) + "\"    ";
                else
                    argParsedString += QString::number(arg) + "    ";
            }

            argParsedString.replace("\n", "\\n");
            return argParsedString.trimmed();
        }
        break;
    }
    case 1:
    {
        if (col == 0)
        {
            // Display hex index headers
            return num_to_hex(row, 4);
        }
        else
        {
            return (*wrd_file).params.at(row);
        }
        break;
    }
    case 2:
    {
        if (col == 0)
        {
            // Display hex index headers
            return num_to_hex(row, 4);
        }
        else
        {
            QString str = (*wrd_file).strings.at(row);
            str.replace("\n", "\\n");
            return str;
        }
        break;
    }
    }

    return QVariant();
}

QVariant WrdUiModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (data_mode != 0 || role != Qt::DisplayRole || orientation != Qt::Horizontal)
        return QVariant();

    switch (section)
    {
    case 0:
        return QString("Opcode");
    case 1:
        return QString("Args");
    case 2:
        return QString("Parsed Args (Read-Only)");
    }

    return QVariant();
}

bool WrdUiModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::EditRole)
        return false;

    const int row = index.row();
    const int col = index.column();

    switch (data_mode)
    {
    case 0:
    {
        // Opcode
        if (col == 0)
        {
            const QString opText = value.toString();
            if (opText.length() != 2)
            {
                QMessageBox errorMsg(QMessageBox::Warning, "Hex Conversion Error", "Opcode length must be exactly 2 hexadecimal digits (1 byte).", QMessageBox::Ok);
                errorMsg.exec();
                return false;
            }

            bool ok;
            const uchar val = (uchar)opText.toUInt(&ok, 16);
            if (!ok)
            {
                QMessageBox errorMsg(QMessageBox::Warning, "Hex Conversion Error", "Invalid hexadecimal value.", QMessageBox::Ok);
                errorMsg.exec();
                return false;
            }

            (*wrd_file).code[row].opcode = val;
            (*wrd_file).code[row].name = "UNKNOWN_CMD";
            (*wrd_file).code[row].arg_types.clear();

            for (const WrdCmd known_cmd : KNOWN_CMDS)
            {
                if (val == known_cmd.opcode)
                {
                    (*wrd_file).code[row].name = known_cmd.name;
                    (*wrd_file).code[row].arg_types = known_cmd.arg_types;
                    break;
                }
            }

            if ((*wrd_file).code[row].arg_types.count() != (*wrd_file).code[row].args.count())
            {
                QMessageBox errorMsg(QMessageBox::Information,
                                     "Unexpected Command Parameters",
                                     "Opcode " + num_to_hex((*wrd_file).code[row].opcode, 2) + " expected " + QString::number((*wrd_file).code[row].arg_types.count()) + " args, but found " + QString::number((*wrd_file).code[row].args.count()) + ".",
                                     QMessageBox::Ok);
                errorMsg.exec();

                for (int i = (*wrd_file).code[row].arg_types.count(); i < (*wrd_file).code[row].args.count(); i++)
                {
                    (*wrd_file).code[row].arg_types.append(0);
                }
            }
        }
        // Args
        else if (col == 1)
        {
            const QString argText = value.toString();
            QVector<ushort> result;

            for (int argNum = 0; argNum < argText.length(); argNum += 4)
            {
                const QString splitText = argText.mid(argNum, 4);
                if (splitText.length() != 4)
                {
                    QMessageBox errorMsg(QMessageBox::Warning, "Hex Conversion Error", "Argument length must be a multiple of 4 hexadecimal digits (2 bytes).", QMessageBox::Ok);
                    errorMsg.exec();
                    return false;
                }

                bool ok;
                const ushort val = splitText.toUShort(&ok, 16);
                if (!ok)
                {
                    QMessageBox errorMsg(QMessageBox::Warning, "Hex Conversion Error", "Invalid hexadecimal value.", QMessageBox::Ok);
                    errorMsg.exec();
                    return false;
                }
                result.append(val);
            }

            (*wrd_file).code[row].args = result;

            if ((*wrd_file).code[row].arg_types.count() < (*wrd_file).code[row].args.count())
                for (int i = (*wrd_file).code[row].arg_types.count(); i < (*wrd_file).code[row].args.count(); i++)
                    (*wrd_file).code[row].arg_types.append(0);
        }
        break;
    }
    case 1:
    {
        if (col == 0)
            return true;

        QString flag = value.toString();
        (*wrd_file).params[row] = flag;
        break;
    }
    case 2:
    {
        if (col == 0)
            return true;

        QString str = value.toString();
        str.replace("\\n", "\n");
        (*wrd_file).strings[row] = str;
        break;
    }
    }

    emit(editCompleted(value.toString()));
    return true;
}

bool WrdUiModel::insertRows(int row, int count, const QModelIndex & /*parent*/)
{
    if (count < 1 || row < 0 || row + (count - 1) >= rowCount())
        return false;

    beginInsertRows(QModelIndex(), row, row + (count - 1));
    for (int r = 0; r < count; r++)
    {
        switch (data_mode)
        {
        case 0:
        {
            const WrdCmd cmd = {0xFF, "UNKNOWN_CMD", {}, {}};
            (*wrd_file).code.insert(row + r, cmd);
            break;
        }
        case 1:
            (*wrd_file).params.insert(row + r, QString());
            break;

        case 2:
            (*wrd_file).strings.insert(row + r, QString());
            break;
        }
    }
    endInsertRows();

    return true;
}

bool WrdUiModel::removeRows(int row, int count, const QModelIndex & /*parent*/)
{
    if (count < 1 || row < 0 || row + (count - 1) >= rowCount())
        return false;

    // Keep removing at the same index, because the next item we want to delete
    // always takes the place of the previous one.
    beginRemoveRows(QModelIndex(), row, row + (count - 1));
    for (int r = 0; r < count; r++)
    {
        switch (data_mode)
        {
        case 0:
            (*wrd_file).code.removeAt(row);
            break;

        case 1:
            (*wrd_file).params.removeAt(row);
            break;

        case 2:
            (*wrd_file).strings.removeAt(row);
            break;
        }

    }
    endRemoveRows();

    return true;
}

bool WrdUiModel::moveRows(const QModelIndex & /*sourceParent*/, int sourceRow, int count, const QModelIndex & /*destinationParent*/, int destinationRow)
{
    if (count < 1 || sourceRow < 0 || destinationRow + (count - 1) >= rowCount())
        return false;

    // The way that beginMoveRows() needs the destination value set is incredibly fucking stupid,
    // and it's inconsistent depending on whether you're moving forward or backward. Fix your shit, Qt!
    int fixedDest;
    if (destinationRow > sourceRow)
        fixedDest = destinationRow + ((destinationRow - sourceRow) % 2);
    else
        fixedDest = destinationRow;

    beginMoveRows(QModelIndex(), sourceRow, sourceRow + (count - 1), QModelIndex(), fixedDest);
    for (int r = 0; r < count; r++)
    {
        switch (data_mode)
        {
        case 0:
            (*wrd_file).code.move(sourceRow + r, destinationRow + r);
            break;

        case 1:
            (*wrd_file).params.move(sourceRow + r, destinationRow + r);
            break;

        case 2:
            (*wrd_file).strings.move(sourceRow + r, destinationRow + r);
            break;
        }
    }
    endMoveRows();

    return true;
}

Qt::ItemFlags WrdUiModel::flags(const QModelIndex &index) const
{
    switch (data_mode)
    {
    case 0:
        if (index.column() == 2)
            return QAbstractTableModel::flags(index) & ~Qt::ItemIsEditable;
        break;

    case 1:
    case 2:
        if (index.column() == 0)
            return QAbstractTableModel::flags(index) & ~Qt::ItemIsEditable;
        break;
    }

    return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
}
