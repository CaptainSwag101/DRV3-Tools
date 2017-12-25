#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFile>
#include <QFileDialog>
#include <QFontMetrics>
#include <QMessageBox>
#include <QtWidgets>
#include <QTextStream>
#include "../utils/binarydata.h"
#include "../utils/drv3_dec.h"

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
    void on_actionSave_As_triggered();

    void on_actionExit_triggered();

private:
    bool CheckUnsaved();
    void OpenStxFile();
    void SaveStxFile();
    void SaveStxFileAs();
    void ReloadStrings();

    Ui::MainWindow *ui;
    QFileDialog openStx;
    QString currentFilename;
    BinaryData currentStx;
    QStringList strings;
    bool unsavedChanges = false;
};

#endif // MAINWINDOW_H
