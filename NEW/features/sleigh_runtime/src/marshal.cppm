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
/// \file marshal.cppm
/// \brief Stream marshalling interfaces and the packed decoder
export module sleigh_runtime:marshal;
import std;

export import :types;
export import :error;
export import :opcodes;

export namespace ghidra {

using std::list;
using std::unordered_map;

class AttributeId;
class ElementId;
extern AttributeId ATTRIB_UNKNOWN; ///< Special attribute to represent an attribute with an unrecognized name
extern ElementId ELEM_UNKNOWN;     ///< Special element to represent an element with an unrecognized name

/// \brief An annotation for a data element to being transferred to/from a stream
///
/// This class parallels the XML concept of an \b attribute on an element. An AttributeId describes
/// a particular piece of data associated with an ElementId. The defining characteristic of the AttributeId is
/// its name. Internally this name is associated with an integer id. The name (and id) uniquely determine
/// the data being labeled, within the context of a specific ElementId. Within this context, an AttributeId labels
/// either
///   - An unsigned integer
///   - A signed integer
///   - A boolean value
///   - A string
///
/// The same AttributeId can be used to label a different type of data when associated with a different ElementId.
class AttributeId {
    static unordered_map<string, uint4> lookupAttributeId; ///< A map of AttributeId names to their associated id

    /// Access static vector of AttributeId objects that are registered during static initialization
    /// The list itself is created once on the first call to this method.
    /// \return a reference to the vector
    static vector<AttributeId*>& getList(void) {
        static vector<AttributeId*> thelist;
        return thelist;
    }

    string name; ///< The name of the attribute
    uint4 id;    ///< The (internal) id of the attribute
public:
    /// This constructor should only be invoked for static objects. It registers the attribute for inclusion
    /// in the global hashtable.
    /// \param nm is the name of the attribute
    /// \param i is an id to associate with the attribute
    /// \param scope is an id for the scope of this attribute
    AttributeId(const string& nm, uint4 i, int4 scope = 0) : name(nm) {
        id = i;
        if (scope == 0)
            getList().push_back(this);
    }

    const string& getName(void) const {
        return name;
    } ///< Get the attribute's name
    uint4 getId(void) const {
        return id;
    } ///< Get the attribute's id
    bool operator==(const AttributeId& op2) const {
        return (id == op2.id);
    } ///< Test equality with another AttributeId

    /// The name is looked up in the scoped list of attributes. If the attribute is not in the list, a special
    /// placeholder attribute, ATTRIB_UNKNOWN, is returned as a placeholder for attributes with unrecognized names.
    /// \param nm is the name of the attribute
    /// \param scope is the id of the scope in which to lookup of the name
    /// \return the associated id
    static uint4 find(const string& nm, int4 scope) {
        if (scope == 0) { // Current only support reverse look up for scope 0
            unordered_map<string, uint4>::const_iterator iter = lookupAttributeId.find(nm);
            if (iter != lookupAttributeId.end())
                return (*iter).second;
        }
        return ATTRIB_UNKNOWN.id;
    }

    /// Fill the hashtable mapping attribute names to their id, from registered attribute objects
    static void initialize(void) {
        vector<AttributeId*>& thelist(getList());
        for (int4 i = 0; i < thelist.size(); ++i) {
            AttributeId* attrib = thelist[i];
            if (lookupAttributeId.find(attrib->name) != lookupAttributeId.end())
                throw DecoderError(attrib->name + " attribute registered more than once");
            lookupAttributeId[attrib->name] = attrib->id;
        }
        thelist.clear();
        thelist.shrink_to_fit();
    }

    friend bool operator==(uint4 id, const AttributeId& op2) {
        return (id == op2.id);
    } ///< Test equality of a raw integer id with an AttributeId
    friend bool operator==(const AttributeId& op1, uint4 id) {
        return (op1.id == id);
    } ///< Test equality of an AttributeId with a raw integer id
};

/// \brief An annotation for a specific collection of hierarchical data
///
/// This class parallels the XML concept of an \b element. An ElementId describes a collection of data, where each
/// piece is annotated by a specific AttributeId. In addition, each ElementId can contain zero or more \e child
/// ElementId objects, forming a hierarchy of annotated data. Each ElementId has a name, which is unique at least
/// within the context of its parent ElementId. Internally this name is associated with an integer id. A special
/// AttributeId ATTRIB_CONTENT is used to label the XML element's text content, which is traditionally not labeled
/// as an attribute.
class ElementId {
    static unordered_map<string, uint4> lookupElementId; ///< A map of ElementId names to their associated id

    /// Access static vector of ElementId objects that are registered during static initialization
    /// The list itself is created once on the first call to this method.
    /// \return a reference to the vector
    static vector<ElementId*>& getList(void) {
        static vector<ElementId*> thelist;
        return thelist;
    }

    string name; ///< The name of the element
    uint4 id;    ///< The (internal) id of the attribute
public:
    /// This constructor should only be invoked for static objects. It registers the element for inclusion
    /// in the global hashtable.
    /// \param nm is the name of the element
    /// \param i is an id to associate with the element
    /// \param scope is an id for the scope of this element
    ElementId(const string& nm, uint4 i, int4 scope = 0) : name(nm) {
        id = i;
        if (scope == 0)
            getList().push_back(this);
    }

    const string& getName(void) const {
        return name;
    } ///< Get the element's name
    uint4 getId(void) const {
        return id;
    } ///< Get the element's id
    bool operator==(const ElementId& op2) const {
        return (id == op2.id);
    } ///< Test equality with another ElementId

