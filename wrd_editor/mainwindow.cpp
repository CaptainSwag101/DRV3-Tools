#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <cmath>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->tableWidget_Code->setColumnWidth(1, 170);
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

    ui->tableWidget_Code->scrollToTop();
    ui->tableWidget_Strings->scrollToTop();
    ui->tableWidget_Flags->scrollToTop();
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
            region.chop(3);
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

    reloadAllLists();
}

void MainWindow::reloadAllLists()
{
    reloadLabelList();
    reloadCodeList();
    reloadStringList();
    reloadFlagList();

    //unsavedChanges = false;
}

void MainWindow::reloadLabelList()
{
    ui->comboBox_SelectLabel->blockSignals(true);
    ui->tableWidget_Code->blockSignals(true);
    ui->comboBox_SelectLabel->clear();

    for (QString label_name : currentWrd.labels)
    {
        ui->comboBox_SelectLabel->addItem(label_name);
    }

    ui->tableWidget_Code->blockSignals(false);
    ui->comboBox_SelectLabel->blockSignals(false);
}

void MainWindow::reloadCodeList(int index)
{
    const int currentLabel = (index != -1) ? index : ui->comboBox_SelectLabel->currentIndex();

    if (currentLabel < 0 || currentLabel >= currentWrd.labels.count())
        return;

    const QList<WrdCmd> cur_label_cmds = currentWrd.code.at(currentLabel);

    ui->tableWidget_Code->blockSignals(true);
    ui->tableWidget_Code->clearContents();
    ui->tableWidget_Code->setRowCount(cur_label_cmds.count());

    for (int i = 0; i < cur_label_cmds.count(); i++)
    {
        const WrdCmd cmd = cur_label_cmds.at(i);

        QString opString = QString::number(cmd.opcode, 16).toUpper().rightJustified(2, '0');
        ui->tableWidget_Code->setItem(i, 0, new QTableWidgetItem(opString));

        QString argHexString;
        for (ushort arg : cmd.args)
        {
            const QString part = QString::number(arg, 16).toUpper().rightJustified(4, '0');
            argHexString += part;
        }
        ui->tableWidget_Code->setItem(i, 1, new QTableWidgetItem(argHexString.simplified()));


        QString argParsedString;
        argParsedString += cmd.name;

        if (cmd.args.count() > 0)
            argParsedString += ": ";

        for (int a = 0; a < cmd.args.count(); a++)
        {
            const ushort arg = cmd.args.at(a);

            if (cmd.arg_types.at(a) == 0 && arg < currentWrd.flags.count())
                argParsedString += currentWrd.flags.at(arg) + " ";
            else if (cmd.arg_types.at(a) == 1 && arg < currentWrd.strings.count())
                argParsedString += currentWrd.strings.at(arg) + " ";
            else
                argParsedString += QString::number(arg) + " ";
        }

        QTableWidgetItem *names = new QTableWidgetItem(argParsedString.simplified());
        names->setFlags(names->flags() & ~Qt::ItemIsEditable);
        ui->tableWidget_Code->setItem(i, 2, names);
    }

    ui->tableWidget_Code->blockSignals(false);
}

void MainWindow::reloadStringList()
{
    ui->tableWidget_Strings->blockSignals(true);
    ui->tableWidget_Strings->clearContents();

    const int strCount = currentWrd.strings.count();
    ui->tableWidget_Strings->setRowCount(strCount);
    for (int i = 0; i < strCount; i++)
    {
        QString str = currentWrd.strings.at(i);
        str.replace("\n", "\\n");
        ui->tableWidget_Strings->setItem(i, 1, new QTableWidgetItem(str));
    }
    updateHexHeaders(ui->tableWidget_Strings);

    ui->tableWidget_Strings->blockSignals(false);
}

