#include <QtTest>
#include <QTemporaryFile>
#include <QTemporaryDir>
#include <QProcess>
#include <QFile>
#include <QTextStream>
#include "cinderde/lsp_types.h"
#include "cinderde/lsp_client.h"
#include "cinderde/lsp_manager.h"
#include "cinderde/preferences.h"

class TestLsp : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        m_mockScriptFile = new QTemporaryFile(this);
        QVERIFY(m_mockScriptFile->open());

        QString scriptContent = R"PY(import sys, json, os

def send_msg(msg_obj):
    body = json.dumps(msg_obj).encode('utf-8')
    header = f"Content-Length: {len(body)}\r\n\r\n".encode('utf-8')
    sys.stdout.buffer.write(header + body)
    sys.stdout.buffer.flush()

buffer = bytearray()
while True:
    chunk = sys.stdin.buffer.read(1)
    if not chunk:
        break
    buffer.extend(chunk)
    while True:
        header_end = buffer.find(b"\r\n\r\n")
        if header_end == -1:
            break
        header = buffer[:header_end].decode('utf-8', errors='ignore')
        content_length = -1
        for line in header.split('\r\n'):
            if line.lower().startswith("content-length:"):
                content_length = int(line.split(':')[1].strip())
        if content_length == -1 or len(buffer) < header_end + 4 + content_length:
            break
        body_bytes = buffer[header_end + 4:header_end + 4 + content_length]
        del buffer[:header_end + 4 + content_length]

        req = json.loads(body_bytes.decode('utf-8'))
        method = req.get("method")
        msg_id = req.get("id")

        if method == "initialize":
            send_msg({
                "jsonrpc": "2.0",
                "id": msg_id,
                "result": {
                    "capabilities": {
                        "completionProvider": {},
                        "hoverProvider": True,
                        "definitionProvider": True,
                        "referencesProvider": True,
                        "renameProvider": True
                    }
                }
            })
        elif method == "initialized":
            send_msg({
                "jsonrpc": "2.0",
                "method": "textDocument/publishDiagnostics",
                "params": {
                    "uri": "file:///tmp/test.cpp",
                    "diagnostics": [
                        {
                            "range": {"start": {"line": 0, "character": 0}, "end": {"line": 0, "character": 5}},
                            "severity": 1,
                            "message": "Test diagnostic error"
                        }
                    ]
                }
            })
        elif method == "textDocument/completion":
            send_msg({
                "jsonrpc": "2.0",
                "id": msg_id,
                "result": {
                    "items": [
                        {"label": "myFunction", "kind": 3, "detail": "void myFunction()"},
                        {"label": "myVariable", "kind": 6, "detail": "int myVariable"}
                    ]
                }
            })
        elif method == "textDocument/hover":
            send_msg({
                "jsonrpc": "2.0",
                "id": msg_id,
                "result": {
                    "contents": "Test Hover Information"
                }
            })
        elif method == "textDocument/definition":
            send_msg({
                "jsonrpc": "2.0",
                "id": msg_id,
                "result": {
                    "uri": "file:///tmp/test.cpp",
                    "range": {"start": {"line": 10, "character": 4}, "end": {"line": 10, "character": 14}}
                }
            })
        elif method == "textDocument/references":
            send_msg({
                "jsonrpc": "2.0",
                "id": msg_id,
                "result": [
                    {
                        "uri": "file:///tmp/test.cpp",
                        "range": {"start": {"line": 10, "character": 4}, "end": {"line": 10, "character": 14}}
                    }
                ]
            })
        elif method == "textDocument/rename":
            send_msg({
                "jsonrpc": "2.0",
                "id": msg_id,
                "result": {
                    "changes": {
                        "file:///tmp/test.cpp": [
                            {
                                "range": {"start": {"line": 0, "character": 0}, "end": {"line": 0, "character": 4}},
                                "newText": "newSymbol"
                            }
                        ]
                    }
                }
            })
        elif method == "shutdown":
            send_msg({"jsonrpc": "2.0", "id": msg_id, "result": None})
        elif method == "exit":
            sys.exit(0)
)PY";

        m_mockScriptFile->write(scriptContent.toUtf8());
        m_mockScriptFile->flush();
    }

    void testUriPathConversion() {
        QString path = "/tmp/sample_project/main.cpp";
        QString uri = CinderDE::pathToUri(path);
        QVERIFY(uri.startsWith("file://"));
        QCOMPARE(CinderDE::uriToPath(uri), path);
    }

    void testLspTypesParsing() {
        QJsonObject posObj;
        posObj["line"] = 5;
        posObj["character"] = 12;
        CinderDE::LspPosition pos = CinderDE::LspPosition::fromJson(posObj);
        QCOMPARE(pos.line, 5);
        QCOMPARE(pos.character, 12);

        QJsonObject diagObj;
        QJsonObject rangeObj;
        rangeObj["start"] = posObj;
        rangeObj["end"] = posObj;
        diagObj["range"] = rangeObj;
        diagObj["severity"] = 1;
        diagObj["message"] = "Syntax Error";
        CinderDE::LspDiagnostic diag = CinderDE::LspDiagnostic::fromJson(diagObj);
        QCOMPARE(diag.severity, 1);
        QCOMPARE(diag.message, QString("Syntax Error"));
    }

    void testConfigPersistence() {
        CinderDE::LspServerConfig cfg;
        cfg.id = "test_pyright";
        cfg.name = "Pyright";
        cfg.languageId = "python";
        cfg.extensions = QStringList() << "py";
        cfg.executable = "pyright-langserver";
        cfg.arguments = QStringList() << "--stdio";
        cfg.enabled = true;

        CinderDE::Preferences::instance().saveLspServerConfig(cfg);

        QList<CinderDE::LspServerConfig> configs = CinderDE::Preferences::instance().lspServerConfigs();
        bool found = false;
        for (const auto &c : configs) {
            if (c.id == "test_pyright") {
                found = true;
                QCOMPARE(c.name, QString("Pyright"));
                QCOMPARE(c.languageId, QString("python"));
                QCOMPARE(c.executable, QString("pyright-langserver"));
                QCOMPARE(c.enabled, true);
            }
        }
        QVERIFY(found);

        CinderDE::Preferences::instance().removeLspServerConfig("test_pyright");
        configs = CinderDE::Preferences::instance().lspServerConfigs();
        found = false;
        for (const auto &c : configs) {
            if (c.id == "test_pyright") found = true;
        }
        QVERIFY(!found);
    }

    void testMockLspClientAndManager() {
        QTemporaryDir tempWorkspace;
        QVERIFY(tempWorkspace.isValid());

        CinderDE::LspServerConfig cfg;
        cfg.id = "mock_python_lsp";
        cfg.name = "Mock Python LSP";
        cfg.languageId = "python";
        cfg.extensions = QStringList() << "py";
        cfg.executable = "python3";
        cfg.arguments = QStringList() << m_mockScriptFile->fileName();
        cfg.enabled = true;

        CinderDE::Preferences::instance().saveLspServerConfig(cfg);
        CinderDE::LspManager::instance().setWorkspacePath(tempWorkspace.path());

        QString testFilePath = tempWorkspace.path() + "/test.py";
        QFile testFile(testFilePath);
        QVERIFY(testFile.open(QIODevice::WriteOnly | QIODevice::Text));
        testFile.write("def foo():\n    pass\n");
        testFile.close();

        // 1. Get client lazily
        CinderDE::LspClient *client = CinderDE::LspManager::instance().getClientForDocument(testFilePath, true);
        QVERIFY(client != nullptr);

        // Wait for initialize response
        QTRY_COMPARE_WITH_TIMEOUT(client->isRunning(), true, 5000);
        QVERIFY(client->capabilities().completionProvider);
        QVERIFY(client->capabilities().hoverProvider);
        QVERIFY(client->capabilities().definitionProvider);

        // 2. Test completion request
        bool completionDone = false;
        client->requestCompletion(testFilePath, 0, 2, [&completionDone](const QJsonValue &result, const QJsonObject &, bool isError) {
            QVERIFY(!isError);
            QVERIFY(result.isObject());
            QJsonArray items = result.toObject().value("items").toArray();
            QCOMPARE(items.size(), 2);
            completionDone = true;
        });
        QTRY_COMPARE_WITH_TIMEOUT(completionDone, true, 3000);

        // 3. Test hover request
        bool hoverDone = false;
        client->requestHover(testFilePath, 0, 2, [&hoverDone](const QJsonValue &result, const QJsonObject &, bool isError) {
            QVERIFY(!isError);
            CinderDE::LspHover hover = CinderDE::LspHover::fromJson(result.toObject());
            QCOMPARE(hover.contents, QString("Test Hover Information"));
            hoverDone = true;
        });
        QTRY_COMPARE_WITH_TIMEOUT(hoverDone, true, 3000);

        // 4. Test definition request
        bool defDone = false;
        client->requestDefinition(testFilePath, 0, 2, [&defDone](const QJsonValue &result, const QJsonObject &, bool isError) {
            QVERIFY(!isError);
            CinderDE::LspLocation loc = CinderDE::LspLocation::fromJson(result.toObject());
            QCOMPARE(loc.range.start.line, 10);
            defDone = true;
        });
        QTRY_COMPARE_WITH_TIMEOUT(defDone, true, 3000);

        // 5. Test RAM estimate
        qint64 ram = client->estimateRamUsageBytes();
        QVERIFY(ram > 0 || ram == -1);

        // 6. Test graceful shutdown
        CinderDE::LspManager::instance().stopAllServers();
        QCOMPARE(client->isRunning(), false);

        CinderDE::Preferences::instance().removeLspServerConfig("mock_python_lsp");
    }

    void testMalformedContentLengthDoesNotPoisonBuffer() {
        CinderDE::LspServerConfig cfg;
        cfg.id = "parser_test";
        cfg.name = "Parser Test";
        cfg.languageId = "cpp";
        cfg.executable = "unused";
        cfg.enabled = true;
        CinderDE::LspClient client(cfg, "/tmp");
        QSignalSpy logSpy(&client, &CinderDE::LspClient::logMessage);

        client.m_readBuffer = "Content-Length: abc\\r\\n\\r\\n{\\"jsonrpc\\":\\"2.0\\"}";
        client.processIncomingBuffer();
        QVERIFY(client.m_readBuffer.isEmpty());
        QCOMPARE(logSpy.count(), 1);
        QVERIFY(logSpy.takeFirst().at(0).toString().contains("Malformed LSP Content-Length"));

        const QByteArray body = R"({"jsonrpc":"2.0","method":"window/logMessage","params":{"message":"recovered"}})";
        client.m_readBuffer = "Content-Length: " + QByteArray::number(body.size()) + "\\r\\n\\r\\n" + body;
        client.processIncomingBuffer();
        QVERIFY(client.m_readBuffer.isEmpty());
        QCOMPARE(logSpy.count(), 1);
        QVERIFY(logSpy.takeFirst().at(0).toString().contains("recovered"));
    }

    void testFailedExecutable() {
        CinderDE::LspServerConfig cfg;
        cfg.id = "invalid_executable";
        cfg.name = "Invalid Executable";
        cfg.languageId = "cpp";
        cfg.executable = "/nonexistent_binary_xyz_123";
        cfg.enabled = true;

        CinderDE::LspClient client(cfg, "/tmp");
        client.start();
        QCOMPARE(client.isRunning(), false);
        QCOMPARE(client.status().state, CinderDE::LspServerState::Failed);
    }

private:
    QTemporaryFile *m_mockScriptFile = nullptr;
};

QTEST_MAIN(TestLsp)
#include "test_lsp.moc"
