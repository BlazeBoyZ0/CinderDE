#include <QtTest>
#include <QTemporaryDir>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include "cinderde/debugger_config.h"

using namespace CinderDE;

class DebuggerConfigTest : public QObject {
    Q_OBJECT
private slots:
    void roundTripWorkspaceConfigurations();
    void preserveExistingWorkspaceSettings();
    void legacyPythonConfigurationDefaultsToPyExtension();
};

void DebuggerConfigTest::roundTripWorkspaceConfigurations()
{
    QTemporaryDir workspace;
    QVERIFY(workspace.isValid());

    DebuggerConfiguration config;
    config.id = QStringLiteral("python-test");
    config.name = QStringLiteral("Python debugpy");
    config.languageId = QStringLiteral("python");
    config.executable = QStringLiteral("python3");
    config.runtimeExecutable = QStringLiteral(".venv/bin/python");
    config.arguments = {QStringLiteral("-m"), QStringLiteral("debugpy.adapter")};
    config.workingDirectory = QStringLiteral("src");
    config.fileExtensions = {QStringLiteral(".cpp"), QStringLiteral(".hpp")};
    config.launchArgumentsJson = QStringLiteral(R"({"program":"${program}","cwd":"${workspace}"})");
    config.enabled = true;

    QVERIFY(DebuggerConfigStore::save(workspace.path(), {config}));
    const QList<DebuggerConfiguration> loaded = DebuggerConfigStore::load(workspace.path());
    QCOMPARE(loaded.size(), 1);
    QCOMPARE(loaded.first().id, config.id);
    QCOMPARE(loaded.first().languageId, config.languageId);
    QCOMPARE(loaded.first().executable, config.executable);
    QCOMPARE(loaded.first().runtimeExecutable, config.runtimeExecutable);
    QCOMPARE(loaded.first().arguments, config.arguments);
    QCOMPARE(loaded.first().workingDirectory, config.workingDirectory);
    QCOMPARE(loaded.first().fileExtensions, config.fileExtensions);
    QCOMPARE(loaded.first().launchArgumentsJson, config.launchArgumentsJson);
    QVERIFY(loaded.first().enabled);
}

void DebuggerConfigTest::legacyPythonConfigurationDefaultsToPyExtension()
{
    QTemporaryDir workspace;
    QVERIFY(workspace.isValid());
    QDir dir(workspace.path());
    QVERIFY(dir.mkpath(QStringLiteral(".cinderde")));
    QFile file(dir.filePath(QStringLiteral(".cinderde/settings.json")));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    file.write(QJsonDocument(QJsonObject{
        {QStringLiteral("debuggers"), QJsonArray{
            QJsonObject{
                {QStringLiteral("id"), QStringLiteral("legacy-python")},
                {QStringLiteral("name"), QStringLiteral("Python debugpy")},
                {QStringLiteral("languageId"), QStringLiteral("python")},
                {QStringLiteral("executable"), QStringLiteral("python3")}
            }
        }}
    }).toJson());
    file.close();

    const auto loaded = DebuggerConfigStore::load(workspace.path());
    QCOMPARE(loaded.size(), 1);
    QCOMPARE(loaded.first().fileExtensions, QStringList{QStringLiteral(".py")});
    QVERIFY(loaded.first().launchArgumentsJson.isEmpty());
}

void DebuggerConfigTest::preserveExistingWorkspaceSettings()
{
    QTemporaryDir workspace;
    QVERIFY(workspace.isValid());
    QDir dir(workspace.path());
    QVERIFY(dir.mkpath(QStringLiteral(".cinderde")));
    QFile file(dir.filePath(QStringLiteral(".cinderde/settings.json")));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    file.write(QJsonDocument(QJsonObject{{QStringLiteral("editor.wordWrap"), true},
                                         {QStringLiteral("custom.setting"), QStringLiteral("keep")}})
                   .toJson());
    file.close();

    DebuggerConfiguration config;
    config.id = QStringLiteral("python");
    config.name = QStringLiteral("Python");
    config.languageId = QStringLiteral("python");
    config.executable = QStringLiteral("python3");
    QVERIFY(DebuggerConfigStore::save(workspace.path(), {config}));

    QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
    const QJsonObject root = QJsonDocument::fromJson(file.readAll()).object();
    QCOMPARE(root.value(QStringLiteral("editor.wordWrap")).toBool(), true);
    QCOMPARE(root.value(QStringLiteral("custom.setting")).toString(), QStringLiteral("keep"));
    QVERIFY(root.value(QStringLiteral("debuggers")).isArray());
}

QTEST_MAIN(DebuggerConfigTest)
#include "test_debugger_config.moc"
