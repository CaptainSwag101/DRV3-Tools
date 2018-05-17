#include "spc_ui_model.h"
#include <QMessageBox>

SpcUiModel::SpcUiModel(QObject * /*parent*/, SpcFile *file)
{
    spc_file = file;
}

int SpcUiModel::rowCount(const QModelIndex & /*parent*/) const
{
    return (*spc_file).subfiles.count();
}

int SpcUiModel::columnCount(const QModelIndex & /*parent*/) const
{
    return 2;
}

QVariant SpcUiModel::data(const QModelIndex &index, int role) const
{
    if (role != Qt::DisplayRole && role != Qt::EditRole)
        return QVariant();

    const int row = index.row();
    const int col = index.column();

    const SpcSubfile subfile = (*spc_file).subfiles.at(row);

    if (col == 0)
    {
        return subfile.filename;
    }
    else
    {
        return QString::number(subfile.cmp_flag);
    }

    return QVariant();
}

QVariant SpcUiModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (data_mode != 0 || role != Qt::DisplayRole || orientation != Qt::Horizontal)
        return QVariant();

    switch (section)
    {
    case 0:
        return QString("Filename");
    case 1:
        return QString("Compression flag");
    }

    return QVariant();
}

bool SpcUiModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::EditRole)
        return false;

    const int row = index.row();
    const int col = index.column();

    // Filename
    if (col == 0)
    {
        const QString opText = value.toString();

        // TODO: Ensure the filename only contains valid characters for a file path?
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

        (*spc_file).code[label][row].opcode = val;
        (*spc_file).code[label][row].name = "UNKNOWN_CMD";
        (*spc_file).code[label][row].arg_types.clear();

        for (const WrdCmd known_cmd : KNOWN_CMDS)
        {
            if (val == known_cmd.opcode)
            {
                (*spc_file).code[label][row].name = known_cmd.name;
                (*spc_file).code[label][row].arg_types = known_cmd.arg_types;
                break;
            }
        }

        if ((*spc_file).code[label][row].arg_types.count() != (*spc_file).code[label][row].args.count())
        {
            QMessageBox errorMsg(QMessageBox::Information,
                                 "Unexpected Command Parameters",
                                 "Opcode " + num_to_hex((*spc_file).code[label][row].opcode, 2) + " expected " + QString::number((*spc_file).code[label][row].arg_types.count()) + " args, but found " + QString::number((*spc_file).code[label][row].args.count()) + ".",
                                 QMessageBox::Ok);
            errorMsg.exec();

            for (int i = (*spc_file).code[label][row].arg_types.count(); i < (*spc_file).code[label][row].args.count(); i++)
            {
                (*spc_file).code[label][row].arg_types.append(0);
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

        (*spc_file).code[label][row].args = result;

        if ((*spc_file).code[label][row].arg_types.count() < (*spc_file).code[label][row].args.count())
            for (int i = (*spc_file).code[label][row].arg_types.count(); i < (*spc_file).code[label][row].args.count(); i++)
                (*spc_file).code[label][row].arg_types.append(0);
    }

    emit(editCompleted(value.toString()));
    return true;
}

bool SpcUiModel::insertRows(int row, int count, const QModelIndex & /*parent*/)
{
    if (count < 1 || row < 0 || row + count > rowCount())
        return false;

    beginInsertRows(QModelIndex(), row, row + count);
    for (int r = 0; r < count; r++)
    {
        switch (data_mode)
        {
        case 0:
        {
            const WrdCmd cmd = {0xFF, "UNKNOWN_CMD", {}, {}};
            (*spc_file).code[label].insert(row + r, cmd);
            break;
        }
        case 1:
            (*spc_file).params.insert(row + r, QString());
            break;

        case 2:
            (*spc_file).strings.insert(row + r, QString());
            break;
        }
    }
    endInsertRows();

    return true;
}

bool SpcUiModel::removeRows(int row, int count, const QModelIndex & /*parent*/)
{
    if (count < 1 || row < 0 || row + count > rowCount())
        return false;

    // Keep removing at the same index, because the next item we want to delete
    // always takes the place of the previous one.
    beginRemoveRows(QModelIndex(), row, row + count);
    for (int r = 0; r < count; r++)
    {
        switch (data_mode)
        {
        case 0:
            (*spc_file).code[label].removeAt(row);
            break;

        case 1:
            (*spc_file).params.removeAt(row);
            break;

        case 2:
            (*spc_file).strings.removeAt(row);
            break;
        }

    }
    endRemoveRows();

    return true;
}

bool SpcUiModel::moveRows(const QModelIndex & /*sourceParent*/, int sourceRow, int count, const QModelIndex & /*destinationParent*/, int destinationRow)
{
    if (sourceRow < 0 || destinationRow + count > rowCount() || count <= 0)
        return false;

    // The way that beginMoveRows() needs the destination value set is incredibly fucking stupid,
    // and it's inconsistent depending on whether you're moving forward or backward. Fix your shit, Qt!
    int fixedDest;
    if (destinationRow > sourceRow)
        fixedDest = destinationRow + ((destinationRow - sourceRow) % 2);
    else
        fixedDest = destinationRow;

    beginMoveRows(QModelIndex(), sourceRow, sourceRow + count - 1, QModelIndex(), fixedDest);
    for (int r = 0; r < count; r++)
    {
        switch (data_mode)
        {
        case 0:
            (*spc_file).code[label].move(sourceRow + r, destinationRow + r);
            break;

        case 1:
            (*spc_file).params.move(sourceRow + r, destinationRow + r);
            break;

        case 2:
            (*spc_file).strings.move(sourceRow + r, destinationRow + r);
            break;
        }
    }
    endMoveRows();

    return true;
}

Qt::ItemFlags SpcUiModel::flags(const QModelIndex &index) const
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
