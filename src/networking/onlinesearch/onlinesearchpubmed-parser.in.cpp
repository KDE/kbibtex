format: xml
entries: PubmedArticleSet/PubmedArticle
field[Entry::ftAbstract]: {{MedlineCitation/Article/Abstract/AbstractText}}
field[Entry::ftYear]: {{MedlineCitation/Article/ArticleDate/Year}}
field[Entry::ftMonth]: OnlineSearchAbstract::monthToMacroKeyText({{MedlineCitation/Article/ArticleDate/Month}})
field["pii"]: {{PubmedData/ArticleIdList/ArticleId[@IdType=pii]}}
valueItemType["pii"]: VerbatimText
field["pmid"]: {{PubmedData/ArticleIdList/ArticleId[@IdType=pubmed]}}
valueItemType["pmid"]: VerbatimText
field[Entry::ftDOI]: {{PubmedData/ArticleIdList/ArticleId[@IdType=doi]}}
value[Entry::ftAuthor]: []() {
                           const QStringList &lastNames = {{QStringList:MedlineCitation/Article/AuthorList/Author/LastName}};
                           const QStringList &firstNames = {{QStringList:MedlineCitation/Article/AuthorList/Author/ForeName}};
                           Value v;
                           for (auto itLastName = lastNames.constBegin(), itFirstName = firstNames.constBegin(); itLastName != lastNames.constEnd() && itFirstName != firstNames.constEnd(); ++itLastName, ++itFirstName)
                                v.append(QSharedPointer<Person>(new Person(*itFirstName, *itLastName)));
                           return v;
                        }()
field[Entry::ftISSN]: {{MedlineCitation/Article/Journal/ISSN}}
field[Entry::ftVolume]: {{MedlineCitation/Article/Journal/JournalIssue/Volume}}
field[Entry::ftNumber]: {{MedlineCitation/Article/Journal/JournalIssue/Issue}}
field[Entry::ftPages]: {{MedlineCitation/Article/Pagination/MedlinePgn}}
field[Entry::ftTitle]: {{MedlineCitation/Article/ArticleTitle}}
field[Entry::ftJournal]: {{MedlineCitation/Article/Journal/Title}}
entrytype: Entry::etArticle
entryid: QStringLiteral("pmid") + {{MedlineCitation/PMID}}
