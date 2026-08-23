#include "Globals.h"

program_temp_struct gprogram_tempInfor;
program_set_struct g_program_setInfor;
CAMERAINFORSTRUCT g_cameraInfor[MAXCAMNUM];
std::vector<tunnel_data_struct> gtunnel_ledger_struct;
bool g_isConnect[MAXCAMNUM];
bool g_isShow[MAXCAMNUM];
std::atomic<bool> g_isSave[MAXCAMNUM];

std::atomic<bool> g_autoBrightnessEnabled{false}; // 默认手动亮度

double m_dmil_GYK;
int m_dspeed_GYK;
double m_dmil_GYK_Cor;

void writeToXmlElement(TiXmlElement *itemElement, const char *childNm, const char *childText)
{
    TiXmlElement *tmpElement = getXmlElement(itemElement, childNm);
    tmpElement->Clear();
    TiXmlText *tmpText = new TiXmlText(childText);
    tmpElement->LinkEndChild(tmpText);
}
TiXmlElement *getXmlElement(TiXmlElement *itemElement, const char *nm)
{
    TiXmlElement *ansElement = itemElement->FirstChildElement(nm);
    if (ansElement == NULL)
    {
        ansElement = new TiXmlElement(nm);
        itemElement->LinkEndChild(ansElement);
    }
    ansElement = itemElement->FirstChildElement(nm);
    return ansElement;
}
template <typename T> std::string stdtoString(const T &t)
{
    std::ostringstream oss; // 创建一个格式化输出流
    oss << t;          // 把值传递如流中
    return oss.str();
}
QStringList kuohao_to_list(QString str)
{
    QStringList strlistDest;
    if (str.isEmpty())
    {
        return strlistDest;
    }

    QStringList strlist = str.split(")(");
    ;
    for (int i = 0; i < MAXCAMNUM; i++)
    {
        if (i == 0)
        {
            QString tmp = strlist.at(i).section('(', 1, 1);
            QString iDest = tmp.section(',', 1, 1);
            strlistDest << iDest;
        }
        else if (i == MAXCAMNUM - 1)
        {
            QString tmp = strlist.at(i).section(')', 0, 0);
            QString iDest = tmp.section(',', 1, 1);
            strlistDest << iDest;
        }
        else
        {
            QString iDest = strlist.at(i).section(',', 1, 1);
            strlistDest << iDest;
        }
    }
    return strlistDest;
}
void kuohao_to_list(QString str, QStringList &xStringList, QStringList &yStringList)
{
    if (str.isEmpty())
    {
        return;
    }

    QStringList strlist = str.split(")(");
    ;
    for (int i = 0; i < strlist.size(); i++)
    {
        if (i == 0)
        {
            QString tmp = strlist.at(i).section('(', 1, 1);
            xStringList << tmp.section(',', 0, 0);
            yStringList << tmp.section(',', 1, 1);
        }
        else if (i == strlist.size() - 1)
        {
            QString tmp = strlist.at(i).section(')', 0, 0);
            xStringList << tmp.section(',', 0, 0);
            yStringList << tmp.section(',', 1, 1);
        }
        else
        {
            xStringList << strlist.at(i).section(',', 0, 0);
            yStringList << strlist.at(i).section(',', 1, 1);
        }
    }
    return;
}

imgName_struct Get_imgName_infor(QString imgName)
{
    imgName_struct imgName_s;

    int startPos = imgName.indexOf('_', 0);
    imgName_s.object_Type = imgName.mid(0, startPos); // 检测对象类型，隧道，声屏障

    QString imgName_tmp = imgName;
    imgName_tmp = imgName_tmp.mid(startPos + 1, imgName_tmp.length() - startPos);
    startPos = imgName_tmp.indexOf('_', 0);
    imgName_s.bureau_name = imgName_tmp.mid(0, startPos); // 路局名称

    imgName_tmp = imgName_tmp.mid(startPos + 1, imgName_tmp.length() - startPos);
    startPos = imgName_tmp.indexOf('_', 0);
    imgName_s.line_name = imgName_tmp.mid(0, startPos); // 线路名称

    imgName_tmp = imgName_tmp.mid(startPos + 1, imgName_tmp.length() - startPos);
    startPos = imgName_tmp.indexOf('_', 0);
    imgName_s.xingbie = imgName_tmp.mid(0, startPos); // 行别

    imgName_tmp = imgName_tmp.mid(startPos + 1, imgName_tmp.length() - startPos);
    startPos = imgName_tmp.indexOf('_', 0);
    imgName_s.start_station = imgName_tmp.mid(0, startPos); // 起始站名

    imgName_tmp = imgName_tmp.mid(startPos + 1, imgName_tmp.length() - startPos);
    startPos = imgName_tmp.indexOf('_', 0);
    imgName_s.end_station = imgName_tmp.mid(0, startPos); // 终到站名

    imgName_tmp = imgName_tmp.mid(startPos + 1, imgName_tmp.length() - startPos);
    startPos = imgName_tmp.indexOf('_', 0);
    imgName_s.set_time = imgName_tmp.mid(0, startPos); // 时间，年月日20240510

    imgName_tmp = imgName_tmp.mid(startPos + 1, imgName_tmp.length() - startPos);
    startPos = imgName_tmp.indexOf('.', 0);
    imgName_s.com_flg = imgName_tmp.mid(0, startPos); // 计算机标识01,02,03

    imgName_s.file_Suffix = imgName_tmp.mid(startPos + 1, imgName_tmp.length() - startPos);

    return imgName_s;
}
double k_mil_to_km(QString sMil)
{
    // k里程转化成米m
    double dmil_m = 0;
    int KPos = sMil.indexOf('K', 0);
    int jiaPos = sMil.indexOf('+', 0);
    QString kstr = sMil.mid(KPos + 1, jiaPos - 1); //
    QString mstr = sMil.mid(jiaPos + 1, sMil.length() - jiaPos);

    double d_kmil = kstr.toDouble();
    double d_mmil = mstr.toDouble() / 1000;

    dmil_m = d_kmil + d_mmil;
    return dmil_m;
}
