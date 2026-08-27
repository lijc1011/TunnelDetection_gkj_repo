#include "QSaveThd.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStorageInfo>

#include <cstring>
#include <memory>
#include <new>

#ifndef TUNNEL_BUILD_ID
#define TUNNEL_BUILD_ID "dev"
#endif

namespace {

constexpr qint64 kFingerprintBlockBytes = 64 * 1024;

QString finalWithoutTmp(const QString& path)
{
    return path.endsWith(QStringLiteral(".tmp")) ? path.left(path.size() - 4) : path;
}

bool openBinaryWrite(const QString& path, FILE** out)
{
    if (!out) return false;
    *out = nullptr;
#if defined(_WIN32)
    return _wfopen_s(out, reinterpret_cast<const wchar_t*>(path.utf16()), L"wb") == 0
           && *out != nullptr;
#else
    const QByteArray encoded = QFile::encodeName(path);
    *out = std::fopen(encoded.constData(), "wb");
    return *out != nullptr;
#endif
}

} // namespace

QSaveThd::QSaveThd(QObject* parent, ICamera* pDel, CAMERAINFORSTRUCT camInfor, int ichl)
    : QObject(parent)
{
    m_ichl = ichl;
    m_cam = pDel;
    m_camInfor = camInfor;

    m_pPicture = new PICTUREIMAGESTRUCT;
    m_pPicture->iBufferSize = MAXFRAME * m_camInfor.iSizeFrame;
    m_pPicture->pDataBuffer = new BYTE[m_pPicture->iBufferSize];
    memset(m_pPicture->pDataBuffer, 0, m_pPicture->iBufferSize);
    m_pPicture->picInfor.iSizePicture = m_pPicture->iBufferSize;

    lpFile = nullptr;
    lpFile_camData = nullptr;
    lpFile_camIdx = nullptr;
    im_encode = nullptr;
    m_iPicCnt = 0;

    m_zarrMode = (g_program_setInfor.SaveZarr != 0);
    m_tsWriter = nullptr;
    m_pTransformBuf = nullptr;
    m_iPacketBytes = m_pPicture->iBufferSize;
    const int channels = (m_camInfor.isColorCamera != 0) ? 3 : 1;
    m_matType = (channels == 3) ? CV_8UC3 : CV_8UC1;
    m_matCols = m_camInfor.ImageWidth;
    m_matRows = (m_matCols > 0)
                    ? static_cast<int>(m_iPacketBytes / (static_cast<__int64>(m_matCols) * channels))
                    : 0;

    m_writeFailed = false;
    m_closeSucceeded = false;
    m_sessionOpen = false;
    m_normalize = false;
    m_zarrRuleId = 0;
    m_verifiedSamples = 0;
    UpdateZarrGeometry();
}

QSaveThd::~QSaveThd()
{
    if (m_sessionOpen) StopSave();
    stopThd();
    delete[] m_pTransformBuf;
    m_pTransformBuf = nullptr;
    delete[] m_pPicture->pDataBuffer;
    delete m_pPicture;
    m_pPicture = nullptr;
}

void QSaveThd::startThd()
{
    if (genOrgThreads.joinable()) return;
    m_isBreakThd_org.store(true);
    m_isCanRun_org.store(true);
    genOrgThreads = std::thread(genOrgJpgFun, this);
}

void QSaveThd::stopThd()
{
    m_isBreakThd_org.store(false);
    m_isCanRun_org.store(false);
    if (genOrgThreads.joinable() && genOrgThreads.get_id() != std::this_thread::get_id()) {
        genOrgThreads.join();
    }
}

void QSaveThd::pauseThd()
{
    m_isCanRun_org.store(false);
}

void QSaveThd::genOrgJpgFun(QSaveThd* pParameter)
{
    pParameter->sub_genOrgJpgFun();
}