void MainWindow::reloadFlagList()
{
    ui->tableWidget_Flags->blockSignals(true);
    ui->tableWidget_Flags->clearContents();

    const int flagCount = currentWrd.flags.count();
    ui->tableWidget_Flags->setRowCount(flagCount);
    for (int i = 0; i < flagCount; i++)
    {
        ui->tableWidget_Flags->setItem(i, 1, new QTableWidgetItem(currentWrd.flags.at(i)));
    }
    updateHexHeaders(ui->tableWidget_Flags);

    ui->tableWidget_Flags->blockSignals(false);
}

void MainWindow::updateHexHeaders(QTableWidget *widget)
{
    if (widget == nullptr)
        return;

    const int rowCount = widget->rowCount();
    for (int i = 0; i < rowCount; i++)
    {
        QTableWidgetItem *item = new QTableWidgetItem(QString::number(i, 16).toUpper().rightJustified(4, '0'));
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        widget->setItem(i, 0, item);
    }
}

void MainWindow::on_comboBox_SelectLabel_currentIndexChanged(int index)
{
    reloadCodeList(index);
    //ui->tableWidget_Code->setCurrentCell(-1, -1);
}

void MainWindow::on_tableWidget_Code_itemChanged(QTableWidgetItem *item)
{
    const int row = ui->tableWidget_Code->row(item);
    const int column = ui->tableWidget_Code->column(item);
    const int label = ui->comboBox_SelectLabel->currentIndex();

    // Opcode
    if (column == 0)
    {
        const QString opText = item->text();
        if (opText.length() != 2)
        {
            QMessageBox errorMsg(QMessageBox::Warning, "Hex Conversion Error", "Opcode length must be exactly 2 hexadecimal digits (1 byte).", QMessageBox::Ok, this);
            errorMsg.exec();
            ui->tableWidget_Code->editItem(item);
            return;
        }

        bool ok;
        const uchar val = (uchar)opText.toUShort(&ok, 16);
        if (!ok)
        {
            QMessageBox errorMsg(QMessageBox::Warning, "Hex Conversion Error", "Invalid hexadecimal value.", QMessageBox::Ok, this);
            errorMsg.exec();
            ui->tableWidget_Code->editItem(item);
            return;
        }

        currentWrd.code[label][row].opcode = val;
        currentWrd.code[label][row].name = "UNKNOWN_CMD";
        currentWrd.code[label][row].arg_types.clear();

        for (const WrdCmd known_cmd : known_commands)
        {
            if (val == known_cmd.opcode)
            {
                currentWrd.code[label][row].name = known_cmd.name;
                currentWrd.code[label][row].arg_types = known_cmd.arg_types;
                break;
            }
        }

        if (currentWrd.code[label][row].arg_types.count() < currentWrd.code[label][row].args.count())
            for (int i = currentWrd.code[label][row].arg_types.count(); i < currentWrd.code[label][row].args.count(); i++)
                currentWrd.code[label][row].arg_types.append(0);
    }
    // Args
    else if (column == 1)
    {
        const QString argText = item->text();

        currentWrd.code[label][row].args.clear();
        for (int argNum = 0; argNum < argText.count() / 4; argNum++)
        {
            const QString splitText = argText.mid(argNum * 4, 4);
            if (splitText.length() != 4)
            {
                QMessageBox errorMsg(QMessageBox::Warning, "Hex Conversion Error", "Argument length must be a multiple of 4 hexadecimal digits (2 bytes).", QMessageBox::Ok, this);
                errorMsg.exec();
                ui->tableWidget_Code->editItem(item);
                return;
            }

            bool ok;
            const ushort val = splitText.toUShort(&ok, 16);
            if (!ok)
            {
                QMessageBox errorMsg(QMessageBox::Warning, "Hex Conversion Error", "Invalid hexadecimal value.", QMessageBox::Ok, this);
                errorMsg.exec();
                ui->tableWidget_Code->editItem(item);
                return;
            }

            currentWrd.code[label][row].args.append(val);
        }

        if (currentWrd.code[label][row].arg_types.count() < currentWrd.code[label][row].args.count())
            for (int i = currentWrd.code[label][row].arg_types.count(); i < currentWrd.code[label][row].args.count(); i++)
                currentWrd.code[label][row].arg_types.append(0);
    }

    reloadCodeList();

    unsavedChanges = true;
}