    /// The name is looked up in the scoped list of elements. If the element is not in the list, a special
    /// placeholder element, ELEM_UNKNOWN, is returned as a placeholder for elements with unrecognized names.
    /// \param nm is the name of the element
    /// \param scope is the id of the scope in which to search
    /// \return the associated id
    static uint4 find(const string& nm, int4 scope) {
        if (scope == 0) {
            unordered_map<string, uint4>::const_iterator iter = lookupElementId.find(nm);
            if (iter != lookupElementId.end())
                return (*iter).second;
        }
        return ELEM_UNKNOWN.id;
    }

    /// Fill the hashtable mapping element names to their id, from registered element objects
    static void initialize(void) {
        vector<ElementId*>& thelist(getList());
        for (int4 i = 0; i < thelist.size(); ++i) {
            ElementId* elem = thelist[i];
            if (lookupElementId.find(elem->name) != lookupElementId.end())
                throw DecoderError(elem->name + " element registered more than once");
            lookupElementId[elem->name] = elem->id;
        }
        thelist.clear();
        thelist.shrink_to_fit();
    }

    friend bool operator==(uint4 id, const ElementId& op2) {
        return (id == op2.id);
    } ///< Test equality of a raw integer id with an ElementId
    friend bool operator==(const ElementId& op1, uint4 id) {
        return (op1.id == id);
    } ///< Test equality of an ElementId with a raw integer id
    friend bool operator!=(uint4 id, const ElementId& op2) {
        return (id != op2.id);
    } ///< Test inequality of a raw integer id with an ElementId
    friend bool operator!=(const ElementId& op1, uint4 id) {
        return (op1.id != id);
    } ///< Test inequality of an ElementId with a raw integer id
};

unordered_map<string, uint4> AttributeId::lookupAttributeId;
unordered_map<string, uint4> ElementId::lookupElementId;

/// \brief Protocol format for PackedDecode
///
/// All bytes in the encoding are expected to be non-zero. Element encoding looks like
///   - 01xiiiii is an element start
///   - 10xiiiii is an element end
///   - 11xiiiii is an attribute start
///
/// Where iiiii is the (first) 5 bits of the element/attribute id.
/// If x=0, the id is complete. If x=1, the next byte contains 7 more bits of the id: 1iiiiiii
///
/// After an attribute start, there follows a \e type byte: ttttllll, where the first 4 bits indicate the
/// type of attribute and final 4 bits are a \b length \b code. The types are:
///   - 1 = boolean (lengthcode=0 for false, lengthcode=1 for true)
///   - 2 = positive signed integer
///   - 3 = negative signed integer (stored in negated form)
///   - 4 = unsigned integer
///   - 5 = basic address space (encoded as the integer index of the space)
///   - 6 = special address space (lengthcode 0=>stack 1=>join 2=>fspec 3=>iop)
///   - 7 = string
///
/// All attribute types except \e boolean and \e special, have an encoded integer after the \e type byte.
/// The \b length \b code, indicates the number bytes used to encode the integer, 7-bits of info per byte, 1iiiiiii.
/// A \b length \b code of zero is used to encode an integer value of 0, with no following bytes.
///
/// For strings, the integer encoded after the \e type byte, is the actual length of the string. The
/// string data itself is stored immediately after the length integer using UTF8 format.
namespace PackedFormat {
inline constexpr uint1 HEADER_MASK = 0xc0;              ///< Bits encoding the record type
inline constexpr uint1 ELEMENT_START = 0x40;            ///< Header for an element start record
inline constexpr uint1 ELEMENT_END = 0x80;              ///< Header for an element end record
inline constexpr uint1 ATTRIBUTE = 0xc0;                ///< Header for an attribute record
inline constexpr uint1 HEADEREXTEND_MASK = 0x20;        ///< Bit indicating the id extends into the next byte
inline constexpr uint1 ELEMENTID_MASK = 0x1f;           ///< Bits encoding (part of) the id in the record header
inline constexpr uint1 RAWDATA_MASK = 0x7f;             ///< Bits of raw data in follow-on bytes
inline constexpr int4 RAWDATA_BITSPERBYTE = 7;          ///< Number of bits used in a follow-on byte
inline constexpr uint1 RAWDATA_MARKER = 0x80;           ///< The unused bit in follow-on bytes. (Always set to 1)
inline constexpr int4 TYPECODE_SHIFT = 4;               ///< Bit position of the type code in the type byte
inline constexpr uint1 LENGTHCODE_MASK = 0xf;           ///< Bits in the type byte forming the length code
inline constexpr uint1 TYPECODE_BOOLEAN = 1;            ///< Type code for the \e boolean type
inline constexpr uint1 TYPECODE_SIGNEDINT_POSITIVE = 2; ///< Type code for the \e signed \e positive \e integer type
inline constexpr uint1 TYPECODE_SIGNEDINT_NEGATIVE = 3; ///< Type code for the \e signed \e negative \e integer type
inline constexpr uint1 TYPECODE_UNSIGNEDINT = 4;        ///< Type code for the \e unsigned \e integer type
inline constexpr uint1 TYPECODE_ADDRESSSPACE = 5;       ///< Type code for the \e address \e space type
inline constexpr uint1 TYPECODE_SPECIALSPACE = 6;       ///< Type code for the \e special \e address \e space type
inline constexpr uint1 TYPECODE_STRING = 7;             ///< Type code for the \e string type
inline constexpr uint4 SPECIALSPACE_STACK = 0;          ///< Special code for the \e stack space
inline constexpr uint4 SPECIALSPACE_JOIN = 1;           ///< Special code for the \e join space
inline constexpr uint4 SPECIALSPACE_FSPEC = 2;          ///< Special code for the \e fspec space
inline constexpr uint4 SPECIALSPACE_IOP = 3;            ///< Special code for the \e iop space
inline constexpr uint4 SPECIALSPACE_SPACEBASE = 4;      ///< Special code for a \e spacebase space
} // namespace PackedFormat

using namespace PackedFormat;

class AddrSpace;
class AddrSpaceManager;

/// Bridges address-space lookup from the marshal partition without importing
/// the translate partition, which would create a circular module dependency.
uint4 addrSpaceManagerNumSpaces(const AddrSpaceManager* manager);
/// Returns the indexed address space for marshal decoding.
AddrSpace* addrSpaceManagerGetSpace(const AddrSpaceManager* manager, uint4 index);
/// Returns the stack address space for marshal decoding.
AddrSpace* addrSpaceManagerGetStackSpace(const AddrSpaceManager* manager);
/// Returns the join address space for marshal decoding.
AddrSpace* addrSpaceManagerGetJoinSpace(const AddrSpaceManager* manager);

/// \brief A class for reading structured data from a stream
///
/// All data is loosely structured as with an XML document. A document contains a nested set
/// of \b elements, with labels corresponding to the ElementId class. A single element can hold
/// zero or more attributes and zero or more child elements. An attribute holds a primitive
/// data element (bool, integer, string) and is labeled by an AttributeId. The document is traversed
/// using a sequence of openElement() and closeElement() calls, intermixed with read*() calls to extract
/// the data. The elements are traversed in a depth first order. Attributes within an element can be
/// traversed in order using repeated calls to the getNextAttributeId() method, followed by a calls to
/// one of the read*(void) methods to extract the data. Alternately a read*(AttributeId) call can be used
/// to extract data for an attribute known to be in the element. There is a special content attribute
/// whose data can be extracted using a read*(AttributeId) call that is passed the special ATTRIB_CONTENT id.
/// This attribute will not be traversed by getNextAttribute().
class Decoder {
protected:
    const AddrSpaceManager* spcManager; ///< Manager for decoding address space attributes
public:
    Decoder(const AddrSpaceManager* spc) {
        spcManager = spc;
    } ///< Base constructor