void QSaveThd::sub_genOrgJpgFun()
{
    while (m_isBreakThd_org.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        if (!m_isCanRun_org.load()) continue;
        if (!g_isSave[m_ichl].load()) {
            emit thread_signal_stopsave(m_ichl);
            continue;
        }
        if (!m_cam->IsSave()) continue;

        m_cam->GetSaveDeqData(m_pPicture);
        double packetFrequency = -2;
        if (m_cam->GetCamFreg(packetFrequency)) {
            std::fill_n(m_pPicture->picInfor.dHFreqPrecision, MAXFRAME, packetFrequency);
        }

        if (!SaveFrameData(m_pPicture, m_iPicCnt)) {
            m_cam->StopAcquisition();
            g_isSave[m_ichl].store(false);
            StopSaveImg();
            emit thread_signal_stopsave(m_ichl);
            continue;
        }
        ++m_iPicCnt;
        memset(m_pPicture->pDataBuffer, 0, m_pPicture->iBufferSize);
        emit thread_signal(m_ichl);
    }
}

void QSaveThd::Capture(BYTE* pBits, int w, int h, QString imgName)
{
    const QString acqImg = QStringLiteral("D:\\") + imgName;
    const QByteArray path = acqImg.toLocal8Bit();
    std::vector<int> compressionParams{cv::IMWRITE_JPEG_QUALITY, 95};
    Mat mat(h, w, CV_8U, pBits);
    imwrite(path.constData(), mat, compressionParams);
}

void QSaveThd::ConfigureZarrRule(bool normalize, const ZarrChannelRule& rule,
                                 int ruleId, const QString& xingbie,
                                 const QString& licheng, const QString& chetou,
                                 const QString& machineId)
{
    m_normalize = normalize;
    m_zarrRule = rule;
    m_zarrRuleId = ruleId;
    m_zarrXingbie = xingbie;
    m_zarrLicheng = licheng;
    m_zarrChetou = chetou;
    m_zarrMachineId = machineId;
    UpdateZarrGeometry();
}

void QSaveThd::UpdateZarrGeometry()
{
    // ZarrNormalize=0 必须保持相机采集方向和几何尺寸不变。
    // 相机级 SaveTransform 是旧 Zarr 原型遗留配置，不得影响 raw Zarr 验证。
    const bool swapWH = m_normalize && m_zarrRule.rotate270cw;
    const int channels = (m_matType == CV_8UC3) ? 3 : 1;
    m_zarrRows = swapWH ? m_matCols : m_matRows;
    m_zarrCols = (swapWH ? static_cast<__int64>(m_matRows)
                         : static_cast<__int64>(m_matCols)) * channels;
}

BYTE* QSaveThd::TransformAndEncrypt(BYTE* pSrc)
{
    BYTE* pOut = pSrc;
    const bool ruleTransform = m_normalize
                               && (m_zarrRule.rotate270cw || m_zarrRule.flipCircum
                                   || m_zarrRule.flipLongit);
    if (ruleTransform && m_pTransformBuf != nullptr) {
        Mat src(m_matRows, m_matCols, m_matType, pSrc);
        const int channels = (m_matType == CV_8UC3) ? 3 : 1;
        Mat dst(static_cast<int>(m_zarrRows), static_cast<int>(m_zarrCols / channels),
                m_matType, m_pTransformBuf);
        if (m_zarrRule.rotate270cw) {
            cv::rotate(src, dst, cv::ROTATE_90_COUNTERCLOCKWISE);
        } else {
            src.copyTo(dst);
        }
        if (m_zarrRule.flipCircum) cv::flip(dst, dst, 0);
        if (m_zarrRule.flipLongit) cv::flip(dst, dst, 1);
        pOut = m_pTransformBuf;
    }
    if (im_encode != nullptr) {
        im_encode->encrypt_data(reinterpret_cast<char*>(pOut), m_iPacketBytes);
    }
    return pOut;
}

QByteArray QSaveThd::PacketFingerprint(const BYTE* data) const
{
    if (!data || m_iPacketBytes <= 0) return {};
    QCryptographicHash hash(QCryptographicHash::Sha256);
    const qint64 block = qMin<qint64>(kFingerprintBlockBytes, m_iPacketBytes);
    const qint64 offsets[] = {0, qMax<qint64>(0, (m_iPacketBytes - block) / 2),
                              qMax<qint64>(0, m_iPacketBytes - block)};
    for (const qint64 offset : offsets) {
        hash.addData(reinterpret_cast<const char*>(data + offset), block);
    }
    hash.addData(reinterpret_cast<const char*>(&m_iPacketBytes), sizeof(m_iPacketBytes));
    return hash.result();
}

