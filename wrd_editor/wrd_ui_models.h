#ifndef WRD_UI_MODELS_H
#define WRD_UI_MODELS_H

#include <QAbstractTableModel>
#include <QtWidgets/QTableView>
#include "../utils/wrd.h"

class WrdDataModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    WrdDataModel(QObject *parent, WrdFile *file, const int mode, const int lbl = 0);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    bool moveRows(const QModelIndex &sourceParent, int sourceRow, int count, const QModelIndex &destinationParent, int destinationChild) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

private:
    WrdFile *wrd_file;
    int data_mode;  // 0 = code, 1 = params, 2 = strings
    int label;

signals:
    void editCompleted(const QString &str);
};

#endif // WRD_UI_MODELS_H
