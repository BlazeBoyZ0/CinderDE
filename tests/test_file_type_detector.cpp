#include <QtTest>
#include <QTemporaryFile>
#include "cinderde/file_type_detector.h"

class TestFileTypeDetector : public QObject {
    Q_OBJECT

private slots:
    void testExtensionDetection() {
        CinderDE::FileTypeInfo cppInfo = CinderDE::FileTypeDetector::detectType("main.cpp");
        QCOMPARE(cppInfo.name, QString("C++ Source"));

        CinderDE::FileTypeInfo pyInfo = CinderDE::FileTypeDetector::detectType("script.py");
        QCOMPARE(pyInfo.name, QString("Python Source"));

        CinderDE::FileTypeInfo hInfo = CinderDE::FileTypeDetector::detectType("header.h");
        QCOMPARE(hInfo.name, QString("Header File"));

        CinderDE::FileTypeInfo cmakeInfo = CinderDE::FileTypeDetector::detectType("CMakeLists.txt");
        QCOMPARE(cmakeInfo.name, QString("CMake Build Script"));

        CinderDE::FileTypeInfo unknownInfo = CinderDE::FileTypeDetector::detectType("file.xyz");
        QCOMPARE(unknownInfo.name, QString("XYZ File"));
    }

    void testBinaryDetection() {
        QTemporaryFile textFile;
        QVERIFY(textFile.open());
        textFile.write("Hello World!\nThis is a plain text file.");
        textFile.close();

        QVERIFY(!CinderDE::FileTypeDetector::isLikelyBinary(textFile.fileName()));

        QTemporaryFile binaryFile;
        QVERIFY(binaryFile.open());
        char binData[] = {0x7f, 'E', 'L', 'F', 0x00, 0x01, 0x01, 0x00};
        binaryFile.write(binData, sizeof(binData));
        binaryFile.close();

        QVERIFY(CinderDE::FileTypeDetector::isLikelyBinary(binaryFile.fileName()));
    }
};

QTEST_MAIN(TestFileTypeDetector)
#include "test_file_type_detector.moc"
