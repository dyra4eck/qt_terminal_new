#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextEdit>
#include <QKeyEvent>
#include "ui_mainwindow.h"
#include "shell.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void handleCommand();

private:
    void updatePrompt();
    bool isCursorAtEditablePosition();

    Ui::MainWindow *ui;
    Shell *shell;
    QStringList commandHistory;
    int historyIndex;
    QString currentCommand;
};

#endif // MAINWINDOW_H
