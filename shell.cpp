#include "shell.h"
#include <QDir>

Shell::Shell(QTextEdit* output, QObject* parent)
    : QObject(parent), output(output), currentProcess(nullptr), nextJobId(1) {} // Инициализация currentProcess

void Shell::processCommand(const QString& line) {
    if (line.isEmpty()) return;

    QStringList tokens = line.split(" ", Qt::SkipEmptyParts);
    if (tokens.isEmpty()) return;

    if (currentProcess) {
        currentProcess->kill();
        currentProcess->deleteLater();
    }

    currentProcess = new QProcess(this);
    connect(currentProcess, &QProcess::readyReadStandardOutput, this, &Shell::readOutput);
    connect(currentProcess, &QProcess::readyReadStandardError, this, &Shell::readError);
    connect(currentProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &Shell::handleProcessFinished);

    currentProcess->start(tokens.first(), tokens.mid(1));
}

void Shell::readOutput() {
    output->append("<span style='color:white'>" + currentProcess->readAllStandardOutput() + "</span>");
}

void Shell::readError() {
    output->append("<span style='color:red'>" + currentProcess->readAllStandardError() + "</span>");
}

void Shell::handleProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    Q_UNUSED(exitStatus);
    output->append(QString("Process exited with code: %1").arg(exitCode));
}
