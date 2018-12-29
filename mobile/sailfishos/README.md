# BibSearch

BibSearch is a SailfishOS application which allows to search for bibliographic data on scientific publications in various catalogs and publishers such as Springer, ACM, IEEE, Google Scholar, or arXiv.

All catalogs will be searched in parallel for author, title, and free text as provided by the user.
The resulting list provides a quick overview on all matching bibliographic entries. Individual entries can be inspected for their full details.
If an entry has an URL or DOI associated, the corresponding webpage can be visited.

Up to and including version 0.4, the core code was based on existing code derived from KBibTeX but maintained in a separate repository. To minimize the overhead for maintaining two related code bases, as of version 0.5 BibSearch resides inside KBibTeX's repository and shares code with it to the greatest extend. Indeed, BibSearch differs from KBibTeX only in respect to the user interface (BibSearch is QML-based), limited functionality (not all libraries are supported on mobile platforms), and *three* C++ source files not used in KBibTeX itself.

