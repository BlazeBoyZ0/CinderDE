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

    void testFindCMakeExecutableIgnoresTests() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        QString cmakePath = QDir(tempDir.path()).filePath("CMakeLists.txt");
        QFile file(cmakePath);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(
            "cmake_minimum_required(VERSION 3.16)\n"
            "project(MyCoolApp)\n"
            "add_executable(mycoolapp_tests test_main.cpp)\n"
            "add_executable(mycoolapp main.cpp)\n"
        );
        file.close();

        QString exec = CinderDE::BuildRunManager::findCMakeExecutable(tempDir.path(), tempDir.path());
        QVERIFY(exec.endsWith("mycoolapp"));
        QVERIFY(!exec.endsWith("mycoolapp_tests"));
    }

    void testExternalTerminalFailureReporting() {
        CinderDE::BuildRunManager manager;
        CinderDE::WorkspaceConfig config;
        config.hasExecutionMode = true;
        config.executionMode = "external_terminal";
        config.hasTerminalCommand = true;
        config.terminalCommand = "nonexistent_terminal_binary_xyz_123";

        QSignalSpy errorSpy(&manager, &CinderDE::BuildRunManager::processError);
        QSignalSpy outputSpy(&manager, &CinderDE::BuildRunManager::outputReceived);

        manager.run("/tmp/test.py", "", config);

        QCOMPARE(errorSpy.count(), 1);
        QVERIFY(errorSpy.at(0).at(0).toString().contains("nonexistent_terminal_binary_xyz_123"));

        bool foundInOutput = false;
        for (const auto &call : outputSpy) {
            if (call.at(0).toString().contains("ERROR")) {
                foundInOutput = true;
                break;
            }
        }
        QVERIFY(foundInOutput);
    }

    void testExternalTerminalExecutionWithSpaces() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        QDir spaceDir(tempDir.path());
        QVERIFY(spaceDir.mkdir("Python codes"));
        QString spaceDirPath = spaceDir.filePath("Python codes");

        QString scriptPath = QDir(spaceDirPath).filePath("learning script.py");
        QString logPath = QDir(spaceDirPath).filePath("out.log");

        QFile scriptFile(scriptPath);
        QVERIFY(scriptFile.open(QIODevice::WriteOnly | QIODevice::Text));
        scriptFile.write(QString("with open(r'%1', 'w') as f:\n    f.write('EXECUTED_SUCCESSFULLY')\n").arg(logPath).toUtf8());
        scriptFile.close();

        // Create a mock terminal script that executes the command argument passed to -e
        QString mockTermPath = QDir(tempDir.path()).filePath("mock_xterm.sh");
        QFile mockTermFile(mockTermPath);
        QVERIFY(mockTermFile.open(QIODevice::WriteOnly | QIODevice::Text));
        mockTermFile.write(
            "#!/bin/bash\n"
            "# Mock terminal emulator script\n"
            "if [ \"$1\" = \"-e\" ]; then\n"
            "    shift\n"
            "fi\n"
            "echo '' | exec \"$@\"\n"
        );
        mockTermFile.close();
        mockTermFile.setPermissions(QFile::ExeOwner | QFile::ReadOwner | QFile::WriteOwner);

        CinderDE::BuildRunManager manager;
        CinderDE::WorkspaceConfig config;
        config.hasExecutionMode = true;
        config.executionMode = "external_terminal";
        config.hasTerminalCommand = true;
        config.terminalCommand = mockTermPath;

        QSignalSpy finishedSpy(&manager, &CinderDE::BuildRunManager::processFinished);
        QSignalSpy outputSpy(&manager, &CinderDE::BuildRunManager::outputReceived);

        manager.run(scriptPath, spaceDirPath, config);

        QVERIFY(finishedSpy.wait(3000));
        QCOMPARE(finishedSpy.count(), 1);

        QFile logFile(logPath);
        QVERIFY(logFile.open(QIODevice::ReadOnly));
        QString logContent = QString::fromUtf8(logFile.readAll()).trimmed();
        QCOMPARE(logContent, QString("EXECUTED_SUCCESSFULLY"));

        bool foundLaunchedMsg = false;
        for (const auto &call : outputSpy) {
            if (call.at(0).toString().contains("EXTERNAL TERMINAL LAUNCHED SUCCESSFULLY")) {
                foundLaunchedMsg = true;
                break;
            }
        }
        QVERIFY(foundLaunchedMsg);
    }
};

QTEST_MAIN(TestBuildRunManager)
#include "test_build_run_manager.moc"
