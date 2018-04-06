#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QComboBox>
#include <QDebug>
#include <QDropEvent>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QTableView>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->tableCode->setColumnWidth(1, 180);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionOpen_triggered()
{
    if (!confirmUnsaved()) return;

    QString newFilename = QFileDialog::getOpenFileName(this, "Open WRD file", QString(), "WRD files (*.wrd);;All files (*.*)");
    if (newFilename.isEmpty())
        return;

    openFile(newFilename);

    ui->centralWidget->setEnabled(true);

    ui->tableCode->scrollToTop();
    ui->tableStrings->scrollToTop();
    ui->tableFlags->scrollToTop();
}

void MainWindow::on_actionSave_triggered()
{
    if (currentWrd.filename.isEmpty())
        return;

    /*
    currentWrd.labels.clear();
    for (int i = 0; i < ui->comboBox_SelectLabel->count(); i++)
    {
        currentWrd.labels.append(ui->comboBox_SelectLabel->itemText(i));
    }
    */

    const QByteArray out_data = wrd_to_bytes(currentWrd);
    QString out_file = currentWrd.filename;
    QFile f(out_file);
    f.open(QFile::WriteOnly);
    f.write(out_data);
    f.close();

    if (currentWrd.external_strings && currentWrd.strings.count() > 0)
    {
        // Strings are stored in the "(current spc name)_text_(region).spc" file,
        // within an STX file with the same name as the current WRD file.
        QString stx_file = QFileInfo(out_file).absolutePath();
        if (stx_file.endsWith(".SPC", Qt::CaseInsensitive))
            stx_file.chop(4);

        QString region = "_US";
        if (stx_file.right(3).startsWith("_"))
        {
            region = stx_file.right(3);
            stx_file.chop(3);
        }

        stx_file.append("_text" + region + ".SPC");
        stx_file.append(QDir::separator());
        stx_file.append(QFileInfo(out_file).fileName());
        stx_file.replace(".wrd", ".stx");

        const QByteArray stx_data = repack_stx_strings(currentWrd.strings);
        QFile f2(stx_file);
        f2.open(QFile::WriteOnly);
        f2.write(stx_data);
        f2.close();
    }

    unsavedChanges = false;
}

void MainWindow::on_actionSaveAs_triggered()
{
    if (currentWrd.filename.isEmpty())
        return;

    QString newFilename = QFileDialog::getSaveFileName(this, "Save WRD file", QString(), "WRD files (*.wrd);;All files (*.*)");
    if (newFilename.isEmpty())
        return;

    currentWrd.filename = newFilename;
    this->setWindowTitle("WRD Editor: " + QFileInfo(newFilename).fileName());
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
    currentWrd = wrd_from_bytes(f.readAll(), filepath);
    f.close();

    this->setWindowTitle("WRD Editor: " + QFileInfo(filepath).fileName());

    reloadLabelList();
    WrdCodeModel *code = new WrdCodeModel(this, &currentWrd, ui->comboBox_SelectLabel->currentIndex());
    WrdStringsModel *strings = new WrdStringsModel(this, &currentWrd);
    WrdFlagsModel *flags = new WrdFlagsModel(this, &currentWrd);
    QObject::connect(code, &WrdCodeModel::editCompleted, this, &MainWindow::on_editCompleted);
    QObject::connect(strings, &WrdStringsModel::editCompleted, this, &MainWindow::on_editCompleted);
    QObject::connect(flags, &WrdFlagsModel::editCompleted, this, &MainWindow::on_editCompleted);
    ui->tableCode->setModel(code);
    ui->tableStrings->setModel(strings);
    ui->tableFlags->setModel(flags);
}

void MainWindow::reloadLabelList()
{
    ui->comboBox_SelectLabel->blockSignals(true);
    ui->tableCode->blockSignals(true);
    ui->comboBox_SelectLabel->clear();

    for (QString label_name : currentWrd.labels)
    {
        ui->comboBox_SelectLabel->addItem(label_name);
    }

    ui->tableCode->blockSignals(false);
    ui->comboBox_SelectLabel->blockSignals(false);
}