    const AddrSpaceManager* getAddrSpaceManager(void) const {
        return spcManager;
    } ///< Get the manager used for address space decoding
    virtual ~Decoder(void) {} ///< Destructor

    /// \brief Prepare to decode a given stream
    ///
    /// Called once before any decoding. Currently this is assumed to make an internal copy of the stream data,
    /// i.e. the input stream is cleared before any decoding takes place.
    /// \param s is the given input stream to be decode
    /// \return \b true if the stream was fully ingested
    virtual void ingestStream(istream& s) = 0;

    /// \brief Peek at the next child element of the current parent, without traversing in (opening) it.
    ///
    /// The element id is returned, which can be compared to ElementId labels.
    /// If there are no remaining child elements to traverse, 0 is returned.
    /// \return the element id or 0
    virtual uint4 peekElement(void) = 0;

    /// \brief Open (traverse into) the next child element of the current parent.
    ///
    /// The child becomes the current parent. The list of attributes is initialized for use with getNextAttributeId.
    /// \return the id of the child element
    virtual uint4 openElement(void) = 0;

    /// \brief Open (traverse into) the next child element, which must be of a specific type
    ///
    /// The child becomes the current parent, and its attributes are initialized for use with getNextAttributeId.
    /// The child must match the given element id or an exception is thrown.
    /// \param elemId is the given element id to match
    /// \return the id of the child element
    virtual uint4 openElement(const ElementId& elemId) = 0;

    /// \brief Close the current element
    ///
    /// The data for the current element is considered fully processed. If the element has additional children,
    /// an exception is thrown. The stream must indicate the end of the element in some way.
    /// \param id is the id of the element to close (which must be the current element)
    virtual void closeElement(uint4 id) = 0;

    /// \brief Close the current element, skipping any child elements that have not yet been parsed
    ///
    /// This closes the given element, which must be current. If there are child elements that have not been
    /// parsed, this is not considered an error, and they are skipped over in the parse.
    /// \param id is the id of the element to close (which must be the current element)
    virtual void closeElementSkipping(uint4 id) = 0;

    /// \brief Get the next attribute id for the current element
    ///
    /// Attributes are automatically set up for traversal using this method, when the element is opened.
    /// If all attributes have been traversed (or there are no attributes), 0 is returned.
    /// \return the id of the next attribute or 0
    virtual uint4 getNextAttributeId(void) = 0;

    /// \brief Get the id for the (current) attribute, assuming it is indexed
    ///
    /// Assuming the previous call to getNextAttributeId() returned the id of ATTRIB_UNKNOWN,
    /// reinterpret the attribute as being an indexed form of the given attribute. If the attribute
    /// matches, return this indexed id, otherwise return ATTRIB_UNKNOWN.
    /// \param attribId is the attribute being indexed
    /// \return the indexed id or ATTRIB_UNKNOWN
    virtual uint4 getIndexedAttributeId(const AttributeId& attribId) = 0;

    /// \brief Reset attribute traversal for the current element
    ///
    /// Attributes for a single element can be traversed more than once using the getNextAttributeId method.
    virtual void rewindAttributes(void) = 0;

    /// \brief Parse the current attribute as a boolean value
    ///
    /// The last attribute, as returned by getNextAttributeId, is treated as a boolean, and its value is returned.
    /// \return the boolean value associated with the current attribute.
    virtual bool readBool(void) = 0;

    /// \brief Find and parse a specific attribute in the current element as a boolean value
    ///
    /// The set of attributes for the current element is searched for a match to the given attribute id.
    /// This attribute is then parsed as a boolean and its value returned.
    /// If there is no attribute matching the id, an exception is thrown.
    /// Parsing via getNextAttributeId is reset.
    /// \param attribId is the specific attribute id to match
    /// \return the boolean value
    virtual bool readBool(const AttributeId& attribId) = 0;

    /// \brief Parse the current attribute as a signed integer value
    ///
    /// The last attribute, as returned by getNextAttributeId, is treated as a signed integer, and its value is
    /// returned.
    /// \return the signed integer value associated with the current attribute.
    virtual intb readSignedInteger(void) = 0;

