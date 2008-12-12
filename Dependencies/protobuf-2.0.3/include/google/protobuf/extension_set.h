// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.
//
// This header is logically internal, but is made public because it is used
// from protocol-compiler-generated code, which may reside in other components.

#ifndef GOOGLE_PROTOBUF_EXTENSION_SET_H__
#define GOOGLE_PROTOBUF_EXTENSION_SET_H__

#include <vector>
#include <stack>
#include <map>
#include <utility>
#include <string>

#include <google/protobuf/message.h>

namespace google {
namespace protobuf {
  class Descriptor;                                    // descriptor.h
  class FieldDescriptor;                               // descriptor.h
  class DescriptorPool;                                // descriptor.h
  class Message;                                       // message.h
  class MessageFactory;                                // message.h
  namespace io {
    class CodedInputStream;                              // coded_stream.h
    class CodedOutputStream;                             // coded_stream.h
  }
  template <typename Element> class RepeatedField;     // repeated_field.h
  template <typename Element> class RepeatedPtrField;  // repeated_field.h
}

namespace protobuf {
namespace internal {

// This is an internal helper class intended for use within the protocol buffer
// library and generated classes.  Clients should not use it directly.  Instead,
// use the generated accessors such as GetExtension() of the class being
// extended.
//
// This class manages extensions for a protocol message object.  The
// message's HasExtension(), GetExtension(), MutableExtension(), and
// ClearExtension() methods are just thin wrappers around the embedded
// ExtensionSet.  When parsing, if a tag number is encountered which is
// inside one of the message type's extension ranges, the tag is passed
// off to the ExtensionSet for parsing.  Etc.
class LIBPROTOBUF_EXPORT ExtensionSet {
 public:
  // Construct an ExtensionSet.
  //   extendee:  Descriptor for the type being extended. We pass in a pointer
  //              to a pointer to the extendee to get around an initialization
  //              problem: when we create the ExtensionSet for a message type,
  //              its descriptor may not exist yet. But we know where that
  //              descriptor pointer will be placed, and by the time it's used
  //              by this ExtensionSet it will be fully initialized, so passing
  //              a pointer to that location works. Note that this problem
  //              will only occur for messages defined in descriptor.proto.
  //   pool:      DescriptorPool to search for extension definitions.
  //   factory:   MessageFactory used to construct implementations of messages
  //              for extensions with message type.  This factory must be able
  //              to construct any message type found in "pool".
  // All three objects remain property of the caller and must outlive the
  // ExtensionSet.
  ExtensionSet(const Descriptor* const* extendee,
               const DescriptorPool* pool,
               MessageFactory* factory);

  ~ExtensionSet();

  // Add all fields which are currently present to the given vector.  This
  // is useful to implement Reflection::ListFields().
  void AppendToList(vector<const FieldDescriptor*>* output) const;

  // =================================================================
  // Accessors
  //
  // Generated message classes include type-safe templated wrappers around
  // these methods.  Generally you should use those rather than call these
  // directly, unless you are doing low-level memory management.
  //
  // When calling any of these accessors, the extension number requested
  // MUST exist in the DescriptorPool provided to the constructor.  Otheriwse,
  // the method will fail an assert.  Normally, though, you would not call
  // these directly; you would either call the generated accessors of your
  // message class (e.g. GetExtension()) or you would call the accessors
  // of the reflection interface.  In both cases, it is impossible to
  // trigger this assert failure:  the generated accessors only accept
  // linked-in extension types as parameters, while the Reflection interface
  // requires you to provide the FieldDescriptor describing the extension.
  //
  // When calling any of these accessors, a protocol-compiler-generated
  // implementation of the extension corresponding to the number MUST
  // be linked in, and the FieldDescriptor used to refer to it MUST be
  // the one generated by that linked-in code.  Otherwise, the method will
  // die on an assert failure.  The message objects returned by the message
  // accessors are guaranteed to be of the correct linked-in type.
  //
  // These methods pretty much match Reflection except that:
  // - They're not virtual.
  // - They identify fields by number rather than FieldDescriptors.
  // - They identify enum values using integers rather than descriptors.
  // - Strings provide Mutable() in addition to Set() accessors.

