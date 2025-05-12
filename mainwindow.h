#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <functional>
#include <unordered_map>
#include "QProcess"

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
    void handleInterrupt();

private:
    Ui::MainWindow *ui;
    QProcess *currentProcess = nullptr;
    void executeCommand(const QString &command);
    std::unordered_map<QString, std::function<void(const QString &)>> commandMap;
    void runLsCommand(const QString &);
    void runPwdCommand(const QString &);
    void runEchoCommand(const QString &);
    void printUnknownCommand(const QString &);
    void runCdCommand(const QStringList &arguments);
    void runClearCommand(const QString &);
};

#endif // MAINWINDOW_H
