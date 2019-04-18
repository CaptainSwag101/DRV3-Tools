#include "dat_ui_model.h"
#include <QDataStream>
#include <QMessageBox>

DatUiModel::DatUiModel(QObject * /*parent*/, DatFile *file, const int mode)
{
    dat_file = file;
    data_mode = mode;
}

int DatUiModel::rowCount(const QModelIndex & /*parent*/) const
{
    switch (data_mode)
    {
    case 0:
    {
        return (*dat_file).data.count();
    }
    case 1:
    {
        return (*dat_file).labels.count();
    }
    case 2:
    {
        return (*dat_file).refs.count();
    }
    }

    return 0;
}

int DatUiModel::columnCount(const QModelIndex & /*parent*/) const
{
    switch (data_mode)
    {
    case 0:
    {
        return (*dat_file).data_types.count();
    }
    case 1:
    case 2:
    {
        return 2;
    }
    }

    return 0;
}

QVariant DatUiModel::data(const QModelIndex &index, int role) const
{
    if (role != Qt::DisplayRole && role != Qt::EditRole)
        return QVariant();

    const int row = index.row();
    const int col = index.column();

    switch (data_mode)
    {
    case 0:
    {
        const QVector<QByteArray> data = (*dat_file).data.at(row);
        const QString data_type = (*dat_file).data_types.at(col);

        int pos = 0;
        if (data_type == "LABEL" || data_type == "ASCII" || data_type == "REFER")
        {
            const ushort label_index = num_from_bytes<ushort>(data.at(col), pos);
            if (label_index < (*dat_file).labels.count() && role == Qt::DisplayRole)
                return (*dat_file).labels.at(label_index);
            else
                return QString::number(label_index, 16);
        }
        else if (data_type == "UTF16")
        {
            const ushort string_index = num_from_bytes<ushort>(data.at(col), pos);
            if (string_index < (*dat_file).refs.count() && role == Qt::DisplayRole)
                return (*dat_file).refs.at(string_index);
            else
                return QString::number(string_index, 16);
        }
        else if (data_type.startsWith("u"))
        {
            if (data_type.right(2) == "16")
                return QString::number(num_from_bytes<ushort>(data.at(col), pos));
            else if (data_type.right(2) == "32")
                return QString::number(num_from_bytes<uint>(data.at(col), pos));
        }
        else if (data_type.startsWith("s"))
        {
            if (data_type.right(2) == "16")
                return QString::number(num_from_bytes<short>(data.at(col), pos));
            else if (data_type.right(2) == "32")
                return QString::number(num_from_bytes<int>(data.at(col), pos));
        }
        else if (data_type.startsWith("f"))
        {
            if (data_type.right(2) == "32")
                return QString::number(num_from_bytes<float>(data.at(col), pos));
            else if (data_type.right(2) == "64")
                return QString::number(num_from_bytes<double>(data.at(col), pos));
        }
        break;
    }
    case 1:
    {
        if (index.column() == 0)    // Display hex index headers
            return num_to_hex(index.row(), 4);
        else
            return (*dat_file).labels.at(index.row());
        break;
    }
    case 2:
    {
        if (index.column() == 0)    // Display hex index headers
            return num_to_hex(index.row(), 4);
        else
            return (*dat_file).refs.at(index.row());
        break;
    }
    }

    return QVariant();
}

QVariant DatUiModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (data_mode != 0 || role != Qt::DisplayRole || orientation != Qt::Horizontal)
        return QVariant();

    return (*dat_file).data_names.at(section) + " " + (*dat_file).data_types.at(section);
}

