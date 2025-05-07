#include "shell.h"
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QProcessEnvironment>

Shell::Shell(QTextEdit* output, QObject* parent)
    : QObject(parent), output(output), nextJobId(1) {}

void Shell::processCommand(const QString& line) {
    QStringList tokens = line.split(" ", Qt::SkipEmptyParts);
    if (tokens.isEmpty()) return;

    QString command = tokens.first();
    if (command == "clear") {
        output->clear();
        return;
    } else if (command == "cd") {
        handleCd(tokens);
    } else if (command == "jobs") {
        listJobs();
    } else if (command == "kill") {
        handleKill(tokens);
    } else {
        bool background = tokens.last() == "&";
        if (background) tokens.removeLast();
        launchProcess(tokens, background);
    }
}

void Shell::handleCd(const QStringList& tokens) {
    if (tokens.size() == 1) {
        QDir::setCurrent(QDir::homePath());
    } else if (tokens.size() == 2) {
        if (!QDir::setCurrent(tokens[1])) {
            output->append("cd: " + tokens[1] + ": Нет такого файла или каталога");
        }
    } else {
        output->append("cd: слишком много аргументов");
    }
}

void Shell::listJobs() {
    for (auto it = backgroundProcesses.begin(); it != backgroundProcesses.end(); ++it) {
        output->append(QString("[%1] %2").arg(it.key()).arg(it.value().second));
    }
}

void Shell::handleKill(const QStringList& tokens) {
    if (tokens.size() != 2) {
        output->append("kill: использование: kill <job_id>");
        return;
    }
    bool ok;
    int jobId = tokens[1].toInt(&ok);
    if (!ok || !backgroundProcesses.contains(jobId)) {
        output->append("kill: неверный job_id");
        return;
    }
    QProcess* proc = backgroundProcesses[jobId].first;
    proc->terminate();
    backgroundProcesses.remove(jobId);
}

void Shell::launchProcess(const QStringList& tokens, bool background) {
    QProcess* proc = new QProcess(this);
    connect(proc, &QProcess::readyReadStandardOutput, this, &Shell::readOutput);
    connect(proc, &QProcess::readyReadStandardError, this, &Shell::readError);
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &Shell::handleProcessFinished);

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("TERM", "xterm");
    proc->setProcessEnvironment(env);

    QString program = tokens.first();
    QStringList args = tokens.mid(1);

    if (QFileInfo::exists(program) || QFileInfo::exists(QStandardPaths::findExecutable(program))) {
        proc->start(program, args);
    } else {
        output->append("Ошибка: команда не найдена - " + program);
        delete proc;
        return;
    }

    if (background) {
        backgroundProcesses[nextJobId++] = {proc, tokens.join(" ")};
        output->append(QString("[%1] Запущен в фоне").arg(nextJobId - 1));
    } else {
        proc->waitForFinished();
    }
}

void Shell::readOutput() {
    QProcess* proc = qobject_cast<QProcess*>(sender());
    if (proc) {
        output->append(proc->readAllStandardOutput());
    }
}

void Shell::readError() {
    QProcess* proc = qobject_cast<QProcess*>(sender());
    if (proc) {
        output->append(proc->readAllStandardError());
    }
}

void Shell::handleProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    Q_UNUSED(exitStatus);
    QProcess* proc = qobject_cast<QProcess*>(sender());
    if (proc) {
        output->append(QString("Завершено с кодом: %1").arg(exitCode));
        proc->deleteLater();
    }
}
