#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "terminaltextedit.h"
#include <QTextCursor>
#include <memory>
#include <array>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Заменяем QTextEdit на наш TerminalTextEdit
    TerminalTextEdit *terminal = new TerminalTextEdit(this);
    setCentralWidget(terminal);

    // Начальная строка для ввода
    terminal->append(">>> ");

    // Соединяем сигнал команды с обработчиком
    connect(terminal, &TerminalTextEdit::commandEntered, this, &MainWindow::onCommandEntered);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onCommandEntered(const QString &command)
{
    executeCommand(command);
}

void MainWindow::executeCommand(const QString &command)
{
    std::string cmd = command.toStdString();

    TerminalTextEdit *terminal = qobject_cast<TerminalTextEdit *>(centralWidget());

    if (cmd == "ls") {
        runLsCommand();
    } else if (cmd == "pwd") {
        runPwdCommand();
    } else if (cmd.substr(0, 4) == "echo") {
        runEchoCommand(command);
    } else {
        terminal->append("Unknown command: " + command);
    }
}

void MainWindow::runLsCommand()
{
    TerminalTextEdit *terminal = qobject_cast<TerminalTextEdit *>(centralWidget());

    std::array<char, 128> buffer;
    std::string result;
    std::shared_ptr<FILE> pipe(popen("ls", "r"), pclose);

    if (!pipe) {
        terminal->append("Failed to run command 'ls'");
        return;
    }

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }

    terminal->append(QString::fromStdString(result));
}

void MainWindow::runPwdCommand()
{
    TerminalTextEdit *terminal = qobject_cast<TerminalTextEdit *>(centralWidget());

    std::array<char, 128> buffer;
    std::string result;
    std::shared_ptr<FILE> pipe(popen("pwd", "r"), pclose);

    if (!pipe) {
        terminal->append("Failed to run command 'pwd'");
        return;
    }

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }

    terminal->append(QString::fromStdString(result));
}

void MainWindow::runEchoCommand(const QString &command)
{
    TerminalTextEdit *terminal = qobject_cast<TerminalTextEdit *>(centralWidget());

    QStringList parts = command.split(" ");
    parts.removeAt(0);
    QString output = parts.join(" ");

    terminal->append(output);
}
