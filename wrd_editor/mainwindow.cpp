#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <cmath>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->tableWidget_Code->setColumnWidth(1, 160);
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

    const QByteArray out_data = wrd_to_data(currentWrd);
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
    currentWrd = wrd_from_data(f.readAll(), filepath);
    f.close();

    this->setWindowTitle("WRD Editor: " + QFileInfo(filepath).fileName());

    ui->tableWidget_Code->setEnabled(true);
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

void MainWindow::reloadCodeList(const int index)
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

        QString opString = QString::number(cmd.opcode, 16).toUpper().rightJustified(4, '0');
        ui->tableWidget_Code->setItem(i, 0, new QTableWidgetItem(opString));

        QString argString;
        for (ushort arg : cmd.args)
        {
            const QString part = QString::number(arg, 16).toUpper().rightJustified(4, '0');
            argString += part;
        }
        ui->tableWidget_Code->setItem(i, 1, new QTableWidgetItem(argString));
    }

    updateArgumentPreviews();

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

// TODO: Include special cases for various opcodes once we know what they do.
void MainWindow::updateArgumentPreviews()
{
    const int currentLabel = ui->comboBox_SelectLabel->currentIndex();
    if (currentLabel < 0 || currentLabel >= currentWrd.labels.count())
        return;

    const QList<WrdCmd> cur_label_cmds = currentWrd.code.at(currentLabel);
    for (int i = 0; i < cur_label_cmds.count(); i++)
    {
        const WrdCmd cmd = cur_label_cmds.at(i);

        // FIXME: There's probably a better way to print
        // the individual opcode names while not duplicating
        // the argument-printing code.
        QString argNames;
        switch (cmd.opcode)
        {
        case LABEL_INDEX:   // args: label_num
            argNames += "LABEL_INDEX: ";
            argNames += QString::number(cmd.args[0], 10);
            break;


        case SPEAKER_ID:    // args: speaker_name
            argNames += "SPEAKER_ID: ";
            argNames += currentWrd.flags[cmd.args[0]];
            break;
        case GOTO:          // args: label_name
            argNames += "GOTO: ";
            argNames += currentWrd.flags[cmd.args[0]];
            break;


        case SET_FLAG:      // args: state, flag
            argNames += "SET_FLAG: ";
            for (const ushort arg : cmd.args)
                if (arg < currentWrd.flags.count())
                    argNames += currentWrd.flags[arg] + " ";
                else
                    argNames += QString::number(arg) + " ";
            break;
        case SET_FLAG_2:    // args: flag, state
            argNames += "SET_FLAG_2: ";
            for (const ushort arg : cmd.args)
                if (arg < currentWrd.flags.count())
                    argNames += currentWrd.flags[arg] + " ";
                else
                    argNames += QString::number(arg) + " ";
            break;
        case GOTO_EXTERN:   // args: file, label_name
            argNames += "GOTO_EXTERN: ";
            for (const ushort arg : cmd.args)
                if (arg < currentWrd.flags.count())
                    argNames += currentWrd.flags[arg] + " ";
                else
                    argNames += QString::number(arg) + " ";
            break;
        case SUB_CALL:      // args: file, label_name
            argNames += "SUB_CALL: ";
            for (const ushort arg : cmd.args)
                if (arg < currentWrd.flags.count())
                    argNames += currentWrd.flags[arg] + " ";
                else
                    argNames += QString::number(arg) + " ";
            break;
        case PLAY_VOICE:    // args: file, volume
            argNames += "PLAY_VOICE: ";
            for (const ushort arg : cmd.args)
                if (arg < currentWrd.flags.count())
                    argNames += currentWrd.flags[arg] + " ";
                else
                    argNames += QString::number(arg) + " ";
            break;
        case PLAY_SFX:      // args: file, volume
            argNames += "PLAY_SFX: ";
            for (const ushort arg : cmd.args)
                if (arg < currentWrd.flags.count())
                    argNames += currentWrd.flags[arg] + " ";
                else
                    argNames += QString::number(arg) + " ";
            break;
        case PLAY_JINGLE:   // args: file, volume
            argNames += "PLAY_JINGLE: ";
            for (const ushort arg : cmd.args)
                if (arg < currentWrd.flags.count())
                    argNames += currentWrd.flags[arg] + " ";
                else
                    argNames += QString::number(arg) + " ";
            break;


        case PLAY_BGM:      // args: file, volume, fade
            argNames += "PLAY_BGM: ";
            for (const ushort arg : cmd.args)
                if (arg < currentWrd.flags.count())
                    argNames += currentWrd.flags[arg] + " ";
                else
                    argNames += QString::number(arg) + " ";
            break;
        case MAP_PARAM:     // args: param, value?, state
            argNames += "MAP_PARAM: ";
            for (const ushort arg : cmd.args)
                if (arg < currentWrd.flags.count())
                    argNames += currentWrd.flags[arg] + " ";
                else
                    argNames += QString::number(arg) + " ";
            break;
        case OBJ_PARAM:     // args: obj, param1?, param2?
            argNames += "OBJ_PARAM: ";
            for (const ushort arg : cmd.args)
                if (arg < currentWrd.flags.count())
                    argNames += currentWrd.flags[arg] + " ";
                else
                    argNames += QString::number(arg) + " ";
            break;
        case SET_VALUE:     // args: out_value, math_operation, in_value (example: "wkWAIT = 10")
            argNames += "SET_VALUE: ";
            for (const ushort arg : cmd.args)
                if (arg < currentWrd.flags.count())
                    argNames += currentWrd.flags[arg] + " ";
                else
                    argNames += QString::number(arg) + " ";
            break;
        case CHECK_VALUE:   // args: value, operator, compare_to
            argNames += "CHECK_VALUE: ";
            argNames += currentWrd.flags[cmd.args[0]] + " ";
            argNames += currentWrd.flags[cmd.args[1]] + " ";
            argNames += QString::number(cmd.args[2]);
            break;
        case LOAD_MAP:      // args: file, start_pos, mode
            argNames += "LOAD_MAP: ";
            for (const ushort arg : cmd.args)
                if (arg < currentWrd.flags.count())
                    argNames += currentWrd.flags[arg] + " ";
                else
                    argNames += QString::number(arg) + " ";
            break;
        case LOAD_OBJ:      // args: param, obj, state, unk1, unk2
            argNames += "LOAD_OBJ: ";
            for (const ushort arg : cmd.args)
                if (arg < currentWrd.flags.count())
                    argNames += currentWrd.flags[arg] + " ";
                else
                    argNames += QString::number(arg) + " ";
            break;
        case SCENE_TRANS:   // args: transition, color, speed
            argNames += "SCENE_TRANS: ";
            for (const ushort arg : cmd.args)
                if (arg < currentWrd.flags.count())
                    argNames += currentWrd.flags[arg] + " ";
                else
                    argNames += QString::number(arg) + " ";
            break;
        case CAMERA_SHAKE:  // args: camera, strength, speed
            argNames += "CAMERA_SHAKE: ";
            for (const ushort arg : cmd.args)
                if (arg < currentWrd.flags.count())
                    argNames += currentWrd.flags[arg] + " ";
                else
                    argNames += QString::number(arg) + " ";
            break;


        case SET_MODIFIER:  // args: modifier, param1, param2, param3
            argNames += "SET_MODIFIER: ";
            for (const ushort arg : cmd.args)
                if (arg < currentWrd.flags.count())
                    argNames += currentWrd.flags[arg] + " ";
                else
                    argNames += QString::number(arg) + " ";
            break;
        case CAMERA_TRANS:  // args: transition, param1?, param2?, param3?
            argNames += "CAMERA_TRANS: ";
            argNames += currentWrd.flags[cmd.args[0]] + " ";
            argNames += QString::number(cmd.args[1]) + " ";
            argNames += QString::number(cmd.args[2]) + " ";
            argNames += QString::number(cmd.args[3]);
            break;
        case EVENT_SCENE:   // args: file, anim, priority, camera
            argNames += "EVENT_SCENE: ";
            for (const ushort arg : cmd.args)
                if (arg < currentWrd.flags.count())
                    argNames += currentWrd.flags[arg] + " ";
                else
                    argNames += QString::number(arg) + " ";
            break;


        case CAMERA_MOVE:   // args: camera, view_pos, chara, anim, transition
            argNames += "CAMERA_MOVE: ";
            for (const ushort arg : cmd.args)
                if (arg < currentWrd.flags.count())
                    argNames += currentWrd.flags[arg] + " ";
                else
                    argNames += QString::number(arg) + " ";
            break;
        case CHARA_PARAM:   // args: draw_mode, chara, anim, pos, mode
            argNames += "CHARA_PARAM: ";
            for (const ushort arg : cmd.args)
                if (arg < currentWrd.flags.count())
                    argNames += currentWrd.flags[arg] + " ";
                else
                    argNames += QString::number(arg) + " ";
            break;
        case CHARA_SHAKE:   // args: chara, strength, speed1, speed2, speed3
            argNames += "CHARA_SHAKE: ";
            for (const ushort arg : cmd.args)
                if (arg < currentWrd.flags.count())
                    argNames += currentWrd.flags[arg] + " ";
                else
                    argNames += QString::number(arg) + " ";
            break;


        case DISPLAY_TEXT:  // args: string_num
            argNames += "DISPLAY_TEXT: ";
            argNames += currentWrd.strings[cmd.args[0]];
            break;


        case WAIT_FOR_INPUT:// No args
            argNames += "WAIT_FOR_INPUT";
            break;
        case SCRIPT_END:
            argNames += "SCRIPT_END";
            break;
        case SUB_RETURN:
            argNames += "SUB_RETURN";
            break;


        default:
            argNames += "UNKNOWN_CMD: ";
            for (const ushort arg : cmd.args)
                if (arg < currentWrd.flags.count())
                    argNames += currentWrd.flags[arg] + " ";
                else
                    argNames += QString::number(arg) + " ";
            break;
        }

        QTableWidgetItem *names = new QTableWidgetItem(argNames.simplified());
        names->setFlags(names->flags() & ~Qt::ItemIsEditable);
        ui->tableWidget_Code->setItem(i, 2, names);
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
    const int currentLabel = ui->comboBox_SelectLabel->currentIndex();

    // Opcode
    if (column == 0)
    {
        const QString opText = item->text();
        if (opText.length() != 4)
        {
            QMessageBox errorMsg(QMessageBox::Warning, "Hex Conversion Error", "Opcode length must be exactly 4 hexadecimal digits (2 bytes).", QMessageBox::Ok, this);
            errorMsg.exec();
            ui->tableWidget_Code->editItem(item);
            return;
        }

        bool ok;
        const ushort val = opText.toUShort(&ok, 16);
        if (!ok)
        {
            QMessageBox errorMsg(QMessageBox::Warning, "Hex Conversion Error", "Invalid hexadecimal value.", QMessageBox::Ok, this);
            errorMsg.exec();
            ui->tableWidget_Code->editItem(item);
            return;
        }

        currentWrd.code[currentLabel][row].opcode = val;
    }
    // Args
    else if (column == 1)
    {
        const QString argText = item->text();
        QTableWidgetItem *argNames = ui->tableWidget_Code->item(row, 2);
        argNames->setText("");

        currentWrd.code[currentLabel][row].args.clear();
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

            currentWrd.code[currentLabel][row].args.append(val);

            // One-argument commands usually are dealing with a raw number instead of a named flag
            if (val < currentWrd.flags.count() && (argText.count() / 4) > 1)
                argNames->setText(argNames->text() + currentWrd.flags[val] + " ");
            else
                argNames->setText(argNames->text() + splitText + " ");
            argNames->setText(argNames->text().simplified());
        }
    }

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
    cmd.opcode = 0x7000;
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
