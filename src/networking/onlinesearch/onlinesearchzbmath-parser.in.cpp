format: xml
entries: OAI-PMH/ListRecords/record
introduction: static const QRegularExpression authorSepRegExp(QStringLiteral(";\\s*")), dashes(QStringLiteral("\\s*[-]+\\s*"));

# Entries with 'zbMATH Open Web Interface contents unavailable due to conflicting licenses' shall be skipped
entryvalidation: entry->contains(Entry::ftTitle) && !PlainTextValue::text(entry->value(Entry::ftTitle)).contains(QStringLiteral("contents unavailable due to conflicting licenses"))

field[Entry::ftTitle]: {{metadata/oai_zb_preview:zbmath/zbmath:document_title}}
value[Entry::ftAuthor]: []() {
                           const QStringList authorList{ {{metadata/oai_zb_preview:zbmath/zbmath:author}}.split(authorSepRegExp, Qt::SkipEmptyParts)};
                           Value v;
                           for (const auto &author : authorList)
                              v.append(FileImporterBibTeX::personFromString(author));
                           return v;
                        }()
field[Entry::ftJournal]: {{metadata/oai_zb_preview:zbmath/zbmath:serial/zbmath:serial_title}}
field[Entry::ftPublisher]: {{metadata/oai_zb_preview:zbmath/zbmath:serial/zbmath:serial_publisher}}
field[Entry::ftYear]: {{metadata/oai_zb_preview:zbmath/zbmath:publication_year}}
field[Entry::ftPages]: QString({{metadata/oai_zb_preview:zbmath/zbmath:pagination}}).replace(dashes, QStringLiteral("\u2013"))
value[Entry::ftKeywords]: []() {
                           Value v;
                           for (const QString &keyword : {{QStringList:metadata/oai_zb_preview:zbmath/zbmath:keywords/zbmath:keyword}})
                              v.append(QSharedPointer<Keyword>(new Keyword(keyword)));
                           return v;
                        }()
field[Entry::ftDOI]: {{metadata/oai_zb_preview:zbmath/zbmath:doi}}

entrytype: []() {
               if ({{metadata/oai_zb_preview:zbmath/zbmath:document_type}} == QStringLiteral("j"))
                  return Entry::etArticle;
               else if ({{metadata/oai_zb_preview:zbmath/zbmath:document_type}} == QStringLiteral("b"))
                  return Entry::etBook;
               else if ({{metadata/oai_zb_preview:zbmath/zbmath:document_type}} == QStringLiteral("a"))
                  return Entry::etInBook;
               else
                  return QString(QStringLiteral("zbmathdocumenttype%1")).arg({{metadata/oai_zb_preview:zbmath/zbmath:document_type}});
           }()
entryid: QStringLiteral("zbmath") + {{metadata/oai_zb_preview:zbmath/zbmath:document_id}}
