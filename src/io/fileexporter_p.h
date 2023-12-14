#ifndef FILEEXPORTER_P_H
#define FILEEXPORTER_P_H

#define check_if_bibtexfile_or_iodevice_invalid(bibtexfile,iodevice)    if (bibtexfile == nullptr) { \
        qCWarning(LOG_KBIBTEX_IO) << "No bibliography to write given"; \
        return false; \
    } else if (iodevice == nullptr || (!iodevice->isWritable() && !iodevice->open(QIODevice::WriteOnly))) { \
        qCWarning(LOG_KBIBTEX_IO) << "Output device not writable"; \
        return false; \
    } else if (bibtexfile->isEmpty()) { \
        qCDebug(LOG_KBIBTEX_IO) << "Bibliography is empty"; \
    }

#define check_if_iodevice_invalid(iodevice)    if (iodevice == nullptr || (!iodevice->isWritable() && !iodevice->open(QIODevice::WriteOnly))) { \
        qCWarning(LOG_KBIBTEX_IO) << "Output device not writable"; \
        return false; \
    }

#endif // FILEEXPORTER_P_H