BOOL QSaveThd::SaveFrameData(pPICTUREIMAGESTRUCT pPicImg, __int64 isn)
{
    std::lock_guard<std::mutex> guard(my_lock_org);
    if (lpFile_camIdx == nullptr || m_writeFailed) return FALSE;
    if (m_zarrMode) {
        if (m_tsWriter == nullptr) return FALSE;
        if ((isn % 32) == 0) {
            QString error;
            if (!CheckFreeSpace(m_strAllfile_zarrTmp, &error)) {
                SetFailure(error);
                return FALSE;
            }
        }
        BYTE* pOut = TransformAndEncrypt(pPicImg->pDataBuffer);
        char errBuf[1024] = {0};
        if (!tszarr_writer_write_packet(m_tsWriter, isn, pOut, errBuf, sizeof(errBuf))) {
            SetFailure(QStringLiteral("Zarr 写入第 %1 包失败: %2")
                           .arg(isn).arg(QString::fromUtf8(errBuf)));
            return FALSE;
        }
        const QByteArray fingerprint = PacketFingerprint(pOut);
        if (isn == 0) m_firstPacketFingerprint = fingerprint;
        m_lastPacketFingerprint = fingerprint;
    } else {
        if (lpFile_camData == nullptr) return FALSE;
        if (im_encode != nullptr) {
            im_encode->encrypt_data(reinterpret_cast<char*>(pPicImg->pDataBuffer),
                                    pPicImg->iBufferSize);
        }
        if (fwrite(pPicImg->pDataBuffer, pPicImg->iBufferSize, 1, lpFile_camData) != 1) {
            SetFailure(QStringLiteral("camdatx 写入第 %1 包失败").arg(isn));
            return FALSE;
        }
    }

    pPicImg->picInfor.iPicCnt = isn;
    const __int64 packetBytes = pPicImg->picInfor.iSizePicture;
    pPicImg->picInfor.startPosition = isn * packetBytes;
    pPicImg->picInfor.recordDimension = packetBytes;
    if (fwrite(&pPicImg->picInfor, sizeof(PICTUREINFORSTRUCT), 1, lpFile_camIdx) != 1) {
        SetFailure(QStringLiteral("camidx 写入第 %1 包失败").arg(isn));
        return FALSE;
    }
    return TRUE;
}

bool QSaveThd::StartSave(QString strPath, QString strFile)
{
    std::lock_guard<std::mutex> guard(my_lock_org);
    g_isSave[m_ichl].store(false);
    if (m_sessionOpen) {
        SetFailure(QStringLiteral("上一采集会话尚未收口"));
        return false;
    }
    m_lastError.clear();
    m_writeFailed = false;
    m_closeSucceeded = false;
    m_firstPacketFingerprint.clear();
    m_lastPacketFingerprint.clear();
    m_verifiedSamples = 0;
    m_iPicCnt = 0;
    m_pPicture->picInfor.startPosition = 0;
    UpdateZarrGeometry();

    if (m_iPacketBytes <= 0 || m_matRows <= 0 || m_matCols <= 0
        || m_zarrRows <= 0 || m_zarrCols <= 0) {
        SetFailure(QStringLiteral("相机包尺寸无效: bytes=%1, raw=%2x%3, zarr=%4x%5")
                       .arg(m_iPacketBytes).arg(m_matRows).arg(m_matCols)
                       .arg(m_zarrRows).arg(m_zarrCols));
        return false;
    }
    if (m_normalize && m_zarrRule.reorder) {
        SetFailure(QStringLiteral("规则 %1 需要跨包倒序，流式直写不能安全完成；请关闭在线转正，采集 raw Zarr")
                       .arg(m_zarrRuleId));
        return false;
    }

    const QString fullPath = QDir(strPath).filePath(strFile);
    if (!fullPath.endsWith(QStringLiteral(".camdatx.tmp"))) {
        SetFailure(QStringLiteral("采集文件名格式无效: %1").arg(fullPath));
        return false;
    }
    QString spaceError;
    if (!CheckFreeSpace(strPath, &spaceError)) {
        SetFailure(spaceError);
        return false;
    }
    if (!OpenCamIdxSaveFile(fullPath)) return false;
    const bool dataOpened = m_zarrMode ? OpenZarrSaveStore(fullPath)
                                      : OpenCamDataSaveFile(fullPath);
    if (!dataOpened) {
        CloseSaveFile();
        if (!m_strAllfile_camIdx.isEmpty() && QFileInfo(m_strAllfile_camIdx).size() == 0) {
            QFile::remove(m_strAllfile_camIdx);
        }
        return false;
    }
    m_sessionOpen = true;
    g_isSave[m_ichl].store(true);
    qInfo() << "Acquisition store ready:" << (m_zarrMode ? m_strAllfile_zarrTmp
                                                          : m_strAllfile_camData);
    return true;
}

