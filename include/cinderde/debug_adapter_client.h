#ifndef CINDERDE_DEBUG_ADAPTER_CLIENT_H
#define CINDERDE_DEBUG_ADAPTER_CLIENT_H

#include <QObject>
#include <QJsonObject>
#include <QProcess>
#include <QByteArray>

namespace CinderDE {

// Minimal Debug Adapter Protocol (DAP) transport. It owns the adapter process,
// frames JSON messages using Content-Length, and exposes protocol messages to
// the debugger controller/UI without imposing a particular adapter.
class DebugAdapterClient : public QObject {
    Q_OBJECT

public:
    explicit DebugAdapterClient(QObject *parent = nullptr);
    ~DebugAdapterClient() override;

    bool start(const QString &program, const QStringList &arguments = QStringList(),
               const QString &workingDirectory = QString());
    void stop();
    void closeInput();
    bool isRunning() const;
    int sendRequest(const QString &command, const QJsonObject &arguments = QJsonObject());
    void sendResponse(int requestSeq, const QString &command, bool success,
                      const QJsonObject &body = QJsonObject(),
                      const QString &message = QString());
    void sendEvent(const QString &event, const QJsonObject &body = QJsonObject());

signals:
    void started();
    void finished(int exitCode, QProcess::ExitStatus status);
    void processError(const QString &message);
    void responseReceived(int requestSeq, const QString &command, bool success,
                          const QJsonObject &body, const QString &message);
    void eventReceived(const QString &event, const QJsonObject &body);
    void requestReceived(int requestSeq, const QString &command, const QJsonObject &arguments);
    void protocolError(const QString &message);
    void outputReceived(const QString &text, bool standardError);

private slots:
    void readStandardOutput();
    void readStandardError();
    void handleFinished(int exitCode, QProcess::ExitStatus status);
    void handleProcessError(QProcess::ProcessError error);

private:
    QProcess *m_process;
    QByteArray m_buffer;
    int m_nextSequence = 1;

    void writeMessage(const QJsonObject &message);
    void parseMessages();
};

} // namespace CinderDE

#endif // CINDERDE_DEBUG_ADAPTER_CLIENT_H
