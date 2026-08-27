#pragma once

#include <QList>
#include <QString>

struct ZarrChannelRule {
    int cameraId = 0;
    int slot = 0;
    bool rotate270cw = false;
    bool flipCircum = false;
    bool flipLongit = false;
    bool reorder = false;
};

struct ZarrSessionRulePlan {
    bool ok = false;
    bool incomplete = false;
    QString error;
    int ruleId = 0;
    QList<ZarrChannelRule> channels;
};

class ZarrSessionRules {
public:
    static ZarrSessionRulePlan compute(const QString& rulesJsonPath,
                                       const QString& xingbie,
                                       const QString& licheng,
                                       const QString& chetou,
                                       const QString& machineId);

    static bool channelForCamera(const ZarrSessionRulePlan& plan, int cameraId,
                                 ZarrChannelRule* out);

    // 采集界面的“车头朝向大/小里程”换算为规则表的“正向/反向”。
    static QString deriveChetouDirection(const QString& mileageDirection,
                                         const QString& headDirection);
};
