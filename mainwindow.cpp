#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "terminaltextedit.h"
#include <QTextCursor>
#include <QDir>
#include <QProcess>
#include <QInputDialog>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    TerminalTextEdit *terminal = new TerminalTextEdit(this);
    setCentralWidget(terminal);

    connect(terminal, &TerminalTextEdit::commandEntered, this, &MainWindow::onCommandEntered);
    connect(terminal, &TerminalTextEdit::interruptRequested, this, &MainWindow::handleInterrupt);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onCommandEntered(const QString &command)
{
    executeCommand(command);
}

void MainWindow::handleInterrupt()
{
    TerminalTextEdit *terminal = findChild<TerminalTextEdit *>();

    if (currentProcess && currentProcess->state() == QProcess::Running) {
        currentProcess->kill();
        terminal->appendOutput("^C\n");
        currentProcess = nullptr;
    }
    terminal->insertPrompt();
}

void MainWindow::executeCommand(const QString &command)
{
    TerminalTextEdit *terminal = qobject_cast<TerminalTextEdit *>(centralWidget());

    QTextCursor cursor = terminal->textCursor();
    cursor.movePosition(QTextCursor::PreviousBlock);
    cursor.movePosition(QTextCursor::StartOfBlock);
    cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    cursor.setCharFormat(terminal->getDefaultCharFormat());
    cursor.movePosition(QTextCursor::End);
    terminal->setTextCursor(cursor);

    QStringList parts = command.split(" ", Qt::SkipEmptyParts);
    if (parts.isEmpty()) {
        terminal->insertPrompt();
        return;
    }

    QString program = parts[0];
    QStringList arguments = parts.mid(1);


    if (program == "sudo") {
        arguments.prepend("-S");
        program = "sudo";
    }

    if (program == "cd") {
        runCdCommand(arguments);
        terminal->insertPrompt();
        return;
    }

    if (program == "clear") {
        terminal->clear();
        terminal->insertPrompt();
        return;
    }

    if (currentProcess) {
        terminal->appendOutput("Error: Another command is still running.\n");
        terminal->insertPrompt();
        return;
    }

    currentProcess = new QProcess(this);
    currentProcess->setProgram(program);
    currentProcess->setArguments(arguments);

    connect(currentProcess, &QProcess::readyReadStandardOutput, [=]() {
        QByteArray output = currentProcess->readAllStandardOutput();
        terminal->appendOutput(QString::fromLocal8Bit(output));
    });

    connect(currentProcess, &QProcess::readyReadStandardError, [=]() {
        QByteArray error = currentProcess->readAllStandardError();
        terminal->appendOutput(QString::fromLocal8Bit(error));
    });

    connect(currentProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), [=](int, QProcess::ExitStatus) {
        terminal->insertPrompt();
        currentProcess->deleteLater();
        currentProcess = nullptr;
    });

    currentProcess->start();

    if (program == "sudo") {
        if (!currentProcess->waitForStarted()) {
            terminal->append("Error: Couldn't start sudo\n");
            return;
        }
        QString password = QInputDialog::getText(
            this,
            tr("Sudo Password"),
            tr("Enter password:"),
            QLineEdit::Password
            );
        if (!password.isEmpty()) {
            password += "\n";
            currentProcess->write(password.toUtf8());
        }
    }

    if (!currentProcess->waitForStarted()) {
        terminal->appendOutput("Error: Could not start command: " + program + "\n");
        terminal->insertPrompt();
        currentProcess->deleteLater();
        currentProcess = nullptr;
    }
}

void MainWindow::runCdCommand(const QStringList &arguments)
{
    TerminalTextEdit *terminal = qobject_cast<TerminalTextEdit *>(centralWidget());

    QString path;
    if (arguments.isEmpty()) {
        path = QDir::homePath();
    } else {
        path = arguments[0];
        if (path == "~") {
            path = QDir::homePath();
        }
    }

    QDir dir;
    bool success = dir.cd(path);
    if (success) {
        QDir::setCurrent(dir.absolutePath());
        terminal->appendOutput("Changed directory to: " + dir.absolutePath() + "\n");
    } else {
        terminal->appendOutput("cd: no such directory: " + path + "\n");
    }
}
