/***************************************************************************
*   Copyright (C) 2004-2011 by Thomas Fischer                             *
*   fischer@unix-ag.uni-kl.de                                             *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
***************************************************************************/

#include <QTextStream>

#include <KLocale>
#include <KDebug>
#include <KStandardDirs>
#include <kio/job.h>

#include "fileimporterbibtex.h"
#include "websearcharxiv.h"
#include "xsltransform.h"

class WebSearchArXiv::WebSearchArXivPrivate
{
private:
    QWidget *w;
    WebSearchArXiv *p;
    QRegExp jourRef1, jourRef2, jourRef3, jourRef4, jourRef5, jourRef6, m_reJour, m_reYear, m_rePages;
    QRegExp regExpPhD, regExpTechRep;

public:
    XSLTransform xslt;

    WebSearchArXivPrivate(QWidget *widget, WebSearchArXiv *parent)
            : w(widget), p(parent),

            /** examples:
                Journal of Inefficient Algorithms 5 (2003) 35-39
                Astrophys.J. 578 (2002) L103-L106
                New J. Phys. 10 (2008) 033023
                Physics Letters A 297 (2002) 4-8
                Appl.Phys. B75 (2002) 655-665
                JHEP 0611 (2006) 045
            */
            jourRef1("^([a-zA-Z. ]+[a-zA-Z.])\\s*(\\d+)\\s+\\((\\d{4})\\)\\s+([0-9A-Z]+)(-([0-9A-Z]+))?$", Qt::CaseInsensitive),
            /** examples:
                Journal of Inefficient Algorithms, Vol. 93, No. 2 (2009), pp. 42-51
                International Journal of Quantum Information, Vol. 1, No. 4 (2003) 427-441
                Stud. Hist. Phil. Mod. Phys., Vol 33 no 3 (2003), pp. 441-468
            */
            jourRef2("^([a-zA-Z. ]+[a-zA-Z.]),\\s+Vol\\.?\\s+(\\d+)[,]?\\s+No\\.?\\s+(\\d+)\\s+\\((\\d{4})\\)[,]?\\s+(pp\\.\\s+)?(\\d+)(-(\\d+))?$", Qt::CaseInsensitive),
            /** examples:
                   Journal of Inefficient Algorithms, volume 4, number 1, pp. 12-21, 2008
                   Scientometrics, volume 69, number 3, pp. 669-687, 2006
               */
            jourRef3("^([a-zA-Z. ]+),\\s+volume\\s+(\\d+),\\s+number\\s+(\\d+),\\s+pp\\.\\s+(\\d+)(-(\\d+))?,\\s+(\\d{4})$", Qt::CaseInsensitive),
            /** examples:
                Journal of Inefficient Algorithms 4(1): 101-122, 2010
                JHEP0809:131,2008
                Phys.Rev.D78:013004,2008
                Lect.NotesPhys.690:107-127,2006
                Europhys. Letters 70:1-7 (2005)
                Journal of Conflict Resolution 51(1): 58 - 88 (2007)
                Journal of Artificial Intelligence Research (JAIR), 9:247-293
            */
            jourRef4("^([a-zA-Z. ()]+)[,]?\\s*(\\d+)(\\((\\d+)\\))?:\\s*(\\d+)(\\s*-\\s*(\\d+))?(,\\s*(\\d{4})|\\s+\\((\\d{4})\\))?$", Qt::CaseInsensitive),
            /** examples:
                Journal of Inefficient Algorithms vol. 31, 4 2000
                Phys. Rev. A 71, 032339 (2005)
                Phys. Rev. Lett. 91, 027901 (2003)
                Phys. Rev. A 78, 013620 (2008)
                Phys. Rev. E 62, 1842 (2000)
                Rev. Mod. Phys. 79, 555 (2007)
                J. Math. Phys. 49, 032105 (2008)
                New J. Phys. 8, 58 (2006)
                Phys. Rev. Lett. 91, 217905 (2003).
                Physical Review B vol. 66, 161320(R) (2002)
                ??? Phys. Rev. Lett. 89, 057902(1--4) (2002).
                ??? J. Mod. Opt., 54, 2211 (2007)
            */
            jourRef5("^([a-zA-Z. ]+)\\s+(vol\\.\\s+)?(\\d+),\\s+(\\d+)(\\([A-Z]+\\))?\\s+\\((\\d{4})\\)[.]?$", Qt::CaseInsensitive),
            /** examples:
                Journal of Inefficient Algorithms, 11(2) (1999) 42-55
                Learned Publishing, 20(1) (January 2007) 16-22
            */
            jourRef6("^([a-zA-Z. ]+),\\s+(\\d+)\\((\\d+)\\)\\s+(\\(([A-Za-z]+\\s+)?(\\d{4})\\))?\\s+(\\d+)(-(\\d+))?$", Qt::CaseInsensitive),
            regExpPhD("Ph\\.?D\\.? Thesis", Qt::CaseInsensitive), regExpTechRep("Tech(\\.|nical) Rep(\\.|ort)", Qt::CaseInsensitive), xslt(KStandardDirs::locate("appdata", "arxiv2bibtex.xsl")) {
        // nothing
    }

