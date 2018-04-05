#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "../utils/dat.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_actionOpen_triggered();
    void on_actionSave_triggered();
    void on_actionSaveAs_triggered();
    void on_actionExit_triggered();
    void closeEvent(QCloseEvent *event);
    //void on_tableStructs_itemChanged(QTableWidgetItem *item);
    //void on_tableStrings_itemChanged(QTableWidgetItem *item);
    //void on_tableUnkData_itemChanged(QTableWidgetItem *item);
    void on_toolButton_StructAdd_clicked();
    void on_toolButton_StructDel_clicked();
    void on_toolButton_StructUp_clicked();
    void on_toolButton_StructDown_clicked();
    void on_toolButton_StringAdd_clicked();
    void on_toolButton_StringDel_clicked();
    void on_toolButton_StringUp_clicked();
    void on_toolButton_StringDown_clicked();
    void on_toolButton_UnkAdd_clicked();
    void on_toolButton_UnkDel_clicked();
    void on_toolButton_UnkUp_clicked();
    void on_toolButton_UnkDown_clicked();

private:
    bool confirmUnsaved();
    void openFile(QString filepath);
    void reloadAllLists();
    void reloadStructList();
    void reloadStringList();
    void reloadUnkDataList();

    Ui::MainWindow *ui;
    DatFile currentDat;
    bool unsavedChanges = false;
};

#endif // MAINWINDOW_H