#ifndef CINDERDE_CODE_COMPLETION_H
#define CINDERDE_CODE_COMPLETION_H

#include <QListWidget>
#include <QSet>
#include <QStringList>
#include "cinderde/syntax_highlighter.h"
#include "cinderde/language_pack.h"

namespace CinderDE {

class CodeEditor;

class CodeCompletionPopup : public QListWidget {
    Q_OBJECT

public:
    explicit CodeCompletionPopup(CodeEditor *editor);

    void updateSuggestions(const QString &prefix, Language lang);
    void updateSuggestions(const QString &prefix, const LanguageDefinition &def);
    void updateLspSuggestions(const QString &prefix, const QList<struct LspCompletionItem> &items);
    void insertCurrentSuggestion();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    CodeEditor *m_editor;
    QString m_currentPrefix;

    QStringList getKeywordsForLanguage(Language lang) const;
    QStringList getKeywordsForLanguage(const LanguageDefinition &def) const;
    QSet<QString> extractDocumentIdentifiers() const;
    void positionPopup();
};

} // namespace CinderDE

#endif // CINDERDE_CODE_COMPLETION_H