    /// \brief Find and parse a specific attribute in the current element as a signed integer
    ///
    /// The set of attributes for the current element is searched for a match to the given attribute id.
    /// This attribute is then parsed as a signed integer and its value returned.
    /// If there is no attribute matching the id, an exception is thrown.
    /// Parsing via getNextAttributeId is reset.
    /// \param attribId is the specific attribute id to match
    /// \return the signed integer value
    virtual intb readSignedInteger(const AttributeId& attribId) = 0;

    /// \brief Parse the current attribute as either a signed integer value or a string.
    ///
    /// If the attribute is an integer, its value is returned. If the attribute is a string, it must match an
    /// expected string passed to the method, and a predetermined integer value associated with the string is returned.
    /// If the attribute neither matches the expected string nor is an integer, the return value is undefined.
    /// \param expect is the string value to expect if the attribute is encoded as a string
    /// \param expectval is the integer value to return if the attribute matches the expected string
    /// \return the encoded integer or the integer value associated with the expected string
    virtual intb readSignedIntegerExpectString(const string& expect, intb expectval) = 0;

    /// \brief Find and parse a specific attribute in the current element as either a signed integer or a string.
    ///
    /// If the attribute is an integer, its value is parsed and returned.
    /// If the attribute is encoded as a string, it must match an expected string passed to this method.
    /// In this case, a predetermined integer value is passed back, indicating a matching string was parsed.
    /// If the attribute neither matches the expected string nor is an integer, the return value is undefined.
    /// If there is no attribute matching the id, an exception is thrown.
    /// \param attribId is the specific attribute id to match
    /// \param expect is the string to expect, if the attribute is not encoded as an integer
    /// \param expectval is the integer value to return if the attribute matches the expected string
    /// \return the encoded integer or the integer value associated with the expected string
    virtual intb readSignedIntegerExpectString(const AttributeId& attribId, const string& expect, intb expectval) = 0;

    /// \brief Parse the current attribute as an unsigned integer value
    ///
    /// The last attribute, as returned by getNextAttributeId, is treated as an unsigned integer, and its value is
    /// returned.
    /// \return the unsigned integer value associated with the current attribute.
    virtual uintb readUnsignedInteger(void) = 0;

    /// \brief Find and parse a specific attribute in the current element as an unsigned integer
    ///
    /// The set of attributes for the current element is searched for a match to the given attribute id.
    /// This attribute is then parsed as an unsigned integer and its value returned.
    /// If there is no attribute matching the id, an exception is thrown.
    /// Parsing via getNextAttributeId is reset.
    /// \param attribId is the specific attribute id to match
    /// \return the unsigned integer value
    virtual uintb readUnsignedInteger(const AttributeId& attribId) = 0;

    /// \brief Parse the current attribute as a string
    ///
    /// The last attribute, as returned by getNextAttributeId, is returned as a string.
    /// \return the string associated with the current attribute.
    virtual string readString(void) = 0;

    /// \brief Find the specific attribute in the current element and return it as a string
    ///
    /// The set of attributes for the current element is searched for a match to the given attribute id.
    /// This attribute is then returned as a string. If there is no attribute matching the id, and exception is thrown.
    /// Parse via getNextAttributeId is reset.
    /// \param attribId is the specific attribute id to match
    /// \return the string associated with the attribute
    virtual string readString(const AttributeId& attribId) = 0;

    /// \brief Parse the current attribute as an address space
    ///
    /// The last attribute, as returned by getNextAttributeId, is returned as an address space.
    /// \return the address space associated with the current attribute.
    virtual AddrSpace* readSpace(void) = 0;

    /// \brief Find the specific attribute in the current element and return it as an address space
    ///
    /// Search attributes from the current element for a match to the given attribute id.
    /// Return this attribute as an address space. If there is no attribute matching the id, an exception is thrown.
    /// Parse via getNextAttributeId is reset.
    /// \param attribId is the specific attribute id to match
    /// \return the address space associated with the attribute
    virtual AddrSpace* readSpace(const AttributeId& attribId) = 0;

    /// \brief Parse the current attribute as a p-code OpCode
    ///
    /// The last attribute, as returned by getNextAttributeId, is returned as an OpCode.
    /// \return the OpCode associated with the current attribute
    virtual OpCode readOpcode(void) = 0;

    /// \brief Find the specific attribute in the current element and return it as an OpCode
    ///
    /// Search attributes from the current element for a match to the given attribute id.
    /// Return this attribute as an OpCode. If there is no matching attribute id, an exception is thrown.
    /// Parse via getNextAttributeId is reset.
    /// \param attribId is the specific attribute id to match
    /// \return the OpCode associated with the attribute
    virtual OpCode readOpcode(AttributeId& attribId) = 0;