void MainWindow::on_comboBox_SelectLabel_currentIndexChanged(int index)
{
    ui->tableCode->setModel(new WrdCodeModel(this, &currentWrd, index));
}



void MainWindow::on_editCompleted(const QString &str)
{
    unsavedChanges = true;
}

void MainWindow::on_toolButton_CmdAdd_clicked()
{
    const int currentRow = ui->tableCode->currentIndex().row();
    ui->tableCode->model()->insertRow(currentRow);
    ui->tableCode->selectRow(currentRow);
    unsavedChanges = true;
}

void MainWindow::on_toolButton_CmdDel_clicked()
{
    const int currentRow = ui->tableCode->currentIndex().row();
    ui->tableCode->model()->removeRow(currentRow);
    if (currentRow >= currentWrd.code[ui->comboBox_SelectLabel->currentIndex()].count())
        ui->tableCode->selectRow(currentRow - 1);
    else
        ui->tableCode->selectRow(currentRow);
    unsavedChanges = true;
}

void MainWindow::on_toolButton_CmdUp_clicked()
{
    const int currentRow = ui->tableCode->currentIndex().row();
    if (currentRow - 1 < 0)
        return;

    ui->tableCode->model()->moveRow(QModelIndex(), currentRow, QModelIndex(), currentRow - 1);
    ui->tableCode->selectRow(currentRow - 1);
    unsavedChanges = true;
}

void MainWindow::on_toolButton_CmdDown_clicked()
{
    const int currentRow = ui->tableCode->currentIndex().row();
    if (currentRow + 1 >= currentWrd.code[ui->comboBox_SelectLabel->currentIndex()].count())
        return;

    ui->tableCode->model()->moveRow(QModelIndex(), currentRow, QModelIndex(), currentRow + 1);
    ui->tableCode->selectRow(currentRow + 1);
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
    if (currentRow >= currentWrd.strings.count())
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
    if (currentRow + 1 >= currentWrd.strings.count())
        return;

    ui->tableStrings->model()->moveRow(QModelIndex(), currentRow, QModelIndex(), currentRow + 1);
    ui->tableStrings->selectRow(currentRow + 1);
    unsavedChanges = true;
}



void MainWindow::on_toolButton_FlagAdd_clicked()
{
    const int currentRow = ui->tableFlags->currentIndex().row();
    ui->tableFlags->model()->insertRow(currentRow);
    ui->tableFlags->selectRow(currentRow);
    unsavedChanges = true;
}

void MainWindow::on_toolButton_FlagDel_clicked()
{
    const int currentRow = ui->tableFlags->currentIndex().row();
    ui->tableFlags->model()->removeRow(currentRow);
    if (currentRow >= currentWrd.flags.count())
        ui->tableFlags->selectRow(currentRow - 1);
    else
        ui->tableFlags->selectRow(currentRow);
    unsavedChanges = true;
}

void MainWindow::on_toolButton_FlagUp_clicked()
{
    const int currentRow = ui->tableFlags->currentIndex().row();
    if (currentRow - 1 < 0)
        return;

    ui->tableFlags->model()->moveRow(QModelIndex(), currentRow, QModelIndex(), currentRow - 1);
    ui->tableFlags->selectRow(currentRow - 1);
    unsavedChanges = true;
}

void MainWindow::on_toolButton_FlagDown_clicked()
{
    const int currentRow = ui->tableFlags->currentIndex().row();
    if (currentRow + 1 >= currentWrd.flags.count())
        return;

    ui->tableFlags->model()->moveRow(QModelIndex(), currentRow, QModelIndex(), currentRow + 1);
    ui->tableFlags->selectRow(currentRow + 1);
    unsavedChanges = true;
}
