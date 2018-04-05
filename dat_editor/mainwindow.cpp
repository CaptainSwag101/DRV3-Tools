#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QCloseEvent>
#include <QComboBox>
#include <QDebug>
#include <QDropEvent>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QTableView>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionOpen_triggered()
{
    if (!confirmUnsaved()) return;

    QString newFilename = QFileDialog::getOpenFileName(this, "Open DAT file", QString(), "DAT files (*.dat);;All files (*.*)");
    if (newFilename.isEmpty())
        return;

    openFile(newFilename);

    ui->centralWidget->setEnabled(true);

    ui->tableStructs->scrollToTop();
    ui->tableStrings->scrollToTop();
    ui->tableUnkData->scrollToTop();
}

void MainWindow::on_actionSave_triggered()
{
    const QByteArray out_data = dat_to_bytes(currentDat);
    QString out_file = currentDat.filename;
    QFile f(out_file);
    f.open(QFile::WriteOnly);
    f.write(out_data);
    f.close();

    unsavedChanges = false;
}

void MainWindow::on_actionSaveAs_triggered()
{
    QString newFilename = QFileDialog::getSaveFileName(this, "Save DAT file", QString(), "DAT files (*.dat);;All files (*.*)");
    if (newFilename.isEmpty())
        return;

    currentDat.filename = newFilename;
    this->setWindowTitle("DAT Editor: " + QFileInfo(newFilename).fileName());
    on_actionSave_triggered();
}

void MainWindow::on_actionExit_triggered()
{
    this->close();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (!confirmUnsaved())
        event->ignore();
    else
        event->accept();
}

bool MainWindow::confirmUnsaved()
{
    if (!unsavedChanges) return true;

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Unsaved Changes",
                                  "Would you like to save your changes?",
                                  QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel);

    if (reply == QMessageBox::Yes)
    {
        on_actionSave_triggered();
        return true;
    }
    else if (reply == QMessageBox::No)
    {
        return true;
    }
    return false;
}

void MainWindow::openFile(QString filepath)
{
    QFile f(filepath);
    f.open(QFile::ReadOnly);
    currentDat = dat_from_bytes(f.readAll());
    f.close();

    this->setWindowTitle("DAT Editor: " + QFileInfo(filepath).fileName());

    //reloadAllLists();
}
