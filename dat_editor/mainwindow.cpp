#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QComboBox>
#include <QDebug>
#include <QDropEvent>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QTableView>
#include "dat_ui_models.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->tableStructs->setColumnWidth(1, 170);
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
    if (currentDat.filename.isEmpty())
        return;

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
    if (currentDat.filename.isEmpty())
        return;

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

    ui->tableStructs->setModel(new DatStructModel(this, &currentDat));
    ui->tableStrings->setModel(new DatStringsModel(this, &currentDat));
    ui->tableUnkData->setModel(new DatUnkDataModel(this, &currentDat));
}



void MainWindow::on_toolButton_StructAdd_clicked()
{
    const int currentRow = ui->tableStructs->currentIndex().row();
    ui->tableStructs->model()->insertRow(currentRow);
    ui->tableStructs->selectRow(currentRow);
    unsavedChanges = true;
}

void MainWindow::on_toolButton_StructDel_clicked()
{
    const int currentRow = ui->tableStructs->currentIndex().row();
    ui->tableStructs->model()->removeRow(currentRow);
    if (currentRow >= currentDat.struct_data.count())
        ui->tableStructs->selectRow(currentRow - 1);
    else
        ui->tableStructs->selectRow(currentRow);
    unsavedChanges = true;
}

void MainWindow::on_toolButton_StructUp_clicked()
{
    const int currentRow = ui->tableStructs->currentIndex().row();
    if (currentRow - 1 < 0)
        return;
    ui->tableStructs->model()->moveRow(QModelIndex(), currentRow, QModelIndex(), currentRow - 1);
    ui->tableStructs->selectRow(currentRow - 1);
    unsavedChanges = true;
}

void MainWindow::on_toolButton_StructDown_clicked()
{
    const int currentRow = ui->tableStructs->currentIndex().row();
    if (currentRow + 1 >= currentDat.struct_data.count())
        return;
    ui->tableStructs->model()->moveRow(QModelIndex(), currentRow, QModelIndex(), currentRow + 1);
    ui->tableStructs->selectRow(currentRow + 1);
    unsavedChanges = true;
}



void MainWindow::on_toolButton_StringAdd_clicked()
{
    const int currentRow = ui->tableStrings->currentIndex().row();
    ui->tableStrings->model()->insertRow(currentRow);
    ui->tableStrings->selectRow(currentRow);
    unsavedChanges = true;
}

void MainWindow::on_toolButton_StringDel_clicked()
{
    const int currentRow = ui->tableStrings->currentIndex().row();
    ui->tableStrings->model()->removeRow(currentRow);
    if (currentRow >= currentDat.strings.count())
        ui->tableStrings->selectRow(currentRow - 1);
    else
        ui->tableStrings->selectRow(currentRow);
    unsavedChanges = true;
}

void MainWindow::on_toolButton_StringUp_clicked()
{
    const int currentRow = ui->tableStrings->currentIndex().row();
    if (currentRow - 1 < 0)
        return;
    ui->tableStrings->model()->moveRow(QModelIndex(), currentRow, QModelIndex(), currentRow - 1);
    ui->tableStrings->selectRow(currentRow - 1);
    unsavedChanges = true;
}

void MainWindow::on_toolButton_StringDown_clicked()
{
    const int currentRow = ui->tableStrings->currentIndex().row();
    if (currentRow + 1 >= currentDat.strings.count())
        return;
    ui->tableStrings->model()->moveRow(QModelIndex(), currentRow, QModelIndex(), currentRow + 1);
    ui->tableStrings->selectRow(currentRow + 1);
    unsavedChanges = true;
}



void MainWindow::on_toolButton_UnkAdd_clicked()
{
    const int currentRow = ui->tableUnkData->currentIndex().row();
    ui->tableUnkData->model()->insertRow(currentRow);
    ui->tableUnkData->selectRow(currentRow);
    unsavedChanges = true;
}

void MainWindow::on_toolButton_UnkDel_clicked()
{
    const int currentRow = ui->tableUnkData->currentIndex().row();
    ui->tableUnkData->model()->removeRow(currentRow);
    if (currentRow >= currentDat.unk_data.count())
        ui->tableUnkData->selectRow(currentRow - 1);
    else
        ui->tableUnkData->selectRow(currentRow);
    unsavedChanges = true;
}

void MainWindow::on_toolButton_UnkUp_clicked()
{
    const int currentRow = ui->tableUnkData->currentIndex().row();
    if (currentRow - 1 < 0)
        return;
    ui->tableUnkData->model()->moveRow(QModelIndex(), currentRow, QModelIndex(), currentRow - 1);
    ui->tableUnkData->selectRow(currentRow - 1);
    unsavedChanges = true;
}

void MainWindow::on_toolButton_UnkDown_clicked()
{
    const int currentRow = ui->tableUnkData->currentIndex().row();
    if (currentRow + 1 >= currentDat.unk_data.count())
        return;
    ui->tableUnkData->model()->moveRow(QModelIndex(), currentRow, QModelIndex(), currentRow + 1);
    ui->tableUnkData->selectRow(currentRow + 1);
    unsavedChanges = true;
}
