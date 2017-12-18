#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //openStx.setViewMode(QFileDialog::Detail);
    openStx.setNameFilter("STX files (*.stx)");

    OpenStxFile();
}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::CheckUnsaved()
{
    if (unsavedChanges)
    {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Unsaved Changes",
                                      "There are unsaved changes to the strings, which will be lost if you continue. Are you sure?",
                                      QMessageBox::Yes|QMessageBox::No);

        return (reply == QMessageBox::Yes);
    }

    return true;
}

void MainWindow::OpenStxFile()
{
    if (CheckUnsaved())
    {
        openStx.setAcceptMode(QFileDialog::AcceptOpen);
        openStx.setFileMode(QFileDialog::ExistingFile);
        currentFilename = openStx.getOpenFileName(this, "Open STX file");
        QFile f(currentFilename);
        f.open(QFile::ReadOnly);
        currentStx = BinaryData(f.readAll());
        f.close();

        ReloadStrings();
    }
}

void MainWindow::SaveStxFile()
{
    unsavedChanges = false;

    QMap<int, QString> stringMap;
    int index = 0;
    int table_len = 0;
    for (QString str : strings)
    {
        stringMap[index] = str;

        index++;
        table_len++;
    }

    currentStx = repack_stx_strings(table_len, stringMap);
    QFile f(currentFilename);
    f.open(QFile::WriteOnly);
    f.write(currentStx.Bytes);
    f.close();
}

void MainWindow::SaveStxFileAs()
{
    openStx.setAcceptMode(QFileDialog::AcceptSave);
    openStx.setFileMode(QFileDialog::AnyFile);
    QString oldFilename = currentFilename;
    currentFilename = openStx.getOpenFileName(this, "Save STX file");
    if (openStx.result() == QFileDialog::Rejected)
    {
        currentFilename = oldFilename;
        return;
    }
    SaveStxFile();
}

void MainWindow::ReloadStrings()
{
    strings = get_stx_strings(currentStx);

    // Repopulate the text edit boxes
    for (auto widget : ui->scrollArea->widget()->layout()->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly))
    {
        delete widget;
    }

    QFrame *frame = new QFrame(ui->scrollArea);
    QVBoxLayout *layout = new QVBoxLayout();
    frame->setLayout(layout);
    ui->scrollArea->setWidgetResizable(true);

    for (QString str : strings)
    {
        QPlainTextEdit *edit = new QPlainTextEdit(str);
        //QRect geo = edit->geometry();
        //int h = edit->fontMetrics().height();
        //geo.setHeight((h * 2) + 4);
        //edit->setGeometry(geo);
        frame->layout()->addWidget(edit);
    }

    //frame->layout()->setSizeConstraint(QLayout::SetMinAndMaxSize);
    ui->scrollArea->setSizeAdjustPolicy(QScrollArea::AdjustToContents);
    ui->scrollArea->setWidget(frame);
    ui->scrollArea->adjustSize();
}

void MainWindow::on_actionOpen_triggered()
{
    OpenStxFile();
}

void MainWindow::on_actionSave_triggered()
{
    SaveStxFile();
}

void MainWindow::on_actionSave_As_triggered()
{
    SaveStxFileAs();
}
