#include "ZarrSessionRules.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace {

QString defaultRulesPath()
{
    const QStringList candidates{
        QDir(QCoreApplication::applicationDirPath()).filePath(
            QStringLiteral("config/tunzarr_rules.json")),
        QStringLiteral("config/tunzarr_rules.json"),
    };
    for (const QString& path : candidates) {
        if (QFile::exists(path)) return path;
    }
    return candidates.first();
}

} // namespace

ZarrSessionRulePlan ZarrSessionRules::compute(const QString& rulesJsonPath,
                                               const QString& xingbie,
                                               const QString& licheng,
                                               const QString& chetou,
                                               const QString& machineId)
{
    ZarrSessionRulePlan plan;
    const QString path = rulesJsonPath.isEmpty() ? defaultRulesPath() : rulesJsonPath;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        plan.error = QStringLiteral("无法打开 Zarr 转正规则: %1").arg(path);
        return plan;
    }
    QJsonParseError parseError{};
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (doc.isNull() || !doc.isObject()) {
        plan.error = QStringLiteral("Zarr 转正规则解析失败: %1").arg(parseError.errorString());
        return plan;
    }

    const QJsonObject root = doc.object();
    const QJsonObject machine = root.value(QStringLiteral("machines"))
                                    .toObject().value(machineId).toObject();
    if (machine.isEmpty()) {
        plan.error = QStringLiteral("未知采集机器编号: %1（应为 A 或 B）").arg(machineId);
        return plan;
    }

    const QJsonArray slotArray = machine.value(QStringLiteral("slots")).toArray();
    const QJsonArray cameras = machine.value(QStringLiteral("cameras_by_chetou"))
                                   .toObject().value(chetou).toArray();
    if (slotArray.isEmpty() || slotArray.size() != cameras.size()) {
        plan.error = QStringLiteral("机器 %1 的槽位/相机映射无效（%2）")
                         .arg(machineId, chetou);
        return plan;
    }

    QJsonObject matched;
    for (const QJsonValue& value : root.value(QStringLiteral("rules")).toArray()) {
        const QJsonObject rule = value.toObject();
        if (rule.value(QStringLiteral("xingbie")).toString() == xingbie
            && rule.value(QStringLiteral("licheng")).toString() == licheng
            && rule.value(QStringLiteral("chetou")).toString() == chetou
            && rule.value(QStringLiteral("machine")).toString() == machineId) {
            matched = rule;
            break;
        }
    }
    if (matched.isEmpty()) {
        plan.error = QStringLiteral("没有匹配的转正规则: %1/%2/%3/机器%4")
                         .arg(xingbie, licheng, chetou, machineId);
        return plan;
    }

    plan.ruleId = matched.value(QStringLiteral("id")).toInt();
    plan.incomplete = matched.value(QStringLiteral("incomplete")).toBool();
    if (plan.incomplete) {
        plan.ok = true;
        return plan;
    }

    bool flipCircum = false;
    bool flipLongit = false;
    for (const QJsonValue& value : matched.value(QStringLiteral("flip")).toArray()) {
        flipCircum = flipCircum || value.toString() == QStringLiteral("环向");
        flipLongit = flipLongit || value.toString() == QStringLiteral("纵向");
    }

    for (int i = 0; i < slotArray.size(); ++i) {
        ZarrChannelRule channel;
        channel.slot = slotArray.at(i).toInt();
        channel.cameraId = cameras.at(i).toInt();
        channel.rotate270cw = matched.value(QStringLiteral("rotate")).toInt() == 270;
        channel.flipCircum = flipCircum;
        channel.flipLongit = flipLongit;
        channel.reorder = matched.value(QStringLiteral("reorder")).toBool();
        plan.channels.append(channel);
    }
    plan.ok = true;
    return plan;
}

bool ZarrSessionRules::channelForCamera(const ZarrSessionRulePlan& plan, int cameraId,
                                        ZarrChannelRule* out)
{
    for (const ZarrChannelRule& channel : plan.channels) {
        if (channel.cameraId == cameraId) {
            if (out) *out = channel;
            return true;
        }
    }
    return false;
}

QString ZarrSessionRules::deriveChetouDirection(const QString& mileageDirection,
                                                const QString& headDirection)
{
    const bool increasing = mileageDirection == QStringLiteral("增加")
                            || mileageDirection == QStringLiteral("增大");
    const bool decreasing = mileageDirection == QStringLiteral("减小");
    const bool headToLarge = headDirection == QStringLiteral("大里程");
    const bool headToSmall = headDirection == QStringLiteral("小里程");
    if ((increasing && headToLarge) || (decreasing && headToSmall)) {
        return QStringLiteral("正向");
    }
    if ((increasing && headToSmall) || (decreasing && headToLarge)) {
        return QStringLiteral("反向");
    }
    return {};
}