  bool Has(int number) const;
  int ExtensionSize(int number) const;   // Size of a repeated extension.
  void ClearExtension(int number);

  // singular fields -------------------------------------------------

  int32  GetInt32 (int number) const;
  int64  GetInt64 (int number) const;
  uint32 GetUInt32(int number) const;
  uint64 GetUInt64(int number) const;
  float  GetFloat (int number) const;
  double GetDouble(int number) const;
  bool   GetBool  (int number) const;
  int    GetEnum  (int number) const;
  const string & GetString (int number) const;
  const Message& GetMessage(int number) const;

  void SetInt32 (int number, int32  value);
  void SetInt64 (int number, int64  value);
  void SetUInt32(int number, uint32 value);
  void SetUInt64(int number, uint64 value);
  void SetFloat (int number, float  value);
  void SetDouble(int number, double value);
  void SetBool  (int number, bool   value);
  void SetEnum  (int number, int    value);
  void SetString(int number, const string& value);
  string * MutableString (int number);
  Message* MutableMessage(int number);

  // repeated fields -------------------------------------------------

  int32  GetRepeatedInt32 (int number, int index) const;
  int64  GetRepeatedInt64 (int number, int index) const;
  uint32 GetRepeatedUInt32(int number, int index) const;
  uint64 GetRepeatedUInt64(int number, int index) const;
  float  GetRepeatedFloat (int number, int index) const;
  double GetRepeatedDouble(int number, int index) const;
  bool   GetRepeatedBool  (int number, int index) const;
  int    GetRepeatedEnum  (int number, int index) const;
  const string & GetRepeatedString (int number, int index) const;
  const Message& GetRepeatedMessage(int number, int index) const;

  void SetRepeatedInt32 (int number, int index, int32  value);
  void SetRepeatedInt64 (int number, int index, int64  value);
  void SetRepeatedUInt32(int number, int index, uint32 value);
  void SetRepeatedUInt64(int number, int index, uint64 value);
  void SetRepeatedFloat (int number, int index, float  value);
  void SetRepeatedDouble(int number, int index, double value);
  void SetRepeatedBool  (int number, int index, bool   value);
  void SetRepeatedEnum  (int number, int index, int    value);
  void SetRepeatedString(int number, int index, const string& value);
  string * MutableRepeatedString (int number, int index);
  Message* MutableRepeatedMessage(int number, int index);

  void AddInt32 (int number, int32  value);
  void AddInt64 (int number, int64  value);
  void AddUInt32(int number, uint32 value);
  void AddUInt64(int number, uint64 value);
  void AddFloat (int number, float  value);
  void AddDouble(int number, double value);
  void AddBool  (int number, bool   value);
  void AddEnum  (int number, int    value);
  void AddString(int number, const string& value);
  string * AddString (int number);
  Message* AddMessage(int number);

  // -----------------------------------------------------------------
  // TODO(kenton):  Hardcore memory management accessors

  // =================================================================
  // convenience methods for implementing methods of Message
  //
  // These could all be implemented in terms of the other methods of this
  // class, but providing them here helps keep the generated code size down.

  void Clear();
  void MergeFrom(const ExtensionSet& other);
  void Swap(ExtensionSet* other);
  bool IsInitialized() const;

  // These parsing and serialization functions all want a pointer to the
  // message object because they hand off the actual work to WireFormat,
  // which works in terms of a reflection interface.  Yes, this means there
  // are some redundant virtual function calls that end up being made, but
  // it probably doesn't matter much in practice, and the alternative would
  // involve reproducing a lot of WireFormat's functionality.

  // Parses a single extension from the input.  The input should start out
  // positioned immediately after the tag.
  bool ParseField(uint32 tag, io::CodedInputStream* input, Message* message);

  // Write all extension fields with field numbers in the range
  //   [start_field_number, end_field_number)
  // to the output stream, using the cached sizes computed when ByteSize() was
  // last called.  Note that the range bounds are inclusive-exclusive.
  bool SerializeWithCachedSizes(int start_field_number,
                                int end_field_number,
                                const Message& message,
                                io::CodedOutputStream* output) const;