void QSaveThd::CreateEncryptorFromFileName(const QString& strAllFile)
{
    QString keyName = QFileInfo(strAllFile).fileName();
    if (keyName.endsWith(QStringLiteral(".tmp"))) keyName.chop(4);
    const int first = keyName.indexOf('-');
    const int second = keyName.indexOf('-', first + 1);
    if (first <= 0 || second <= first + 1) {
        SetFailure(QStringLiteral("无法从文件名推导加密密钥: %1").arg(keyName));
        return;
    }
    const QString key = keyName.left(first) + '-' + keyName.mid(first + 1, second - first - 1);
    delete im_encode;
    im_encode = new ImageEncryptor(key.toStdString());
}

bool QSaveThd::OpenCamDataSaveFile(QString path)
{
    m_strAllfile_camData = path;
    if (QFileInfo::exists(path)) {
        SetFailure(QStringLiteral("拒绝覆盖已有 camdatx 临时文件: %1").arg(path));
        return false;
    }
    CreateEncryptorFromFileName(path);
    if (m_writeFailed || !openBinaryWrite(path, &lpFile_camData)) {
        SetFailure(QStringLiteral("无法创建 camdatx: %1").arg(path));
        return false;
    }
    return true;
}

bool QSaveThd::OpenZarrSaveStore(QString path)
{
    CreateEncryptorFromFileName(path);
    if (m_writeFailed) return false;
    const QString suffix = QStringLiteral(".camdatx.tmp");
    const QString base = path.left(path.size() - suffix.size());
    m_strAllfile_zarrFinal = base + QStringLiteral(".zarr");
    m_strAllfile_zarrTmp = m_strAllfile_zarrFinal + QStringLiteral(".tmp");
    if (QFileInfo::exists(m_strAllfile_zarrFinal) || QFileInfo::exists(m_strAllfile_zarrTmp)) {
        SetFailure(QStringLiteral("拒绝覆盖已有 Zarr 目标或临时目录: %1")
                       .arg(m_strAllfile_zarrFinal));
        return false;
    }

    const bool needsTransform = m_normalize
                                && (m_zarrRule.rotate270cw || m_zarrRule.flipCircum
                                    || m_zarrRule.flipLongit);
    if (needsTransform && m_pTransformBuf == nullptr) {
        m_pTransformBuf = new (std::nothrow) BYTE[m_iPacketBytes];
        if (!m_pTransformBuf) {
            SetFailure(QStringLiteral("无法分配 Zarr 变换缓冲区: %1 字节").arg(m_iPacketBytes));
            return false;
        }
    }

    const QString idxFinal = finalWithoutTmp(m_strAllfile_camIdx);
    QJsonObject attributes{
        {QStringLiteral("schema"), QStringLiteral("tunneldetection.zarr_acquisition.v2")},
        {QStringLiteral("build_id"), QStringLiteral(TUNNEL_BUILD_ID)},
        {QStringLiteral("camera_name"), m_camInfor.Name},
        {QStringLiteral("camera_index"), m_ichl + 1},
        {QStringLiteral("encrypted"), true},
        {QStringLiteral("frames_per_packet"), MAXFRAME},
        {QStringLiteral("lines_per_frame"), m_camInfor.ImageHeight},
        {QStringLiteral("image_width"), m_camInfor.ImageWidth},
        {QStringLiteral("color_channels"), (m_camInfor.isColorCamera != 0) ? 3 : 1},
        {QStringLiteral("save_transform"), 0},
        {QStringLiteral("configured_save_transform"), m_camInfor.SaveTransform},
        {QStringLiteral("raw_direction"), !m_normalize},
        {QStringLiteral("normalized"), m_normalize},
        {QStringLiteral("rule_id"), m_zarrRuleId},
        {QStringLiteral("slot"), m_zarrRule.slot},
        {QStringLiteral("index_file"), QFileInfo(idxFinal).fileName()},
    };
    if (m_normalize) {
        attributes.insert(QStringLiteral("xingbie"), m_zarrXingbie);
        attributes.insert(QStringLiteral("licheng"), m_zarrLicheng);
        attributes.insert(QStringLiteral("chetou"), m_zarrChetou);
        attributes.insert(QStringLiteral("machine_id"), m_zarrMachineId);
        attributes.insert(QStringLiteral("rotate_270_cw"), m_zarrRule.rotate270cw);
        attributes.insert(QStringLiteral("flip_circum"), m_zarrRule.flipCircum);
        attributes.insert(QStringLiteral("flip_longit"), m_zarrRule.flipLongit);
    }
    const QByteArray attrUtf8 = QJsonDocument(attributes).toJson(QJsonDocument::Compact);
    const QByteArray pathUtf8 = m_strAllfile_zarrTmp.toUtf8();

    TsZarrWriterOptions opts{};
    opts.packet_height = m_zarrRows;
    opts.packet_width = m_zarrCols;
    opts.initial_capacity = 256;
    opts.max_pending = 8;
    opts.overwrite_existing = 0;
    opts.attributes_json_utf8 = attrUtf8.constData();
    char errBuf[1024] = {0};
    m_tsWriter = tszarr_writer_create(pathUtf8.constData(), &opts, errBuf, sizeof(errBuf));
    if (!m_tsWriter) {
        SetFailure(QStringLiteral("无法创建 Zarr 临时目录 %1: %2")
                       .arg(m_strAllfile_zarrTmp, QString::fromUtf8(errBuf)));
        return false;
    }
    return true;
}

