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

    defaultCharFormat.setForeground(QColor("#179a7e"));
    setStyleSheet("background-color: #1e2229; color: #179a7e;");

    createHistoryFileIfNeeded();
    loadHistory();
    insertPrompt();
}

QTextCharFormat TerminalTextEdit::getDefaultCharFormat() const
{
    return defaultCharFormat;
}

void TerminalTextEdit::insertPrompt()
{
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.setCharFormat(defaultCharFormat);
    cursor.insertBlock();
    prompt = QDir::currentPath() + "$ ";
    cursor.insertText(prompt, defaultCharFormat);
    setTextCursor(cursor);
    ensureCursorVisible();
}

void TerminalTextEdit::appendOutput(const QString &text)
{
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.setCharFormat(defaultCharFormat);
    cursor.insertText(text);
    setTextCursor(cursor);
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
        highlightCommand();
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
        highlightCommand();
    }
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
            format.setForeground(QColor("#44853a")); // Green for valid
        } else {
            format.setForeground(QColor("#bc352a")); // Red for invalid
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

    if (currentInput.trimmed().isEmpty()) {
        insertPlainText("\t");
        return;
    }

    QStringList parts = currentInput.split(' ', Qt::SkipEmptyParts);
    QString prefix = parts.isEmpty() ? "" : parts.last();

    QStringList matches = findCompletions(prefix);
    if (matches.size() == 1) {
        parts.removeLast();
        parts.append(matches.first());
        QString completedLine = parts.join(' ');
        cursor.insertText(prompt + completedLine);

        if (completedLine != getCurrentCommand()) {
            commandHistory.append(completedLine);
            historyIndex = commandHistory.size();
            saveHistory();
        }
    } else if (matches.size() > 1) {
        appendOutput("\n" + matches.join("  "));
        appendPrompt();
    }
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

QStringList TerminalTextEdit::findCompletions(const QString &prefix)
{
    QStringList matches;

    QString pathEnv = qgetenv("PATH");
    QStringList paths = pathEnv.split(':');
    for (const QString &dir : paths) {
        QDir d(dir);
        QStringList files = d.entryList(QDir::Files | QDir::Executable);
        for (const QString &file : files) {
            if (file.startsWith(prefix) && !matches.contains(file))
                matches.append(file);
        }
    }

    QDir currentDir = QDir::current();
    QStringList localFiles = currentDir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot);
    for (const QString &file : localFiles) {
        if (file.startsWith(prefix) && !matches.contains(file))
            matches.append(file);
    }

    matches.sort();
    return matches;
}

void TerminalTextEdit::appendPrompt()
{
    append(prompt);
}
