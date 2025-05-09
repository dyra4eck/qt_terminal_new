#include "mainwindow.h"
#include "shell.h"
#include "ui_mainwindow.h"
#include <QFile>
#include <QTextStream>
#include <QKeyEvent>
#include <QDir>
#include <QScrollBar>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), historyIndex(-1) {
    ui->setupUi(this);

    // Настройка стилей
    ui->centralwidget->setStyleSheet("background-color: black;");
    ui->verticalLayout->setContentsMargins(0, 0, 0, 0);
    ui->verticalLayout->setSpacing(0);

    // Настройка вывода
    ui->outputTextEdit->setReadOnly(true);
    ui->outputTextEdit->setWordWrapMode(QTextOption::NoWrap);
    ui->outputTextEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    ui->outputTextEdit->setTextInteractionFlags(Qt::TextSelectableByKeyboard | Qt::TextSelectableByMouse);

    // Настройка ввода
    ui->inputLineEdit->setFocus();
    ui->inputLineEdit->setFrame(false);
    ui->inputLineEdit->installEventFilter(this);

    // Шрифт
    QFont font("Monospace", 10);
    ui->outputTextEdit->setFont(font);
    ui->inputLineEdit->setFont(font);

    // Инициализация оболочки
    shell = new Shell(ui->outputTextEdit, this);
    connect(ui->inputLineEdit, &QLineEdit::returnPressed, this, &MainWindow::handleCommand);

    // Загрузка истории
    loadHistory();

    // Приветственное сообщение
    ui->outputTextEdit->append(
        "<span style='color:#00ff00'>user@shmel:~$ </span>"
        "<span style='color:white'>Terminal ready</span>"
        );
}

MainWindow::~MainWindow() {
    saveHistory();
    delete shell;
    delete ui;
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    if (obj == ui->inputLineEdit && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Up) {
            if (!commandHistory.isEmpty()) {
                if (historyIndex < commandHistory.size() - 1) {
                    historyIndex++;
                }
                ui->inputLineEdit->setText(commandHistory[commandHistory.size() - 1 - historyIndex]);
            }
            return true;
        } else if (keyEvent->key() == Qt::Key_Down) {
            if (historyIndex > 0) {
                historyIndex--;
                ui->inputLineEdit->setText(commandHistory[commandHistory.size() - 1 - historyIndex]);
            } else if (historyIndex == 0) {
                historyIndex = -1;
                ui->inputLineEdit->clear();
            }
            return true;
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::handleCommand() {
    QString command = ui->inputLineEdit->text().trimmed();
    if (command.isEmpty()) return;

    // Добавление в историю
    if (commandHistory.isEmpty() || commandHistory.last() != command) {
        commandHistory.append(command);
    }
    historyIndex = -1;

    // Очистка и вывод команды
    ui->inputLineEdit->clear();
    ui->outputTextEdit->append(
        "<span style='color:#00ff00'>user@shmel:~$ </span>"
        "<span style='color:white'>" + command + "</span>"
        );

    // Обработка команды
    shell->processCommand(command);

    // Автопрокрутка
    QScrollBar *scrollBar = ui->outputTextEdit->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());

    // Ограничение истории
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
