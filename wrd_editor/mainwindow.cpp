#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <cmath>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
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

    QString newFilename = QFileDialog::getOpenFileName(this, "Open WRD file", QString(), "WRD files (*.wrd);;All files (*.*)");
    if (newFilename.isEmpty())
        return;

    openFile(newFilename);
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

    const QByteArray out_data = wrd_to_data(currentWrd);
    QString out_file = currentWrd.filename;
    QFile f(out_file);
    f.open(QFile::WriteOnly);
    f.write(out_data);
    f.close();

    if (currentWrd.external_strings)
    {
        // Strings are stored in the "(current spc name)_text_(region).spc" file,
        // within an STX file with the same name as the current WRD file.
        QString stx_file = QFileInfo(out_file).absolutePath();
        if (stx_file.endsWith(".SPC", Qt::CaseInsensitive))
            stx_file.chop(4);
        if (stx_file.endsWith("_US"))
            stx_file.chop(3);
        stx_file.append("_text_US.SPC");
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
    currentWrd = wrd_from_data(f.readAll(), filepath);
    f.close();

    this->setWindowTitle("WRD Editor: " + QFileInfo(filepath).fileName());

    ui->tableWidget_LabelCode->setEnabled(true);
    ui->tableWidget_Strings->setEnabled(true);
    ui->tableWidget_Flags->setEnabled(true);
    ui->comboBox_SelectLabel->setEnabled(true);
    ui->toolButton_CmdAdd->setEnabled(true);
    ui->toolButton_CmdDel->setEnabled(true);
    ui->toolButton_CmdUp->setEnabled(true);
    ui->toolButton_CmdDown->setEnabled(true);
    ui->toolButton_StringAdd->setEnabled(true);
    ui->toolButton_StringDel->setEnabled(true);
    ui->toolButton_StringUp->setEnabled(true);
    ui->toolButton_StringDown->setEnabled(true);
    ui->toolButton_FlagAdd->setEnabled(true);
    ui->toolButton_FlagDel->setEnabled(true);
    ui->toolButton_FlagUp->setEnabled(true);
    ui->toolButton_FlagDown->setEnabled(true);

    reloadLists();
}

void MainWindow::reloadLists()
{
    ui->comboBox_SelectLabel->blockSignals(true);
    ui->comboBox_SelectLabel->clear();
    for (QString label_name : currentWrd.labels)
    {
        ui->comboBox_SelectLabel->addItem(label_name);
    }
    ui->comboBox_SelectLabel->blockSignals(false);
    this->on_comboBox_SelectLabel_currentIndexChanged(0);

    ui->tableWidget_Flags->blockSignals(true);
    ui->tableWidget_Flags->clearContents();
    ui->tableWidget_Flags->setColumnCount(2);
    ui->tableWidget_Flags->setRowCount(currentWrd.flags.count());
    for (int i = 0; i < currentWrd.flags.count(); i++)
    {
        ui->tableWidget_Flags->setItem(i, 1, new QTableWidgetItem(currentWrd.flags.at(i)));
    }
    updateHexHeaders(ui->tableWidget_Flags);
    ui->tableWidget_Flags->blockSignals(false);

    ui->tableWidget_Strings->blockSignals(true);
    ui->tableWidget_Strings->clearContents();
    ui->tableWidget_Strings->setColumnCount(2);
    ui->tableWidget_Strings->setRowCount(currentWrd.strings.count());
    for (int i = 0; i < currentWrd.strings.count(); i++)
    {
        QString str = currentWrd.strings.at(i);
        str.replace("\n", "\\n");
        ui->tableWidget_Strings->setItem(i, 1, new QTableWidgetItem(str));
    }
    updateHexHeaders(ui->tableWidget_Strings);
    ui->tableWidget_Strings->blockSignals(false);

    unsavedChanges = false;
}

