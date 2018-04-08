#include "wrd_ui_models.h"
#include <QMessageBox>

WrdCodeModel::WrdCodeModel(QObject * /*parent*/, WrdFile *file, const int lbl)
{
    wrd_file = file;
    label = lbl;
}
int WrdCodeModel::rowCount(const QModelIndex & /*parent*/) const
{
    return (*wrd_file).code.at(label).count();
}
int WrdCodeModel::columnCount(const QModelIndex & /*parent*/) const
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

            if (cmd.args.count() > 0 && !cmd.name.endsWith("="))
                argParsedString += ":";

            argParsedString += "    ";

            for (int a = 0; a < cmd.args.count(); a++)
            {
                const ushort arg = cmd.args.at(a);

                if (cmd.arg_types.at(a) == 0 && arg < (*wrd_file).flags.count())
                    argParsedString += (*wrd_file).flags.at(arg) + "    ";
                else if (cmd.arg_types.at(a) == 1 && arg < (*wrd_file).strings.count())
                    argParsedString += (*wrd_file).strings.at(arg) + "    ";
                else
                    argParsedString += QString::number(arg) + "    ";
            }

            argParsedString.replace("\n", "\\n");
            return argParsedString.trimmed();
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
            QList<ushort> result;

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

            (*wrd_file).code[label][row].args = result;

            if ((*wrd_file).code[label][row].arg_types.count() < (*wrd_file).code[label][row].args.count())
                for (int i = (*wrd_file).code[label][row].arg_types.count(); i < (*wrd_file).code[label][row].args.count(); i++)
                    (*wrd_file).code[label][row].arg_types.append(0);
        }
    }
    emit(editCompleted(value.toString()));

    return true;
}
bool WrdCodeModel::insertRows(int row, int count, const QModelIndex & /*parent*/)
{
    if (count < 1 || row < 0 || row + count > rowCount())
        return false;

    beginInsertRows(QModelIndex(), row, row + count);

    for (int r = 0; r < count; r++)
    {
        const WrdCmd cmd = {0xFF, "UNKNOWN_CMD", {}, {}};
        (*wrd_file).code[label].insert(row + r, cmd);
    }

    endInsertRows();
    return true;
}
bool WrdCodeModel::removeRows(int row, int count, const QModelIndex & /*parent*/)
{
    if (count < 1 || row < 0 || row + count > rowCount())
        return false;

    beginRemoveRows(QModelIndex(), row, row + count);

    for (int r = 0; r < count; r++)
    {
        // Keep removing at the same index, because the next item we want to delete
        // always takes the place of the previous one.
        (*wrd_file).code[label].removeAt(row);
    }

    endRemoveRows();
    return true;
}
bool WrdCodeModel::moveRows(const QModelIndex & /*sourceParent*/, int sourceRow, int count, const QModelIndex & /*destinationParent*/, int destinationChild)
{
    if (count < 1 || sourceRow < 0 || destinationChild + count > rowCount())
        return false;

    beginMoveRows(QModelIndex(), sourceRow, sourceRow + count, QModelIndex(), destinationChild);

    for (int r = 0; r < count; r++)
    {
        (*wrd_file).code[label].move(sourceRow + r, destinationChild + r);
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



WrdStringsModel::WrdStringsModel(QObject * /*parent*/, WrdFile *file)
{
    wrd_file = file;
}
int WrdStringsModel::rowCount(const QModelIndex & /*parent*/) const
{
    return (*wrd_file).strings.count();
}
int WrdStringsModel::columnCount(const QModelIndex & /*parent*/) const
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

    emit(editCompleted(value.toString()));
    return true;
}
bool WrdStringsModel::insertRows(int row, int count, const QModelIndex & /*parent*/)
{
    if (count < 1 || row < 0 || row + count > rowCount())
        return false;

    beginInsertRows(QModelIndex(), row, row + count);

    for (int r = 0; r < count; r++)
    {
        (*wrd_file).strings.insert(row + r, QString());
    }

    endInsertRows();
    return true;
}
bool WrdStringsModel::removeRows(int row, int count, const QModelIndex & /*parent*/)
{
    if (count < 1 || row < 0 || row + count > rowCount())
        return false;

    beginRemoveRows(QModelIndex(), row, row + count);

    for (int r = 0; r < count; r++)
    {
        // Keep removing at the same index, because the next item we want to delete
        // always takes the place of the previous one.
        (*wrd_file).strings.removeAt(row);
    }

    endRemoveRows();
    return true;
}
bool WrdStringsModel::moveRows(const QModelIndex & /*sourceParent*/, int sourceRow, int count, const QModelIndex & /*destinationParent*/, int destinationChild)
{
    if (count < 1 || sourceRow < 0 || destinationChild + count > rowCount())
        return false;

    beginMoveRows(QModelIndex(), sourceRow, sourceRow + count, QModelIndex(), destinationChild);

    for (int r = 0; r < count; r++)
    {
        (*wrd_file).strings.move(sourceRow + r, destinationChild + r);
    }

    return true;
}
Qt::ItemFlags WrdStringsModel::flags(const QModelIndex &index) const
{
    if (index.column() == 0)
        return QAbstractTableModel::flags(index) & ~Qt::ItemIsEditable;
    else
        return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
}



WrdFlagsModel::WrdFlagsModel(QObject * /*parent*/, WrdFile *file)
{
    wrd_file = file;
}
int WrdFlagsModel::rowCount(const QModelIndex & /*parent*/) const
{
    return (*wrd_file).flags.count();
}
int WrdFlagsModel::columnCount(const QModelIndex & /*parent*/) const
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

    emit(editCompleted(value.toString()));
    return true;
}
bool WrdFlagsModel::insertRows(int row, int count, const QModelIndex & /*parent*/)
{
    if (count < 1 || row < 0 || row + count > rowCount())
        return false;

    beginInsertRows(QModelIndex(), row, row + count);

    for (int r = 0; r < count; r++)
    {
        (*wrd_file).flags.insert(row + r, QString());
    }

    endInsertRows();
    return true;
}
bool WrdFlagsModel::removeRows(int row, int count, const QModelIndex & /*parent*/)
{
    if (count < 1 || row < 0 || row + count > rowCount())
        return false;

    beginRemoveRows(QModelIndex(), row, row + count);

    for (int r = 0; r < count; r++)
    {
        // Keep removing at the same index, because the next item we want to delete
        // always takes the place of the previous one.
        (*wrd_file).flags.removeAt(row);
    }

    endRemoveRows();
    return true;
}
bool WrdFlagsModel::moveRows(const QModelIndex & /*sourceParent*/, int sourceRow, int count, const QModelIndex & /*destinationParent*/, int destinationChild)
{
    if (count < 1 || sourceRow < 0 || destinationChild + count > rowCount())
        return false;

    beginMoveRows(QModelIndex(), sourceRow, sourceRow + count, QModelIndex(), destinationChild);

    for (int r = 0; r < count; r++)
    {
        (*wrd_file).flags.move(sourceRow + r, destinationChild + r);
    }

    return true;
}
Qt::ItemFlags WrdFlagsModel::flags(const QModelIndex &index) const
{
    if (index.column() == 0)
        return QAbstractTableModel::flags(index) & ~Qt::ItemIsEditable;
    else
        return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
}
