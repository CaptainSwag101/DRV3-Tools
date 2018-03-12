#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->listWidget, &QListWidget::currentTextChanged, this, &MainWindow::on_textBox_textChanged);

    //openStx.setViewMode(QFileDialog::Detail);
    openStx.setNameFilter("STX files (*.stx)");

    //OpenStxFile();
}

MainWindow::~MainWindow()
{
    delete ui;
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

void MainWindow::reloadStrings()
{
    ui->listWidget->clear();

    // TODO: Do these individually so we can make them editable
    QStringList strings = get_stx_strings(currentStx);
    ui->listWidget->addItems(strings);

    unsavedChanges = false;
}

void MainWindow::on_actionOpen_triggered()
{
    if (!confirmUnsaved()) return;

    openStx.setAcceptMode(QFileDialog::AcceptOpen);
    openStx.setFileMode(QFileDialog::ExistingFile);
    QString newFilename = openStx.getOpenFileName(this, "Open STX file", QString(), "STX files (*.stx)");
    if (newFilename.isEmpty()) return;
    currentFilename = newFilename;

    QFile f(currentFilename);
    f.open(QFile::ReadOnly);
    currentStx = f.readAll();
    f.close();

    reloadStrings();
}

void MainWindow::on_actionSave_triggered()
{
    QStringList strings;
    for(int i = 0; i < ui->listWidget->count(); i++)
    {
        QListWidgetItem *item = ui->listWidget->item(i);
        strings.append(item->text());
    }

    currentStx = repack_stx_strings(strings);
    QFile f(currentFilename);
    f.open(QFile::WriteOnly);
    f.write(currentStx);
    f.close();
    unsavedChanges = false;
}

void MainWindow::on_actionSaveAs_triggered()
{
    openStx.setAcceptMode(QFileDialog::AcceptSave);
    openStx.setFileMode(QFileDialog::AnyFile);
    QString newFilename = openStx.getSaveFileName(this, "Save STX file", QString(), "STX files (*.stx)");
    if (newFilename.isEmpty()) return;
    currentFilename = newFilename;

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

void MainWindow::on_textBox_textChanged()
{
    unsavedChanges = true;
}