void MainWindow::updateHexHeaders(QTableWidget *widget)
{
    if (widget == nullptr)
        return;

    const int rows = widget->rowCount();
    for (int i = 0; i < rows; i++)
    {
        QTableWidgetItem *item = new QTableWidgetItem(QString::number(i, 16).toUpper().rightJustified(4, '0'));
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        QTableWidgetItem *old = widget->takeItem(i, 0);
        delete old;
        widget->setItem(i, 0, item);
    }
}

void MainWindow::on_comboBox_SelectLabel_currentIndexChanged(int index)
{
    if (index < 0 || index > currentWrd.cmds.count())
        return;

    const QList<WrdCmd> cur_label_cmds = currentWrd.cmds.at(index);

    ui->tableWidget_LabelCode->blockSignals(true);
    ui->tableWidget_LabelCode->clearContents();
    ui->tableWidget_LabelCode->setRowCount(cur_label_cmds.count());

    for (int i = 0; i < cur_label_cmds.count(); i++)
    {
        const WrdCmd cmd = cur_label_cmds.at(i);

        QTableWidgetItem *newOpcode = new QTableWidgetItem(QString::number(cmd.opcode, 16).toUpper().rightJustified(4, '0'));
        ui->tableWidget_LabelCode->setItem(i, 0, newOpcode);

        QString argString;
        for (ushort arg : cmd.args)
        {
            argString += QString::number(arg, 16).toUpper().rightJustified(4, '0');
        }
        QTableWidgetItem *newArgs = new QTableWidgetItem(argString);
        ui->tableWidget_LabelCode->setItem(i, 1, newArgs);
    }
    ui->tableWidget_LabelCode->blockSignals(false);
}

void MainWindow::on_tableWidget_Strings_itemChanged(QTableWidgetItem *item)
{
    QString str = item->text();
    if (ui->tableWidget_Strings->column(item) == 0 || str.isEmpty())
        return;

    str.replace("\\n", "\n");
    currentWrd.strings[ui->tableWidget_Strings->row(item)] = str;
    unsavedChanges = true;
}

void MainWindow::on_tableWidget_LabelCode_cellChanged(int row, int column)
{
    QTableWidgetItem *item = ui->tableWidget_LabelCode->item(row, column);

    if (column == 0)   // Opcode
    {
        const QString opText = item->text();

        bool ok;
        ushort val = opText.toUShort(&ok, 16);
        if (!ok || opText.isEmpty())
        {
            QMessageBox errorMsg(QMessageBox::Warning, "Hex Conversion Error", "Hex Conversion Error", QMessageBox::Ok, this);
            errorMsg.exec();
            return;
        }

        currentWrd.cmds[ui->comboBox_SelectLabel->currentIndex()][row].opcode = val;
    }
    else            // Args
    {
        const QString argText = item->text();

        currentWrd.cmds[ui->comboBox_SelectLabel->currentIndex()][row].args.clear();
        for (int argNum = 0; argNum < argText.count() / 4; argNum++)
        {
            bool ok;
            ushort val = argText.mid(argNum * 4, 4).toUShort(&ok, 16);
            if (!ok)
            {
                QMessageBox errorMsg(QMessageBox::Warning, "Hex Conversion Error", "Hex Conversion Error", QMessageBox::Ok, this);
                errorMsg.exec();
                return;
            }

            currentWrd.cmds[ui->comboBox_SelectLabel->currentIndex()][row].args.append(val);
        }
    }

    unsavedChanges = true;
}

void MainWindow::on_tableWidget_Flags_itemChanged(QTableWidgetItem *item)
{
    QString flag = item->text();
    if (ui->tableWidget_Flags->column(item) == 0 || flag.isEmpty())
        return;

    currentWrd.flags[ui->tableWidget_Flags->row(item)] = flag;
    unsavedChanges = true;
}


void MainWindow::on_toolButton_CmdAdd_clicked()
{
    int currentRow = ui->tableWidget_LabelCode->currentRow();
    if (currentRow < 0 || currentRow >= ui->tableWidget_LabelCode->rowCount())
        return;

    ui->tableWidget_LabelCode->insertRow(currentRow + 1);
    ui->tableWidget_LabelCode->setItem(currentRow + 1, 0, new QTableWidgetItem());
    ui->tableWidget_LabelCode->setItem(currentRow + 1, 1, new QTableWidgetItem());

    unsavedChanges = true;
}

