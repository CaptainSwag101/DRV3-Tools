#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QCloseEvent>
#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QProgressDialog>
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
    void closeEvent(QCloseEvent *event);
    void on_actionExtractAll_triggered();
    void on_actionExtractSelected_triggered();
    void on_actionInjectFile_triggered();

private:
    bool confirmUnsaved();
    void reloadSubfileList();
    void extractFile(QString outDir, SpcSubfile subfile);

    Ui::MainWindow *ui;
    SpcFile currentSpc;
    bool unsavedChanges = false;
};

#endif // MAINWINDOW_H
