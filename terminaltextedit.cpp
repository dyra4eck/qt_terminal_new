#include "terminaltextedit.h"
#include <QKeyEvent>

TerminalTextEdit::TerminalTextEdit(QWidget *parent) : QTextEdit(parent) {}

void TerminalTextEdit::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::StartOfLine, QTextCursor::KeepAnchor);
        QString command = cursor.selectedText().replace(">>> ", "").trimmed();

        if (!command.isEmpty()) {
            emit commandEntered(command);
        }

        cursor.movePosition(QTextCursor::EndOfLine);
        setTextCursor(cursor);
        append(">>> ");
    } else {
        QTextEdit::keyPressEvent(event);
    }
}