    /// \brief Skip parsing of the next element
    ///
    /// The element skipped is the one that would be opened by the next call to openElement.
    void skipElement(void) {
        uint4 elemId = openElement();
        closeElementSkipping(elemId);
    }
};

/// Abstract interface for serialization methods retained by legacy data structures.
class Encoder {
public:
    /// Destroy the encoder through its interface.
    virtual ~Encoder(void) {}
    /// Begin an element in the encoded stream.
    virtual void openElement(const ElementId& elemId) = 0;
    /// End the current element in the encoded stream.
    virtual void closeElement(const ElementId& elemId) = 0;
    /// Write a boolean attribute.
    virtual void writeBool(const AttributeId& attribId, bool val) = 0;
    /// Write a signed integer attribute.
    virtual void writeSignedInteger(const AttributeId& attribId, intb val) = 0;
    /// Write an unsigned integer attribute.
    virtual void writeUnsignedInteger(const AttributeId& attribId, uintb val) = 0;
    /// Write a string attribute.
    virtual void writeString(const AttributeId& attribId, const string& val) = 0;
    /// Write an indexed string attribute.
    virtual void writeStringIndexed(const AttributeId& attribId, uint4 index, const string& val) = 0;
    /// Write an address-space attribute.
    virtual void writeSpace(const AttributeId& attribId, const AddrSpace* spc) = 0;
    /// Write an opcode attribute.
    virtual void writeOpcode(const AttributeId& attribId, OpCode opc) = 0;
};

/// \brief A byte-based decoder designed to marshal info to the decompiler efficiently
///
/// The decoder expects an encoding as described in PackedFormat. When ingested, the stream bytes are
/// held in a sequence of arrays (ByteChunk). During decoding, \b this object maintains a Position in the
/// stream at the start and end of the current open element, and a Position of the next attribute to read to
/// facilitate getNextAttributeId() and associated read*() methods.
class PackedDecode : public Decoder {
public:
    static const int4 BUFFER_SIZE; ///< The size, in bytes, of a single cached chunk of the input stream
private:
    /// \brief A bounded array of bytes
    class ByteChunk {
        friend class PackedDecode;
        std::unique_ptr<uint1[]> storage; ///< Owned byte storage
        uint1* start;                     ///< Start of the byte array view
        uint1* end;                       ///< End of the byte array view
    public:
        /// Allocates a chunk and exposes a half-open byte view.
        explicit ByteChunk(int4 size)
            : storage(std::make_unique<uint1[]>(size)), start(storage.get()), end(storage.get() + size) {}

        /// Prevents accidental shallow copies of owned input bytes.
        ByteChunk(const ByteChunk&) = delete;

        /// Transfers chunk ownership while rebuilding its internal view.
        ByteChunk(ByteChunk&& other) noexcept
            : storage(std::move(other.storage)), start(storage.get()), end(storage.get() + (other.end - other.start)) {}

        /// Prevents accidental shallow assignment of owned input bytes.
        ByteChunk& operator=(const ByteChunk&) = delete;

        /// Transfers chunk ownership while rebuilding its internal view.
        ByteChunk& operator=(ByteChunk&& other) noexcept {
            const auto size = other.end - other.start;
            storage = std::move(other.storage);
            start = storage.get();
            end = start + size;
            return *this;
        }
    };

    /// \brief An iterator into input stream
    class Position {
        friend class PackedDecode;
        list<ByteChunk>::const_iterator seqIter; ///< Current byte sequence
        uint1* current;                          ///< Current position in sequence
        uint1* end;                              ///< End of current sequence
    };

    list<ByteChunk> inStream; ///< Incoming raw data as a sequence of byte arrays
    Position startPos;        ///< Position at the start of the current open element
    Position curPos;          ///< Position of the next attribute as returned by getNextAttributeId
    Position endPos;          ///< Ending position after all attributes in current open element
    bool attributeRead;       ///< Has the last attribute returned by getNextAttributeId been read

    uint1 getByte(Position& pos) {
        return *pos.current;
    } ///< Get the byte at the current position, do not advance

    /// An exception is thrown if the position currently points to the last byte in the stream
    /// \param pos is the position in the stream to look ahead from
    /// \return the next byte
    uint1 getBytePlus1(Position& pos) {
        uint1* ptr = pos.current + 1;
        if (ptr == pos.end) {
            list<ByteChunk>::const_iterator iter = pos.seqIter;
            ++iter;
            if (iter == inStream.end())
                throw DecoderError("Unexpected end of stream");
            ptr = (*iter).start;
        }
        return *ptr;
    }

    /// An exception is thrown if there are no additional bytes in the stream
    /// \param pos is the position of the byte
    /// \return the byte at the current position
    uint1 getNextByte(Position& pos) {
        uint1 res = *pos.current;
        pos.current += 1;
        if (pos.current != pos.end)
            return res;
        ++pos.seqIter;
        if (pos.seqIter == inStream.end())
            throw DecoderError("Unexpected end of stream");
        pos.current = (*pos.seqIter).start;
        pos.end = (*pos.seqIter).end;
        return res;
    }

    /// An exception is thrown of position is advanced past the end of the stream
    /// \param pos is the position being advanced
    /// \param skip is the number of bytes to advance
    void advancePosition(Position& pos, uint4 skip) {
        while (pos.end - pos.current <= skip) {
            skip -= (pos.end - pos.current);
            ++pos.seqIter;
            if (pos.seqIter == inStream.end())
                throw DecoderError("Unexpected end of stream");
            pos.current = (*pos.seqIter).start;
            pos.end = (*pos.seqIter).end;
        }
        pos.current += skip;
    }

    /// The integer is encoded, 7-bits per byte, starting with the most significant 7-bits.
    /// The integer is decode from the \e current position, and the position is advanced.
    /// \param len is the number of bytes to extract
    uint8 readInteger(int4 len) {
        uint8 res = 0;
        while (len > 0) {
            res <<= RAWDATA_BITSPERBYTE;
            res |= (getNextByte(curPos) & RAWDATA_MASK);
            len -= 1;
        }
        return res;
    }

    uint4 readLengthCode(uint1 typeByte) {
        return ((uint4)typeByte & PackedFormat::LENGTHCODE_MASK);
    } ///< Extract length code from type byte

    /// The \e current position is reset to the start of the current open element. Attributes are scanned
    /// and skipped until the attribute matching the given id is found. The \e current position is set to the
    /// start of the matching attribute, in preparation for one of the read*() methods.
    /// If the id is not found an exception is thrown.
    /// \param attribId is the attribute id to scan for.
    void findMatchingAttribute(const AttributeId& attribId) {
        curPos = startPos;
        for (;;) {
            uint1 header1 = getByte(curPos);
            if ((header1 & HEADER_MASK) != ATTRIBUTE)
                break;
            uint4 id = header1 & ELEMENTID_MASK;
            if ((header1 & HEADEREXTEND_MASK) != 0) {
                id <<= RAWDATA_BITSPERBYTE;
                id |= (getBytePlus1(curPos) & RAWDATA_MASK);
            }
            if (attribId.getId() == id)
                return; // Found it
            skipAttribute();
        }
        throw DecoderError("Attribute " + attribId.getName() + " is not present");
    }

