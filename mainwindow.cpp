#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "terminaltextedit.h"
#include <QTextCursor>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    TerminalTextEdit *terminal = new TerminalTextEdit(this);
    setCentralWidget(terminal);

    m_process = new QProcess(this);
    m_process->setProcessChannelMode(QProcess::MergedChannels);

    // Настройка переменных окружения
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("PATH", env.value("PATH") + ":/bin:/usr/bin");
    m_process->setProcessEnvironment(env);

    connect(terminal, &TerminalTextEdit::commandEntered, this, &MainWindow::onCommandEntered);
    connect(m_process, &QProcess::readyReadStandardOutput, this, &MainWindow::onProcessOutput);
    connect(m_process, &QProcess::errorOccurred, this, &MainWindow::onProcessError);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &MainWindow::onProcessFinished);
}

MainWindow::~MainWindow() {
    delete ui;
    delete m_process;
}

void MainWindow::onCommandEntered(const QString &command) {
    if (command.trimmed().isEmpty()) return;

    // Обработка команды cd
    if (command.startsWith("cd ")) {
        QString newDir = command.section(' ', 1, -1).trimmed();
        QDir dir(newDir);
        if (dir.exists()) {
            QDir::setCurrent(newDir);
            TerminalTextEdit *terminal = qobject_cast<TerminalTextEdit*>(centralWidget());
            terminal->append("Current directory: " + QDir::currentPath());
        } else {
            TerminalTextEdit *terminal = qobject_cast<TerminalTextEdit*>(centralWidget());
            terminal->append("cd: no such directory: " + newDir);
        }
        return;
    }

    // Разделение команды на части
    QStringList parts = command.split(" ", Qt::SkipEmptyParts);
    if (parts.isEmpty()) return;

    // Поиск исполняемого файла
    QString program = parts[0];
    QString fullPath = QStandardPaths::findExecutable(program);
    if (fullPath.isEmpty()) {
        TerminalTextEdit *terminal = qobject_cast<TerminalTextEdit*>(centralWidget());
        terminal->append("Command not found: " + program);
        return;
    }

    // Запуск процесса
    m_process->setWorkingDirectory(QDir::currentPath());
    m_process->start(fullPath, parts.mid(1));
}

void MainWindow::onProcessOutput() {
    TerminalTextEdit *terminal = qobject_cast<TerminalTextEdit*>(centralWidget());
    terminal->append(m_process->readAllStandardOutput());
}

void MainWindow::onProcessError(QProcess::ProcessError error) {
    Q_UNUSED(error);
    TerminalTextEdit *terminal = qobject_cast<TerminalTextEdit*>(centralWidget());
    terminal->append("Error: " + m_process->errorString());
}

void MainWindow::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    Q_UNUSED(exitCode);
    Q_UNUSED(exitStatus);
    TerminalTextEdit *terminal = qobject_cast<TerminalTextEdit*>(centralWidget());
    terminal->append(">>> ");
}
