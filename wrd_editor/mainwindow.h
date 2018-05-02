#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QCloseEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include "wrd_ui_models.h"

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
    void on_comboBox_SelectLabel_currentIndexChanged(int index);
    void on_editCompleted(const QString &str);
    void on_toolButton_Add_clicked();
    void on_toolButton_Del_clicked();
    void on_toolButton_Up_clicked();
    void on_toolButton_Down_clicked();
    void on_actionUS_toggled(bool checked);
    void on_actionJP_toggled(bool checked);
    void on_actionFR_toggled(bool checked);
    void on_actionZH_toggled(bool checked);
    void on_actionCN_toggled(bool checked);

private:
    bool confirmUnsaved();
    bool openFile(QString newFilepath = QString());
    bool saveFile(QString newFilepath = QString());
    void reloadAllLists();
    void reloadLabelList();
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);

    Ui::MainWindow *ui;
    WrdFile currentWrd;
    bool unsavedChanges = false;
};

#endif // MAINWINDOW_H
