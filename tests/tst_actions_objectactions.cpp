#include <actcraft/actions/commandregistry.h>
#include <actcraft/actions/objectactions.h>

#include <QSet>
#include <QtTest>

using namespace actcraft::actions;

namespace {
const ActionCategory kCategories[] = {ActionCategory::Command, ActionCategory::Page, ActionCategory::Object,
                                      ActionCategory::Document};
} // namespace

class TstActionsObjectActions : public QObject
{
    Q_OBJECT
    ObjectActionCatalog m_catalog;
    CommandRegistry m_registry;
private slots:
    void initTestCase()
    {
        QString error;
        QVERIFY2(m_catalog.load(defaultObjectActionsJson(), error), qPrintable(error));
        QVERIFY2(m_registry.loadDeclarations(defaultCommandsJson(), error), qPrintable(error));
    }
    void fourCategoriesNoVote()
    {
        for (auto c : kCategories)
            QVERIFY(!m_catalog.inCategory(c).isEmpty());
        for (auto c : kCategories)
            for (const auto* a : m_catalog.inCategory(c))
                QVERIFY2(!a->id.contains(QStringLiteral("vote")) && !a->id.contains(QStringLiteral("poll")),
                         qPrintable(a->id));
    }
    void everyCommandActionNamesARegisteredCommand()
    {
        for (auto c : kCategories)
            for (const auto* a : m_catalog.inCategory(c))
                if (!a->command.isEmpty())
                    QVERIFY2(m_registry.find(a->command), qPrintable(a->id + QStringLiteral(" -> ") + a->command));
        QVERIFY(!m_catalog.find(QStringLiteral("page.duplicate-next"))); // commande non déclarée : pas d'entrée
    }
    void parametersAreTyped()
    {
        const QSet<QString> types = {QStringLiteral("number"), QStringLiteral("angle"), QStringLiteral("text"),
                                     QStringLiteral("url"),    QStringLiteral("file"),  QStringLiteral("page"),
                                     QStringLiteral("translucency")};
        const auto* gotoPage = m_catalog.find(QStringLiteral("page.goto"));
        QVERIFY(gotoPage);
        QCOMPARE(gotoPage->parameters.size(), 1);
        QCOMPARE(gotoPage->parameters[0].type, QStringLiteral("page"));
        for (auto c : kCategories)
            for (const auto* a : m_catalog.inCategory(c))
                for (const auto& p : a->parameters)
                    QVERIFY2(types.contains(p.type), qPrintable(a->id));
    }
    void dragDropListHasEighteen()
    {
        QCOMPARE(m_catalog.dragDropList().size(), 18); // sans les deux entrées de vote (D9)
        QVERIFY(m_catalog.find(QStringLiteral("command.tool.pen"))->dragDrop);
    }
    void labelsAreOurs()
    {
        QVERIFY(m_catalog.find(QStringLiteral("command.tool.reveal-ink")));
        QCOMPARE(m_catalog.find(QStringLiteral("command.tool.reveal-ink"))->label, QStringLiteral("Encre révélatrice"));
        QVERIFY(!m_catalog.find(QStringLiteral("command.planet")));
    }
    void idsAreUnique()
    {
        QSet<QString> seen;
        for (auto c : kCategories)
            for (const auto* a : m_catalog.inCategory(c)) {
                QVERIFY2(!seen.contains(a->id), qPrintable(a->id));
                seen.insert(a->id);
            }
        QCOMPARE(seen.size(), m_catalog.size());
    }
};

QTEST_MAIN(TstActionsObjectActions)
#include "tst_actions_objectactions.moc"
