#include "mainwindow.h"
#include "shell.h"
#include <QVBoxLayout>
#include <QFile>
#include <QTextStream>
#include <QKeyEvent>
#include <QDir>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), historyIndex(-1) {
    outputTextEdit = new QTextEdit(this);
    outputTextEdit->setReadOnly(true);
    inputLineEdit = new QLineEdit(this);
    inputLineEdit->installEventFilter(this);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(outputTextEdit);
    layout->addWidget(inputLineEdit);

    QWidget *centralWidget = new QWidget(this);
    centralWidget->setLayout(layout);
    setCentralWidget(centralWidget);

    shell = new Shell(outputTextEdit, this);
    connect(inputLineEdit, &QLineEdit::returnPressed, this, &MainWindow::handleCommand);

    loadHistory();
}

MainWindow::~MainWindow() {
    saveHistory();
    delete shell;
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    if (obj == inputLineEdit && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Up) {
            if (!commandHistory.isEmpty()) {
                if (historyIndex < commandHistory.size() - 1) {
                    historyIndex++;
                }
                inputLineEdit->setText(commandHistory[commandHistory.size() - 1 - historyIndex]);
            }
            return true;
        } else if (keyEvent->key() == Qt::Key_Down) {
            if (historyIndex > 0) {
                historyIndex--;
                inputLineEdit->setText(commandHistory[commandHistory.size() - 1 - historyIndex]);
            } else if (historyIndex == 0) {
                historyIndex = -1;
                inputLineEdit->clear();
            }
            return true;
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::handleCommand() {
    QString command = inputLineEdit->text().trimmed();
    if (command.isEmpty()) return;

    if (commandHistory.isEmpty() || commandHistory.last() != command) {
        commandHistory.append(command);
    }
    historyIndex = -1;

    inputLineEdit->clear();
    outputTextEdit->append("$ " + command);
    shell->processCommand(command);

    if (commandHistory.size() > 100) {
        commandHistory.removeFirst();
    }
}

void MainWindow::loadHistory() {
    QFile file(QDir::homePath() + "/.shmel_history");
    if (file.open(QIODevice::ReadOnly)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            commandHistory << in.readLine();
        }
        file.close();
    }
}

void MainWindow::saveHistory() {
    QFile file(QDir::homePath() + "/.shmel_history");
    if (file.open(QIODevice::WriteOnly)) {
        QTextStream out(&file);
        for (const QString &cmd : commandHistory) {
            out << cmd << "\n";
        }
        file.close();
    }
}