bool DatUiModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::EditRole)
        return false;

    const int row = index.row();
    const int col = index.column();

    switch (data_mode)
    {
    case 0:
    {
        const QString type = (*dat_file).data_types.at(col);

        bool ok;
        QByteArray val;
        if (type == "LABEL" || type == "ASCII" || type == "REFER" || type == "UTF16")
        {
            val = num_to_bytes(value.toString().toUShort(&ok));
        }
        else if (type.startsWith("u"))
        {
            if (type.right(2) == "16")
                val = num_to_bytes(value.toString().toUShort(&ok));
            else if (type.right(2) == "32")
                val = num_to_bytes(value.toString().toUInt(&ok));
            else if (type.right(2) == "64")
                val = num_to_bytes(value.toString().toULongLong(&ok));
        }
        else if (type.startsWith("s"))
        {
            if (type.right(2) == "16")
                val = num_to_bytes(value.toString().toShort(&ok));
            else if (type.right(2) == "32")
                val = num_to_bytes(value.toString().toInt(&ok));
            else if (type.right(2) == "64")
                val = num_to_bytes(value.toString().toLongLong(&ok));
        }
        else if (type.startsWith("f"))
        {
            if (type.right(2) == "32")
                val = num_to_bytes(value.toString().toFloat(&ok));
            else if (type.right(2) == "64")
                val = num_to_bytes(value.toString().toDouble(&ok));
        }

        if (!ok)
        {
            QMessageBox errorMsg(QMessageBox::Warning, "Conversion Error", "Invalid number.", QMessageBox::Ok);
            errorMsg.exec();
            return false;
        }

        (*dat_file).data[row][col] = val;
        break;
    }
    case 1:
    {
        if (col == 0)
            return false;

        (*dat_file).labels[row] = value.toString();
        break;
    }
    case 2:
    {
        if (col == 0)
            return false;

        (*dat_file).refs[row] = value.toString();
        break;
    }
    }

    emit(editCompleted(value.toString()));
    return true;
}

bool DatUiModel::insertRows(int row, int count, const QModelIndex & /*parent*/)
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
            QVector<QByteArray> blank;
            for (const QString data_type : (*dat_file).data_types)
            {
                if (data_type == "LABEL" || data_type == "ASCII" || data_type == "REFER" || data_type == "UTF16")
                {
                    blank.append(QByteArray(2, 0x00));
                }
                else if (data_type.startsWith("u") || data_type.startsWith("s") || data_type.startsWith("f"))
                {
                    const int data_size = data_type.right(2).toInt() / 8;
                    blank.append(QByteArray(data_size, 0x00));
                }
            }
            (*dat_file).data.insert(row + r, blank);
            break;
        }
        case 1:
        {
            (*dat_file).labels.insert(row + r, QString());
            break;
        }
        case 2:
        {
            (*dat_file).refs.insert(row + r, QString());
            break;
        }
        }


    }
    endInsertRows();

    return true;
}

bool DatUiModel::removeRows(int row, int count, const QModelIndex & /*parent*/)
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
        {
            (*dat_file).data.removeAt(row);
            break;
        }
        case 1:
        {
            (*dat_file).labels.removeAt(row);
            break;
        }
        case 2:
        {
            (*dat_file).refs.removeAt(row);
            break;
        }
        }

    }
    endRemoveRows();

    return true;
}

bool DatUiModel::moveRows(const QModelIndex & /*sourceParent*/, int sourceRow, int count, const QModelIndex & /*destinationParent*/, int destinationRow)
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
        {
            (*dat_file).data.move(sourceRow + r, destinationRow + r);
            break;
        }
        case 1:
        {
            (*dat_file).labels.move(sourceRow + r, destinationRow + r);
            break;
        }
        case 2:
        {
            (*dat_file).refs.move(sourceRow + r, destinationRow + r);
            break;
        }
        }

    }
    endMoveRows();

    return true;
}

Qt::ItemFlags DatUiModel::flags(const QModelIndex &index) const
{
    switch (data_mode)
    {
    case 0:
    {
        return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
    }
    case 1:
    case 2:
    {
        if (index.column() == 0)
            return QAbstractTableModel::flags(index) & ~Qt::ItemIsEditable;
        else
            return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
    }
    }
    return QAbstractTableModel::flags(index);
}
