#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "ui_mainwindow.h"

class Shell;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void handleCommand();

private:
    void loadHistory();
    void saveHistory();

    Ui::MainWindow *ui;
    Shell *shell;
    QStringList commandHistory;
    int historyIndex;
};

#endif // MAINWINDOW_H
