#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onCommandEntered(const QString &command);

private:
    Ui::MainWindow *ui;
    void executeCommand(const QString &command);
    void runLsCommand();
    void runPwdCommand();
    void runEchoCommand(const QString &command);
};

#endif // MAINWINDOW_H