  // Returns the total serialized size of all the extensions.
  int ByteSize(const Message& message) const;

  // Returns (an estimate of) the total number of bytes used for storing the
  // extensions in memory, excluding sizeof(*this).
  int SpaceUsedExcludingSelf() const;

 private:
  // Like FindKnownExtension(), but GOOGLE_CHECK-fail if not found.
  const FieldDescriptor* FindKnownExtensionOrDie(int number) const;

  // Get the prototype for the message.
  const Message* GetPrototype(const Descriptor* message_type) const;

  struct Extension {
    union {
      int32    int32_value;
      int64    int64_value;
      uint32   uint32_value;
      uint64   uint64_value;
      float    float_value;
      double   double_value;
      bool     bool_value;
      int      enum_value;
      string*  string_value;
      Message* message_value;

      RepeatedField   <int32  >* repeated_int32_value;
      RepeatedField   <int64  >* repeated_int64_value;
      RepeatedField   <uint32 >* repeated_uint32_value;
      RepeatedField   <uint64 >* repeated_uint64_value;
      RepeatedField   <float  >* repeated_float_value;
      RepeatedField   <double >* repeated_double_value;
      RepeatedField   <bool   >* repeated_bool_value;
      RepeatedField   <int    >* repeated_enum_value;
      RepeatedPtrField<string >* repeated_string_value;
      RepeatedPtrField<Message>* repeated_message_value;
    };

    const FieldDescriptor* descriptor;

    // For singular types, indicates if the extension is "cleared".  This
    // happens when an extension is set and then later cleared by the caller.
    // We want to keep the Extension object around for reuse, so instead of
    // removing it from the map, we just set is_cleared = true.  This has no
    // meaning for repeated types; for those, the size of the RepeatedField
    // simply becomes zero when cleared.
    bool is_cleared;

    Extension(): descriptor(NULL), is_cleared(false) {}

    // Some helper methods for operations on a single Extension.
    bool SerializeFieldWithCachedSizes(
        const Message& message,
        io::CodedOutputStream* output) const;
    int64 ByteSize(const Message& message) const;
    void Clear();
    int GetSize() const;
    void Free();
    int SpaceUsedExcludingSelf() const;
  };

  // The Extension struct is small enough to be passed by value, so we use it
  // directly as the value type in the map rather than use pointers.  We use
  // a map rather than hash_map here because we expect most ExtensionSets will
  // only contain a small number of extensions whereas hash_map is optimized
  // for 100 elements or more.  Also, we want AppendToList() to order fields
  // by field number.
  map<int, Extension> extensions_;
  const Descriptor* const* extendee_;
  const DescriptorPool* descriptor_pool_;
  MessageFactory* message_factory_;

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(ExtensionSet);
};

// These are just for convenience...
inline void ExtensionSet::SetString(int number, const string& value) {
  MutableString(number)->assign(value);
}
inline void ExtensionSet::SetRepeatedString(int number, int index,
                                            const string& value) {
  MutableRepeatedString(number, index)->assign(value);
}
inline void ExtensionSet::AddString(int number, const string& value) {
  AddString(number)->assign(value);
}

// ===================================================================
// Implementation details
//
// DO NOT DEPEND ON ANYTHING BELOW THIS POINT.  This is for use from
// generated code only.

// -------------------------------------------------------------------
// Template magic

// First we have a set of classes representing "type traits" for different
// field types.  A type traits class knows how to implement basic accessors
// for extensions of a particular type given an ExtensionSet.  The signature
// for a type traits class looks like this:
//
//   class TypeTraits {
//    public:
//     typedef ? ConstType;
//     typedef ? MutableType;
//
//     static inline ConstType Get(int number, const ExtensionSet& set);
//     static inline void Set(int number, ConstType value, ExtensionSet* set);
//     static inline MutableType Mutable(int number, ExtensionSet* set);
//
//     // Variants for repeated fields.
//     static inline ConstType Get(int number, const ExtensionSet& set,
//                                 int index);
//     static inline void Set(int number, int index,
//                            ConstType value, ExtensionSet* set);
//     static inline MutableType Mutable(int number, int index,
//                                       ExtensionSet* set);
//     static inline void Add(int number, ConstType value, ExtensionSet* set);
//     static inline MutableType Add(int number, ExtensionSet* set);
//   };
//
// Not all of these methods make sense for all field types.  For example, the
// "Mutable" methods only make sense for strings and messages, and the
// repeated methods only make sense for repeated types.  So, each type
// traits class implements only the set of methods from this signature that it
// actually supports.  This will cause a compiler error if the user tries to
// access an extension using a method that doesn't make sense for its type.
// For example, if "foo" is an extension of type "optional int32", then if you
// try to write code like:
//   my_message.MutableExtension(foo)
// you will get a compile error because PrimitiveTypeTraits<int32> does not
// have a "Mutable()" method.

// -------------------------------------------------------------------
// PrimitiveTypeTraits

// Since the ExtensionSet has different methods for each primitive type,
// we must explicitly define the methods of the type traits class for each
// known type.
template <typename Type>
class PrimitiveTypeTraits {
 public:
  typedef Type ConstType;

