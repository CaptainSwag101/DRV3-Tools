#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QCloseEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
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
    void on_editCompleted(const QString &str);
    void on_toolButton_Add_clicked();
    void on_toolButton_Del_clicked();
    void on_toolButton_Up_clicked();
    void on_toolButton_Down_clicked();

private:
    bool confirmUnsaved();
    bool openFile(QString newFilepath = QString());
    bool saveFile(QString newFilepath = QString());
    void reloadAllLists();
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);

    Ui::MainWindow *ui;
    DatFile currentDat;
    bool unsavedChanges = false;
};

#endif // MAINWINDOW_H
