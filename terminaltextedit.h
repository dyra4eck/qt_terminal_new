#ifndef TERMINALTEXTEDIT_H
#define TERMINALTEXTEDIT_H

#include <QTextEdit>
#include <QDir>

class TerminalTextEdit : public QTextEdit
{
    Q_OBJECT
public:
    explicit TerminalTextEdit(QWidget *parent = nullptr);
    QTextCharFormat getDefaultCharFormat() const;
    void insertPrompt();
    void appendOutput(const QString &text);
    void appendPrompt();
    void highlightCommand();
    QString lastPrompt;
    QString getPartialInput() const;

signals:
    void commandEntered(const QString &command);
    void interruptRequested();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void showCustomContextMenu(const QPoint &pos);

private:
    void scrollToBottom();
    QString getCurrentCommand() const;
    void loadHistory();
    void saveHistory();
    void createHistoryFileIfNeeded();
    QStringList findCompletions(const QString &prefix);
    void handleTabCompletion();
    bool isValidCommand(const QString &command);

    QStringList commandHistory;
    int historyIndex = 0;
    QString prompt = QDir::currentPath() +"$ ";
    const int MaxHistorySize = 100;
    const QString HistoryFileName = "history.txt";
    bool selectingText = false;
    QTextCharFormat defaultCharFormat;
};

#endif // TERMINALTEXTEDIT_H
