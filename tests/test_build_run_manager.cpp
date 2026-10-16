#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include "cinderde/build_run_manager.h"
#include "cinderde/workspace_config.h"

class TestBuildRunManager : public QObject {
    Q_OBJECT

private slots:
    void testMacroSubstitution() {
        QString filePath = "/tmp/project/src/main.cpp";
        QString workspacePath = "/tmp/project";
        QString cmd = "g++ ${file} -o ${fileDir}/${fileBase} -I${workspace}";

        QString substituted = CinderDE::BuildRunManager::substituteMacros(cmd, filePath, workspacePath, workspacePath);
        QCOMPARE(substituted, QString("g++ /tmp/project/src/main.cpp -o /tmp/project/src/main -I/tmp/project"));
    }

    void testResolveWorkingDirectory() {
        CinderDE::WorkspaceConfig config;
        config.hasWorkingDirectory = true;
        config.workingDirectory = "${workspace}/build";

        QString dir = CinderDE::BuildRunManager::resolveWorkingDirectory("/tmp/project/src/main.cpp", "/tmp/project", config);
        QCOMPARE(dir, QString("/tmp/project/build"));
    }

    void testResolveCommandsCppDefaults() {
        CinderDE::WorkspaceConfig config;
        auto cmds = CinderDE::BuildRunManager::resolveCommands("/tmp/project/main.cpp", "", config);

        QVERIFY(cmds.first.contains("g++"));
        QString subRun = CinderDE::BuildRunManager::substituteMacros(cmds.second, "/tmp/project/main.cpp", "", "");
        QVERIFY(subRun.contains("main"));
    }

    void testResolveCommandsPythonDefaults() {
        CinderDE::WorkspaceConfig config;
        auto cmds = CinderDE::BuildRunManager::resolveCommands("/tmp/project/script.py", "", config);

        QVERIFY(cmds.first.isEmpty());
        QVERIFY(cmds.second.contains("python3"));
    }

    void testResolveCommandsConfiguredOverride() {
        CinderDE::WorkspaceConfig config;
        config.hasBuildCommand = true;
        config.buildCommand = "make -j4";
        config.hasRunCommand = true;
        config.runCommand = "./bin/app";

        auto cmds = CinderDE::BuildRunManager::resolveCommands("/tmp/project/main.cpp", "/tmp/project", config);
        QCOMPARE(cmds.first, QString("make -j4"));
        QCOMPARE(cmds.second, QString("./bin/app"));
    }
};

QTEST_MAIN(TestBuildRunManager)
#include "test_build_run_manager.moc"
