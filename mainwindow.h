#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextEdit>
#include <QLineEdit>
#include <QStringList>

class Shell;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void handleCommand();

private:
    void loadHistory();
    void saveHistory();

    QTextEdit *outputTextEdit;
    QLineEdit *inputLineEdit;
    Shell *shell;
    QStringList commandHistory;
    int historyIndex;
};

#endif // MAINWINDOW_H
