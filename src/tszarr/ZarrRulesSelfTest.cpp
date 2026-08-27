#include "../ZarrSessionRules.h"

#include <QCoreApplication>

#include <cstdio>

namespace {

bool expect(bool condition, const char* message)
{
    if (condition) return true;
    std::printf("RULE SELF-TEST FAIL: %s\n", message);
    return false;
}

} // namespace

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    bool ok = true;
    ok = expect(ZarrSessionRules::deriveChetouDirection(QStringLiteral("增加"), QStringLiteral("大里程"))
                    == QStringLiteral("正向"), "increase/head-large should be forward") && ok;
    ok = expect(ZarrSessionRules::deriveChetouDirection(QStringLiteral("增加"), QStringLiteral("小里程"))
                    == QStringLiteral("反向"), "increase/head-small should be reverse") && ok;
    ok = expect(ZarrSessionRules::deriveChetouDirection(QStringLiteral("减小"), QStringLiteral("小里程"))
                    == QStringLiteral("正向"), "decrease/head-small should be forward") && ok;
    ok = expect(ZarrSessionRules::deriveChetouDirection(QStringLiteral("减小"), QStringLiteral("大里程"))
                    == QStringLiteral("反向"), "decrease/head-large should be reverse") && ok;

    const ZarrSessionRulePlan rule1 = ZarrSessionRules::compute(
        QString(), QStringLiteral("下行"), QStringLiteral("增大"),
        QStringLiteral("正向"), QStringLiteral("A"));
    ok = expect(rule1.ok && !rule1.incomplete && rule1.ruleId == 1
                    && rule1.channels.size() == 4 && !rule1.channels.first().reorder,
                "rule 1") && ok;

    const ZarrSessionRulePlan rule3 = ZarrSessionRules::compute(
        QString(), QStringLiteral("下行"), QStringLiteral("减小"),
        QStringLiteral("正向"), QStringLiteral("A"));
    ok = expect(rule3.ok && rule3.incomplete && rule3.ruleId == 3, "rule 3 must be incomplete") && ok;

    const ZarrSessionRulePlan rule4 = ZarrSessionRules::compute(
        QString(), QStringLiteral("下行"), QStringLiteral("减小"),
        QStringLiteral("反向"), QStringLiteral("A"));
    ok = expect(rule4.ok && !rule4.incomplete && rule4.ruleId == 4
                    && !rule4.channels.isEmpty() && rule4.channels.first().reorder,
                "rule 4 must require reorder") && ok;

    const ZarrSessionRulePlan invalid = ZarrSessionRules::compute(
        QString(), QStringLiteral("下行"), QStringLiteral("增大"),
        QStringLiteral("正向"), QStringLiteral("B"));
    ok = expect(!invalid.ok && !invalid.error.isEmpty(), "mismatched machine must fail") && ok;

    if (!ok) return 1;
    std::printf("ZARR RULE SELF-TEST PASS\n");
    return 0;
}
