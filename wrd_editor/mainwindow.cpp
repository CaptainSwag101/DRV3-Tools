#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <cmath>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->tableCode->setColumnWidth(1, 170);
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
    ui->tableCode->setModel(new WrdCodeModel(this, &currentWrd, ui->comboBox_SelectLabel->currentIndex()));
    ui->tableStrings->setModel(new WrdStringsModel(this, &currentWrd));
    ui->tableFlags->setModel(new WrdFlagsModel(this, &currentWrd));
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



void MainWindow::on_toolButton_CmdAdd_clicked()
{
    const int currentLabel = ui->comboBox_SelectLabel->currentIndex();
    const int currentRow = ui->tableCode->currentIndex().row();
    if (currentRow < -1 || currentRow >= currentWrd.code[currentLabel].count())
        return;

    WrdCmd cmd;
    cmd.opcode = 0x47;
    currentWrd.code[currentLabel].insert(currentRow, cmd);

    ui->tableCode->setModel(new WrdCodeModel(this, &currentWrd, ui->comboBox_SelectLabel->currentIndex()));
    ui->tableCode->selectRow(currentRow);
    unsavedChanges = true;
}

void MainWindow::on_toolButton_CmdDel_clicked()
{
    const int currentLabel = ui->comboBox_SelectLabel->currentIndex();
    const int currentRow = ui->tableCode->currentIndex().row();
    if (currentRow < 0 || currentRow >= currentWrd.code[currentLabel].count())
        return;

    currentWrd.code[currentLabel].removeAt(currentRow);

    ui->tableCode->setModel(new WrdCodeModel(this, &currentWrd, ui->comboBox_SelectLabel->currentIndex()));
    if (currentRow >= currentWrd.code[currentLabel].count())
        ui->tableCode->selectRow(currentRow - 1);
    else
        ui->tableCode->selectRow(currentRow);

    unsavedChanges = true;
}

void MainWindow::on_toolButton_CmdUp_clicked()
{
    const int currentLabel = ui->comboBox_SelectLabel->currentIndex();
    const int currentRow = ui->tableCode->currentIndex().row();
    if (currentRow < 1 || currentRow >= currentWrd.code[currentLabel].count())
        return;

    const WrdCmd cmd1 = currentWrd.code[currentLabel][currentRow];
    const WrdCmd cmd2 = currentWrd.code[currentLabel][currentRow - 1];

    currentWrd.code[currentLabel].replace(currentRow, cmd2);
    currentWrd.code[currentLabel].replace(currentRow - 1, cmd1);

    ui->tableCode->setModel(new WrdCodeModel(this, &currentWrd, ui->comboBox_SelectLabel->currentIndex()));
    ui->tableCode->selectRow(currentRow - 1);
    unsavedChanges = true;
}

void MainWindow::on_toolButton_CmdDown_clicked()
{
    const int currentLabel = ui->comboBox_SelectLabel->currentIndex();
    const int currentRow = ui->tableCode->currentIndex().row();
    if (currentRow < 0 || currentRow + 1 >= currentWrd.code[currentLabel].count())
        return;

    const WrdCmd cmd1 = currentWrd.code[currentLabel][currentRow];
    const WrdCmd cmd2 = currentWrd.code[currentLabel][currentRow + 1];

    currentWrd.code[currentLabel].replace(currentRow, cmd2);
    currentWrd.code[currentLabel].replace(currentRow + 1, cmd1);

    ui->tableCode->setModel(new WrdCodeModel(this, &currentWrd, ui->comboBox_SelectLabel->currentIndex()));
    ui->tableCode->selectRow(currentRow + 1);
    unsavedChanges = true;
}



void MainWindow::on_toolButton_StringAdd_clicked()
{
    const int currentRow = ui->tableStrings->currentIndex().row();
    if (currentRow < -1 || currentRow >= currentWrd.strings.count())
        return;

    currentWrd.strings.insert(currentRow, QString());

    ui->tableStrings->setModel(new WrdStringsModel(this, &currentWrd));
    ui->tableStrings->selectRow(currentRow);
    unsavedChanges = true;
}

void MainWindow::on_toolButton_StringDel_clicked()
{
    const int currentRow = ui->tableStrings->currentIndex().row();
    if (currentRow < 0 || currentRow >= currentWrd.strings.count())
        return;

    currentWrd.strings.removeAt(currentRow);

    ui->tableStrings->setModel(new WrdStringsModel(this, &currentWrd));
    if (currentRow >= currentWrd.strings.count())
        ui->tableStrings->selectRow(currentRow - 1);
    else
        ui->tableStrings->selectRow(currentRow);
    unsavedChanges = true;
}

