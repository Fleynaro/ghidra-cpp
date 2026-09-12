module;
#include <pugixml.hpp>
module ghidra.decompiler;
import std;

// Port provenance: the public DOM and SAX-compatible API is preserved from
// Ghidra/Features/Decompiler/src/decompile/cpp/xml.cc.
// The generated parser is replaced by a pugixml-backed implementation while
// retaining the original ghidra Element/Document/DocumentStorage behavior.
/* ###
 * IP: GHIDRA
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

namespace ghidra {

string Attributes::bogus_uri("http://unused.uri");

namespace {

/// Uses pugixml while disabling its XML line-ending and attribute-value
/// normalization, both of which differ from the original generated scanner.
const unsigned int XML_PARSE_OPTIONS = (pugi::parse_default & ~(pugi::parse_wconv_attribute | pugi::parse_eol)) |
                                       pugi::parse_ws_pcdata | pugi::parse_comments | pugi::parse_declaration |
                                       pugi::parse_pi | pugi::parse_doctype;

/// Stores one source-level entity replaced before pugixml parsing so that its
/// result can retain the original parser's single-byte conversion semantics.
struct LegacyReference {
    string marker;
    char value;
};

/// Returns whether a string begins with the supplied literal at the given byte
/// offset without interpreting or normalizing any source bytes.
bool startsWith(const string& source, uint4 offset, const char* literal) {
    const string needle(literal);
    return offset <= source.size() && source.size() - offset >= needle.size() &&
           source.compare(offset, needle.size(), needle) == 0;
}

/// Creates a marker absent from the original source and records its byte value.
string makeReferenceMarker(const string& source, vector<LegacyReference>& references, char value) {
    uint4 index = static_cast<uint4>(references.size());
    string marker;
    do {
        marker = "__ghidra_xml_reference_" + std::to_string(index++) + "__";
    } while (source.find(marker) != string::npos);
    references.push_back(LegacyReference{marker, value});
    return marker;
}

/// Converts the original parser's numeric character reference accumulator to
/// its final byte. The generated parser appended an int4 to std::string,
/// which converted the accumulated value to one char instead of UTF-8.
char legacyNumericValue(const string& digits, bool hexadecimal) {
    unsigned int value = 0;
    const unsigned int radix = hexadecimal ? 16U : 10U;
    for (char digit : digits) {
        unsigned int part;
        if (digit >= '0' && digit <= '9')
            part = static_cast<unsigned int>(digit - '0');
        else if (digit >= 'A' && digit <= 'F')
            part = 10U + static_cast<unsigned int>(digit - 'A');
        else
            part = 10U + static_cast<unsigned int>(digit - 'a');
        value = value * radix + part;
    }
    return static_cast<char>(value & 0xffU);
}

/// Replaces the five named entities and numeric references recognized by the
/// original parser, retaining its 0xff fallback for unknown named entities.
/// Comments and CDATA remain byte-for-byte intact.
string preserveLegacyReferences(const string& source, vector<LegacyReference>& references) {
    string result;
    result.reserve(source.size());
    for (uint4 index = 0; index < source.size();) {
        if (startsWith(source, index, "<!--")) {
            const string::size_type end = source.find("-->", index + 4);
            const string::size_type limit = end == string::npos ? source.size() : end + 3;
            result.append(source, index, limit - index);
            index = static_cast<uint4>(limit);
            continue;
        }
        if (startsWith(source, index, "<![CDATA[")) {
            const string::size_type end = source.find("]]>", index + 9);
            const string::size_type limit = end == string::npos ? source.size() : end + 3;
            result.append(source, index, limit - index);
            index = static_cast<uint4>(limit);
            continue;
        }
        if (source[index] == '&') {
            uint4 cursor = index + 1;
            bool hexadecimal = false;
            if (cursor < source.size() && source[cursor] == '#') {
                ++cursor;
                if (cursor < source.size() && source[cursor] == 'x') {
                    hexadecimal = true;
                    ++cursor;
                }
                const uint4 digitsStart = cursor;
                while (cursor < source.size()) {
                    const char digit = source[cursor];
                    const bool valid = hexadecimal ? ((digit >= '0' && digit <= '9') ||
                                                      (digit >= 'A' && digit <= 'F') || (digit >= 'a' && digit <= 'f'))
                                                   : (digit >= '0' && digit <= '9');
                    if (!valid)
                        break;
                    ++cursor;
                }
                if (cursor > digitsStart && cursor < source.size() && source[cursor] == ';') {
                    const string digits = source.substr(digitsStart, cursor - digitsStart);
                    result += makeReferenceMarker(source, references, legacyNumericValue(digits, hexadecimal));
                    index = cursor + 1;
                    continue;
                }
            } else if (cursor < source.size() && ((source[cursor] >= 'A' && source[cursor] <= 'Z') ||
                                                  (source[cursor] >= 'a' && source[cursor] <= 'z'))) {
                const uint4 nameStart = cursor++;
                while (cursor < source.size()) {
                    const char nameChar = source[cursor];
                    if (!((nameChar >= 'A' && nameChar <= 'Z') || (nameChar >= 'a' && nameChar <= 'z') ||
                          (nameChar >= '0' && nameChar <= '9') || nameChar == '.' || nameChar == '-' ||
                          nameChar == '_' || nameChar == ':'))
                        break;
                    ++cursor;
                }
                if (cursor < source.size() && source[cursor] == ';') {
                    const string name = source.substr(nameStart, cursor - nameStart);
                    char value = 0;
                    if (name == "lt")
                        value = '<';
                    else if (name == "amp")
                        value = '&';
                    else if (name == "gt")
                        value = '>';
                    else if (name == "quot")
                        value = '"';
                    else if (name == "apos")
                        value = '\'';
                    else
                        value = static_cast<char>(-1);
                    result += makeReferenceMarker(source, references, value);
                    index = cursor + 1;
                    continue;
                }
            }
        }
        result += source[index++];
    }
    return result;
}

/// Restores source-level entity bytes in an attribute or text value returned
/// by pugixml. Markers are guaranteed not to collide with the original input.
string restoreLegacyReferences(const char* value, const vector<LegacyReference>& references) {
    string result(value == nullptr ? "" : value);
    for (const LegacyReference& reference : references) {
        for (string::size_type position = result.find(reference.marker); position != string::npos;
             position = result.find(reference.marker, position + 1)) {
            result.replace(position, reference.marker.size(), 1, reference.value);
        }
    }
    return result;
}

/// Finds the first unsupported processing instruction or DTD in source order,
/// including malformed constructs that pugixml might diagnose too early.
string findUnsupportedMarkup(const string& source) {
    for (uint4 index = 0; index < source.size(); ++index) {
        if (!startsWith(source, index, "<"))
            continue;
        if (startsWith(source, index, "<!--")) {
            const string::size_type end = source.find("-->", index + 4);
            if (end == string::npos)
                return "";
            index = static_cast<uint4>(end + 2);
        } else if (startsWith(source, index, "<![CDATA[")) {
            const string::size_type end = source.find("]]>", index + 9);
            if (end == string::npos)
                return "";
            index = static_cast<uint4>(end + 2);
        } else if (startsWith(source, index, "<!DOCTYPE")) {
            return "DTD's not supported";
        } else if (startsWith(source, index, "<?") && !startsWith(source, index, "<?xml")) {
            return "Processing instructions are not supported";
        }
    }
    return "";
}

/// Reports a pugixml parse failure using the diagnostic format exposed by the
/// existing ContentHandler error callback.
void reportParseError(ContentHandler* handler, const pugi::xml_parse_result& result) {
    string message("XML parse error");
    if (result.description() != nullptr && result.description()[0] != '\0') {
        message += ": ";
        message += result.description();
    }
    message += " at byte offset ";
    message += std::to_string(result.offset);
    handler->setError(message);
}

/// Reports a structural XML error that pugixml accepts for an empty document.
void reportStructuralError(ContentHandler* handler, const string& message) {
    handler->setError(message);
}

/// Returns true when text consists only of the four whitespace characters
/// classified as ignorable by the original generated parser.
bool isIgnorableWhitespace(const string& text) {
    for (char cur : text) {
        if (cur != ' ' && cur != '\n' && cur != '\r' && cur != '\t')
            return false;
    }
    return true;
}

/// Replays one text node through the same character/ignorable-whitespace split
/// used by the original XML scanner.
void emitText(ContentHandler* handler, const char* text) {
    if (text == nullptr || *text == '\0')
        return;
    const string value(text);
    const int4 length = static_cast<int4>(value.size());
    if (isIgnorableWhitespace(value))
        handler->ignorableWhitespace(text, 0, length);
    else
        handler->characters(text, 0, length);
}

/// Replays an XML declaration in the order used by the original grammar.
void emitDeclaration(const pugi::xml_node& node, ContentHandler* handler, const vector<LegacyReference>& references) {
    const pugi::xml_attribute version = node.attribute("version");
    if (version)
        handler->setVersion(restoreLegacyReferences(version.value(), references));

    const pugi::xml_attribute encoding = node.attribute("encoding");
    if (encoding)
        handler->setEncoding(restoreLegacyReferences(encoding.value(), references));
}

/// Returns the first unsupported node in a parsed DOM using document order.
string findUnsupportedNode(const pugi::xml_node& node) {
    for (pugi::xml_node child = node.first_child(); child; child = child.next_sibling()) {
        if (child.type() == pugi::node_doctype)
            return "DTD's not supported";
        if (child.type() == pugi::node_pi)
            return "Processing instructions are not supported";
        const string nested = findUnsupportedNode(child);
        if (!nested.empty())
            return nested;
    }
    return "";
}

/// Emits one already-restored text segment using the original whitespace split.
void emitTextSegment(ContentHandler* handler, const string& text) {
    if (text.empty())
        return;
    const int4 length = static_cast<int4>(text.size());
    if (isIgnorableWhitespace(text))
        handler->ignorableWhitespace(text.data(), 0, length);
    else
        handler->characters(text.data(), 0, length);
}

/// Replays text while retaining event boundaries at source-level references.
void emitText(ContentHandler* handler, const char* text, const vector<LegacyReference>& references) {
    const string raw(text == nullptr ? "" : text);
    string::size_type position = 0;
    while (position < raw.size()) {
        string::size_type next = string::npos;
        const LegacyReference* matched = nullptr;
        for (const LegacyReference& reference : references) {
            const string::size_type candidate = raw.find(reference.marker, position);
            if (candidate != string::npos && (next == string::npos || candidate < next)) {
                next = candidate;
                matched = &reference;
            }
        }
        if (next == string::npos) {
            emitTextSegment(handler, restoreLegacyReferences(raw.substr(position).c_str(), references));
            break;
        }
        emitTextSegment(handler, restoreLegacyReferences(raw.substr(position, next - position).c_str(), references));
        emitTextSegment(handler, string(1, matched->value));
        position = next + matched->marker.size();
    }
}

/// Replays a pugixml DOM node through the legacy SAX-compatible callbacks.
void emitNode(const pugi::xml_node& node, ContentHandler* handler, const vector<LegacyReference>& references) {
    switch (node.type()) {
        case pugi::node_element: {
            Attributes attributes(new string(node.name()));
            for (pugi::xml_attribute attribute = node.first_attribute(); attribute;
                 attribute = attribute.next_attribute()) {
                attributes.add_attribute(new string(attribute.name()),
                                         new string(restoreLegacyReferences(attribute.value(), references)));
            }

            const string& name = attributes.getelemName();
            const string& uri = attributes.getelemURI();
            handler->startElement(uri, name, name, attributes);
            for (pugi::xml_node child = node.first_child(); child; child = child.next_sibling())
                emitNode(child, handler, references);
            handler->endElement(uri, name, name);
            break;
        }
        case pugi::node_pcdata:
        case pugi::node_cdata:
            emitText(handler, node.value(), references);
            break;
        case pugi::node_declaration:
            emitDeclaration(node, handler, references);
            break;
        case pugi::node_comment:
        case pugi::node_pi:
        case pugi::node_doctype:
        case pugi::node_null:
        case pugi::node_document:
            // Comments are intentionally ignored after parsing; processing
            // instructions and DTDs are rejected before this replay phase.
            break;
    }
}

/// Replays all top-level nodes after the XML document has been validated.
void emitDocument(const pugi::xml_document& document, ContentHandler* handler,
                  const vector<LegacyReference>& references) {
    for (pugi::xml_node node = document.first_child(); node; node = node.next_sibling())
        emitNode(node, handler, references);
}

/// Counts document element nodes so an empty parsed stream cannot produce the
/// undefined getRoot() result permitted by the legacy empty Document object.
int4 countDocumentElements(const pugi::xml_document& document) {
    int4 count = 0;
    for (pugi::xml_node node = document.first_child(); node; node = node.next_sibling()) {
        if (node.type() == pugi::node_element)
            ++count;
    }
    return count;
}

} // namespace

/// Adds a newly opened element to the DOM and copies its attributes.
void TreeHandler::startElement(const string& namespaceURI, const string& localName, const string& qualifiedName,
                               const Attributes& atts) {
    (void)namespaceURI;
    (void)qualifiedName;
    Element* newel = new Element(cur);
    cur->addChild(newel);
    cur = newel;
    newel->setName(localName);
    for (int4 i = 0; i < atts.getLength(); ++i)
        newel->addAttribute(atts.getLocalName(i), atts.getValue(i));
}

/// Moves the tree-building cursor back to the parent of the closed element.
void TreeHandler::endElement(const string& namespaceURI, const string& localName, const string& qualifiedName) {
    (void)namespaceURI;
    (void)localName;
    (void)qualifiedName;
    cur = cur->getParent();
}

/// Appends parsed character data to the current DOM element.
void TreeHandler::characters(const char* text, int4 start, int4 length) {
    cur->addContent(text, start, length);
}

/// Recursively destroys all child elements owned by this element.
Element::~Element(void) {
    for (List::iterator iter = children.begin(); iter != children.end(); ++iter)
        delete *iter;
}

/// Finds an attribute by name and preserves the original missing-attribute
/// exception contract.
const string& Element::getAttributeValue(const string& nm) const {
    for (uint4 i = 0; i < attr.size(); ++i) {
        if (attr[i] == nm)
            return value[i];
    }
    throw DecoderError("Unknown attribute: " + nm);
}

/// Releases every document owned by this storage object.
DocumentStorage::~DocumentStorage(void) {
    for (int4 i = 0; i < doclist.size(); ++i) {
        if (doclist[i] != (Document*)0)
            delete doclist[i];
    }
}

/// Parses and retains one XML document in this storage object's ownership list.
Document* DocumentStorage::parseDocument(istream& s) {
    doclist.push_back((Document*)0);
    doclist.back() = xml_tree(s);
    return doclist.back();
}

/// Opens, parses, and retains an XML document from the local filesystem.
Document* DocumentStorage::openDocument(const string& filename) {
    ifstream s(filename.c_str());
    if (!s)
        throw DecoderError("Unable to open xml document " + filename);
    Document* res = parseDocument(s);
    s.close();
    return res;
}

/// Registers an element under its tag name, replacing an existing registration.
void DocumentStorage::registerTag(const Element* el) {
    tagmap[el->getName()] = el;
}

/// Looks up a previously registered element by tag name.
const Element* DocumentStorage::getTag(const string& nm) const {
    map<string, const Element*>::const_iterator iter = tagmap.find(nm);
    if (iter != tagmap.end())
        return (*iter).second;
    return (const Element*)0;
}

/// Parses an XML stream with pugixml and replays the legacy ContentHandler
/// callbacks without exposing pugixml types to the decompiler engine.
int4 xml_parse(istream& i, ContentHandler* hand, int4 dbg) {
    (void)dbg;
    if (hand == (ContentHandler*)0)
        return 1;

    hand->startDocument();

    ostringstream sourceStream;
    sourceStream << i.rdbuf();
    const string source = sourceStream.str();
    const string sourceDiagnostic = findUnsupportedMarkup(source);
    if (!sourceDiagnostic.empty()) {
        reportStructuralError(hand, sourceDiagnostic);
        return 1;
    }

    vector<LegacyReference> references;
    const string parserInput = preserveLegacyReferences(source, references);
    istringstream parserStream(parserInput);
    pugi::xml_document document;
    const pugi::xml_parse_result result = document.load(parserStream, XML_PARSE_OPTIONS);
    if (!result) {
        reportParseError(hand, result);
        return static_cast<int4>(result.status);
    }

    const string parsedDiagnostic = findUnsupportedNode(document);
    if (!parsedDiagnostic.empty()) {
        reportStructuralError(hand, parsedDiagnostic);
        return 1;
    }
    if (countDocumentElements(document) != 1) {
        reportStructuralError(hand, "XML document must contain exactly one root element");
        return 1;
    }

    emitDocument(document, hand, references);
    hand->endDocument();
    return 0;
}

/// Builds an owned Element DOM by routing the stream through TreeHandler.
Document* xml_tree(istream& i) {
    Document* doc = new Document();
    TreeHandler handle(doc);
    if (0 != xml_parse(i, &handle)) {
        delete doc;
        throw DecoderError(handle.getError());
    }
    return doc;
}

/// Escapes the five XML characters used by the original stream encoder.
void xml_escape(ostream& s, const char* str) {
    if (str == (const char*)0)
        return;
    while (*str != '\0') {
        if (*str < '?') {
            if (*str == '<')
                s << "&lt;";
            else if (*str == '>')
                s << "&gt;";
            else if (*str == '&')
                s << "&amp;";
            else if (*str == '"')
                s << "&quot;";
            else if (*str == '\'')
                s << "&apos;";
            else
                s << *str;
        } else
            s << *str;
        ++str;
    }
}

} // End namespace ghidra