void MainWindow::on_tableWidget_Strings_itemChanged(QTableWidgetItem *item)
{
    if (ui->tableWidget_Strings->column(item) == 0)
        return;

    QString str = item->text();
    str.replace("\\n", "\n");
    currentWrd.strings[ui->tableWidget_Strings->row(item)] = str;
    unsavedChanges = true;
}

void MainWindow::on_tableWidget_Flags_itemChanged(QTableWidgetItem *item)
{
    if (ui->tableWidget_Flags->column(item) == 0)
        return;

    QString flag = item->text();
    currentWrd.flags[ui->tableWidget_Flags->row(item)] = flag;

    // We need to update the argument name previews now
    reloadCodeList();
    unsavedChanges = true;
}



void MainWindow::on_toolButton_CmdAdd_clicked()
{
    const int currentLabel = ui->comboBox_SelectLabel->currentIndex();
    const int currentRow = ui->tableWidget_Code->currentRow();
    if (currentRow < -1 || currentRow >= currentWrd.code[currentLabel].count())
        return;

    WrdCmd cmd;
    cmd.opcode = 0x47;
    currentWrd.code[currentLabel].insert(currentRow, cmd);

    reloadCodeList();
    ui->tableWidget_Code->selectRow(currentRow);
    unsavedChanges = true;
}

void MainWindow::on_toolButton_CmdDel_clicked()
{
    const int currentLabel = ui->comboBox_SelectLabel->currentIndex();
    const int currentRow = ui->tableWidget_Code->currentRow();
    if (currentRow < 0 || currentRow >= currentWrd.code[currentLabel].count())
        return;

    currentWrd.code[currentLabel].removeAt(currentRow);

    reloadCodeList();
    if (currentRow >= currentWrd.code[currentLabel].count())
        ui->tableWidget_Code->selectRow(currentRow - 1);
    else
        ui->tableWidget_Code->selectRow(currentRow);

    unsavedChanges = true;
}

void MainWindow::on_toolButton_CmdUp_clicked()
{
    const int currentLabel = ui->comboBox_SelectLabel->currentIndex();
    const int currentRow = ui->tableWidget_Code->currentRow();
    if (currentRow < 1 || currentRow >= currentWrd.code[currentLabel].count())
        return;

    const WrdCmd cmd1 = currentWrd.code[currentLabel][currentRow];
    const WrdCmd cmd2 = currentWrd.code[currentLabel][currentRow - 1];

    currentWrd.code[currentLabel].replace(currentRow, cmd2);
    currentWrd.code[currentLabel].replace(currentRow - 1, cmd1);

    reloadCodeList();
    ui->tableWidget_Code->selectRow(currentRow - 1);
    unsavedChanges = true;
}

void MainWindow::on_toolButton_CmdDown_clicked()
{
    const int currentLabel = ui->comboBox_SelectLabel->currentIndex();
    const int currentRow = ui->tableWidget_Code->currentRow();
    if (currentRow < 0 || currentRow + 1 >= currentWrd.code[currentLabel].count())
        return;

    const WrdCmd cmd1 = currentWrd.code[currentLabel][currentRow];
    const WrdCmd cmd2 = currentWrd.code[currentLabel][currentRow + 1];

    currentWrd.code[currentLabel].replace(currentRow, cmd2);
    currentWrd.code[currentLabel].replace(currentRow + 1, cmd1);

    reloadCodeList();
    ui->tableWidget_Code->selectRow(currentRow + 1);
    unsavedChanges = true;
}



void MainWindow::on_toolButton_StringAdd_clicked()
{
    const int currentRow = ui->tableWidget_Strings->currentRow();
    if (currentRow < -1 || currentRow >= currentWrd.strings.count())
        return;

    currentWrd.strings.insert(currentRow, QString());

    reloadStringList();
    ui->tableWidget_Strings->selectRow(currentRow);
    unsavedChanges = true;
}

