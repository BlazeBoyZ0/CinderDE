#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <QSignalSpy>
#include "cinderde/build_run_manager.h"
#include "cinderde/workspace_config.h"

class TestV05Features : public QObject {
    Q_OBJECT

private slots:
    void testStandaloneCppBuildAndRun() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        QString cppPath = QDir(tempDir.path()).filePath("hello.cpp");
        QFile file(cppPath);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("#include <iostream>\nint main() { std::cout << \"HELLO FROM CINDERDE V0.5\" << std::endl; return 0; }\n");
        file.close();

        CinderDE::BuildRunManager manager;
        CinderDE::WorkspaceConfig config;

        QSignalSpy finishedSpy(&manager, &CinderDE::BuildRunManager::processFinished);
        QSignalSpy outputSpy(&manager, &CinderDE::BuildRunManager::outputReceived);

        manager.buildAndRun(cppPath, "", config);

        // Wait for both build and run processes to finish
        QTRY_COMPARE_WITH_TIMEOUT(finishedSpy.count(), 2, 10000);

        bool foundOutput = false;
        for (const auto &call : outputSpy) {
            QString txt = call.at(0).toString();
            if (txt.contains("HELLO FROM CINDERDE V0.5")) {
                foundOutput = true;
                break;
            }
        }
        QVERIFY(foundOutput);
    }

    void testBuildFailurePreventsExecution() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        QString cppPath = QDir(tempDir.path()).filePath("bad.cpp");
        QFile file(cppPath);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("invalid cpp code syntax error line 1;\n");
        file.close();

        CinderDE::BuildRunManager manager;
        CinderDE::WorkspaceConfig config;

        QSignalSpy finishedSpy(&manager, &CinderDE::BuildRunManager::processFinished);

        manager.buildAndRun(cppPath, "", config);

        // Wait for build to finish (should fail)
        QTRY_COMPARE_WITH_TIMEOUT(finishedSpy.count(), 1, 10000);

        // Process should finish with non-zero exit code on build failure
        QCOMPARE(finishedSpy.count(), 1);
        int exitCode = finishedSpy.at(0).at(0).toInt();
        QVERIFY(exitCode != 0);

        // Run process MUST NOT have been launched
        QCOMPARE(manager.state(), CinderDE::BuildRunManager::Idle);
    }

    void testCMakeWorkspaceDetection() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        QString cmakePath = QDir(tempDir.path()).filePath("CMakeLists.txt");
        QFile file(cmakePath);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("cmake_minimum_required(VERSION 3.16)\nproject(TestApp)\n");
        file.close();

        CinderDE::WorkspaceConfig config;
        auto cmds = CinderDE::BuildRunManager::resolveCommands("", tempDir.path(), config);

        QVERIFY(cmds.first.contains("cmake"));
    }

    void testCMakeWorkspaceBuildAndRun() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        QString cmakePath = QDir(tempDir.path()).filePath("CMakeLists.txt");
        QFile fileCMake(cmakePath);
        QVERIFY(fileCMake.open(QIODevice::WriteOnly));
        fileCMake.write(
            "cmake_minimum_required(VERSION 3.16)\n"
            "project(SampleApp LANGUAGES CXX)\n"
            "add_executable(sample_app main.cpp)\n"
            "add_executable(sample_app_tests test.cpp)\n"
        );
        fileCMake.close();

        QString mainPath = QDir(tempDir.path()).filePath("main.cpp");
        QFile fileMain(mainPath);
        QVERIFY(fileMain.open(QIODevice::WriteOnly));
        fileMain.write("#include <iostream>\nint main() { std::cout << \"RUNNING SAMPLE APP BINARY\" << std::endl; return 0; }\n");
        fileMain.close();

        QString testPath = QDir(tempDir.path()).filePath("test.cpp");
        QFile fileTest(testPath);
        QVERIFY(fileTest.open(QIODevice::WriteOnly));
        fileTest.write("#include <iostream>\nint main() { std::cout << \"RUNNING TEST BINARY\" << std::endl; return 0; }\n");
        fileTest.close();

        CinderDE::BuildRunManager manager;
        CinderDE::WorkspaceConfig config;

        QSignalSpy finishedSpy(&manager, &CinderDE::BuildRunManager::processFinished);
        QSignalSpy outputSpy(&manager, &CinderDE::BuildRunManager::outputReceived);

        manager.buildAndRun("", tempDir.path(), config);

        // Wait for both cmake build and sample_app executable run to finish
        QTRY_COMPARE_WITH_TIMEOUT(finishedSpy.count(), 2, 15000);

        bool foundSampleAppOutput = false;
        bool foundTestAppOutput = false;

        for (const auto &call : outputSpy) {
            QString txt = call.at(0).toString();
            if (txt.contains("RUNNING SAMPLE APP BINARY")) {
                foundSampleAppOutput = true;
            }
            if (txt.contains("RUNNING TEST BINARY")) {
                foundTestAppOutput = true;
            }
        }

        QVERIFY(foundSampleAppOutput);
        QVERIFY(!foundTestAppOutput);
    }
};

QTEST_MAIN(TestV05Features)
#include "test_v05_features.moc"