    /// The attribute at the \e current position is scanned enough to determine its length, and the position
    /// is advanced to the following byte.
    void skipAttribute(void) {
        uint1 header1 = getNextByte(curPos); // Attribute header
        if ((header1 & HEADEREXTEND_MASK) != 0)
            getNextByte(curPos);              // Extra byte for extended id
        uint1 typeByte = getNextByte(curPos); // Type (and length) byte
        uint1 attribType = typeByte >> TYPECODE_SHIFT;
        if (attribType == TYPECODE_BOOLEAN || attribType == TYPECODE_SPECIALSPACE)
            return;                              // has no additional data
        uint4 length = readLengthCode(typeByte); // Length of data in bytes
        if (attribType == TYPECODE_STRING) {
            length = readInteger(length); // Read length field to get final length of string
        }
        advancePosition(curPos, length); // Skip -length- data
    }

    /// This assumes the header and \b type \b byte have been read. Decode type and length info and finish
    /// skipping over the attribute so that the next call to getNextAttributeId() is on cut.
    /// \param typeByte is the previously scanned type byte
    void skipAttributeRemaining(uint1 typeByte) {
        uint1 attribType = typeByte >> TYPECODE_SHIFT;
        if (attribType == TYPECODE_BOOLEAN || attribType == TYPECODE_SPECIALSPACE)
            return;                              // has no additional data
        uint4 length = readLengthCode(typeByte); // Length of data in bytes
        if (attribType == TYPECODE_STRING) {
            length = readInteger(length); // Read length field to get final length of string
        }
        advancePosition(curPos, length); // Skip -length- data
    }

protected:
    /// Allocate a chunk of BUFFER_SIZE bytes and add it to the in-memory stream
    /// \param pad is the number of bytes of padding to add to the allocation size, above BUFFER_SIZE
    /// \return the newly allocated buffer
    uint1* allocateNextInputBuffer(int4 pad) {
        inStream.emplace_back(BUFFER_SIZE + pad);
        return inStream.back().start;
    }

    /// Set decoder to beginning of the stream. Add padding to end of the stream.
    /// \param bufPos is the number of bytes used by the last input buffer
    void endIngest(int4 bufPos) {
        endPos.seqIter = inStream.begin(); // Set position to beginning of stream
        if (endPos.seqIter != inStream.end()) {
            endPos.current = (*endPos.seqIter).start;
            endPos.end = (*endPos.seqIter).end;
            // Make sure there is at least one character after ingested buffer
            if (bufPos == BUFFER_SIZE) {
                // Last buffer was entirely filled
                inStream.emplace_back(1); // Add one more buffer
                bufPos = 0;
            }
            uint1* buf = inStream.back().start;
            buf[bufPos] = ELEMENT_END;
        } else {
            throw DecoderError("Ended ingestion without any input");
        }
    }

public:
    PackedDecode(const AddrSpaceManager* spcManager) : Decoder(spcManager) {} ///< Constructor
    virtual ~PackedDecode(void) = default;

    virtual void ingestStream(istream& s) {
        int4 gcount = 0;
        while (s.peek() > 0) {
            uint1* buf = allocateNextInputBuffer(1);
            s.get((char*)buf, BUFFER_SIZE + 1, '\0');
            gcount = s.gcount();
        }
        endIngest(gcount);
    }

    virtual uint4 peekElement(void) {
        uint1 header1 = getByte(endPos);
        if ((header1 & HEADER_MASK) != ELEMENT_START)
            return 0;
        uint4 id = header1 & ELEMENTID_MASK;
        if ((header1 & HEADEREXTEND_MASK) != 0) {
            id <<= RAWDATA_BITSPERBYTE;
            id |= (getBytePlus1(endPos) & RAWDATA_MASK);
        }
        return id;
    }

    virtual uint4 openElement(void) {
        uint1 header1 = getByte(endPos);
        if ((header1 & HEADER_MASK) != ELEMENT_START)
            return 0;
        getNextByte(endPos);
        uint4 id = header1 & ELEMENTID_MASK;
        if ((header1 & HEADEREXTEND_MASK) != 0) {
            id <<= RAWDATA_BITSPERBYTE;
            id |= (getNextByte(endPos) & RAWDATA_MASK);
        }
        startPos = endPos;
        curPos = endPos;
        header1 = getByte(curPos);
        while ((header1 & HEADER_MASK) == ATTRIBUTE) {
            skipAttribute();
            header1 = getByte(curPos);
        }
        endPos = curPos;
        curPos = startPos;
        attributeRead = true; // "Last attribute was read" is vacuously true
        return id;
    }

    virtual uint4 openElement(const ElementId& elemId) {
        uint4 id = openElement();
        if (id != elemId.getId()) {
            if (id == 0)
                throw DecoderError("Expecting <" + elemId.getName() + "> but did not scan an element");
            throw DecoderError("Expecting <" + elemId.getName() + "> but id did not match");
        }
        return id;
    }

    virtual void closeElement(uint4 id) {
        uint1 header1 = getNextByte(endPos);
        if ((header1 & HEADER_MASK) != ELEMENT_END)
            throw DecoderError("Expecting element close");
        uint4 closeId = header1 & ELEMENTID_MASK;
        if ((header1 & HEADEREXTEND_MASK) != 0) {
            closeId <<= RAWDATA_BITSPERBYTE;
            closeId |= (getNextByte(endPos) & RAWDATA_MASK);
        }
        if (id != closeId)
            throw DecoderError("Did not see expected closing element");
    }

