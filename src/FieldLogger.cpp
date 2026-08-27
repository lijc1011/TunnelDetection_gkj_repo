#include "FieldLogger.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QMessageLogContext>
#include <QMutex>
#include <QMutexLocker>
#include <QTextStream>
#include <QThread>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace {

QMutex g_logMutex;
QFile* g_logFile = nullptr;

const char* levelName(QtMsgType type)
{
    switch (type) {
    case QtDebugMsg: return "DEBUG";
    case QtInfoMsg: return "INFO";
    case QtWarningMsg: return "WARN";
    case QtCriticalMsg: return "ERROR";
    case QtFatalMsg: return "FATAL";
    }
    return "UNKNOWN";
}

void fieldMessageHandler(QtMsgType type, const QMessageLogContext& context,
                         const QString& message)
{
    const QString line = QStringLiteral("%1 [%2] [T%3] %4 (%5:%6)\n")
                             .arg(QDateTime::currentDateTime().toString(Qt::ISODateWithMs))
                             .arg(QString::fromLatin1(levelName(type)))
                             .arg(reinterpret_cast<quintptr>(QThread::currentThreadId()), 0, 16)
                             .arg(message)
                             .arg(QString::fromUtf8(context.file ? context.file : "-"))
                             .arg(context.line);
    {
        QMutexLocker locker(&g_logMutex);
        if (g_logFile && g_logFile->isOpen()) {
            g_logFile->write(line.toUtf8());
            g_logFile->flush();
        }
    }
#if defined(_WIN32)
    OutputDebugStringW(reinterpret_cast<const wchar_t*>(line.utf16()));
#endif
    if (type == QtFatalMsg) abort();
}

} // namespace

void FieldLogger::install()
{
    QMutexLocker locker(&g_logMutex);
    if (g_logFile) return;
    const QString logDir = QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("logs"));
    QDir().mkpath(logDir);
    const QString logPath = QDir(logDir).filePath(
        QStringLiteral("TunnelDetection_%1.log")
            .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss"))));
    g_logFile = new QFile(logPath);
    if (!g_logFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        delete g_logFile;
        g_logFile = nullptr;
    }
    qInstallMessageHandler(fieldMessageHandler);
}

void FieldLogger::shutdown()
{
    qInstallMessageHandler(nullptr);
    QMutexLocker locker(&g_logMutex);
    if (g_logFile) {
        g_logFile->close();
        delete g_logFile;
        g_logFile = nullptr;
    }
}