void MainWindow::on_toolButton_CmdDel_clicked()
{
    int currentRow = ui->tableWidget_LabelCode->currentRow();
    if (currentRow < 0 || currentRow >= ui->tableWidget_LabelCode->rowCount())
        return;

    ui->tableWidget_LabelCode->removeRow(currentRow);
    unsavedChanges = true;
}

void MainWindow::on_toolButton_CmdUp_clicked()
{
    int currentRow = ui->tableWidget_LabelCode->currentRow();
    if (currentRow < 1 || currentRow >= ui->tableWidget_LabelCode->rowCount())
        return;

    QString op1 = ui->tableWidget_LabelCode->item(currentRow, 0)->text();
    QString arg1 = ui->tableWidget_LabelCode->item(currentRow, 1)->text();
    QString op2 = ui->tableWidget_LabelCode->item(currentRow - 1, 0)->text();
    QString arg2 = ui->tableWidget_LabelCode->item(currentRow - 1, 1)->text();

    ui->tableWidget_LabelCode->setItem(currentRow - 1, 0, new QTableWidgetItem(op1));
    ui->tableWidget_LabelCode->setItem(currentRow - 1, 1, new QTableWidgetItem(arg1));
    ui->tableWidget_LabelCode->setItem(currentRow, 0, new QTableWidgetItem(op2));
    ui->tableWidget_LabelCode->setItem(currentRow, 1, new QTableWidgetItem(arg2));

    ui->tableWidget_LabelCode->setCurrentCell(currentRow - 1, ui->tableWidget_LabelCode->currentColumn());
    unsavedChanges = true;
}

void MainWindow::on_toolButton_CmdDown_clicked()
{
    int currentRow = ui->tableWidget_LabelCode->currentRow();
    if (currentRow < 0 || currentRow + 1 >= ui->tableWidget_LabelCode->rowCount())
        return;

    QString op1 = ui->tableWidget_LabelCode->item(currentRow, 0)->text();
    QString arg1 = ui->tableWidget_LabelCode->item(currentRow, 1)->text();
    QString op2 = ui->tableWidget_LabelCode->item(currentRow + 1, 0)->text();
    QString arg2 = ui->tableWidget_LabelCode->item(currentRow + 1, 1)->text();

    ui->tableWidget_LabelCode->setItem(currentRow + 1, 0, new QTableWidgetItem(op1));
    ui->tableWidget_LabelCode->setItem(currentRow + 1, 1, new QTableWidgetItem(arg1));
    ui->tableWidget_LabelCode->setItem(currentRow, 0, new QTableWidgetItem(op2));
    ui->tableWidget_LabelCode->setItem(currentRow, 1, new QTableWidgetItem(arg2));

    ui->tableWidget_LabelCode->setCurrentCell(currentRow + 1, ui->tableWidget_LabelCode->currentColumn());
    unsavedChanges = true;
}


void MainWindow::on_toolButton_StringAdd_clicked()
{
    int currentRow = ui->tableWidget_Strings->currentRow();
    if (currentRow < 0 || currentRow >= ui->tableWidget_Strings->rowCount())
        return;

    ui->tableWidget_Strings->insertRow(currentRow + 1);
    ui->tableWidget_Strings->setItem(currentRow + 1, 1, new QTableWidgetItem());

    updateHexHeaders(ui->tableWidget_Strings);
    unsavedChanges = true;
}

void MainWindow::on_toolButton_StringDel_clicked()
{
    int currentRow = ui->tableWidget_Strings->currentRow();
    if (currentRow < 0 || currentRow >= ui->tableWidget_Strings->rowCount())
        return;

    ui->tableWidget_Strings->removeRow(currentRow);

    updateHexHeaders(ui->tableWidget_Strings);
    unsavedChanges = true;
}

