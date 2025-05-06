#ifndef TERMINALTEXTEDIT_H
#define TERMINALTEXTEDIT_H

#include <QTextEdit>

class TerminalTextEdit : public QTextEdit
{
    Q_OBJECT
public:
    explicit TerminalTextEdit(QWidget *parent = nullptr);

signals:
    void commandEntered(const QString &command);

protected:
    void keyPressEvent(QKeyEvent *event) override;
};

#endif // TERMINALTEXTEDIT_H
