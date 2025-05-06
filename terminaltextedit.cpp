#include "terminaltextedit.h"
#include <QKeyEvent>

TerminalTextEdit::TerminalTextEdit(QWidget *parent)
    : QTextEdit(parent)
{
}

void TerminalTextEdit::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        QTextCursor cursor = this->textCursor();
        cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);
        QString line = cursor.selectedText();

        QString command = line;
        if (command.startsWith(">>> ")) {
            command = command.mid(4);
        }

        emit commandEntered(command);

        QTextEdit::keyPressEvent(event);

        this->append(">>> ");
    } else {
        QTextEdit::keyPressEvent(event);
    }
}
