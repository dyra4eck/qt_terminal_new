#include "terminaltextedit.h"
#include <QScrollBar>
#include <QDir>
#include <QMouseEvent>
#include <QTextCursor>
#include <QTextCharFormat>
#include <QProcessEnvironment>

TerminalTextEdit::TerminalTextEdit(QWidget *parent)
    : QTextEdit(parent)
{
    setUndoRedoEnabled(false);
    setAcceptRichText(false);
    setCursorWidth(2);

    QFont font("Monospace");
    font.setStyleHint(QFont::TypeWriter);
    setFont(font);

    setStyleSheet("background-color: #1e2229; color: #176d45;");

    createHistoryFileIfNeeded();
    loadHistory();
    insertPrompt();
}

void TerminalTextEdit::insertPrompt()
{
    moveCursor(QTextCursor::End);
    QTextCursor cursor = textCursor();

    cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);
    QString line = cursor.selectedText();

    if (line.trimmed().isEmpty()) {
        cursor.movePosition(QTextCursor::End);
        setTextCursor(cursor);
        insertPlainText(prompt);
    }

    ensureCursorVisible();
}

void TerminalTextEdit::scrollToBottom()
{
    verticalScrollBar()->setValue(verticalScrollBar()->maximum());
}

QString TerminalTextEdit::getCurrentCommand() const
{
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);
    QString line = cursor.selectedText();
    if (line.startsWith(prompt))
        return line.mid(prompt.length()).trimmed();
    return QString();
}

void TerminalTextEdit::keyPressEvent(QKeyEvent *event)
{
    // Reset completion state if the key is not Tab
    if (event->key() != Qt::Key_Tab) {
        completionMatches.clear();
        completionIndex = -1;
        completionPrefix = "";
    }

    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::End);
    setTextCursor(cursor);

    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        QString currentLine = getCurrentCommand();
        insertPlainText("\n");

        if (!currentLine.isEmpty()) {
            commandHistory.append(currentLine);
            historyIndex = commandHistory.size();
            saveHistory();
            emit commandEntered(currentLine);
        } else {
            insertPrompt();
        }
        scrollToBottom();
    }
    else if (event->key() == Qt::Key_Up) {
        if (historyIndex > 0) {
            historyIndex--;
            QTextCursor cursor = textCursor();
            cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);
            cursor.removeSelectedText();
            cursor.insertText(prompt + commandHistory.value(historyIndex));
        }
    }
    else if (event->key() == Qt::Key_Down) {
        if (historyIndex < commandHistory.size() - 1) {
            historyIndex++;
            QTextCursor cursor = textCursor();
            cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);
            cursor.removeSelectedText();
            cursor.insertText(prompt + commandHistory.value(historyIndex));
        }
        else {
            historyIndex = commandHistory.size();
            QTextCursor cursor = textCursor();
            cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);
            cursor.removeSelectedText();
            cursor.insertText(prompt);
        }
    }
    else if (event->key() == Qt::Key_Backspace) {
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);
        QString line = cursor.selectedText();
        if (line.length() <= prompt.length()) {
            return;
        }
        QTextEdit::keyPressEvent(event);
    }
    else if (event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_C) {
        emit interruptRequested();
        return;
    }
    else if (event->key() == Qt::Key_Tab) {
        event->accept();
        handleTabCompletion();
    }
    else {
        QTextEdit::keyPressEvent(event);
    }

    // Highlight the command after processing the key event
    highlightCommand();
}

void TerminalTextEdit::highlightCommand()
{
    QTextCursor cursor(document());
    cursor.movePosition(QTextCursor::End);
    cursor.movePosition(QTextCursor::StartOfBlock);
    cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, prompt.length());
    int startPos = cursor.position();
    cursor.movePosition(QTextCursor::NextWord, QTextCursor::KeepAnchor);
    QString commandName = cursor.selectedText().trimmed();

    if (!commandName.isEmpty()) {
        bool valid = isValidCommand(commandName);
        QTextCharFormat format;
        if (valid) {
            format.setForeground(QColor("#44853a")); // Valid command color
        } else {
            format.setForeground(QColor("#bc352a")); // Invalid command color
        }
        cursor.setPosition(startPos);
        cursor.movePosition(QTextCursor::NextWord, QTextCursor::KeepAnchor);
        cursor.setCharFormat(format);
    }
}

