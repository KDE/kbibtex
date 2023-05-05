format: xml
entries: articles/article
field[Entry::ftYear]: {{publication_year}}
field[Entry::ftMonth]: []() {
                          const QStringList components = {{publication_date}}.split(QStringLiteral(" "));
                          if (components.length() == 3 && components[1].length() >= 3 && components[1][0].isLetter() && components[1][2].isLetter())
                             return OnlineSearchAbstract::monthToMacroKeyText(components[1]);
                          return QString();
                       }()
field[Entry::ftPublisher]: {{publisher}}
field[Entry::ftTitle]: {{title}}
field[Entry::ftBookTitle]: []() {
                              if ({{content_type}} == QStringLiteral("Books") || {{content_type}} == QStringLiteral("Conferences"))
                                 return {{publication_title}};
                              else
                                 return QString();
                           }()
field[Entry::ftAbstract]: {{abstract}}
value[Entry::ftAuthor]: []() {
                           Value v;
                           for (const QString &author : {{QStringList:authors/author/full_name}})
                              v.append(FileImporterBibTeX::personFromString(author));
                           return v;
                        }()
field[Entry::ftPages]: {{start_page}} + ({{end_page}}.isEmpty() ? QString() : (QChar(0x2013) + {{end_page}}))
field[Entry::ftISBN]: []() {
                         if (!{{QStringList:isbn_formats/isbn_format/value}}.isEmpty())
                            return {{QStringList:isbn_formats/isbn_format/value}}.first();
                         else
                            return QString();
                      }()
field[Entry::ftDOI]: {{doi}}
field[Entry::ftUrl]: {{html_url}}
value[Entry::ftKeywords]: []() {
                           Value v;
                           for (const QString &keyword : {{QStringList:index_terms/ieee_terms/term}})
                              v.append(QSharedPointer<Keyword>(new Keyword(keyword)));
                           return v;
                        }()
entrytype: []() {
              if ({{content_type}} == QStringLiteral("Journals"))
                 return Entry::etArticle;
              else if ({{content_type}} == QStringLiteral("Conferences"))
                 return Entry::etInProceedings;
              else if ({{content_type}} == QStringLiteral("Books")) {
                 if (!{{publication_title}}.isEmpty() && {{publication_title}} != {{title}})
                    return Entry::etInBook;
                 else
                    return Entry::etBook;
              } else
                 return Entry::etMisc;
           }()
entryid: QStringLiteral("ieee") + {{article_number}}
