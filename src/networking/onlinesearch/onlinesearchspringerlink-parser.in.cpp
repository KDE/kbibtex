format: xml
entries: response/records/pam:message
field[Entry::ftTitle]: {{xhtml:head/pam:article/dc:title}}
field[Entry::ftAbstract]: {{xhtml:body}}
value[Entry::ftAuthor]: []() {
                           Value v;
                           for (const QString &author : {{QStringList:xhtml:head/pam:article/dc:creator}}) {
                              if (author.contains(QStringLiteral(",")))
                                 // Real person's name have comma to separate first from last name
                                 v.append(FileImporterBibTeX::personFromString(author));
                              else
                                 // Organizations' names do not have commas
                                 v.append(QSharedPointer<PlainText>(new PlainText(author)));
                           }
                           return v;
                        }()
field[Entry::ftBookTitle]: []() {
                           if ({{xhtml:head/pam:article/prism:contentType}}.startsWith(QStringLiteral("Chapter")))
                              return {{xhtml:head/pam:article/prism:publicationName}};
                           else
                              return QString();
                        }()
field[Entry::ftJournal]: []() {
                           if ({{xhtml:head/pam:article/prism:contentType}} == QStringLiteral("Article"))
                              return {{xhtml:head/pam:article/prism:publicationName}};
                           else
                              return QString();
                        }()
field[Entry::ftDOI]: {{xhtml:head/pam:article/prism:doi}}
field[Entry::ftISBN]: {{xhtml:head/pam:article/prism:isbn}}
field[Entry::ftISSN]: {{xhtml:head/pam:article/prism:issn}}
field[Entry::ftNumber]: {{xhtml:head/pam:article/prism:number}}
field[Entry::ftVolume]: {{xhtml:head/pam:article/prism:volume}}
field[Entry::ftPages]: {{xhtml:head/pam:article/prism:startingPage}} + ({{xhtml:head/pam:article/prism:endingPage}}.isEmpty() ? QString() : (QChar(0x2013) + {{xhtml:head/pam:article/prism:endingPage}}))
field[Entry::ftPublisher]: {{xhtml:head/pam:article/dc:publisher}}
field[Entry::ftYear]: {{xhtml:head/pam:article/prism:publicationDate}}.left(4)
value[Entry::ftKeywords]: []() {
                           Value v;
                           for (const QString &keyword : {{QStringList:xhtml:head/pam:article/dc:subject}})
                              v.append(QSharedPointer<Keyword>(new Keyword(keyword)));
                           return v;
                        }()
entrytype: []() {
            const QString prismContentType = {{xhtml:head/pam:article/prism:contentType}};
            if (prismContentType.startsWith(QStringLiteral("Chapter"))) {
                if (prismContentType.contains(QStringLiteral("ConferencePaper")))
                   return Entry::etInProceedings;
                else
                   return Entry::etInBook;
            } else if (prismContentType == QStringLiteral("Article"))
               return Entry::etArticle;
            else
               return Entry::etMisc;
         }()
entryid: QStringLiteral("PAM:") + {{xhtml:head/pam:article/dc:identifier}}.replace(QStringLiteral("doi:"), QString()).replace(QStringLiteral(","), QString())
