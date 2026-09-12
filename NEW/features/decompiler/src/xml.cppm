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

/// Uses pugixml's standards-compliant XML features while retaining whitespace
/// and declaration nodes needed by the legacy ContentHandler contract.
const unsigned int XML_PARSE_OPTIONS =
    pugi::parse_default | pugi::parse_ws_pcdata | pugi::parse_declaration | pugi::parse_doctype;

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
bool isIgnorableWhitespace(const char* text) {
    if (text == nullptr)
        return true;
    for (const char* cur = text; *cur != '\0'; ++cur) {
        if (*cur != ' ' && *cur != '\n' && *cur != '\r' && *cur != '\t')
            return false;
    }
    return true;
}

/// Replays one text node through the same character/ignorable-whitespace split
/// used by the original XML scanner.
void emitText(ContentHandler* handler, const char* text) {
    if (text == nullptr || *text == '\0')
        return;
    const int4 length = static_cast<int4>(std::char_traits<char>::length(text));
    if (isIgnorableWhitespace(text))
        handler->ignorableWhitespace(text, 0, length);
    else
        handler->characters(text, 0, length);
}

/// Replays an XML declaration in the order used by the original grammar.
void emitDeclaration(const pugi::xml_node& node, ContentHandler* handler) {
    const pugi::xml_attribute version = node.attribute("version");
    if (version)
        handler->setVersion(version.value());

    const pugi::xml_attribute encoding = node.attribute("encoding");
    if (encoding)
        handler->setEncoding(encoding.value());
}

/// Returns whether a parsed DOM contains a doctype, which the original parser
/// deliberately rejected instead of attempting DTD processing.
bool containsDoctype(const pugi::xml_node& node) {
    for (pugi::xml_node child = node.first_child(); child; child = child.next_sibling()) {
        if (child.type() == pugi::node_doctype || containsDoctype(child))
            return true;
    }
    return false;
}

/// Replays a pugixml DOM node through the legacy SAX-compatible callbacks.
void emitNode(const pugi::xml_node& node, ContentHandler* handler) {
    switch (node.type()) {
        case pugi::node_element: {
            Attributes attributes(new string(node.name()));
            for (pugi::xml_attribute attribute = node.first_attribute(); attribute;
                 attribute = attribute.next_attribute()) {
                attributes.add_attribute(new string(attribute.name()), new string(attribute.value()));
            }

            const string& name = attributes.getelemName();
            const string& uri = attributes.getelemURI();
            handler->startElement(uri, name, name, attributes);
            for (pugi::xml_node child = node.first_child(); child; child = child.next_sibling())
                emitNode(child, handler);
            handler->endElement(uri, name, name);
            break;
        }
        case pugi::node_pcdata:
        case pugi::node_cdata:
            emitText(handler, node.value());
            break;
        case pugi::node_declaration:
            emitDeclaration(node, handler);
            break;
        case pugi::node_comment:
        case pugi::node_pi:
        case pugi::node_doctype:
        case pugi::node_null:
        case pugi::node_document:
            // The original parser ignored comments, processing instructions, and
            // DTD content after reporting DTDs as unsupported.
            break;
    }
}

/// Replays all top-level nodes after the XML document has been validated.
void emitDocument(const pugi::xml_document& document, ContentHandler* handler) {
    for (pugi::xml_node node = document.first_child(); node; node = node.next_sibling())
        emitNode(node, handler);
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

    pugi::xml_document document;
    const pugi::xml_parse_result result = document.load(i, XML_PARSE_OPTIONS);
    if (!result) {
        reportParseError(hand, result);
        return static_cast<int4>(result.status);
    }

    if (containsDoctype(document)) {
        reportStructuralError(hand, "DTD's not supported");
        return 1;
    }
    if (countDocumentElements(document) != 1) {
        reportStructuralError(hand, "XML document must contain exactly one root element");
        return 1;
    }

    emitDocument(document, hand);
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
