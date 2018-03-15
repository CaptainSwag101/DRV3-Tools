#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QCloseEvent>
#include <QDebug>
#include <QDropEvent>
#include <QFile>
#include <QFileDialog>
#include <QFutureWatcher>
#include <QMessageBox>
#include <QProgressDialog>
#include <QtConcurrent/QtConcurrent>
#include "../utils/binarydata.h"
#include "../utils/spc.h"

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
    void closeEvent(QCloseEvent *event);
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);

private:
    bool confirmUnsaved();
    void reloadSubfileList();
    void openFile(QString filename);
    void extractFile(QString outDir, const SpcSubfile &subfile);
    void injectFile(QString name, const QByteArray &fileData);

    Ui::MainWindow *ui;
    SpcFile currentSpc;
    bool unsavedChanges = false;
};

#endif // MAINWINDOW_H