void MainWindow::on_toolButton_StringUp_clicked()
{
    int currentRow = ui->tableWidget_Strings->currentRow();
    if (currentRow < 1 || currentRow >= ui->tableWidget_Strings->rowCount())
        return;

    QString str1 = ui->tableWidget_Strings->item(currentRow, 1)->text();
    QString str2 = ui->tableWidget_Strings->item(currentRow - 1, 1)->text();

    ui->tableWidget_Strings->setItem(currentRow - 1, 1, new QTableWidgetItem(str1));
    ui->tableWidget_Strings->setItem(currentRow, 1, new QTableWidgetItem(str2));

    ui->tableWidget_Strings->setCurrentCell(currentRow - 1, ui->tableWidget_Strings->currentColumn());
    unsavedChanges = true;
}

void MainWindow::on_toolButton_StringDown_clicked()
{
    int currentRow = ui->tableWidget_Strings->currentRow();
    if (currentRow < 0 || currentRow + 1 >= ui->tableWidget_Strings->rowCount())
        return;

    QString str1 = ui->tableWidget_Strings->item(currentRow, 1)->text();
    QString str2 = ui->tableWidget_Strings->item(currentRow + 1, 1)->text();

    ui->tableWidget_Strings->setItem(currentRow + 1, 1, new QTableWidgetItem(str1));
    ui->tableWidget_Strings->setItem(currentRow, 1, new QTableWidgetItem(str2));

    ui->tableWidget_Strings->setCurrentCell(currentRow + 1, ui->tableWidget_Strings->currentColumn());
    unsavedChanges = true;
}


void MainWindow::on_toolButton_FlagAdd_clicked()
{
    int currentRow = ui->tableWidget_Flags->currentRow();
    if (currentRow < 0 || currentRow >= ui->tableWidget_Flags->rowCount())
        return;

    ui->tableWidget_Flags->insertRow(currentRow + 1);
    ui->tableWidget_Flags->setItem(currentRow + 1, 1, new QTableWidgetItem());

    updateHexHeaders(ui->tableWidget_Flags);
    unsavedChanges = true;
}

void MainWindow::on_toolButton_FlagDel_clicked()
{
    int currentRow = ui->tableWidget_Flags->currentRow();
    if (currentRow < 0 || currentRow >= ui->tableWidget_Flags->rowCount())
        return;

    ui->tableWidget_Flags->removeRow(currentRow);

    updateHexHeaders(ui->tableWidget_Flags);
    unsavedChanges = true;
}

void MainWindow::on_toolButton_FlagUp_clicked()
{
    int currentRow = ui->tableWidget_Flags->currentRow();
    if (currentRow < 1 || currentRow >= ui->tableWidget_Flags->rowCount())
        return;

    QString flag1 = ui->tableWidget_Flags->item(currentRow, 1)->text();
    QString flag2 = ui->tableWidget_Flags->item(currentRow - 1, 1)->text();

    ui->tableWidget_Flags->setItem(currentRow - 1, 1, new QTableWidgetItem(flag1));
    ui->tableWidget_Flags->setItem(currentRow, 1, new QTableWidgetItem(flag2));

    ui->tableWidget_Flags->setCurrentCell(currentRow - 1, ui->tableWidget_Flags->currentColumn());
    unsavedChanges = true;
}

void MainWindow::on_toolButton_FlagDown_clicked()
{
    int currentRow = ui->tableWidget_Flags->currentRow();
    if (currentRow < 0 || currentRow + 1 >= ui->tableWidget_Flags->rowCount())
        return;

    QString flag1 = ui->tableWidget_Flags->item(currentRow, 1)->text();
    QString flag2 = ui->tableWidget_Flags->item(currentRow + 1, 1)->text();

    ui->tableWidget_Flags->setItem(currentRow + 1, 1, new QTableWidgetItem(flag1));
    ui->tableWidget_Flags->setItem(currentRow, 1, new QTableWidgetItem(flag2));

    ui->tableWidget_Flags->setCurrentCell(currentRow + 1, ui->tableWidget_Flags->currentColumn());
    unsavedChanges = true;
}
