#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QtDebug>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QFileInfo>
#include <QTreeWidgetItem>
#include <QUuid>
#include <QStandardPaths>
#include <QProcess>
#include <QDir>
#include <QDateTime>
#include <QApplication>

constexpr auto EditorRole = Qt::UserRole + 100;
constexpr auto EditingRole = EditorRole + 1;
constexpr auto ItemProperty = "PairItem";
#define PrintLog(log) ui->browser_log->append(QDateTime::currentDateTime().toString("[hh:mm:ss]\n") + log + "\n");

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    initUI();
    initSlot();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::addFile(const QString &filePath)
{
    if(m_itemMap.contains(filePath)) {
        return;
    }

    const QFileInfo fileInfo(filePath);
    auto *item = new QTreeWidgetItem({fileInfo.fileName()});
    item->setData(0, Qt::UserRole, fileInfo.absoluteFilePath());
    ui->tree_files->addTopLevelItem(item);

    m_itemMap.insert(filePath, item);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    const auto fileList = event->mimeData()->urls();
    bool allFileHKX = true;
    for (const auto& url : fileList) {
        if(!url.isLocalFile() || QFileInfo(url.fileName()).suffix().compare("hkx", Qt::CaseInsensitive) != 0) {
            allFileHKX = false;
            break;
        }
    }

    if(allFileHKX) {
        event->accept();
    }


    QMainWindow::dragEnterEvent(event);
}

void MainWindow::dropEvent(QDropEvent *event)
{
    const auto urls = event->mimeData()->urls();
    for (const auto& url : urls) {
        addFile(url.toLocalFile());
    }

    QMainWindow::dropEvent(event);
}

void MainWindow::initUI()
{
    ui->splitter_h->setStretchFactor(0, 0);
    ui->splitter_h->setStretchFactor(1, 1);

    ui->splitter_v->setStretchFactor(0, 3);
    ui->splitter_v->setStretchFactor(1, 1);
}

void MainWindow::initSlot()
{
    connect(ui->tree_files, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem * item, int) {
        auto editor = item->data(0, EditorRole).value<QPointer<QTextEdit>>();
        if(editor.isNull()) {
            const auto filePath = item->data(0, Qt::UserRole).toString();
            if(!QFile::exists(filePath)) {
                return;
            }

            editor = openFile(filePath);
            editor->setProperty(ItemProperty, QVariant::fromValue(item));
            ui->tabWidget->addTab(editor, item->data(0, Qt::DisplayRole).toString());
            item->setData(0, EditorRole, QVariant::fromValue(editor));
        }
        ui->tabWidget->setCurrentWidget(editor);
    });

    connect(ui->tree_files, &QTreeWidget::currentItemChanged, this, [this](QTreeWidgetItem * current, QTreeWidgetItem*) {
        auto editor = current->data(0, EditorRole).value<QPointer<QTextEdit>>();
        if(!editor.isNull()) {
            ui->tabWidget->setCurrentWidget(editor);
        }
    });

    connect(ui->tabWidget, &QTabWidget::tabCloseRequested, this, [this](int index) {
        ui->tabWidget->widget(index)->deleteLater();
    });

    connect(ui->tabWidget, &QTabWidget::currentChanged, this, [this](int index) {
        auto widget = ui->tabWidget->widget(index);
        if(widget == nullptr) {
            return;
        }

        auto *item = widget->property(ItemProperty).value<QTreeWidgetItem*>();
        if(item != nullptr) {
            item->treeWidget()->setCurrentItem(item);
        }
    });

    connect(ui->actionSave, &QAction::triggered, this, [this]() {
        saveCurrent();
    });

    connect(ui->btn_clear, &QPushButton::clicked, this, [this]() {
        ui->browser_log->clear();
    });
}

QTextEdit *MainWindow::openFile(const QString &filePath)
{
    // setup editor
    auto *editor = new QTextEdit();

    // extract anno
    const QString execPath = QDir(qApp->applicationDirPath()).filePath("hkanno64");
    const QString tempFile = QDir(execPath).filePath(QUuid::createUuid().toString(QUuid::WithoutBraces));
    const QString execFile = QDir(execPath).filePath("hkanno64.exe");

    auto *process = new QProcess();
    process->setWorkingDirectory(execPath);
    process->setProgram(execFile);
    process->setArguments({"dump", "-o", tempFile, QDir::toNativeSeparators(filePath)});
    process->start();
    connect(process, &QProcess::readyReadStandardOutput, this, [ = ]() {
        PrintLog(process->readAllStandardOutput());
    });
    connect(process, &QProcess::readyReadStandardError, this, [ = ]() {
        PrintLog(process->readAllStandardError());
    });
    connect(process, &QProcess::finished, this, [ = ]() {
        QFile file(tempFile);
        if(file.open(QIODevice::ReadOnly)) {
            editor->setText(file.readAll());
        }
        file.close();

        connect(editor, &QTextEdit::textChanged, this, [ = ]() {
            editor->setWindowModified(true);
            int index = ui->tabWidget->indexOf(editor);
            ui->tabWidget->setTabIcon(index, QIcon(":/res/editing.svg"));
            auto *item = editor->property(ItemProperty).value<QTreeWidgetItem*>();
            item->setIcon(0, QIcon(":/res/editing.svg"));
        });

        QFile::remove(tempFile);
        process->deleteLater();
    });

    return editor;
}

bool MainWindow::saveFile(const QString &content, const QString &filePath)
{
    const QString execPath = QDir(qApp->applicationDirPath()).filePath("hkanno64");
    const QString execFile = QDir(execPath).filePath("hkanno64.exe");
    const QString tempFile = QDir(execPath).filePath(QUuid::createUuid().toString(QUuid::WithoutBraces));
    QFile file(tempFile);
    if(!file.open(QIODevice::WriteOnly)) {
        PrintLog(tr("Save File Fail:").arg(file.errorString()));
        return false;
    }
    file.write(content.toUtf8());
    file.close();

    QProcess process;
    process.setWorkingDirectory(execPath);
    process.setProgram(execFile);
    process.setArguments({"update", "-i", tempFile, QDir::toNativeSeparators(filePath)});
    process.start();
    process.waitForFinished();

    QFile::remove(tempFile);
    if(process.exitStatus() != QProcess::NormalExit) {
        return false;
    }

    const QString error = process.readAllStandardError();
    const QString output = process.readAllStandardOutput();
    PrintLog(output);

    if(!error.isEmpty()) {
        PrintLog(error);
        return false;
    }

    return true;
}

void MainWindow::saveCurrent()
{
    auto *currentEditor = qobject_cast<QTextEdit*>(ui->tabWidget->currentWidget());
    if(currentEditor == nullptr || !currentEditor->isWindowModified()) {
        return;
    }

    auto *item = currentEditor->property(ItemProperty).value<QTreeWidgetItem*>();
    if(item == nullptr) {
        return;
    }
    const auto filePath = item->data(0, Qt::UserRole).toString();
    const auto content = currentEditor->toPlainText();
    bool result = saveFile(content, filePath);
    if(result) {
        const int index = ui->tabWidget->indexOf(currentEditor);
        ui->tabWidget->setTabIcon(index, QIcon());
        item->setIcon(0, QIcon());
        currentEditor->setWindowModified(false);
        PrintLog(tr("%1 saved").arg(filePath));
    }
}