    /**
     * Split a string along spaces, but keep text in quotation marks together
     */
    QStringList splitRespectingQuotationMarks(const QString &text) {
        int p1 = 0, p2, max = text.length();
        QStringList result;

        while (p1 < max) {
            while (text[p1] == ' ') ++p1;
            p2 = p1;
            if (text[p2] == '"') {
                ++p2;
                while (p2 < max && text[p2] != '"')  ++p2;
            } else
                while (p2 < max && text[p2] != ' ') ++p2;
            result << text.mid(p1, p2 - p1 + 1);
            p1 = p2 + 1;
        }
        return result;
    }

    KUrl buildQueryUrl(const QMap<QString, QString> &query, int numResults) {
        QString url = QLatin1String("http://export.arxiv.org/api/query?");

        /// append search terms
        QStringList queryFragments;
        for (QMap<QString, QString>::ConstIterator it = query.constBegin(); it != query.constEnd(); ++it) {
            // FIXME: Is there a need for percent encoding?
            queryFragments.append(splitRespectingQuotationMarks(it.value()));
        }
        url.append("search_query=all:" + queryFragments.join("+AND+all:") + "");

        /// set number of expected results
        url.append(QString("&start=0&max_results=%1").arg(numResults));

        kDebug() << "url = " << url;
        return KUrl(url);
    }

    void sanitizeJournal(Entry *entry) {
        const QString journal = PlainTextValue::text(entry->value(Entry::ftJournal));

        QString jTitle = "";
        QString jVol = "";
        QString jNumber = "";
        QString jYear = "";
        QString jPage1 = "";
        QString jPage2 = "";

        if (journal.indexOf(jourRef1) == 0) {
            jTitle = jourRef1.cap(1);
            jVol = jourRef1.cap(2);
            jYear = jourRef1.cap(3);
            jPage1 = jourRef1.cap(4);
            jPage2 = jourRef1.cap(6);
        } else if (journal.indexOf(jourRef2) == 0) {
            jTitle = jourRef2.cap(1);
            jVol = jourRef2.cap(2);
            jNumber = jourRef2.cap(3);
            jYear = jourRef2.cap(4);
            jPage1 = jourRef2.cap(6);
            jPage2 = jourRef2.cap(8);
        } else if (journal.indexOf(jourRef3) == 0) {
            jTitle = jourRef3.cap(1);
            jVol = jourRef3.cap(2);
            jNumber = jourRef3.cap(3);
            jPage1 = jourRef3.cap(4);
            jPage2 = jourRef3.cap(6);
            jYear = jourRef3.cap(7);
        } else if (journal.indexOf(jourRef4) == 0) {
            jTitle = jourRef4.cap(1);
            jVol = jourRef4.cap(2);
            jNumber = jourRef4.cap(4);
            jPage1 = jourRef4.cap(5);
            jPage2 = jourRef4.cap(7);
            jYear = jourRef4.cap(9).append(jourRef4.cap(10));
        } else if (journal.indexOf(jourRef5) == 0) {
            jTitle = jourRef5.cap(1);
            jVol = jourRef5.cap(3);
            jPage1 = jourRef5.cap(4);
            jYear = jourRef5.cap(6);
        } else if (journal.indexOf(jourRef6) == 0) {
            jTitle = jourRef6.cap(1);
            jVol = jourRef6.cap(2);
            jNumber = jourRef6.cap(3);
            jYear = jourRef6.cap(6);
            jPage1 = jourRef6.cap(7);
            jPage2 = jourRef6.cap(9);
        }

        if (!jTitle.isEmpty()) {
            Value v = entry->value(Entry::ftJournal);
            v.clear();
            v.append(new PlainText(jTitle));
            entry->setType(Entry::etArticle);
        }
        if (!jVol.isEmpty()) {
            Value v = entry->value(Entry::ftVolume);
            v.clear();
            v.append(new PlainText(jVol));
        }
        if (!jNumber.isEmpty()) {
            Value v = entry->value(Entry::ftNumber);
            v.clear();
            v.append(new PlainText(jNumber));
        }
        if (!jYear.isEmpty()) {
            Value v = entry->value(Entry::ftYear);
            v.clear();
            v.append(new PlainText(jYear));
        }
        if (!jPage1.isEmpty()) {
            Value v = entry->value(Entry::ftPages);
            v.clear();
            QString text = jPage1;
            if (!jPage2.isEmpty()) text.append("--").append(jPage2);
            v.append(new PlainText(text));
        }
    }

