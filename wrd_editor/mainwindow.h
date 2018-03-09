#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QComboBox>
#include <QCloseEvent>
#include <QDebug>
#include <QDropEvent>
#include <QFile>
#include <QFileDialog>
#include <QFutureWatcher>
#include <QMessageBox>
#include <QProgressDialog>
#include <QTableWidget>
#include <QtConcurrent/QtConcurrent>
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

    void on_comboBox_SelectLabel_currentIndexChanged(int index);

private:
    bool confirmUnsaved();
    void openFile(QString filepath);
    void reloadLists();
    void updateLabelCodeWidget();

    Ui::MainWindow *ui;
    WrdFile currentWrd;
    bool unsavedChanges = false;
};

#endif // MAINWINDOW_H
