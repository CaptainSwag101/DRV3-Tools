#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include "../utils/binarydata.h"
#include "../utils/data_formats.h"

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
    void on_actionExtractAll_triggered();
    void on_actionExtractSelected_triggered();
    void on_actionInjectFile_triggered();

private:
    bool confirmUnsaved();
    void reloadSubfileList();

    Ui::MainWindow *ui;
    QFileDialog fileDlg;
    SpcFile currentSpc;
    bool unsavedChanges = false;
};

#endif // MAINWINDOW_H