    void sanitizePhD(Entry *entry) {
        const QString text = PlainTextValue::text(entry->value(Entry::ftComment)) + PlainTextValue::text(entry->value(Entry::ftJournal)) + PlainTextValue::text(entry->value(Entry::ftAbstract));
        if (text.indexOf(regExpPhD) >= 0) {
            /// entry contains a string which suggest this is a PhD thesis
            entry->setType(Entry::etPhDThesis);

            if (entry->contains(Entry::ftJournal) && !entry->contains(Entry::ftSchool)) {
                Value v = entry->value(Entry::ftJournal);
                entry->remove(Entry::ftJournal);
                entry->insert(Entry::ftSchool, v);
            }
        }
    }

    void sanitizeTechRep(Entry *entry) {
        const QString text = PlainTextValue::text(entry->value(Entry::ftComment)) + PlainTextValue::text(entry->value(Entry::ftJournal)) + PlainTextValue::text(entry->value(Entry::ftAbstract));
        if (text.indexOf(regExpTechRep) >= 0) {
            /// entry contains a string which suggest this is a technical report
            entry->setType(Entry::etTechReport);
        }
    }

    void sanitize(Entry *entry) {
        sanitizeJournal(entry);
        sanitizePhD(entry);
        sanitizeTechRep(entry);
    }
};

WebSearchArXiv::WebSearchArXiv(QWidget *parent)
        : WebSearchAbstract(parent), d(new WebSearchArXiv::WebSearchArXivPrivate(parent, this))
{
    // nothing
}

void WebSearchArXiv::startSearch()
{
    // TODO
}

void WebSearchArXiv::startSearch(const QMap<QString, QString> &query, int numResults)
{
    KIO::StoredTransferJob *job = KIO::storedGet(d->buildQueryUrl(query, numResults));
    connect(job, SIGNAL(result(KJob *)), this, SLOT(jobDone(KJob*)));
}

QString WebSearchArXiv::label() const
{
    return i18n("arXiv.org");
}

QString WebSearchArXiv::favIconUrl() const
{
    return QLatin1String("http://arxiv.org/favicon.ico");
}

WebSearchQueryFormAbstract* WebSearchArXiv::customWidget(QWidget *parent)
{
    Q_UNUSED(parent);
    // TODO
    return NULL;
}

KUrl WebSearchArXiv::homepage() const
{
    return KUrl("http://arxiv.org/");
}

void WebSearchArXiv::cancel()
{
// TODO
}


void WebSearchArXiv::jobDone(KJob *j)
{
    if (j->error() == 0) {
        KIO::StoredTransferJob *job = static_cast<KIO::StoredTransferJob*>(j);
        QTextStream ts(job->data());
        QString result = ts.readAll();
        result = result.replace("xmlns=\"http://www.w3.org/2005/Atom\"", ""); // FIXME fix arxiv2bibtex.xsl to handle namespace

        QString bibTeXcode = d->xslt.transform(result);

        FileImporterBibTeX importer;
        File *bibtexFile = importer.fromString(bibTeXcode);

        if (bibtexFile != NULL) {
            for (File::ConstIterator it = bibtexFile->constBegin(); it != bibtexFile->constEnd(); ++it) {
                Entry *entry = dynamic_cast<Entry*>(*it);
                if (entry != NULL) {
                    d->sanitize(entry);
                    emit foundEntry(entry);
                }

            }
            emit stoppedSearch(bibtexFile->isEmpty() ? resultUnspecifiedError : resultNoError);
            delete bibtexFile;
        } else
            emit stoppedSearch(resultUnspecifiedError);
    } else {
        kWarning() << "Search using " << label() << "failed: " << j->errorString();
        emit stoppedSearch(resultUnspecifiedError);
    }
}
