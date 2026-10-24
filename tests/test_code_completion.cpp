#include <QtTest>
#include "cinderde/editor_tab.h"
#include "cinderde/code_completion.h"
#include "cinderde/language_pack.h"
#include <QJsonDocument>

class TestCodeCompletion : public QObject {
    Q_OBJECT

private slots:
    void testCompletionPopupFilteringAndInsertion() {
        CinderDE::EditorTab tab;
        tab.setFilePath("main.cpp");

        auto *editor = tab.editor();
        editor->setPlainText("int myVariable = 42;\nmy");

        QTextCursor cursor = editor->textCursor();
        cursor.movePosition(QTextCursor::End);
        editor->setTextCursor(cursor);

        CinderDE::CodeCompletionPopup popup(editor);
        CinderDE::LanguageRegistry::instance().clear();
        const auto pack = QJsonDocument::fromJson(R"({"id":"custom-cpp","name":"Custom C++","extensions":["cpp"],"keywords":["int","return"]})").object();
        QVERIFY(CinderDE::LanguageRegistry::instance().loadFromJson(pack));
        popup.updateSuggestions("my", CinderDE::LanguageRegistry::instance().definitionForExtension("cpp"));

        QVERIFY(popup.count() > 0);

        bool foundMyVar = false;
        for (int i = 0; i < popup.count(); ++i) {
            if (popup.item(i)->text() == "myVariable") {
                foundMyVar = true;
                popup.setCurrentRow(i);
                break;
            }
        }
        QVERIFY(foundMyVar);

        popup.insertCurrentSuggestion();
        QCOMPARE(editor->toPlainText(), QString("int myVariable = 42;\nmyVariable"));
    }
};

QTEST_MAIN(TestCodeCompletion)
#include "test_code_completion.moc"