bool QSaveThd::OpenCamIdxSaveFile(QString path)
{
    const QString suffix = QStringLiteral(".camdatx.tmp");
    const QString base = path.left(path.size() - suffix.size());
    m_strAllfile_camIdx = base + QStringLiteral(".camidx.tmp");
    m_str_IdxFileName = QFileInfo(m_strAllfile_camIdx).fileName();
    if (QFileInfo::exists(m_strAllfile_camIdx)
        || QFileInfo::exists(finalWithoutTmp(m_strAllfile_camIdx))) {
        SetFailure(QStringLiteral("拒绝覆盖已有 camidx: %1").arg(m_strAllfile_camIdx));
        return false;
    }
    if (!openBinaryWrite(m_strAllfile_camIdx, &lpFile_camIdx)) {
        SetFailure(QStringLiteral("无法创建 camidx: %1").arg(m_strAllfile_camIdx));
        return false;
    }
    return true;
}

bool QSaveThd::CloseZarrWriter()
{
    if (m_tsWriter == nullptr) return !m_writeFailed;
    char errBuf[1024] = {0};
    const bool ok = tszarr_writer_close(m_tsWriter, m_iPicCnt, errBuf, sizeof(errBuf));
    m_tsWriter = nullptr;
    if (!ok) SetFailure(QStringLiteral("Zarr 收口失败: %1").arg(QString::fromUtf8(errBuf)));
    return ok;
}

bool QSaveThd::ValidateIndexCount(qint64* recordCount)
{
    const QFileInfo info(m_strAllfile_camIdx);
    const qint64 recordBytes = static_cast<qint64>(sizeof(PICTUREINFORSTRUCT));
    if (recordBytes <= 0 || info.size() % recordBytes != 0) {
        SetFailure(QStringLiteral("camidx 文件大小不是记录长度的整数倍: %1 字节").arg(info.size()));
        return false;
    }
    const qint64 count = info.size() / recordBytes;
    if (recordCount) *recordCount = count;
    if (count != m_iPicCnt) {
        SetFailure(QStringLiteral("camidx 记录数与 Zarr 包数不一致: idx=%1, zarr=%2")
                       .arg(count).arg(m_iPicCnt));
        return false;
    }
    return true;
}

