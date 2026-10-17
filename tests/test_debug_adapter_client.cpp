#include <QtTest>
#include <QTemporaryFile>
#include <QSignalSpy>
#include "cinderde/debug_adapter_client.h"

class TestDebugAdapterClient : public QObject {
    Q_OBJECT

private slots:
    void testFramedRequestResponse() {
        QTemporaryFile script;
        QVERIFY(script.open());
        const QByteArray source = R"PY(import json, sys

while True:
    header = bytearray()
    while not header.endswith(b"\r\n\r\n"):
        byte = sys.stdin.buffer.read(1)
        if not byte:
            sys.exit(0)
        header.extend(byte)
    length = 0
    for line in header.decode().split("\r\n"):
        if line.lower().startswith("content-length:"):
            length = int(line.split(":", 1)[1].strip())
    body = sys.stdin.buffer.read(length)
    request = json.loads(body.decode())
    response = {
        "seq": 1,
        "type": "response",
        "request_seq": request["seq"],
        "command": request["command"],
        "success": True,
        "body": {"answer": 42}
    }
    encoded = json.dumps(response, separators=(",", ":")).encode()
    sys.stdout.buffer.write(b"Content-Length: " + str(len(encoded)).encode() + b"\r\n\r\n" + encoded)
    sys.stdout.buffer.flush()
)PY";
        QCOMPARE(script.write(source), qint64(source.size()));
        script.flush();

        CinderDE::DebugAdapterClient client;
        QSignalSpy responseSpy(&client, &CinderDE::DebugAdapterClient::responseReceived);
        QVERIFY(responseSpy.isValid());
        QVERIFY(client.start(QStringLiteral("python3"), {script.fileName()}));
        QTRY_VERIFY_WITH_TIMEOUT(client.isRunning(), 2000);

        const int seq = client.sendRequest(QStringLiteral("test/request"));
        QTRY_COMPARE_WITH_TIMEOUT(responseSpy.count(), 1, 3000);

        const QList<QVariant> args = responseSpy.takeFirst();
        QCOMPARE(args.at(0).toInt(), seq);
        QCOMPARE(args.at(1).toString(), QStringLiteral("test/request"));
        QCOMPARE(args.at(2).toBool(), true);
        QCOMPARE(args.at(3).toJsonObject().value(QStringLiteral("answer")).toInt(), 42);
        client.stop();
    }
};

QTEST_GUILESS_MAIN(TestDebugAdapterClient)
#include "test_debug_adapter_client.moc"
