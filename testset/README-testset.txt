Dear user and developer,

this directory used to contain example bibliography files such as files
originally submitted in bug reports or specially crafted files to test
border cases in the importer modules of KBibTeX.

However, it turned out to be impractical to ship 40MB of bibliography
files for a project where the source code weights only 3MB.

Historically, tar balls for KBibTeX were created based on a Git checkout,
of course, but before building the actual tar balls, the files in the
checkout got modified (such as creating a 'version.h') or removed (said
testset directory). This approach is deprecated now; tar balls should
instead closely resemble the content of the Git checkout without any
modification.

Therefore, all teset files including their revision control history
got migrated to a separate Git repository, also available at the KDE
infrastructure:

  https://cgit.kde.org/kbibtex-testset.git
  git://anongit.kde.org/kbibtex-testset.git
  https://anongit.kde.org/kbibtex-testset.git

Attempts have been made to integrate the testset repository as a Git
submodule at the same location, however hooks at KDE's Git server
block pushing such a solution.

In src/test, executable kbibtexfilestest depends on knowing the correct
location of the testset directory. The location is set by creating a
cache entry when invoking cmake:

  cmake [...] -DTESTSET_DIRECTORY:PATH=/path/to/testset [...]

For clarity, to have the kbibtex-testset repository locally available
and setting above cache entry is only necessary if you want to run
kbibtexfilestest. Normal compilation and usage of KBibTeX does not
require it.
