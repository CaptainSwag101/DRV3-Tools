#ifndef DAT_UI_MODEL_H
#define DAT_UI_MODEL_H

#include <QAbstractTableModel>
#include <QtWidgets/QTableView>
#include "../utils/dat.h"

class DatUiModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    DatUiModel(QObject *parent, DatFile *file, const int mode = 0);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    bool moveRows(const QModelIndex &sourceParent, int sourceRow, int count, const QModelIndex &destinationParent, int destinationRow) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

private:
    DatFile *dat_file;
    int data_mode;  // 0 = data, 1 = strings (ascii), 2 = strings (utf16)

signals:
        void editCompleted(const QString &str);
};

#endif // DAT_UI_MODEL_H
