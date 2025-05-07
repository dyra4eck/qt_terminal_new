#ifndef SHELL_H
#define SHELL_H

#include <QObject>
#include <QMap>
#include <QProcess>
#include <QTextEdit>
#include <QStandardPaths>
#include <QProcessEnvironment>

class Shell : public QObject {
    Q_OBJECT
public:
    Shell(QTextEdit* output, QObject* parent = nullptr);
    void processCommand(const QString& line);

private slots:
    void readOutput();
    void readError();
    void handleProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    void handleCd(const QStringList& tokens);
    void listJobs();
    void handleKill(const QStringList& tokens);
    void launchProcess(const QStringList& tokens, bool background);

    QTextEdit* output;
    QMap<int, QPair<QProcess*, QString>> backgroundProcesses;
    int nextJobId;
};

#endif // SHELL_H
