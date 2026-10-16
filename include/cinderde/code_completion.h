#ifndef CINDERDE_CODE_COMPLETION_H
#define CINDERDE_CODE_COMPLETION_H

#include <QListWidget>
#include <QSet>
#include <QStringList>
#include "cinderde/syntax_highlighter.h"

namespace CinderDE {

class CodeEditor;

class CodeCompletionPopup : public QListWidget {
    Q_OBJECT

public:
    explicit CodeCompletionPopup(CodeEditor *editor);

    void updateSuggestions(const QString &prefix, Language lang);
    void insertCurrentSuggestion();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    CodeEditor *m_editor;
    QString m_currentPrefix;

    QStringList getKeywordsForLanguage(Language lang) const;
    QSet<QString> extractDocumentIdentifiers() const;
    void positionPopup();
};

} // namespace CinderDE

#endif // CINDERDE_CODE_COMPLETION_H
