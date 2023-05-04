format: xml
entries: feed/entry
introduction: static const QRegularExpression arxivIdVersion(QStringLiteral("v(?<version>[1-9]\\d*)$"));
field[Entry::ftYear]: []() {
                         bool ok = false;
                         const int year = {{updated}}.left(4).toInt(&ok);
                         if (ok && year >= 1600 && year < 2100)
                            return QString::number(year);
                         else
                            return QString();
                      }()
field[Entry::ftMonth]: OnlineSearchAbstract::monthToMacroKeyText({{updated}}.mid(5, 2))
field[Entry::ftNote]: {{arxiv:comment}}
field[Entry::ftTitle]: {{title}}
field[Entry::ftAbstract]: {{summary}}
value[Entry::ftAuthor]: []() {
                           Value v;
                           for (const QString &author : {{QStringList:author/name}})
                              v.append(FileImporterBibTeX::personFromString(author));
                           return v;
                        }()
field[Entry::ftDOI]: []() {
                        const QRegularExpressionMatch m = KBibTeX::doiRegExp.match({{arxiv:doi}});
                        if (m.hasMatch())
                           return m.captured();
                        else
                           return QString(QStringLiteral("10.48550/arXiv.")).append({{id}}.replace(QStringLiteral("http://arxiv.org/abs/"), QString()).replace(QStringLiteral("https://arxiv.org/abs/"), QString()).replace(arxivIdVersion, QString()));
                     }()
field[Entry::ftUrl]: []() {
                        if ({{id}}.startsWith(QStringLiteral("http")))
                           return {{id}};
                        else if ({{QStringList:link/@href}}.isEmpty())
                           return QString();
                        else
                           return {{QStringList:link/@href}}.first();
                     }()
field["archivePrefix"]: {QStringLiteral("arXiv")}
field["eprint"]: {{id}}.replace(QStringLiteral("http://arxiv.org/abs/"), QString()).replace(QStringLiteral("https://arxiv.org/abs/"), QString()).replace(arxivIdVersion, QString())
valueItemType["eprint"]: VerbatimText
field["primaryClass"]: {{arxiv:primary_category/@term}}
valueItemType["primaryClass"]: VerbatimText
postprocessingfields: const QString journalref = {{arxiv:journal_ref}};
                      if (!journalref.isEmpty())
                         evaluateJournal(journalref, entry);
entrytype: journalref.isEmpty() ? Entry::etMisc : Entry::etArticle
entryid: QStringLiteral("arXiv:") + {{id}}.replace(QStringLiteral("http://arxiv.org/abs/"), QString()).replace(QStringLiteral("https://arxiv.org/abs/"), QString())
