#include "dat_ui_models.h"
#include <QDataStream>
#include <QMessageBox>

DatStructModel::DatStructModel(QObject * /*parent*/, DatFile *file)
{
    dat_file = file;
}
int DatStructModel::rowCount(const QModelIndex & /*parent*/) const
{
    return (*dat_file).data.count();
}
int DatStructModel::columnCount(const QModelIndex & /*parent*/) const
{
    return (*dat_file).data_types.count();
}
QVariant DatStructModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole || role == Qt::EditRole)
    {
        const int column = index.column();
        const QList<QByteArray> data = (*dat_file).data.at(index.row());
        const QString data_type = (*dat_file).data_types.at(index.column());

        int pos = 0;
        if (data_type == "LABEL" || data_type == "ASCII")
        {
            const ushort label_index = num_from_bytes<ushort>(data.at(column), pos);
            if (label_index < (*dat_file).labels.count())
                return (*dat_file).labels.at(label_index);
            else
                return QString::number(label_index);
        }
        else if (data_type == "REFER" || data_type == "UTF16")
        {
            const ushort refer_index = num_from_bytes<ushort>(data.at(column), pos);
            if (refer_index < (*dat_file).refs.count())
                return (*dat_file).refs.at(refer_index);
            else
                return QString::number(refer_index);
        }
        else if (data_type.startsWith("u"))
        {
            if (data_type.right(2) == "16")
            {
                return QString::number(num_from_bytes<ushort>(data.at(column), pos));
            }
            else if (data_type.right(2) == "32")
            {
                return QString::number(num_from_bytes<uint>(data.at(column), pos));
            }
        }
        else if (data_type.startsWith("s"))
        {
            if (data_type.right(2) == "16")
            {
                return QString::number(num_from_bytes<short>(data.at(column), pos));
            }
            else if (data_type.right(2) == "32")
            {
                return QString::number(num_from_bytes<int>(data.at(column), pos));
            }
        }
        else if (data_type.startsWith("f"))
        {
            if (data_type.right(2) == "32")
            {
                return QString::number(num_from_bytes<float>(data.at(column), pos));
            }
            else if (data_type.right(2) == "64")
            {
                return QString::number(num_from_bytes<double>(data.at(column), pos));
            }
        }
    }

    return QVariant();
}
QVariant DatStructModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole)
    {
        if (orientation == Qt::Horizontal) {
            return (*dat_file).data_names.at(section) + " " + (*dat_file).data_types.at(section);
        }
    }
    return QVariant();
}
bool DatStructModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role == Qt::EditRole)
    {
        const int row = index.row();
        const int column = index.column();


    }

    return true;
}
bool DatStructModel::insertRows(int row, int count, const QModelIndex & /*parent*/)
{
    if (count < 1 || row < 0 || row + count > rowCount())
        return false;

    beginInsertRows(QModelIndex(), row, row + count);

    for (int r = 0; r < count; r++)
    {
        (*dat_file).data.insert(row + r, QList<QByteArray>());
    }

    endInsertRows();
    return true;
}

bool DatStructModel::removeRows(int row, int count, const QModelIndex & /*parent*/)
{
    if (count < 1 || row < 0 || row + count > rowCount())
        return false;

    beginRemoveRows(QModelIndex(), row, row + count);

    for (int r = 0; r < count; r++)
    {
        // Keep removing at the same index, because the next item we want to delete
        // always takes the place of the previous one.
        (*dat_file).data.removeAt(row);
    }

    endRemoveRows();
    return true;
}

bool DatStructModel::moveRows(const QModelIndex & /*sourceParent*/, int sourceRow, int count, const QModelIndex & /*destinationParent*/, int destinationChild)
{
    if (count < 1 || sourceRow < 0 || destinationChild + count > rowCount())
        return false;

    beginMoveRows(QModelIndex(), sourceRow, sourceRow + count, QModelIndex(), destinationChild);

    for (int r = 0; r < count; r++)
    {
        (*dat_file).data.move(sourceRow + r, destinationChild + r);
    }

    return true;
}
Qt::ItemFlags DatStructModel::flags(const QModelIndex &index) const
{
    return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
}



