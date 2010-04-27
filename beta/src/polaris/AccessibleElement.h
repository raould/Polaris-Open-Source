// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#pragma once
#include <oleacc.h>

class AccessibleChildIterator;

// Use this class to manipulate windows UI elements using Windows accessibility
// features.  An AccessibleElement represents a single control such as a window,
// button, or listbox.  The GetRole, GetState, GetName, and GetValue methods
// retrieve information about the element; the SetName and SetValue methods modify
// the element.  The Focus, Select, and Activate methods move the focus, make a
// selection, and activate buttons.  Call GetChildren to get an iterator over an
// element's children.

class AccessibleElement {
public:
	// For a leaf node, myAcc != NULL, myParent == myAcc, myChild == CHILDID_SELF == 0.
	// For a non-leaf node, myAcc == NULL, myParent != NULL, myChild > 0.
	IAccessible* myAcc;
	IAccessible* myParent;
	VARIANT myChildId;

public:
	AccessibleElement(AccessibleElement& element); // Copy an existing AccessibleElement.
	AccessibleElement(HWND window); // Wrap a window handle.
	AccessibleElement(IAccessible* self); // Wrap an IAccessible non-leaf node.
	AccessibleElement(IAccessible* parent, int childId); // Wrap an IAccessible leaf node.
	~AccessibleElement();

	int GetRole(); // Get the role of this element (see the ROLE_* constants in oleacc.h).
	int GetState(); // Get the state of the element (see the STATE_* constants in oleacc.h).
	WCHAR* GetName(); // Get the name of this element as a newly allocated string.
	void SetName(WCHAR* name); // Set the name of this element.
	WCHAR* GetValue(); // Get the value of this element as a newly allocated string.
	void SetValue(WCHAR* value); // Set the value of this element.

	void Focus(); // Focus on this element.
	void Select(); // Select this element.
	void Activate(); // Activate this element.

	int CountChildren(); // Return the number of child elements.
	AccessibleChildIterator GetChildren(); // Get an iterator over this element's children.
};

class AccessibleChildIterator {
	IAccessible* myAcc;
	long myCount;
	VARIANT* myElements;
	long myIndex;

public:
	AccessibleChildIterator(IAccessible* acc);
	~AccessibleChildIterator();

	bool HasNext(); // Return TRUE if there are more children.
	AccessibleElement GetNext(); // Get the next child and advance the iterator.
};