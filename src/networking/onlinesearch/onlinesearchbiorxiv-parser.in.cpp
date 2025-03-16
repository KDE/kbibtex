format: json
entries: collection

field[Entry::ftDOI]: {{doi}}

field[Entry::ftTitle]: {{title}}
value[Entry::ftAuthor]: []() {
            Value v;
            static const QRegularExpression commaSpaceRegExp{QStringLiteral("\\s*;\\s+")};
            const QString authors{ {{authors}} };
            const QStringList names{authors.split(commaSpaceRegExp, Qt::SkipEmptyParts)};
            for (const QString &author : names) {
                v.append(FileImporterBibTeX::personFromString(author));
            }
            return v;
        }()
field[Entry::ftYear]: []() {
            bool ok = false;
            const int year = {{date}}.left(4).toInt(&ok);
            if (ok && year >= 1600 && year < 2100)
                return QString::number(year);
            else
                return QString();
        }()
field[Entry::ftAbstract]: {{abstract}}


entrytype: []() {
           return Entry::etMisc;
        }()
entryid: [entry]() {
           const QString server{ {{server}} };
           const QString doi{ {{doi}} };
           return QString(QStringLiteral("%1:%2")).arg(server, doi);
        }()