DatStringsModel::DatStringsModel(QObject * /*parent*/, DatFile *file)
{
    dat_file = file;
}
int DatStringsModel::rowCount(const QModelIndex & /*parent*/) const
{
    return (*dat_file).labels.count();
}
int DatStringsModel::columnCount(const QModelIndex & /*parent*/) const
{
    return 2;
}
QVariant DatStringsModel::data(const QModelIndex &index, int role) const
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
            return (*dat_file).labels.at(index.row());
        }
    }

    return QVariant();
}
bool DatStringsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (index.column() == 0)
        return true;

    QString str = value.toString();
    (*dat_file).labels[index.row()] = str;

    return true;
}
bool DatStringsModel::insertRows(int row, int count, const QModelIndex & /*parent*/)
{
    if (count < 1 || row < 0 || row + count > rowCount())
        return false;

    beginInsertRows(QModelIndex(), row, row + count);

    for (int r = 0; r < count; r++)
    {
        (*dat_file).labels.insert(row + r, QString());
    }

    endInsertRows();
    return true;
}
bool DatStringsModel::removeRows(int row, int count, const QModelIndex & /*parent*/)
{
    if (count < 1 || row < 0 || row + count > rowCount())
        return false;

    beginRemoveRows(QModelIndex(), row, row + count);

    for (int r = 0; r < count; r++)
    {
        // Keep removing at the same index, because the next item we want to delete
        // always takes the place of the previous one.
        (*dat_file).labels.removeAt(row);
    }

    endRemoveRows();
    return true;
}
bool DatStringsModel::moveRows(const QModelIndex & /*sourceParent*/, int sourceRow, int count, const QModelIndex & /*destinationParent*/, int destinationChild)
{
    if (count < 1 || sourceRow < 0 || destinationChild + count > rowCount())
        return false;

    beginMoveRows(QModelIndex(), sourceRow, sourceRow + count, QModelIndex(), destinationChild);

    for (int r = 0; r < count; r++)
    {
        (*dat_file).labels.move(sourceRow + r, destinationChild + r);
    }

    return true;
}
Qt::ItemFlags DatStringsModel::flags(const QModelIndex &index) const
{
    return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
}



DatRefsModel::DatRefsModel(QObject * /*parent*/, DatFile *file)
{
    dat_file = file;
}
int DatRefsModel::rowCount(const QModelIndex & /*parent*/) const
{
    return (*dat_file).refs.count();
}
int DatRefsModel::columnCount(const QModelIndex & /*parent*/) const
{
    return 2;
}
QVariant DatRefsModel::data(const QModelIndex &index, int role) const
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
            QString result;
            for (int i = 0; i < (*dat_file).refs.at(index.row()).length(); i++)
            {
                result += (*dat_file).refs.at(index.row()).at(i);
            }
            return result;
        }
    }

    return QVariant();
}
bool DatRefsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    QString valString = value.toString();
    QByteArray result;
    for (int i = 0; i < valString.length() / 2; i++)
    {
        const QString split = valString.mid(i * 2, 2);
        if (split.length() != 2)
        {
            QMessageBox errorMsg(QMessageBox::Warning, "Hex Conversion Error", "Argument length must be a multiple of 2 hexadecimal digits (1 byte).", QMessageBox::Ok);
            errorMsg.exec();
            return false;
        }

        bool ok;
        const uchar c = (uchar)split.toUShort(&ok, 16);
        if (!ok)
        {
            QMessageBox errorMsg(QMessageBox::Warning, "Hex Conversion Error", "Invalid hexadecimal value.", QMessageBox::Ok);
            errorMsg.exec();
            return false;
        }

        result.append(c);
    }
    (*dat_file).refs[index.row()] = result;

    return true;
}
bool DatRefsModel::insertRows(int row, int count, const QModelIndex & /*parent*/)
{
    if (count < 1 || row < 0 || row + count > rowCount())
        return false;

    beginInsertRows(QModelIndex(), row, row + count);

    for (int r = 0; r < count; r++)
    {
        (*dat_file).refs.insert(row + r, QByteArray());
    }

    endInsertRows();
    return true;
}

bool DatRefsModel::removeRows(int row, int count, const QModelIndex & /*parent*/)
{
    if (count < 1 || row < 0 || row + count > rowCount())
        return false;

    beginRemoveRows(QModelIndex(), row, row + count);

    for (int r = 0; r < count; r++)
    {
        // Keep removing at the same index, because the next item we want to delete
        // always takes the place of the previous one.
        (*dat_file).refs.removeAt(row);
    }

    endRemoveRows();
    return true;
}

bool DatRefsModel::moveRows(const QModelIndex & /*sourceParent*/, int sourceRow, int count, const QModelIndex & /*destinationParent*/, int destinationChild)
{
    if (count < 1 || sourceRow < 0 || destinationChild + count > rowCount())
        return false;

    beginMoveRows(QModelIndex(), sourceRow, sourceRow + count, QModelIndex(), destinationChild);

    for (int r = 0; r < count; r++)
    {
        (*dat_file).refs.move(sourceRow + r, destinationChild + r);
    }

    return true;
}
Qt::ItemFlags DatRefsModel::flags(const QModelIndex &index) const
{
    if (index.column() == 0)
        return QAbstractTableModel::flags(index) & ~Qt::ItemIsEditable;
    else
        return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
}