bool TerminalTextEdit::isValidCommand(const QString &command)
{
    static QStringList builtInCommands = {"cd", "clear"};
    if (builtInCommands.contains(command)) {
        return true;
    }
    if (command.contains('/')) {
        QFileInfo fileInfo(command);
        return fileInfo.exists() && fileInfo.isExecutable();
    } else {
        QString pathEnv = QProcessEnvironment::systemEnvironment().value("PATH");
        QStringList paths = pathEnv.split(':');
        for (const QString &dir : paths) {
            QFileInfo fileInfo(dir + "/" + command);
            if (fileInfo.exists() && fileInfo.isExecutable()) {
                return true;
            }
        }
        return false;
    }
}

void TerminalTextEdit::handleTabCompletion()
{
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);
    QString line = cursor.selectedText();
    QString currentInput = line.mid(prompt.length());

    // Insert tab if input is empty or only spaces
    if (currentInput.trimmed().isEmpty()) {
        insertPlainText("\t");
        return;
    }

    QStringList parts = currentInput.split(' ', Qt::SkipEmptyParts);
    QString prefix = parts.isEmpty() ? "" : parts.last();

    if (!completionMatches.isEmpty() && completionPrefix == prefix) {
        // Cycle to next match
        completionIndex = (completionIndex + 1) % completionMatches.size();
        QString nextMatch = completionMatches[completionIndex];
        parts[parts.size() - 1] = nextMatch;
        QString newLine = parts.join(' ');
        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        cursor.insertText(prompt + newLine);
    } else {
        // Find new matches
        completionMatches = findCompletions(prefix);
        if (completionMatches.isEmpty()) {
            return;
        } else if (completionMatches.size() == 1) {
            parts[parts.size() - 1] = completionMatches.first();
            QString newLine = parts.join(' ');
            cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
            cursor.insertText(prompt + newLine);
            completionMatches.clear();
            completionIndex = -1;
            completionPrefix = "";
        } else {
            completionPrefix = prefix;
            completionIndex = 0;
            parts[parts.size() - 1] = completionMatches.first();
            QString newLine = parts.join(' ');
            cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
            cursor.insertText(prompt + newLine);
        }
    }
}

QStringList TerminalTextEdit::findCompletions(const QString &prefix)
{
    QStringList matches;

    // Check built-in commands
    static QStringList builtInCommands = {"cd", "clear"};
    for (const QString &cmd : builtInCommands) {
        if (cmd.startsWith(prefix)) {
            matches.append(cmd);
        }
    }

    // Check PATH
    QString pathEnv = QProcessEnvironment::systemEnvironment().value("PATH");
    QStringList paths = pathEnv.split(':');
    for (const QString &dir : paths) {
        QDir d(dir);
        QStringList files = d.entryList(QDir::Files | QDir::Executable);
        for (const QString &file : files) {
            if (file.startsWith(prefix) && !matches.contains(file)) {
                matches.append(file);
            }
        }
    }

    // Check current directory
    QDir currentDir = QDir::current();
    QStringList localFiles = currentDir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot);
    for (const QString &file : localFiles) {
        if (file.startsWith(prefix) && !matches.contains(file)) {
            matches.append(file);
        }
    }

    matches.sort();
    return matches;
}

void TerminalTextEdit::loadHistory()
{
    QFile file(HistoryFileName);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (!line.isEmpty()) {
                commandHistory.append(line);
            }
        }
        file.close();
    }
    historyIndex = commandHistory.size();
}

void TerminalTextEdit::saveHistory()
{
    while (commandHistory.size() > MaxHistorySize) {
        commandHistory.removeFirst();
    }

    QFile file(HistoryFileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        QTextStream out(&file);
        for (const QString &cmd : commandHistory) {
            out << cmd << "\n";
        }
        out.flush();
        file.flush();
        file.close();
    }
}

void TerminalTextEdit::createHistoryFileIfNeeded()
{
    QFile file(HistoryFileName);
    if (!file.exists()) {
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            file.close();
        }
    }
}

void TerminalTextEdit::mousePressEvent(QMouseEvent *event)
{
    selectingText = true;
    QTextEdit::mousePressEvent(event);
}

void TerminalTextEdit::mouseReleaseEvent(QMouseEvent *event)
{
    selectingText = false;
    QTextEdit::mouseReleaseEvent(event);
}

void TerminalTextEdit::appendPrompt()
{
    append(prompt);
}
