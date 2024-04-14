###########################################################################
#   SPDX-License-Identifier: GPL-2.0-or-later
#                                                                         #
#   SPDX-FileCopyrightText: 2023-2024 Thomas Fischer <fischer@unix-ag.uni-kl.de>
#                                                                         #
#   This script is free software; you can redistribute it and/or modify   #
#   it under the terms of the GNU General Public License as published by  #
#   the Free Software Foundation; either version 2 of the License, or     #
#   (at your option) any later version.                                   #
#                                                                         #
#   This script is distributed in the hope that it will be useful,        #
#   but WITHOUT ANY WARRANTY; without even the implied warranty of        #
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
#   GNU General Public License for more details.                          #
#                                                                         #
#   You should have received a copy of the GNU General Public License     #
#   along with this script; if not, see <https://www.gnu.org/licenses/>.  #
###########################################################################


import sys
import string
import typing
import re
import zlib

# Store information read from .txt files
format = ""
entries = ""
introduction = ""
postprocessingmapping = ""
postprocessingfields = ""
entryvalidation = ""
entrytype = ""
entryid = ""
field: typing.Dict[str, str] = dict()
valueItemType: typing.Dict[str, str] = dict()
valuemap: typing.Dict[str, str] = dict()


def alphanumhash(text: str) -> str:
    """Hash arbitrary text (string) into a sequence of upper-case letters (length 6 is fixed)"""
    i = zlib.adler32(text.encode("utf-8"))
    result = ""
    l = len(string.ascii_uppercase)
    for _ in range(6):
        result += string.ascii_uppercase[i % l]
        i = int(i // l)
    return result


lambdaprefixregexp = re.compile(r"\[(?P<capture>.*?)\]\(\)")


def rewriteVariablePlaceholder(text: str) -> str:
    """In a given text, replace placeholders (those in double curly brackets) with data read from the XML file"""
    p1 = text.find("{{")
    while p1 >= 0:
        p2 = text.find("}}", p1 + 2)
        if p2 > p1:
            replacement = text[p1 + 2 : p2]
            if replacement.startswith("field["):
                # {{field[Entry::ftDOI]}}  or  {{field["doi"]}}  allows to get a textual representation
                # of data already stored in the entry under constructions
                replacement = replacement[6:-1]
                if len(replacement) > 2 and replacement[0] == '"' and replacement[-1] == '"':
                    # Wrap double-quoted strings into a QStringLiteral
                    replacement = f"QStringLiteral({replacement})"
                replacement = f"PlainTextValue::text(entry->value({replacement}))"
            elif replacement.startswith("QStringList:"):
                # {{QStringList:authors/author}}  will create a QStringList of data found in XML tags as specified after the colon
                replacement = replacement[12:]  # clip away  QStringList:  at the beginning
                replacement = f'const_cast<const QStringList &>(mapping[QStringLiteral("{replacement}")])'
            else:
                # {{article/title}}  will create a string from the data found in XML tags matching the argument
                # If several instances of the XML tags exist, they are separated by line breaks
                replacement = f'mapping[QStringLiteral("{replacement}")].join(QStringLiteral("\\n"))'
            text = text[:p1] + replacement + text[p2 + 2 :]
        p1 = text.find("{{", p1 + 2)

    # Final replacement in case text to process contains a lambda function:
    # pass  mapping  to the inside of the lambda function
    m = lambdaprefixregexp.search(text)
    if not m is None:
        existingcapture = m.group("capture")
        if len(existingcapture) == 0:
            capture = "&mapping"
        else:
            capture = "&mapping," + existingcapture
        text = text[: m.start() + 1] + capture + "]()" + text[m.end() :]

    return text


def indentCppCode(text: str, depth: int) -> str:
    """Indent C/C++ code starting at a certain depth, except for the first line that does not get indented"""
    lines = text.split("\n")
    if len(lines) <= 1:
        # As first line does not get indented, return  text  immediately for single-line text
        return text
    else:
        result: str = ""
        singlelineindent: bool = False
        for i, l in enumerate(lines):
            l.strip()  # remove surrounding whitespace, which will be added later if needed
            if i == 0:
                # First line does not get indented, so put it directly into the result
                result = l
                continue
            else:
                # Increase or decrease indent if code blocks are entered or left, respectively
                if len(lines[i - 1]) > 0 and lines[i - 1][-1] == "{":
                    depth += 4
                if len(l) > 0 and l[0] == "}":
                    depth -= 4
                # Add line's text with current indentation
                result += "\n" + depth * " " + l
                if singlelineindent and not l.startswith("//"):
                    # Stop indenting unless line is a comment
                    singlelineindent = False
                    depth -= 4
                elif (l.startswith("if") or l.startswith("while") or l.startswith("for") or "else if" in l or l.endswith("else")) and l[
                    -1
                ] != "{":
                    # If special commands like  if  or  while  are in the current line but no opening curly bracket is followed,
                    # the remember to indent only the next line
                    singlelineindent = True
                    depth += 4
        return result


def assembleEntry(depth):
    # Maybe some post-processing is necessary after mapping has been filled?
    if not postprocessingmapping is None and len(postprocessingmapping) > 0:
        print(rewriteVariablePlaceholder(postprocessingmapping), sep="", end="\n\n")

    # Create an entry object, but postpone setting type and id for later
    print(
        depth * " ",
        'QSharedPointer<Entry> entry = QSharedPointer<Entry>(new Entry(QStringLiteral("placeholderType"), QStringLiteral("placeholderId")));',
        sep="",
    )

    # Fill the entry with data

    # Set all fields, i.e. where the .txt file had configuration where keys started with  field[
    for fkey, fvalue in field.items():
        # Determine type of field data: Either from configuration file or via educated guess
        fieldType = "PlainText"
        if fkey in valueItemType:
            fieldType = valueItemType[fkey]
        elif fkey in {"Entry::ftDOI", "Entry::ftUrl"}:
            fieldType = "VerbatimText"
        elif fkey in {"Entry::ftMonth"}:
            fieldType = "MacroKey"

        varname = f"value{alphanumhash(fkey)}"
        # Handle field names with quotation marks instead of predefined ones like Entry::ftDOI
        if len(fkey) > 2 and fkey[0] == '"' and fkey[-1] == '"':
            fkey = f"QStringLiteral({fkey})"
        # Replace/rewrite any  {{...}}  variables inside the configuration file data for this field
        fvalue = indentCppCode(rewriteVariablePlaceholder(fvalue), depth)

        # First, assign the computed value for this field in a QString
        print(depth * " ", f"const QString {varname} ", sep="", end="")
        if len(fvalue) > 2 and fvalue[0] == "{" and fvalue[-1] == "}":
            print(fvalue, ";", sep="")
        else:
            print("= ", fvalue, ";", sep="")
        # Then, only if it is a non-empty string, store this string in the entry
        print(depth * " ", f"if (!{varname}.isEmpty())", sep="")
        print((depth + 4) * " ", f"entry->insert({fkey}, Value() << QSharedPointer<{fieldType}>(new {fieldType}({varname})));", sep="")

    # Set fields where not a string is given, but the configuration file contains code fragments that
    # generate Value objects that can be directly assigned to the entry's fields
    for vkey, vvalue in valuemap.items():
        varname = f"value{alphanumhash(vkey)}"
        if len(vkey) > 2 and vkey[0] == '"' and vkey[-1] == '"':
            vkey = f"QStringLiteral({vkey})"
        print(depth * " ", f"const Value {varname} = {indentCppCode(rewriteVariablePlaceholder(vvalue),depth)};", sep="")
        print(depth * " ", f"if (!{varname}.isEmpty())", sep="")
        print((depth + 4) * " ", f"entry->insert({vkey}, {varname});", sep="")

    # Maybe some post-processing is necessary after all fields have been set?
    if not postprocessingfields is None and len(postprocessingfields) > 0:
        print("\n", depth * " ", indentCppCode(rewriteVariablePlaceholder(postprocessingfields), depth), sep="")

    # As it was postponed, set entry's id and type now
    print("\n", depth * " ", f"entry->setId({rewriteVariablePlaceholder(entryid)});", sep="")
    print(depth * " ", f"entry->setType({indentCppCode(rewriteVariablePlaceholder(entrytype), depth)});", sep="")

    # Finally, append the entry to the list of resulting entries
    if len(entryvalidation) == 0:
        print(depth * " ", "result.append(entry);", sep="")
    else:
        print(depth * " ", "if (", entryvalidation, ")", sep="")
        print((depth + 4) * " ", "result.append(entry);", sep="")


def xmlParser():
    """Generate C++ that can parse XML code as specified in the .txt file (various global variables hold this data already)."""

    if not introduction is None and len(introduction) > 0:
        print(introduction, sep="", end="\n\n")  # Print 'introduction' sans << ... >>

    depth: int = 8
    # Read XML file until reaching entries' tags
    print(depth * " ", "*ok = true;", sep="")
    print(depth * " ", "QXmlStreamReader xsr(xmlData);", sep="")

    # Dive into XML document to get to the point where the bibliographic data begins
    entriesPath = entries.split("/")
    print(depth * " ", "if (xsr.readNextStartElement()) {", sep="")
    depth += 4
    print(depth * " ", f'if (xsr.qualifiedName() == QStringLiteral("{entriesPath[0]}"))', " {", sep="")
    for step in entriesPath[1:]:
        depth += 4
        print(depth * " ", "while (!xsr.atEnd() && xsr.readNext() != QXmlStreamReader::Invalid) {", sep="")
        depth += 4
        print(depth * " ", f'if (xsr.isStartElement() && xsr.qualifiedName() == QStringLiteral("{step}"))', " {", sep="")
    depth += 4

    # Stack variable keeps track of position inside XML data
    print(depth * " ", "QStringList stack;", sep="")
    # Some XML tags have a 'type' attribute which will be tracked as it is useful in some situations
    # Example  <id type="doi">10.100/abc</id>  allows to address the text via  id/[@type=doi]
    print(depth * " ", "QPair<QString, QString> typeAttribute;", sep="")
    # Keep track of data extract from XML data, e.g. "articles/authors/author"  -> ["John Doe", "Jane Done"]
    print(depth * " ", "QMap<QString, QStringList> mapping;", sep="")
    # Inside the following loop, one entry is processed
    print(
        depth * " ",
        f'while (!xsr.atEnd() && xsr.readNext() != QXmlStreamReader::Invalid && xsr.qualifiedName() != QStringLiteral("{entriesPath[-1]}"))',
        " {",
        sep="",
    )
    depth += 4

    print(depth * " ", "if (xsr.isStartElement()) {", sep="")
    depth += 4

    # If at the opening of a XML element ...
    # Append element's qualified name to stack
    print(depth * " ", "stack.append(xsr.qualifiedName().toString());", sep="")
    # Clear previous type attributes
    print(depth * " ", "typeAttribute = qMakePair(QString(), QString());", sep="")

    print(depth * " ", "for (const QXmlStreamAttribute &attr : xsr.attributes()) {", sep="")
    # Go over all attributes of this XML element ...
    depth += 4

    print(depth * " ", "const QString text{OnlineSearchAbstract::deHTMLify(attr.value().toString().trimmed())};", sep="")
    print(depth * " ", "if (!text.isEmpty()) {", sep="")
    depth += 4

    # For attributes with non-empty text ...
    print(depth * " ", 'if (attr.qualifiedName().toString().toLower().contains(QStringLiteral("type")))', sep="")
    # If this attribute looks like a 'type' attribute, ...
    print((depth + 4) * " ", "typeAttribute = qMakePair(attr.qualifiedName().toString(), text);", sep="")
    print(depth * " ", 'const QString key{stack.join(QStringLiteral("/")) + QStringLiteral("/@") + attr.qualifiedName().toString()};', sep="")
    # ... store its name and its value for later use
    print(depth * " ", "if (mapping.contains(key))", sep="")
    print((depth + 4) * " ", "mapping[key].append(text);", sep="")
    print(depth * " ", "else", sep="")
    print((depth + 4) * " ", "mapping.insert(key, QStringList() << text);", sep="")
    depth -= 4
    print(depth * " ", "}", sep="")
    depth -= 4
    print(depth * " ", "}", sep="")
    depth -= 4
    print(depth * " ", "} else if (xsr.isEndElement() && stack.length() > 0 && stack.last() == xsr.qualifiedName())", sep="")
    # If at the closing of a XML element ...
    # Remove element's qualified name from stack
    print((depth + 4) * " ", "stack.removeLast();", sep="")
    print(depth * " ", "else if (xsr.isCharacters()) {", sep="")
    depth += 4
    # If reading plain text from the XML stream ...
    print(depth * " ", "const QString text{OnlineSearchAbstract::deHTMLify(xsr.text().toString().trimmed())};", sep="")
    print(depth * " ", "if (!text.isEmpty()) {", sep="")
    depth += 4
    # Record this text as content for the XML element's path as recorded by the stack
    print(depth * " ", 'const QString key{stack.join(QStringLiteral("/"))};', sep="")
    print(depth * " ", "if (mapping.contains(key))", sep="")
    print((depth + 4) * " ", "mapping[key].append(text);", sep="")
    print(depth * " ", "else", sep="")
    print((depth + 4) * " ", "mapping.insert(key, QStringList() << text);", sep="")
    print(depth * " ", "if (!typeAttribute.first.isEmpty() && !typeAttribute.second.isEmpty()) {", sep="")
    depth += 4
    # If a type attribute was recoded, store the plain text for this type attribute specifically, too
    print(
        depth * " ",
        'const QString key{stack.join(QStringLiteral("/")) + QStringLiteral("[@") + typeAttribute.first + QStringLiteral("=") + typeAttribute.second + QStringLiteral("]")};',
        sep="",
    )
    print(depth * " ", "if (mapping.contains(key))", sep="")
    print((depth + 4) * " ", "mapping[key].append(text);", sep="")
    print(depth * " ", "else", sep="")
    print((depth + 4) * " ", "mapping.insert(key, QStringList() << text);", sep="")
    depth -= 4
    print(depth * " ", "}", sep="")
    depth -= 4
    print(depth * " ", "}", sep="")
    depth -= 4
    print(depth * " ", "}", sep="")
    depth -= 4
    print(
        depth * " ", "}", sep=""
    )  # while (!xsr.atEnd() && xsr.readNext() != QXmlStreamReader::Invalid and xsr.qualifiedName() != QStringLiteral( ...

    print(depth * " ", "if (xsr.tokenType() == QXmlStreamReader::Invalid)", sep="")
    print(
        (depth + 4) * " ",
        'qCWarning(LOG_KBIBTEX_NETWORKING) << "Invalid XML while parsing data at offset" << xsr.characterOffset() << ":" << xsr.errorString();',
        sep="",
        end="\n\n",
    )

    assembleEntry(depth)

    # Close all loops and earlier checks

    for _ in entriesPath[1:]:
        depth -= 4
        print(depth * " ", "}", sep="")  # if (xsr.isStartElement() && xsr.qualifiedName()==QStringLiteral( ...
        depth -= 4
        print(depth * " ", "}", sep="")  # while (!xsr.atEnd() && xsr.readNext() != QXmlStreamReader::Invalid) { ...
        print(depth * " ", "if (xsr.tokenType() == QXmlStreamReader::Invalid)", sep="")
        print(
            (depth + 4) * " ",
            'qCWarning(LOG_KBIBTEX_NETWORKING) << "Invalid XML while parsing data at offset" << xsr.characterOffset() << ":" << xsr.errorString();',
            sep="",
            end="\n\n",
        )
    depth -= 4
    print(depth * " ", "} else {", sep="")
    depth += 4
    print(
        depth * " ",
        'qCWarning(LOG_KBIBTEX_NETWORKING) << "Expected ',
        "'",
        entriesPath[0],
        "'",
        ', got" << xsr.qualifiedName() << "at XML line" << xsr.lineNumber() << ":" << xsr.errorString();',
        sep="",
    )
    print(depth * " ", "*ok = false;", sep="")
    depth -= 4
    print(depth * " ", "}", sep="")
    depth -= 4
    print(depth * " ", "} else {", sep="")
    depth += 4
    print(
        depth * " ",
        'qCWarning(LOG_KBIBTEX_NETWORKING) << "Could not read start element at XML line" << xsr.lineNumber() << ":" << xsr.errorString();',
        sep="",
    )
    print(depth * " ", "*ok = false;", sep="")
    depth -= 4
    print(depth * " ", "}", sep="")
    print("\n")


def jsonParser():
    """Generate C++ that can parse JSON code as specified in the .txt file (various global variables hold this data already)."""

    if not introduction is None and len(introduction) > 0:
        print(introduction, sep="", end="\n\n")  # Print 'introduction' sans << ... >>

    depth: int = 8
    # Read JSON file and build a 'global map', mapping the path to a value to its value as string
    print(depth * " ", "QJsonParseError parseError;", sep="")
    print(depth * " ", "const QJsonDocument document = QJsonDocument::fromJson(jsonData, &parseError);", sep="")
    print(depth * " ", "if (parseError.error == QJsonParseError::NoError) {", sep="")
    depth += 4
    print(depth * " ", "if (document.isObject()) {", sep="")
    depth += 4
    print(depth * " ", "QMap<QString, QStringList> globalMapping;", sep="")
    print(depth * " ", "QQueue<QPair<QJsonValue, QStringList>> queue;", sep="")
    print(depth * " ", "queue.enqueue(qMakePair(document.object(), QStringList()));", sep="")
    print(depth * " ", "while (!queue.isEmpty()) {", sep="")
    depth += 4
    print(depth * " ", "const auto p{queue.dequeue()};", sep="")
    print(depth * " ", "const QJsonValue cur {p.first};", sep="")
    print(depth * " ", "const QStringList path{p.second};", sep="")
    print(depth * " ", "if (cur.isArray()) {", sep="")
    depth += 4
    # If current JSON value is an array, iterate over array and add '/0', '/1', ... to the path for each item
    # and add each item to the queue
    print(depth * " ", "const QJsonArray curArray = cur.toArray();", sep="")
    print(depth * " ", "for (int i = 0; i < curArray.size(); ++i) {", sep="")
    depth += 4
    print(depth * " ", "const QJsonValue &curItem = curArray[i];", sep="")
    print(depth * " ", "const QStringList newPath{QStringList(path) << QString::number(i)};", sep="")
    print(depth * " ", "queue.enqueue(qMakePair(curItem, newPath));", sep="")
    depth -= 4
    print(depth * " ", "}", sep="")
    depth -= 4
    print(depth * " ", "} else if (cur.isObject()) {", sep="")
    depth += 4
    # If current JSON value is an object (i.e. a dictionary), add the key to the path and add each item to the queue
    print(depth * " ", "const QJsonObject curObj = cur.toObject();", sep="")
    print(depth * " ", "for (auto it = curObj.constBegin(); it != curObj.constEnd(); ++it) {", sep="")
    depth += 4
    print(depth * " ", "const QStringList newPath{QStringList(path) << it.key()};", sep="")
    print(depth * " ", "queue.enqueue(qMakePair(it.value(), newPath));", sep="")
    depth -= 4
    print(depth * " ", "}", sep="")
    depth -= 4
    print(depth * " ", "} else if (cur.isString() || cur.isDouble() || cur.isBool()) {", sep="")
    depth += 4
    # 'Simple' values like strings, numbers, or boolean values are converted to a string and use as values in the global map
    print(
        depth * " ",
        'const QString text {cur.isString() ? cur.toString() : (cur.isDouble() ? QString::number(cur.toDouble()) : (cur.isBool() ? (cur.toBool() ? QStringLiteral("True") : QStringLiteral("False")) : QString()))};',
        sep="",
    )
    print(depth * " ", 'const QString key {path.join(QStringLiteral("/"))};', sep="")
    print(depth * " ", "if (globalMapping.contains(key))", sep="")
    print(depth * " ", "    globalMapping[key].append(text);", sep="")
    print(depth * " ", "else", sep="")
    print(depth * " ", "    globalMapping.insert(key, QStringList() << text);", sep="")
    depth -= 4
    print(depth * " ", "} else", sep="")
    # Can only happen for non-supported JSON value types
    print(depth * " ", '    qCWarning(LOG_KBIBTEX_NETWORKING) << "This should never happen";', sep="")
    depth -= 4
    print(depth * " ", "}", sep="")
    print("#define LARGE_NUMBER 1024")
    print(depth * " ", "for (int n = 0; n < LARGE_NUMBER; ++n) {", sep="")
    depth += 4
    print(
        depth * " ",
        'const QString keyPrefix {QString(QStringLiteral("%1/%2/")).arg(QStringLiteral("',
        entries,
        '"), QString::number(n))};',
        sep="",
    )
    print(depth * " ", "QMap<QString, QStringList> mapping;", sep="")
    print(depth * " ", 'static const QRegularExpression endsWithNumbersRegExp{QStringLiteral("/(0|[1-9][0-9]*)$")};', sep="")
    print(depth * " ", "for (auto it = globalMapping.constBegin(); it != globalMapping.constEnd(); ++it)", sep="")
    print(depth * " ", "    if (it.key().startsWith(keyPrefix)) {", sep="")
    depth += 8
    print(depth * " ", "QString key {it.key().mid(keyPrefix.length())};", sep="")
    print(depth * " ", "const auto endsWithNumbersMatch {endsWithNumbersRegExp.match(key)};", sep="")
    print(depth * " ", "if (endsWithNumbersMatch.hasMatch())", sep="")
    print(depth * " ", "    key = key.left(key.length() - endsWithNumbersMatch.capturedLength());", sep="")
    print(depth * " ", "if (mapping.contains(key))", sep="")
    print(depth * " ", "    mapping[key].append(it.value());", sep="")
    print(depth * " ", "else", sep="")
    print(depth * " ", "    mapping[key] = it.value();", sep="")
    depth -= 8
    print(depth * " ", "    }", sep="")
    print(depth * " ", "if (mapping.isEmpty()) break;", sep="")

    assembleEntry(depth)

    print(depth * " ", "*ok = true;", sep="")
    depth -= 4
    print(depth * " ", "}", sep="")
    depth -= 4
    print(depth * " ", "}", sep="")
    depth -= 4
    print(depth * " ", "} else {", sep="")
    depth += 4
    print(depth * " ", 'qCWarning(LOG_KBIBTEX_NETWORKING) << "Problem with JSON data: " << parseError.errorString();', sep="")
    print(depth * " ", "*ok = false;", sep="")
    depth -= 4
    print(depth * " ", "}", sep="")


# Read configuration file provided as the single argument to this Python script invocation
with open(sys.argv[-1]) as input:
    for line in input:
        # Remove whitespace on right side
        line = line.rstrip()
        # Skip empty lines or comments
        if len(line) == 0 or line[0] == "#":
            continue
        # Maybe line can be split into key-value pairs?
        colonpos = line.find(": ")
        if len(line) > 1 and line[0] == " ":
            # Lines that start with spaces are continuations of a previous line,
            # so the value of this line is its content, stripped from surrounding spaces
            value = "\n" + line.strip()
        elif colonpos > 0:
            # So there is a separator string (': '), so split into key-value pair
            key = line[:colonpos].strip()
            value = line[colonpos + 2 :].strip()

        # Update various global variables, depending on value read from line
        if key == "format":
            format += value
        elif key == "entries":
            entries += value
        elif key == "introduction":
            introduction += "        " + value
        elif key == "entryvalidation":
            entryvalidation += value
        elif key == "postprocessingmapping":
            postprocessingmapping += value
        elif key == "postprocessingfields":
            postprocessingfields += value
        elif key == "entrytype":
            entrytype += value
        elif key == "entryid":
            entryid += value
        elif key.startswith("field["):
            fkey = key[6:-1]
            field.setdefault(fkey, "")
            field[fkey] += value
        elif key.startswith("valueItemType["):
            vitkey = key[14:-1]
            valueItemType.setdefault(vitkey, "")
            valueItemType[vitkey] += value
        elif key.startswith("value["):
            vitkey = key[6:-1]
            valuemap.setdefault(vitkey, "")
            valuemap[vitkey] += value


if not format in {"xml", "json"}:
    raise ValueError("Missing or unsupported format: " + str(format))

# Beginning of generated C++ code, hinting that the following code is generated by this script
# and shall not be manually messed around
print("        // Source code generated by Python script 'codegenerator-dataparser.py'")
print(f"        // using information from configuration file '{sys.argv[-1]}'", end="\n\n")

if format == "xml":
    xmlParser()
elif format == "json":
    jsonParser()
