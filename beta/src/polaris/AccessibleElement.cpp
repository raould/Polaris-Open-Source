// Copyright 2010 Hewlett-Packard under the terms of the MIT X license
// found at http://www.opensource.org/licenses/mit-license.html

#include "stdafx.h"
#include "AccessibleElement.h"

AccessibleElement::AccessibleElement(AccessibleElement& element) {
	myAcc = element.myAcc;
	myParent = element.myParent;
	VariantInit(&myChildId);
	myChildId.vt = VT_I4;
	myChildId.intVal = element.myChildId.intVal;
}

AccessibleElement::AccessibleElement(HWND window) {
	CoInitialize(NULL); // without initialization, get_accChild sometimes fails
	AccessibleObjectFromWindow(window, OBJID_WINDOW, IID_IAccessible, (void**) &myAcc);
	myParent = myAcc;
	VariantInit(&myChildId);
	myChildId.vt = VT_I4;
	myChildId.intVal = CHILDID_SELF;
}

AccessibleElement::AccessibleElement(IAccessible* self) {
	CoInitialize(NULL); // without initialization, get_accChild sometimes fails
	myAcc = myParent = self;
	VariantInit(&myChildId);
	myChildId.vt = VT_I4;
	myChildId.intVal = CHILDID_SELF;
}

AccessibleElement::AccessibleElement(IAccessible* parent, int childId) {
	CoInitialize(NULL); // without initialization, get_accChild sometimes fails
	VariantInit(&myChildId);
	myChildId.vt = VT_I4;
	myChildId.intVal = childId;
	IDispatch* disp;
	HRESULT result = parent->get_accChild(myChildId, &disp);
	if (result == S_OK) {
		disp->QueryInterface(IID_IAccessible, (void**) &myAcc);
		myParent = myAcc;
		myChildId.intVal = CHILDID_SELF;
	} else {
		myAcc = NULL;
		myParent = parent;
	}
}

AccessibleElement::~AccessibleElement(void) {
	VariantClear(&myChildId);
}

int AccessibleElement::GetRole() {
	VARIANT var;
	VariantInit(&var);
	myParent->get_accRole(myChildId, &var);
	int role = var.vt == VT_I4 ? var.intVal : 0;
	VariantClear(&var);
	return role;
}

int AccessibleElement::GetState() {
	VARIANT var;
	VariantInit(&var);
	myParent->get_accState(myChildId, &var);
	int state = var.vt == VT_I4 ? var.intVal : 0;
	VariantClear(&var);
	return state;
}

WCHAR* AccessibleElement::GetName() {
	WCHAR* name;
	myParent->get_accName(myChildId, &name);
	return name;
}

void AccessibleElement::SetName(WCHAR* name) {
	myParent->put_accName(myChildId, name);
}

WCHAR* AccessibleElement::GetValue() {
	WCHAR* value = NULL;
	myParent->get_accValue(myChildId, &value);
	return value;
}

void AccessibleElement::SetValue(WCHAR* value) {
	myParent->put_accValue(myChildId, value);
}

void AccessibleElement::Focus() {
	myParent->accSelect(SELFLAG_TAKEFOCUS, myChildId);
}

void AccessibleElement::Select() {
	myParent->accSelect(SELFLAG_TAKESELECTION, myChildId);
}

void AccessibleElement::Activate() {
	myParent->accDoDefaultAction(myChildId);
}

int AccessibleElement::CountChildren() {
	long count = 0;
	myParent->get_accChildCount(&count);
	return count;
}

AccessibleChildIterator AccessibleElement::GetChildren() {
	return AccessibleChildIterator(myAcc);
}

AccessibleChildIterator::AccessibleChildIterator(IAccessible* acc) {
	myAcc = acc;
	myCount = 0;
	myElements = NULL;
	myIndex = 0;
	if (myAcc) {
		// Get an array of VARIANTs representing the children.
		myAcc->get_accChildCount(&myCount);
		myElements = new VARIANT[myCount];
		AccessibleChildren(myAcc, 0, myCount, myElements, &myCount);
	}
}

AccessibleChildIterator::~AccessibleChildIterator() {
	if (myElements) {
		for (int i = 0; i < myCount; i++) {
			VariantClear(&myElements[i]);
		}
		delete[] myElements;
	}
}

bool AccessibleChildIterator::HasNext() {
	return myIndex < myCount;
}

AccessibleElement AccessibleChildIterator::GetNext() {
	if (myIndex < myCount) {
		if (myElements[myIndex].vt == VT_I4) {
			// Convert a child ID to an AccessibleElement*.
			int childId = myElements[myIndex].intVal;
			myIndex++;
			return AccessibleElement(myAcc, childId);
		}
		if (myElements[myIndex].vt == VT_DISPATCH) {
			// Convert an IDispatch* to an AccessibleElement*.
			IAccessible* childAcc = NULL;
			myElements[myIndex].pdispVal->QueryInterface(IID_IAccessible, (void**) &childAcc);
			myIndex++;
			return AccessibleElement(childAcc);
		}
	}
	return AccessibleElement((IAccessible*) NULL);
}