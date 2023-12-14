#ifndef FILEIMPORTER_P_H
#define FILEIMPORTER_P_H

#define check_if_iodevice_invalid(iodevice)    if (iodevice == nullptr || (!iodevice->isReadable() && !iodevice->open(QIODevice::ReadOnly))) { \
        qCWarning(LOG_KBIBTEX_IO) << "Input device not readable"; \
        Q_EMIT message(MessageSeverity::Error, QStringLiteral("Input device not readable")); \
        return nullptr; \
    } else if (iodevice->atEnd() || iodevice->size() <= 0) { \
        qCWarning(LOG_KBIBTEX_IO) << "Input device at end or does not contain any data"; \
        Q_EMIT message(MessageSeverity::Warning, QStringLiteral("Input device at end or does not contain any data")); \
        return new File(); \
    }

#endif // FILEIMPORTER_P_H