  static inline ConstType Get(int number, const ExtensionSet& set);
  static inline void Set(int number, ConstType value, ExtensionSet* set);
};

template <typename Type>
class RepeatedPrimitiveTypeTraits {
 public:
  typedef Type ConstType;

  static inline Type Get(int number, const ExtensionSet& set, int index);
  static inline void Set(int number, int index, Type value, ExtensionSet* set);
  static inline void Add(int number, Type value, ExtensionSet* set);
};

#define PROTOBUF_DEFINE_PRIMITIVE_TYPE(TYPE, METHOD)                       \
template<> inline TYPE PrimitiveTypeTraits<TYPE>::Get(                     \
    int number, const ExtensionSet& set) {                                 \
  return set.Get##METHOD(number);                                          \
}                                                                          \
template<> inline void PrimitiveTypeTraits<TYPE>::Set(                     \
    int number, ConstType value, ExtensionSet* set) {                      \
  set->Set##METHOD(number, value);                                         \
}                                                                          \
                                                                           \
template<> inline TYPE RepeatedPrimitiveTypeTraits<TYPE>::Get(             \
    int number, const ExtensionSet& set, int index) {                      \
  return set.GetRepeated##METHOD(number, index);                           \
}                                                                          \
template<> inline void RepeatedPrimitiveTypeTraits<TYPE>::Set(             \
    int number, int index, ConstType value, ExtensionSet* set) {           \
  set->SetRepeated##METHOD(number, index, value);                          \
}                                                                          \
template<> inline void RepeatedPrimitiveTypeTraits<TYPE>::Add(             \
    int number, ConstType value, ExtensionSet* set) {                      \
  set->Add##METHOD(number, value);                                         \
}

PROTOBUF_DEFINE_PRIMITIVE_TYPE( int32,  Int32)
PROTOBUF_DEFINE_PRIMITIVE_TYPE( int64,  Int64)
PROTOBUF_DEFINE_PRIMITIVE_TYPE(uint32, UInt32)
PROTOBUF_DEFINE_PRIMITIVE_TYPE(uint64, UInt64)
PROTOBUF_DEFINE_PRIMITIVE_TYPE( float,  Float)
PROTOBUF_DEFINE_PRIMITIVE_TYPE(double, Double)
PROTOBUF_DEFINE_PRIMITIVE_TYPE(  bool,   Bool)

#undef PROTOBUF_DEFINE_PRIMITIVE_TYPE

// -------------------------------------------------------------------
// StringTypeTraits

// Strings support both Set() and Mutable().
class LIBPROTOBUF_EXPORT StringTypeTraits {
 public:
  typedef const string& ConstType;
  typedef string* MutableType;

  static inline const string& Get(int number, const ExtensionSet& set) {
    return set.GetString(number);
  }
  static inline void Set(int number, const string& value, ExtensionSet* set) {
    set->SetString(number, value);
  }
  static inline string* Mutable(int number, ExtensionSet* set) {
    return set->MutableString(number);
  }
};

class LIBPROTOBUF_EXPORT RepeatedStringTypeTraits {
 public:
  typedef const string& ConstType;
  typedef string* MutableType;

