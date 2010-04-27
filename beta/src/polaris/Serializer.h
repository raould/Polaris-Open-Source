// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#pragma once

class Serializer {
protected:
    size_t m_promised;
    size_t m_written;
	BYTE* m_buffer;
	size_t m_capacity;

    // Forbidden operations.
	Serializer& operator=(const Serializer& x);
	Serializer(const Serializer& x);

public:
    /**
     * Constructs a Serializer.
     * @param promised  The number of bytes promised.
     */
    Serializer(size_t promised);
    virtual ~Serializer();

    /**
     * Get the number of bytes sent so far.
     */
    size_t count() const;

    /**
     * Allocate a block to be sent later.
     * The block pointer is sent at the current index.
     * @param ptr		The buffer pointer.
     * @param size		The allocated block size.
     */
    void promise(const void* ptr, size_t size);

    /**
     * Allocate a string to be sent later.
     * @param ptr		The string pointer.
     */
    void promise(const char* ptr);

    /**
     * Allocate a wide string to be sent later.
     * @param ptr		The wide string pointer.
     */
    void promise(const wchar_t* ptr);

    /**
     * Write out bytes at the current request index.
     * @param buffer	The byte buffer.
     * @param len       The number of bytes to send.
     */
    void send(const void* buffer, size_t len);

    /**
     * Write out string bytes.
     * @param buffer    The string.
     */
    void send(const char* buffer);

    /**
     * Write out wide string bytes.
     * @param buffer    The wide string.
     */
    void send(const wchar_t* buffer);

	/**
	 * Copy the serialized data into a buffer.
	 * @param buffer	The output buffer.
	 * @param size		The size of the output buffer.
	 */
	size_t dump(BYTE* buffer, size_t size);
};