format: json
entries: items

field[Entry::ftTitle]: []() {
            const QString title = {{volumeInfo/title}}.trimmed();
            const QString subTitle = {{volumeInfo/subtitle}}.trimmed();
            if (subTitle.isEmpty())
                return title;
            else if (!title.isEmpty()) {
                // No colon as separator between title and subtitle if title ends with '?' or '!'
                static const QString noSeparatorAfter {QStringLiteral("?!")};
                const QString separator {noSeparatorAfter.contains(title[title.length()-1])?QStringLiteral(" "):QStringLiteral(": ")};
                return QString(QStringLiteral("%1%2%3")).arg(title, separator, subTitle);
            } else
                return QString();
        }()
value[Entry::ftAuthor]: []() {
            Value v;
            for (const QString &author : {{QStringList:volumeInfo/authors}}) {
                v.append(FileImporterBibTeX::personFromString(author));
            }
            return v;
        }()
field[QStringLiteral("x-google-id")]: {{id}}
field[Entry::ftPublisher]: {{volumeInfo/publisher}}
field[Entry::ftYear]: []() {
            bool ok = false;
            const int year = {{volumeInfo/publishedDate}}.left(4).toInt(&ok);
            if (ok && year >= 1600 && year < 2100)
                return QString::number(year);
            else
                return QString();
        }()
field[Entry::ftISBN]: []() {
            // Unfortunately, need to hard-code several alternatives here
            const QSet<QString> alternativesISBN { ISBN::locate({{volumeInfo/infoLink}}), ISBN::locate({{volumeInfo/previewLink}}), ISBN::locate({{volumeInfo/industryIdentifiers/0/identifier}}), ISBN::locate({{volumeInfo/industryIdentifiers/1/identifier}}), ISBN::locate({{volumeInfo/industryIdentifiers/2/identifier}}), ISBN::locate({{volumeInfo/industryIdentifiers/3/identifier}}), ISBN::locate({{volumeInfo/industryIdentifiers/4/identifier}}), ISBN::locate({{volumeInfo/industryIdentifiers/5/identifier}}) };
            QMap<int, QString> lengthToISBN; //< some sort of list comprehension would be nice here
            for (const QString &isbn : alternativesISBN)
                lengthToISBN[isbn.length()] = isbn;
            if (lengthToISBN.contains(13))
                // If available, pick the 13-digit ISBN first
                return lengthToISBN[13];
            else if (lengthToISBN.contains(10))
                // Alternatively, pick the 10-digit ISBN first
                return lengthToISBN[10];
            else
                // No ISBN found
                return QString();
        }()
field[Entry::ftUrl]: QStringLiteral("https://books.google.com/books?id=") + {{id}}
entrytype: []() {
            const QString printType = {{volumeInfo/printType}};
            if (printType == QStringLiteral("BOOK"))
                return Entry::etBook;
            else
                return Entry::etMisc;
        }()
entryid: [entry]() {
           const QString isbn{ {{field[Entry::ftISBN]}} };
           const QString id{ {{id}} };
           return QString(QStringLiteral("GoogleBooks:%1")).arg(isbn.isEmpty()?id:isbn);
        }()