void MainWindow::on_toolButton_StringDel_clicked()
{
    const int currentRow = ui->tableWidget_Strings->currentRow();
    if (currentRow < 0 || currentRow >= currentWrd.strings.count())
        return;

    currentWrd.strings.removeAt(currentRow);

    reloadStringList();
    if (currentRow >= currentWrd.strings.count())
        ui->tableWidget_Strings->selectRow(currentRow - 1);
    else
        ui->tableWidget_Strings->selectRow(currentRow);
    unsavedChanges = true;
}

void MainWindow::on_toolButton_StringUp_clicked()
{
    const int currentRow = ui->tableWidget_Strings->currentRow();
    if (currentRow < 1 || currentRow >= currentWrd.strings.count())
        return;

    const QString str1 = currentWrd.strings[currentRow];
    const QString str2 = currentWrd.strings[currentRow - 1];

    currentWrd.strings.replace(currentRow, str2);
    currentWrd.strings.replace(currentRow - 1, str1);

    reloadStringList();
    ui->tableWidget_Strings->selectRow(currentRow - 1);
    unsavedChanges = true;
}

void MainWindow::on_toolButton_StringDown_clicked()
{
    const int currentRow = ui->tableWidget_Strings->currentRow();
    if (currentRow < 0 || currentRow + 1 >= currentWrd.strings.count())
        return;

    const QString str1 = currentWrd.strings[currentRow];
    const QString str2 = currentWrd.strings[currentRow + 1];

    currentWrd.strings.replace(currentRow, str2);
    currentWrd.strings.replace(currentRow + 1, str1);

    reloadStringList();
    ui->tableWidget_Strings->selectRow(currentRow + 1);
    unsavedChanges = true;
}



void MainWindow::on_toolButton_FlagAdd_clicked()
{
    const int currentRow = ui->tableWidget_Flags->currentRow();
    if (currentRow < -1 || currentRow >= currentWrd.flags.count())
        return;

    currentWrd.flags.insert(currentRow, QString());

    reloadFlagList();
    ui->tableWidget_Flags->selectRow(currentRow);

    // We need to update the argument name previews now
    reloadCodeList();
    unsavedChanges = true;
}

void MainWindow::on_toolButton_FlagDel_clicked()
{
    const int currentRow = ui->tableWidget_Flags->currentRow();
    if (currentRow < 0 || currentRow >= currentWrd.flags.count())
        return;

    currentWrd.flags.removeAt(currentRow);

    reloadFlagList();
    if (currentRow >= currentWrd.flags.count())
        ui->tableWidget_Flags->selectRow(currentRow - 1);
    else
        ui->tableWidget_Flags->selectRow(currentRow);

    // We need to update the argument name previews now
    reloadCodeList();
    unsavedChanges = true;
}

void MainWindow::on_toolButton_FlagUp_clicked()
{
    const int currentRow = ui->tableWidget_Flags->currentRow();
    if (currentRow < 1 || currentRow >= currentWrd.flags.count())
        return;

    const QString flag1 = currentWrd.flags[currentRow];
    const QString flag2 = currentWrd.flags[currentRow - 1];

    currentWrd.flags.replace(currentRow, flag2);
    currentWrd.flags.replace(currentRow - 1, flag1);

    reloadFlagList();
    ui->tableWidget_Flags->selectRow(currentRow - 1);

    // We need to update the argument name previews now
    reloadCodeList();
    unsavedChanges = true;
}

void MainWindow::on_toolButton_FlagDown_clicked()
{
    const int currentRow = ui->tableWidget_Flags->currentRow();
    if (currentRow < 0 || currentRow + 1 >= currentWrd.flags.count())
        return;

    const QString flag1 = currentWrd.flags[currentRow];
    const QString flag2 = currentWrd.flags[currentRow + 1];

    currentWrd.flags.replace(currentRow, flag2);
    currentWrd.flags.replace(currentRow + 1, flag1);

    reloadFlagList();
    ui->tableWidget_Flags->selectRow(currentRow + 1);

    // We need to update the argument name previews now
    reloadCodeList();
    unsavedChanges = true;
}
