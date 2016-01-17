#!/usr/bin/env python3

import dbus
import re
import sys

# Define a function for error output and
# exiting script with some exit code
def error(exitcode, *objs):
	print("ERROR: ", *objs, file = sys.stderr)
	sys.exit(exitcode)

# Define a function for informational output
def info(*objs):
	print("info:  ", *objs, file = sys.stderr)

# Get current session's bus
bus = dbus.SessionBus()

# Locate KBibTeX on session bus
info("Searching KBibTeX process")
names = bus.list_names()
try:
	kbibtex_name = next(filter(lambda name: re.match(r"\Aorg.gna.kbibtex-\d+\Z", name), names))
except StopIteration:
	error(1, "KBibTeX not found on DBus session bus")
info("KBibTeX's name in DBus session bus: " + kbibtex_name)

# Locating KBibTeX's file manager object
# TODO check for errors
file_manager = bus.get_object(kbibtex_name, '/FileManager')
# Create an object representing the file manager that allows calling functions
# TODO check for errors
file_manager_interface = dbus.Interface(file_manager, dbus_interface = 'org.gna.KBibTeX.FileManager')

# TODO file to open should be argument passed to this script?
# TODO check for errors
file_url = 'file:///tmp/foo.bib'
info("Opening file: " + file_url)
opened_file_id = int(file_manager.open(file_url))
if opened_file_id < 0:
	error(2, "Could not open file '" + file_url + "'")
info("Numeric identifier for opened file: " + str(opened_file_id))

info("Setting the file to be current one")
# TODO check for errors
file_manager.setCurrentFileId(opened_file_id)
current_file_id = int(file_manager_interface.currentFileId())
info("Current file id: " + str(current_file_id))
assert opened_file_id == current_file_id

info("Retrieving the document for current file")
# TODO check for errors
current_file = bus.get_object(kbibtex_name, '/FileManager/' + str(current_file_id))
current_file_interface = dbus.Interface(current_file, dbus_interface = 'org.gna.KBibTeX.FileManager.FileInfo')
current_url = str(current_file_interface.url())
info("Current url: " + current_url)
assert file_url == current_url
# TODO check for errors
current_document_id = int(current_file_interface.documentId())
if current_document_id < 0:
	error(3, "No document is open")
info("Current document id: " + str(current_document_id))

# TODO check for errors
info("Adding example entries to the current document")
current_document = bus.get_object(kbibtex_name, '/KBibTeX/Documents/' + str(current_document_id))
current_document_interface = dbus.Interface(current_document, dbus_interface = 'org.gna.KBibTeX.Document')

# TODO check for errors
indices = current_document_interface.insertEntries("""\
@book{Knuth86,
  author    = {Donald E. Knuth},
  title     = {The TeXbook},
  publisher = {Addison-Wesley},
  year      = {1986},
  isbn      = {0-201-13447-0}
}
@book{Lamport86,
  author    = {Leslie Lamport},
  title     = {LaTeX: User's Guide and Reference Manual},
  publisher = {Addison-Wesley},
  year      = {1986},
  isbn      = {0-201-15790-X}
}""", 'text/x-bibtex')

if not indices:
	error(4, "Adding entries failed")
for i in indices:
	info("Index of recently added example entry: " + str(i))

# TODO check for errors
current_document_interface.documentSave()