bool QSaveThd::VerifyZarrSamples()
{
    m_verifiedSamples = 0;
    if (g_program_setInfor.ZarrVerifyOnClose == 0 || m_iPicCnt <= 0) return true;
    std::unique_ptr<BYTE[]> readBack(new (std::nothrow) BYTE[m_iPacketBytes]);
    if (!readBack) {
        SetFailure(QStringLiteral("抽样回读无法分配 %1 字节缓冲区").arg(m_iPacketBytes));
        return false;
    }
    const QByteArray pathUtf8 = m_strAllfile_zarrTmp.toUtf8();
    const qint64 samples[] = {0, m_iPicCnt - 1};
    for (int sample = 0; sample < 2; ++sample) {
        if (sample == 1 && m_iPicCnt == 1) break;
        const qint64 packetIndex = samples[sample];
        char errBuf[1024] = {0};
        if (!tszarr_read_packet(pathUtf8.constData(), packetIndex, readBack.get(),
                                m_zarrRows, m_zarrCols, errBuf, sizeof(errBuf))) {
            SetFailure(QStringLiteral("Zarr 抽样回读第 %1 包失败: %2")
                           .arg(packetIndex).arg(QString::fromUtf8(errBuf)));
            return false;
        }
        const QByteArray expected = sample == 0 ? m_firstPacketFingerprint
                                                : m_lastPacketFingerprint;
        if (PacketFingerprint(readBack.get()) != expected) {
            SetFailure(QStringLiteral("Zarr 抽样回读第 %1 包指纹不一致").arg(packetIndex));
            return false;
        }
        ++m_verifiedSamples;
    }
    return true;
}

bool QSaveThd::WriteSessionManifest(const QString& status, const QString& error)
{
    if (m_strAllfile_zarrTmp.isEmpty()) return false;
    QDir().mkpath(m_strAllfile_zarrTmp);
    qint64 indexRecords = -1;
    const QFileInfo idxInfo(m_strAllfile_camIdx);
    if (idxInfo.exists()) {
        indexRecords = idxInfo.size() / static_cast<qint64>(sizeof(PICTUREINFORSTRUCT));
    }
    QJsonObject manifest{
        {QStringLiteral("schema"), QStringLiteral("tunneldetection.zarr_acquisition.v2")},
        {QStringLiteral("build_id"), QStringLiteral(TUNNEL_BUILD_ID)},
        {QStringLiteral("status"), status},
        {QStringLiteral("finished_at"), QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)},
        {QStringLiteral("camera_name"), m_camInfor.Name},
        {QStringLiteral("camera_index"), m_ichl + 1},
        {QStringLiteral("packet_count"), m_iPicCnt},
        {QStringLiteral("index_records"), indexRecords},
        {QStringLiteral("index_record_bytes"), static_cast<qint64>(sizeof(PICTUREINFORSTRUCT))},
        {QStringLiteral("packet_rows"), m_zarrRows},
        {QStringLiteral("packet_row_bytes"), m_zarrCols},
        {QStringLiteral("verified_samples"), m_verifiedSamples},
        {QStringLiteral("encrypted"), true},
        {QStringLiteral("normalized"), m_normalize},
        {QStringLiteral("rule_id"), m_zarrRuleId},
        {QStringLiteral("slot"), m_zarrRule.slot},
        {QStringLiteral("error"), error},
    };
    const QString name = status == QStringLiteral("ok")
                             ? QStringLiteral("acquisition_manifest.json")
                             : QStringLiteral("acquisition_failure.json");
    QSaveFile file(QDir(m_strAllfile_zarrTmp).filePath(name));
    if (!file.open(QIODevice::WriteOnly)) return false;
    file.write(QJsonDocument(manifest).toJson(QJsonDocument::Indented));
    return file.commit();
}

bool QSaveThd::FinalizeZarrSession()
{
    bool ok = CloseZarrWriter();
    if (lpFile_camIdx != nullptr) {
        if (fflush(lpFile_camIdx) != 0) {
            SetFailure(QStringLiteral("camidx 刷盘失败"));
            ok = false;
        }
        fclose(lpFile_camIdx);
        lpFile_camIdx = nullptr;
    }
    qint64 indexRecords = 0;
    ok = ValidateIndexCount(&indexRecords) && ok;
    ok = VerifyZarrSamples() && ok;
    ok = !m_writeFailed && ok;
    if (!ok) {
        WriteSessionManifest(QStringLiteral("failed"), m_lastError);
        delete im_encode;
        im_encode = nullptr;
        return false;
    }
    if (!WriteSessionManifest(QStringLiteral("ok"), QString())) {
        SetFailure(QStringLiteral("无法写入 Zarr 采集清单"));
        WriteSessionManifest(QStringLiteral("failed"), m_lastError);
        delete im_encode;
        im_encode = nullptr;
        return false;
    }
    if (!QDir().rename(m_strAllfile_zarrTmp, m_strAllfile_zarrFinal)) {
        SetFailure(QStringLiteral("Zarr 临时目录无法收口: %1 -> %2")
                       .arg(m_strAllfile_zarrTmp, m_strAllfile_zarrFinal));
        WriteSessionManifest(QStringLiteral("failed"), m_lastError);
        delete im_encode;
        im_encode = nullptr;
        return false;
    }
    if (!ReNameIdxFile()) {
        QDir().rename(m_strAllfile_zarrFinal, m_strAllfile_zarrTmp);
        WriteSessionManifest(QStringLiteral("failed"), m_lastError);
        delete im_encode;
        im_encode = nullptr;
        return false;
    }
    m_str_ImgFileName = QFileInfo(m_strAllfile_zarrFinal).fileName();
    delete im_encode;
    im_encode = nullptr;
    return true;
}

