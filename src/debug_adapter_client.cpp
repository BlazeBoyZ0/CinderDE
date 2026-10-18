#include "cinderde/debug_adapter_client.h"

#include <QJsonDocument>
#include <QJsonParseError>
#include <QDir>

namespace CinderDE {

DebugAdapterClient::DebugAdapterClient(QObject *parent)
    : QObject(parent), m_process(new QProcess(this))
{
    connect(m_process, &QProcess::started, this, &DebugAdapterClient::started);
    connect(m_process, &QProcess::readyReadStandardOutput, this, &DebugAdapterClient::readStandardOutput);
    connect(m_process, &QProcess::readyReadStandardError, this, &DebugAdapterClient::readStandardError);
    connect(m_process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this, &DebugAdapterClient::handleFinished);
    connect(m_process, &QProcess::errorOccurred, this, &DebugAdapterClient::handleProcessError);
}

DebugAdapterClient::~DebugAdapterClient()
{
    stop();
}

bool DebugAdapterClient::start(const QString &program, const QStringList &arguments,
                               const QString &workingDirectory)
{
    if (isRunning()) {
        emit processError(QStringLiteral("A debug adapter is already running."));
        return false;
    }

    m_buffer.clear();
    m_nextSequence = 1;
    m_process->setProgram(program);
    m_process->setArguments(arguments);
    if (!workingDirectory.isEmpty())
        m_process->setWorkingDirectory(workingDirectory);
    m_process->start();
    return true;
}

void DebugAdapterClient::stop()
{
    if (m_process->state() == QProcess::NotRunning)
        return;

    m_process->terminate();
    if (!m_process->waitForFinished(500)) {
        m_process->kill();
        m_process->waitForFinished(1000);
    }
    m_buffer.clear();
}

void DebugAdapterClient::closeInput()
{
    if (m_process->state() != QProcess::NotRunning)
        m_process->closeWriteChannel();
}

bool DebugAdapterClient::isRunning() const
{
    return m_process->state() != QProcess::NotRunning;
}

int DebugAdapterClient::sendRequest(const QString &command, const QJsonObject &arguments)
{
    const int seq = m_nextSequence++;
    QJsonObject message{
        {QStringLiteral("seq"), seq},
        {QStringLiteral("type"), QStringLiteral("request")},
        {QStringLiteral("command"), command},
        {QStringLiteral("arguments"), arguments}
    };
    writeMessage(message);
    return seq;
}

void DebugAdapterClient::sendResponse(int requestSeq, const QString &command, bool success,
                                      const QJsonObject &body, const QString &message)
{
    QJsonObject response{
        {QStringLiteral("seq"), m_nextSequence++},
        {QStringLiteral("type"), QStringLiteral("response")},
        {QStringLiteral("request_seq"), requestSeq},
        {QStringLiteral("command"), command},
        {QStringLiteral("success"), success},
        {QStringLiteral("body"), body}
    };
    if (!message.isEmpty())
        response.insert(QStringLiteral("message"), message);
    writeMessage(response);
}

void DebugAdapterClient::sendEvent(const QString &event, const QJsonObject &body)
{
    writeMessage(QJsonObject{
        {QStringLiteral("seq"), m_nextSequence++},
        {QStringLiteral("type"), QStringLiteral("event")},
        {QStringLiteral("event"), event},
        {QStringLiteral("body"), body}
    });
}

void DebugAdapterClient::writeMessage(const QJsonObject &message)
{
    if (!isRunning()) {
        emit processError(QStringLiteral("Cannot send a DAP message: the adapter is not running."));
        return;
    }

    const QByteArray body = QJsonDocument(message).toJson(QJsonDocument::Compact);
    QByteArray frame = "Content-Length: ";
    frame += QByteArray::number(body.size());
    frame += "\r\n\r\n";
    frame += body;
    if (m_process->write(frame) == -1)
        emit processError(QStringLiteral("Failed to write to the debug adapter."));
}

void DebugAdapterClient::readStandardOutput()
{
    m_buffer += m_process->readAllStandardOutput();
    parseMessages();
}

void DebugAdapterClient::readStandardError()
{
    const QByteArray data = m_process->readAllStandardError();
    if (!data.isEmpty())
        emit outputReceived(QString::fromLocal8Bit(data), true);
}

void DebugAdapterClient::parseMessages()
{
    while (true) {
        const int headerEnd = m_buffer.indexOf("\r\n\r\n");
        if (headerEnd < 0)
            return;

        int contentLength = -1;
        const QList<QByteArray> headers = m_buffer.left(headerEnd).split('\n');
        for (QByteArray header : headers) {
            header = header.trimmed();
            const int colon = header.indexOf(':');
            if (colon > 0 && header.left(colon).trimmed().toLower() == "content-length") {
                bool ok = false;
                const int parsed = header.mid(colon + 1).trimmed().toInt(&ok);
                if (ok && parsed >= 0)
                    contentLength = parsed;
                break;
            }
        }

        if (contentLength < 0) {
            emit protocolError(QStringLiteral("Invalid DAP message: missing or invalid Content-Length header."));
            m_buffer.remove(0, headerEnd + 4);
            continue;
        }

        const int frameLength = headerEnd + 4 + contentLength;
        if (m_buffer.size() < frameLength)
            return;

        const QByteArray body = m_buffer.mid(headerEnd + 4, contentLength);
        m_buffer.remove(0, frameLength);

        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(body, &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            emit protocolError(QStringLiteral("Invalid JSON from debug adapter: %1").arg(parseError.errorString()));
            continue;
        }

        const QJsonObject object = document.object();
        const QString type = object.value(QStringLiteral("type")).toString();
        if (type == QStringLiteral("response")) {
            emit responseReceived(
                object.value(QStringLiteral("request_seq")).toInt(),
                object.value(QStringLiteral("command")).toString(),
                object.value(QStringLiteral("success")).toBool(),
                object.value(QStringLiteral("body")).toObject(),
                object.value(QStringLiteral("message")).toString());
        } else if (type == QStringLiteral("event")) {
            emit eventReceived(object.value(QStringLiteral("event")).toString(),
                               object.value(QStringLiteral("body")).toObject());
        } else if (type == QStringLiteral("request")) {
            emit requestReceived(object.value(QStringLiteral("seq")).toInt(),
                                 object.value(QStringLiteral("command")).toString(),
                                 object.value(QStringLiteral("arguments")).toObject());
        } else {
            emit protocolError(QStringLiteral("Unknown DAP message type: %1").arg(type));
        }
    }
}

void DebugAdapterClient::handleFinished(int exitCode, QProcess::ExitStatus status)
{
    // QProcess may emit finished() while unread bytes remain available. Drain
    // stdout and parse complete frames before treating the remaining buffer as
    // a truncated DAP message.
    m_buffer += m_process->readAllStandardOutput();
    parseMessages();

    if (!m_buffer.isEmpty()) {
        emit protocolError(QStringLiteral("Debug adapter exited with an incomplete DAP message."));
        m_buffer.clear();
    }
    emit finished(exitCode, status);
}

void DebugAdapterClient::handleProcessError(QProcess::ProcessError error)
{
    Q_UNUSED(error);
    emit processError(m_process->errorString());
}

} // namespace CinderDE