  static inline const string& Get(int number, const ExtensionSet& set,
                                  int index) {
    return set.GetRepeatedString(number, index);
  }
  static inline void Set(int number, int index,
                         const string& value, ExtensionSet* set) {
    set->SetRepeatedString(number, index, value);
  }
  static inline string* Mutable(int number, int index, ExtensionSet* set) {
    return set->MutableRepeatedString(number, index);
  }
  static inline void Add(int number, const string& value, ExtensionSet* set) {
    set->AddString(number, value);
  }
  static inline string* Add(int number, ExtensionSet* set) {
    return set->AddString(number);
  }
};

// -------------------------------------------------------------------
// EnumTypeTraits

// ExtensionSet represents enums using integers internally, so we have to
// static_cast around.
template <typename Type>
class EnumTypeTraits {
 public:
  typedef Type ConstType;

  static inline ConstType Get(int number, const ExtensionSet& set) {
    return static_cast<Type>(set.GetEnum(number));
  }
  static inline void Set(int number, ConstType value, ExtensionSet* set) {
    set->SetEnum(number, value);
  }
};

template <typename Type>
class RepeatedEnumTypeTraits {
 public:
  typedef Type ConstType;

  static inline ConstType Get(int number, const ExtensionSet& set, int index) {
    return static_cast<Type>(set.GetRepeatedEnum(number, index));
  }
  static inline void Set(int number, int index,
                         ConstType value, ExtensionSet* set) {
    set->SetRepeatedEnum(number, index, value);
  }
  static inline void Add(int number, ConstType value, ExtensionSet* set) {
    set->AddEnum(number, value);
  }
};

// -------------------------------------------------------------------
// MessageTypeTraits

// ExtensionSet guarantees that when manipulating extensions with message
// types, the implementation used will be the compiled-in class representing
// that type.  So, we can static_cast down to the exact type we expect.
template <typename Type>
class MessageTypeTraits {
 public:
  typedef const Type& ConstType;
 typedef Type* MutableType;

  static inline ConstType Get(int number, const ExtensionSet& set) {
    return static_cast<const Type&>(set.GetMessage(number));
  }
  static inline MutableType Mutable(int number, ExtensionSet* set) {
    return static_cast<Type*>(set->MutableMessage(number));
  }
};

template <typename Type>
class RepeatedMessageTypeTraits {
 public:
  typedef const Type& ConstType;
  typedef Type* MutableType;

  static inline ConstType Get(int number, const ExtensionSet& set, int index) {
    return static_cast<const Type&>(set.GetRepeatedMessage(number, index));
  }
  static inline MutableType Mutable(int number, int index, ExtensionSet* set) {
    return static_cast<Type*>(set->MutableRepeatedMessage(number, index));
  }
  static inline MutableType Add(int number, ExtensionSet* set) {
    return static_cast<Type*>(set->AddMessage(number));
  }
};

// -------------------------------------------------------------------
// ExtensionIdentifier

// This is the type of actual extension objects.  E.g. if you have:
//   extends Foo with optional int32 bar = 1234;
// then "bar" will be defined in C++ as:
//   ExtensionIdentifier<Foo, PrimitiveTypeTraits<int32>> bar(1234);
//
// Note that we could, in theory, supply the field number as a template
// parameter, and thus make an instance of ExtensionIdentifier have no
// actual contents.  However, if we did that, then using at extension
// identifier would not necessarily cause the compiler to output any sort
// of reference to any simple defined in the extension's .pb.o file.  Some
// linkers will actually drop object files that are not explicitly referenced,
// but that would be bad because it would cause this extension to not be
// registered at static initialization, and therefore using it would crash.

template <typename ExtendeeType, typename TypeTraitsType>
class ExtensionIdentifier {
 public:
  typedef TypeTraitsType TypeTraits;
  typedef ExtendeeType Extendee;

  ExtensionIdentifier(int number): number_(number) {}
  inline int number() const { return number_; }
 private:
  const int number_;
};

}  // namespace internal
}  // namespace protobuf

}  // namespace google
#endif  // GOOGLE_PROTOBUF_EXTENSION_SET_H__