    virtual void closeElementSkipping(uint4 id) {
        vector<uint4> idstack;
        idstack.push_back(id);
        do {
            uint1 header1 = getByte(endPos) & HEADER_MASK;
            if (header1 == ELEMENT_END) {
                closeElement(idstack.back());
                idstack.pop_back();
            } else if (header1 == ELEMENT_START) {
                idstack.push_back(openElement());
            } else
                throw DecoderError("Corrupt stream");
        } while (!idstack.empty());
    }

    virtual void rewindAttributes(void) {
        curPos = startPos;
        attributeRead = true;
    }

    virtual uint4 getNextAttributeId(void) {
        if (!attributeRead)
            skipAttribute();
        uint1 header1 = getByte(curPos);
        if ((header1 & HEADER_MASK) != ATTRIBUTE)
            return 0;
        uint4 id = header1 & ELEMENTID_MASK;
        if ((header1 & HEADEREXTEND_MASK) != 0) {
            id <<= RAWDATA_BITSPERBYTE;
            id |= (getBytePlus1(curPos) & RAWDATA_MASK);
        }
        attributeRead = false;
        return id;
    }

    virtual uint4 getIndexedAttributeId(const AttributeId& attribId) {
        return ATTRIB_UNKNOWN.getId(); // PackedDecode never needs to reinterpret an attribute
    }

    virtual bool readBool(void) {
        uint1 header1 = getNextByte(curPos);
        if ((header1 & HEADEREXTEND_MASK) != 0)
            getNextByte(curPos);
        uint1 typeByte = getNextByte(curPos);
        attributeRead = true;
        if ((typeByte >> TYPECODE_SHIFT) != TYPECODE_BOOLEAN)
            throw DecoderError("Expecting boolean attribute");
        return ((typeByte & LENGTHCODE_MASK) != 0);
    }

    virtual bool readBool(const AttributeId& attribId) {
        findMatchingAttribute(attribId);
        bool res = readBool();
        curPos = startPos;
        return res;
    }

    virtual intb readSignedInteger(void) {
        uint1 header1 = getNextByte(curPos);
        if ((header1 & HEADEREXTEND_MASK) != 0)
            getNextByte(curPos);
        uint1 typeByte = getNextByte(curPos);
        uint4 typeCode = typeByte >> TYPECODE_SHIFT;
        intb res;
        if (typeCode == TYPECODE_SIGNEDINT_POSITIVE) {
            res = readInteger(readLengthCode(typeByte));
        } else if (typeCode == TYPECODE_SIGNEDINT_NEGATIVE) {
            res = readInteger(readLengthCode(typeByte));
            res = -res;
        } else {
            skipAttributeRemaining(typeByte);
            attributeRead = true;
            throw DecoderError("Expecting signed integer attribute");
        }
        attributeRead = true;
        return res;
    }

    virtual intb readSignedInteger(const AttributeId& attribId) {
        findMatchingAttribute(attribId);
        intb res = readSignedInteger();
        curPos = startPos;
        return res;
    }

    virtual intb readSignedIntegerExpectString(const string& expect, intb expectval) {
        intb res;
        Position tmpPos = curPos;
        uint1 header1 = getNextByte(tmpPos);
        if ((header1 & HEADEREXTEND_MASK) != 0)
            getNextByte(tmpPos);
        uint1 typeByte = getNextByte(tmpPos);
        uint4 typeCode = typeByte >> TYPECODE_SHIFT;
        if (typeCode == TYPECODE_STRING) {
            string val = readString();
            if (val != expect) {
                ostringstream s;
                s << "Expecting string \"" << expect << "\" but read \"" << val << "\"";
                throw DecoderError(s.str());
            }
            res = expectval;
        } else {
            res = readSignedInteger();
        }
        return res;
    }

    virtual intb readSignedIntegerExpectString(const AttributeId& attribId, const string& expect, intb expectval) {
        findMatchingAttribute(attribId);
        intb res = readSignedIntegerExpectString(expect, expectval);
        curPos = startPos;
        return res;
    }

    virtual uintb readUnsignedInteger(void) {
        uint1 header1 = getNextByte(curPos);
        if ((header1 & HEADEREXTEND_MASK) != 0)
            getNextByte(curPos);
        uint1 typeByte = getNextByte(curPos);
        uint4 typeCode = typeByte >> TYPECODE_SHIFT;
        uintb res;
        if (typeCode == TYPECODE_UNSIGNEDINT) {
            res = readInteger(readLengthCode(typeByte));
        } else {
            skipAttributeRemaining(typeByte);
            attributeRead = true;
            throw DecoderError("Expecting unsigned integer attribute");
        }
        attributeRead = true;
        return res;
    }

    virtual uintb readUnsignedInteger(const AttributeId& attribId) {
        findMatchingAttribute(attribId);
        uintb res = readUnsignedInteger();
        curPos = startPos;
        return res;
    }

    virtual string readString(void) {
        uint1 header1 = getNextByte(curPos);
        if ((header1 & HEADEREXTEND_MASK) != 0)
            getNextByte(curPos);
        uint1 typeByte = getNextByte(curPos);
        uint4 typeCode = typeByte >> TYPECODE_SHIFT;
        if (typeCode != TYPECODE_STRING) {
            skipAttributeRemaining(typeByte);
            attributeRead = true;
            throw DecoderError("Expecting string attribute");
        }
        uint8 length = readInteger(readLengthCode(typeByte));

        attributeRead = true;
        uint8 curLen = curPos.end - curPos.current;
        if (curLen >= length) {
            string res((const char*)curPos.current, length);
            advancePosition(curPos, length);
            return res;
        }
        string res((const char*)curPos.current, curLen);
        length -= curLen;
        advancePosition(curPos, curLen);
        while (length > 0) {
            curLen = curPos.end - curPos.current;
            if (curLen > length)
                curLen = length;
            res.append((const char*)curPos.current, curLen);
            length -= curLen;
            advancePosition(curPos, curLen);
        }
        return res;
    }

