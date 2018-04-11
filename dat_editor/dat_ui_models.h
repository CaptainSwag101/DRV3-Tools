#ifndef DAT_UI_MODELS_H
#define DAT_UI_MODELS_H

#include <QAbstractTableModel>
#include <QtWidgets/QTableView>
#include "../utils/dat.h"

class DatStructModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    DatStructModel(QObject *parent, DatFile *file);
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
    DatFile *dat_file;
    int label;

signals:
        void editCompleted(const QString &str);
};

class DatStringsModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    DatStringsModel(QObject *parent, DatFile *file);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    bool moveRows(const QModelIndex &sourceParent, int sourceRow, int count, const QModelIndex &destinationParent, int destinationChild) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

private:
    DatFile *dat_file;

signals:
    void editCompleted(const QString &str);
};

class DatRefsModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    DatRefsModel(QObject *parent, DatFile *file);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    bool moveRows(const QModelIndex &sourceParent, int sourceRow, int count, const QModelIndex &destinationParent, int destinationChild) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

private:
    DatFile *dat_file;

signals:
    void editCompleted(const QString &str);
};

#endif // DAT_UI_MODELS_H
