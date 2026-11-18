#ifndef QTCOMPAT_H
#define QTCOMPAT_H

#include <QTextStream>

/**
 * Qt5 / Qt6 兼容工具头
 *
 * 本工程同时支持 Qt 5.15 与 Qt 6.x：
 *   - Qt5: QTextStream::setCodec("UTF-8")
 *   - Qt6: QTextStream::setEncoding(QStringConverter::Utf8)（setCodec 已移除）
 *
 * 统一用 setStreamUtf8(stream) 代替直接调 setCodec。
 */

inline void setStreamUtf8(QTextStream &stream)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    stream.setEncoding(QStringConverter::Utf8);
#else
    stream.setCodec("UTF-8");
#endif
}

#endif // QTCOMPAT_H