    virtual string readString(const AttributeId& attribId) {
        findMatchingAttribute(attribId);
        string res = readString();
        curPos = startPos;
        return res;
    }

    virtual AddrSpace* readSpace(void) {
        uint1 header1 = getNextByte(curPos);
        if ((header1 & HEADEREXTEND_MASK) != 0)
            getNextByte(curPos);
        uint1 typeByte = getNextByte(curPos);
        uint4 typeCode = typeByte >> TYPECODE_SHIFT;
        AddrSpace* spc;
        if (typeCode == TYPECODE_ADDRESSSPACE) {
            uint8 res = readInteger(readLengthCode(typeByte));
            if (res >= addrSpaceManagerNumSpaces(spcManager))
                throw DecoderError("Invalid address space index");

            spc = addrSpaceManagerGetSpace(spcManager, res);
            if (spc == (AddrSpace*)0)
                throw DecoderError("Unknown address space index");
        } else if (typeCode == TYPECODE_SPECIALSPACE) {
            uint4 specialCode = readLengthCode(typeByte);
            if (specialCode == SPECIALSPACE_STACK)
                spc = addrSpaceManagerGetStackSpace(spcManager);
            else if (specialCode == SPECIALSPACE_JOIN) {
                spc = addrSpaceManagerGetJoinSpace(spcManager);
            } else {
                throw DecoderError("Cannot marshal special address space");
            }
        } else {
            skipAttributeRemaining(typeByte);
            attributeRead = true;
            throw DecoderError("Expecting space attribute");
        }
        attributeRead = true;
        return spc;
    }

    virtual AddrSpace* readSpace(const AttributeId& attribId) {
        findMatchingAttribute(attribId);
        AddrSpace* res = readSpace();
        curPos = startPos;
        return res;
    }

    virtual OpCode readOpcode(void) {
        int4 val = (int4)readSignedInteger();
        if (val < 0 || val >= CPUI_MAX)
            throw DecoderError("Bad encoded OpCode");
        return (OpCode)val;
    }

    virtual OpCode readOpcode(AttributeId& attribId) {
        findMatchingAttribute(attribId);
        OpCode opc = readOpcode();
        curPos = startPos;
        return opc;
    }
};

const int4 PackedDecode::BUFFER_SIZE = 1024;

// Common attributes. Attributes with multiple uses
AttributeId ATTRIB_CONTENT = AttributeId("XMLcontent", 1);
AttributeId ATTRIB_ALIGN = AttributeId("align", 2);
AttributeId ATTRIB_BIGENDIAN = AttributeId("bigendian", 3);
AttributeId ATTRIB_CONSTRUCTOR = AttributeId("constructor", 4);
AttributeId ATTRIB_DESTRUCTOR = AttributeId("destructor", 5);
AttributeId ATTRIB_EXTRAPOP = AttributeId("extrapop", 6);
AttributeId ATTRIB_FORMAT = AttributeId("format", 7);
AttributeId ATTRIB_HIDDENRETPARM = AttributeId("hiddenretparm", 8);
AttributeId ATTRIB_ID = AttributeId("id", 9);
AttributeId ATTRIB_INDEX = AttributeId("index", 10);
AttributeId ATTRIB_INDIRECTSTORAGE = AttributeId("indirectstorage", 11);
AttributeId ATTRIB_METATYPE = AttributeId("metatype", 12);
AttributeId ATTRIB_MODEL = AttributeId("model", 13);
AttributeId ATTRIB_NAME = AttributeId("name", 14);
AttributeId ATTRIB_NAMELOCK = AttributeId("namelock", 15);
AttributeId ATTRIB_OFFSET = AttributeId("offset", 16);
AttributeId ATTRIB_READONLY = AttributeId("readonly", 17);
AttributeId ATTRIB_REF = AttributeId("ref", 18);
AttributeId ATTRIB_SIZE = AttributeId("size", 19);
AttributeId ATTRIB_SPACE = AttributeId("space", 20);
AttributeId ATTRIB_THISPTR = AttributeId("thisptr", 21);
AttributeId ATTRIB_TYPE = AttributeId("type", 22);
AttributeId ATTRIB_TYPELOCK = AttributeId("typelock", 23);
AttributeId ATTRIB_VAL = AttributeId("val", 24);
AttributeId ATTRIB_VALUE = AttributeId("value", 25);
AttributeId ATTRIB_WORDSIZE = AttributeId("wordsize", 26);
AttributeId ATTRIB_STORAGE = AttributeId("storage", 149);
AttributeId ATTRIB_STACKSPILL = AttributeId("stackspill", 150);

AttributeId ATTRIB_UNKNOWN = AttributeId("XMLunknown", 159); // Number serves as next open index

ElementId ELEM_DATA = ElementId("data", 1);
ElementId ELEM_INPUT = ElementId("input", 2);
ElementId ELEM_OFF = ElementId("off", 3);
ElementId ELEM_OUTPUT = ElementId("output", 4);
ElementId ELEM_RETURNADDRESS = ElementId("returnaddress", 5);
ElementId ELEM_SYMBOL = ElementId("symbol", 6);
ElementId ELEM_TARGET = ElementId("target", 7);
ElementId ELEM_VAL = ElementId("val", 8);
ElementId ELEM_VALUE = ElementId("value", 9);
ElementId ELEM_VOID = ElementId("void", 10);

ElementId ELEM_UNKNOWN = ElementId("XMLunknown", 291); // Number serves as next open index

} // End namespace ghidra