void MainWindow::on_toolButton_StringUp_clicked()
{
    const int currentRow = ui->tableStrings->currentIndex().row();
    if (currentRow < 1 || currentRow >= currentWrd.strings.count())
        return;

    const QString str1 = currentWrd.strings[currentRow];
    const QString str2 = currentWrd.strings[currentRow - 1];

    currentWrd.strings.replace(currentRow, str2);
    currentWrd.strings.replace(currentRow - 1, str1);

    ui->tableStrings->setModel(new WrdStringsModel(this, &currentWrd));
    ui->tableStrings->selectRow(currentRow - 1);
    unsavedChanges = true;
}

void MainWindow::on_toolButton_StringDown_clicked()
{
    const int currentRow = ui->tableStrings->currentIndex().row();
    if (currentRow < 0 || currentRow + 1 >= currentWrd.strings.count())
        return;

    const QString str1 = currentWrd.strings[currentRow];
    const QString str2 = currentWrd.strings[currentRow + 1];

    currentWrd.strings.replace(currentRow, str2);
    currentWrd.strings.replace(currentRow + 1, str1);

    ui->tableStrings->setModel(new WrdStringsModel(this, &currentWrd));
    ui->tableStrings->selectRow(currentRow + 1);
    unsavedChanges = true;
}



void MainWindow::on_toolButton_FlagAdd_clicked()
{
    const int currentRow = ui->tableFlags->currentIndex().row();
    if (currentRow < -1 || currentRow >= currentWrd.flags.count())
        return;

    currentWrd.flags.insert(currentRow, QString());

    ui->tableFlags->setModel(new WrdFlagsModel(this, &currentWrd));
    ui->tableFlags->selectRow(currentRow);

    // We need to update the argument name previews now
    ui->tableCode->setModel(new WrdCodeModel(this, &currentWrd, ui->comboBox_SelectLabel->currentIndex()));
    unsavedChanges = true;
}

void MainWindow::on_toolButton_FlagDel_clicked()
{
    const int currentRow = ui->tableFlags->currentIndex().row();
    if (currentRow < 0 || currentRow >= currentWrd.flags.count())
        return;

    currentWrd.flags.removeAt(currentRow);

    ui->tableFlags->setModel(new WrdFlagsModel(this, &currentWrd));
    if (currentRow >= currentWrd.flags.count())
        ui->tableFlags->selectRow(currentRow - 1);
    else
        ui->tableFlags->selectRow(currentRow);

    // We need to update the argument name previews now
    ui->tableCode->setModel(new WrdCodeModel(this, &currentWrd, ui->comboBox_SelectLabel->currentIndex()));
    unsavedChanges = true;
}

void MainWindow::on_toolButton_FlagUp_clicked()
{
    const int currentRow = ui->tableFlags->currentIndex().row();
    if (currentRow < 1 || currentRow >= currentWrd.flags.count())
        return;

    const QString flag1 = currentWrd.flags[currentRow];
    const QString flag2 = currentWrd.flags[currentRow - 1];

    currentWrd.flags.replace(currentRow, flag2);
    currentWrd.flags.replace(currentRow - 1, flag1);

    ui->tableFlags->setModel(new WrdFlagsModel(this, &currentWrd));
    ui->tableFlags->selectRow(currentRow - 1);

    // We need to update the argument name previews now
    ui->tableCode->setModel(new WrdCodeModel(this, &currentWrd, ui->comboBox_SelectLabel->currentIndex()));
    unsavedChanges = true;
}

void MainWindow::on_toolButton_FlagDown_clicked()
{
    const int currentRow = ui->tableFlags->currentIndex().row();
    if (currentRow < 0 || currentRow + 1 >= currentWrd.flags.count())
        return;

    const QString flag1 = currentWrd.flags[currentRow];
    const QString flag2 = currentWrd.flags[currentRow + 1];

    currentWrd.flags.replace(currentRow, flag2);
    currentWrd.flags.replace(currentRow + 1, flag1);

    ui->tableFlags->setModel(new WrdFlagsModel(this, &currentWrd));
    ui->tableFlags->selectRow(currentRow + 1);

    // We need to update the argument name previews now
    ui->tableCode->setModel(new WrdCodeModel(this, &currentWrd, ui->comboBox_SelectLabel->currentIndex()));
    unsavedChanges = true;
}
