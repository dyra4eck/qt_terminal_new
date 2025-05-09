#include "mainwindow.h"
#include "shell.h"
#include "ui_mainwindow.h"
#include <QScrollBar>
#include <QTextBlock>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), historyIndex(-1) {
    ui->setupUi(this);
    shell = new Shell(ui->terminalEdit, this);

    // Настройка терминала
    ui->terminalEdit->setReadOnly(false);
    ui->terminalEdit->installEventFilter(this);
    ui->terminalEdit->setUndoRedoEnabled(false);
    ui->terminalEdit->setStyleSheet("background-color: black; color: #00ff00;");

    // Первое приглашение
    updatePrompt();
}

MainWindow::~MainWindow() {
    delete ui;
    delete shell;
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Up || event->key() == Qt::Key_Down) {
        // Навигация по истории
        if (!commandHistory.isEmpty()) {
            if (event->key() == Qt::Key_Up && historyIndex < commandHistory.size() - 1) {
                historyIndex++;
            } else if (event->key() == Qt::Key_Down && historyIndex > 0) {
                historyIndex--;
            }
            ui->terminalEdit->textCursor().removeSelectedText();
            ui->terminalEdit->insertPlainText(commandHistory[historyIndex]);
        }
    }
    QMainWindow::keyPressEvent(event);
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    if (obj == ui->terminalEdit && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

        // Запрет редактирования вне последней строки
        if (!isCursorAtEditablePosition() && keyEvent->key() != Qt::Key_Up && keyEvent->key() != Qt::Key_Down) {
            return true;
        }

        // Обработка Enter
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            handleCommand();
            return true;
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::handleCommand() {
    QTextCursor cursor = ui->terminalEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.select(QTextCursor::LineUnderCursor);
    QString command = cursor.selectedText().mid(14).trimmed(); // Удаление приглашения "user@shmel:~$ "

    if (!command.isEmpty()) {
        commandHistory.append(command);
        historyIndex = -1;
        shell->processCommand(command);
    }
    updatePrompt();
}

void MainWindow::updatePrompt() {
    ui->terminalEdit->append("user@shmel:~$ ");
    QTextCursor cursor = ui->terminalEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    ui->terminalEdit->setTextCursor(cursor);
}

bool MainWindow::isCursorAtEditablePosition() {
    QTextCursor cursor = ui->terminalEdit->textCursor();
    int promptLength = 14; // "user@shmel:~$ "
    return (cursor.position() >= ui->terminalEdit->document()->lastBlock().position() + promptLength);
}
