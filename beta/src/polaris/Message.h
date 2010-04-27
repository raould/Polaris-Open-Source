// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#pragma once

// An instance of Message holds the data of a serialized message and has
// methods for deserializing the data.  The data in a message begins with
// a root struct, which may contain pointers to other structs that appear
// later in the message.  When the message is serialized, these pointers
// are translated into relative pointers (from the start of the message).
// The fix() methods in Message resolve these relative pointers back into
// usable pointers.
//
// A Message may be constructed from a data buffer either by handing over
// ownership of a buffer (the Message will modify its contents and delete[]
// the buffer), or by copying the contents of a provided buffer (the
// original buffer will be left untouched and the Message will manage its
// own internal copy of the data).

class Message {
    bool m_owner;
    BYTE* m_buffer;
    size_t m_min;
    size_t m_max;
    size_t m_fixed;

    void release();

public:
    // Create a null (zero-length) message.
    Message();

    // Create a Message that claims ownership of the provided buffer and
	// will take responsibility for calling delete[] on the provided buffer.
    // @param buffer    The binary blob.
    // @param min       The start index into the blob.
    // @param max       The end index into the blob.
    Message(BYTE* buffer, size_t min, size_t max);

	// Create a Message that makes its own internal copy of the data in
	// the provided buffer and will leave the provided buffer untouched.
	// @param buffer	Pointer to the binary data.
	// @param size		The size of the data.
	Message(const BYTE* buffer, size_t size);

    // Create a message that takes ownership of the buffer within another Message.
    Message(Message& x);

    // Destructor.  If this Message owns its buffer, it will delete[] it.
    ~Message();

    // Take ownership of the buffer within another Message.
    Message& operator=(Message& x);

    // Get the message length.
    size_t length() const;

	// Set the size of the root struct of the message.
	// @param slot		The address of the pointer to adjust.
    // @param size		The size of the referred to struct.
    void cast(void* slot, size_t size);
    void cast(char** slot);
    void cast(wchar_t** slot);

    // Resolve a struct pointer within the message.
    // @param slot		The address of the pointer to adjust.
    // @param size		The size of the referred to struct.
    void fix(void* slot, size_t size);

    // Resolve a string pointer within the message.
    // @param slot		The address of the pointer to adjust.
    void fix(const char** slot);
    void fix(char** slot);

    // Resolve a wide-string pointer within the message.
    // @param slot		The address of the pointer to adjust.
    void fix(const wchar_t** slot);
    void fix(wchar_t** slot);

    // Restore a pointer in the message to its serialized form.
    // @param slot		The address of the pointer to adjust.
    void serialize(void* slot);
};
