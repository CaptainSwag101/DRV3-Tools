#include "spc_ui_model.h"
#include <QFileInfo>
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
        return QString::number(subfile.cmp_flag);
    }
    else
    {
        return subfile.filename;
    }

    return QVariant();
}

QVariant SpcUiModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
        return QVariant();

    switch (section)
    {
    case 0:
        return QString("Compression flag");
    case 1:
        return QString("Filename");
    }

    return QVariant();
}

bool SpcUiModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::EditRole)
        return false;

    const int row = index.row();
    const int col = index.column();

    // Compression flag
    if (col == 0)
    {
        bool ok;
        uchar flag = (uchar)value.toUInt(&ok);
        if (!ok || flag < 1 || flag > 2)
        {
            QMessageBox errorMsg(QMessageBox::Warning, "Error", "Invalid value.", QMessageBox::Ok);
            errorMsg.exec();
            return false;
        }

        if (flag == 1 && (*spc_file).subfiles[row].cmp_flag != 1)
        {
            (*spc_file).subfiles[row].data = spc_dec((*spc_file).subfiles[row].data, (*spc_file).subfiles[row].dec_size);
            (*spc_file).subfiles[row].dec_size = (*spc_file).subfiles[row].data.size();
        }
        else if (flag == 2 && (*spc_file).subfiles[row].cmp_flag != 2)
        {
            (*spc_file).subfiles[row].data = spc_cmp((*spc_file).subfiles[row].data);
            (*spc_file).subfiles[row].cmp_size = (*spc_file).subfiles[row].data.size();
        }

        (*spc_file).subfiles[row].cmp_flag = flag;
    }
    // Filename
    else
    {
        const QString filename = value.toString().trimmed();

        // TODO: Ensure the filename only contains valid characters for a file path?
        if (filename.isEmpty())
        {
            QMessageBox errorMsg(QMessageBox::Warning, "Filename Error", "You must enter a filename.", QMessageBox::Ok);
            errorMsg.exec();
            return false;
        }

        for (int i = 0; i < (*spc_file).subfiles.count(); i++)
        {
            if (filename.compare((*spc_file).subfiles.at(i).filename, Qt::CaseInsensitive) == 0)
            {
                QMessageBox::StandardButton reply = QMessageBox::question(nullptr, "Confirm overwrite",
                          filename + " already exists in this location. Would you like to overwrite it?",
                          QMessageBox::Yes|QMessageBox::No);

                if (reply == QMessageBox::No)
                    return false;

                (*spc_file).subfiles[row].filename = (*spc_file).subfiles[i].filename;
                (*spc_file).subfiles[i] = (*spc_file).subfiles[row];
                (*spc_file).subfiles.removeAt(row);
                break;
            }
        }
    }

    emit(editCompleted(value.toString()));
    return true;
}

bool SpcUiModel::insertRows(int row, int count, const QModelIndex & /*parent*/)
{
    if (count < 1 || row < 0 || row + (count - 1) >= rowCount())
        return false;

    beginInsertRows(QModelIndex(), row, row + (count - 1));
    for (int r = 0; r < count; r++)
    {
        // TODO: Open a file dialog so the user can choose a file to add/replace.
        // FIXME: We don't actually need to do anything here, we will just
        // handle adding a new file from the main window directly, using
        // on_actionInjectFile_triggered();
    }
    endInsertRows();

    return true;
}

bool SpcUiModel::removeRows(int row, int count, const QModelIndex & /*parent*/)
{
    if (count < 1 || row < 0 || row + (count - 1) >= rowCount())
        return false;

    QMessageBox::StandardButton reply = QMessageBox::question(nullptr, "Confirm delete",
              "Are you sure you want to delete " + (*spc_file).subfiles[row].filename + "?",
              QMessageBox::Yes|QMessageBox::No);

    if (reply == QMessageBox::No)
        return false;

    // Keep removing at the same index, because the next item we want to delete
    // always takes the place of the previous one.
    beginRemoveRows(QModelIndex(), row, row + (count - 1));
    for (int r = 0; r < count; r++)
    {
        (*spc_file).subfiles.removeAt(row);
    }
    endRemoveRows();

    return true;
}

bool SpcUiModel::moveRows(const QModelIndex & /*sourceParent*/, int sourceRow, int count, const QModelIndex & /*destinationParent*/, int destinationRow)
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
        (*spc_file).subfiles.move(sourceRow + r, destinationRow + r);
        break;
    }
    endMoveRows();

    return true;
}

Qt::ItemFlags SpcUiModel::flags(const QModelIndex &index) const
{
    return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
}
