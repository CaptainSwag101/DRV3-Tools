#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    textBoxFrame = new QFrame(ui->scrollArea);
    ui->scrollArea->setSizeAdjustPolicy(QScrollArea::AdjustToContents);
    ui->scrollArea->setLayout(new QVBoxLayout());
    ui->scrollArea->layout()->addWidget(textBoxFrame);
    textBoxFrame->setLayout(new QVBoxLayout());

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
    QStringList strings = get_stx_strings(currentStx);

    // Repopulate the text edit boxes
    for (QPlainTextEdit *old : textBoxFrame->findChildren<QPlainTextEdit *>(QString(), Qt::FindDirectChildrenOnly))
    {
        disconnect(old, &QPlainTextEdit::textChanged, this, &MainWindow::on_textBox_textChanged);
        textBoxFrame->layout()->removeWidget(old);
        delete old;
    }

    for (QString str : strings)
    {
        QPlainTextEdit *tb = new QPlainTextEdit(str);
        tb->setFixedHeight(tb->fontMetrics().height() * 3);
        connect(tb, &QPlainTextEdit::textChanged, this, &MainWindow::on_textBox_textChanged);

        textBoxFrame->layout()->addWidget(tb);
    }

    //textBoxFrame->layout()->setSizeConstraint(QLayout::SetMinAndMaxSize);
    ui->scrollArea->setWidget(textBoxFrame);
    ui->scrollArea->setWidgetResizable(true);
    ui->scrollArea->adjustSize();

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
    QHash<int, QString> stringMap;
    int index = 0;
    int table_len = 0;
    for (QPlainTextEdit *tb : textBoxFrame->findChildren<QPlainTextEdit *>(QString(), Qt::FindDirectChildrenOnly))
    {
        stringMap[index] = tb->document()->toRawText();
        index++;
        table_len++;
    }

    currentStx = repack_stx_strings(table_len, stringMap);
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