bool QSaveThd::StopSave()
{
    g_isSave[m_ichl].store(false);
    return StopSaveImg();
}

bool QSaveThd::StopSaveImg()
{
    std::lock_guard<std::mutex> guard(my_lock_org);
    if (!m_sessionOpen) return m_closeSucceeded;
    Sleep(100);
    bool ok = false;
    if (m_zarrMode) {
        ok = FinalizeZarrSession();
    } else {
        CloseSaveFile();
        ok = !m_writeFailed && ReNameDatFile() && ReNameIdxFile();
    }
    m_sessionOpen = false;
    m_closeSucceeded = ok;
    if (!ok && m_lastError.isEmpty()) SetFailure(QStringLiteral("采集文件收口失败"));
    m_iPicCnt = 0;
    m_pPicture->picInfor.startPosition = 0;
    return ok;
}

void QSaveThd::CloseSaveFile()
{
    if (lpFile_camData != nullptr) {
        fclose(lpFile_camData);
        lpFile_camData = nullptr;
    }
    if (lpFile_camIdx != nullptr) {
        fclose(lpFile_camIdx);
        lpFile_camIdx = nullptr;
    }
    delete im_encode;
    im_encode = nullptr;
}

bool QSaveThd::ReNameDatFile()
{
    const QString newPath = finalWithoutTmp(m_strAllfile_camData);
    if (QFileInfo::exists(newPath) || !QFile::rename(m_strAllfile_camData, newPath)) {
        SetFailure(QStringLiteral("camdatx 临时文件无法收口: %1").arg(m_strAllfile_camData));
        return false;
    }
    m_str_ImgFileName = QFileInfo(newPath).fileName();
    return true;
}

bool QSaveThd::ReNameIdxFile()
{
    const QString newPath = finalWithoutTmp(m_strAllfile_camIdx);
    if (QFileInfo::exists(newPath) || !QFile::rename(m_strAllfile_camIdx, newPath)) {
        SetFailure(QStringLiteral("camidx 临时文件无法收口: %1").arg(m_strAllfile_camIdx));
        return false;
    }
    m_str_IdxFileName = QFileInfo(newPath).fileName();
    return true;
}

bool QSaveThd::CheckFreeSpace(const QString& path, QString* error) const
{
    QStorageInfo storage(path);
    if (!storage.isValid() || !storage.isReady()) {
        if (error) *error = QStringLiteral("无法读取输出盘状态: %1").arg(path);
        return false;
    }
    const qint64 minimum = qMax(1, g_program_setInfor.ZarrMinFreeGB)
                           * 1024LL * 1024LL * 1024LL;
    if (storage.bytesAvailable() < minimum) {
        if (error) {
            *error = QStringLiteral("输出盘剩余空间不足: 可用 %1 GB，要求至少 %2 GB")
                         .arg(storage.bytesAvailable() / 1024.0 / 1024.0 / 1024.0, 0, 'f', 1)
                         .arg(qMax(1, g_program_setInfor.ZarrMinFreeGB));
        }
        return false;
    }
    return true;
}

void QSaveThd::SetFailure(const QString& error)
{
    if (error.isEmpty()) return;
    if (!m_writeFailed) {
        m_lastError = error;
        emit thread_signal_saveerror(m_ichl, error);
    }
    m_writeFailed = true;
    qCritical() << "Channel" << (m_ichl + 1) << error;
}
