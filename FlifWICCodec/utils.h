#pragma once

#include <assert.h>
#include <windows.h>
#include "dllmain.h"

class SectionLock {
public:
	SectionLock(CRITICAL_SECTION* cs) :cs_(cs) { EnterCriticalSection(cs_); }
	~SectionLock() { LeaveCriticalSection(cs_); }
private:
	CRITICAL_SECTION* cs_;
};

// A wrapper around a pointer to a COM object that does a Release on
// descruction. T should be a subclass of IUnknown.
template<typename T>
class ComPtr {
public:
	ComPtr() :ptr_(NULL) { }
	// ptr should be already AddRef'ed for this reference.
	ComPtr(T* ptr) :ptr_(ptr) { }
	~ComPtr() { if (ptr_) ptr_->Release(); }

	T* get() { return ptr_; }
	T* new_ref() { ptr_->AddRef(); return ptr_; }
	// new_ptr should be already AddRef'ed for this new reference.
	void reset(T* new_ptr) { if (ptr_ != NULL) ptr_->Release(); ptr_ = new_ptr; }
	// Allows to pass the the pointer as an 'out' parameter. If a non-NULL value
	// is written to it, it should be a valid pointer and already AddRef'ed for
	// this new reference.
	T** get_out_storage() { reset(NULL); return &ptr_; }
	T* operator->() { return ptr_; }
	T& operator*() { return *ptr_; }
private:
	T* ptr_;

	// No copy and assign.
	ComPtr(const ComPtr&) = delete;
	void operator=(const ComPtr&) = delete;
};

// Implements handling of object's and DLL's COM reference counts.
// T should be a subinterface of IUnknown. Templating used to avoid unnecessary
// mulitple inheritance.
template<typename T>
class ComObjectBase : public T {
public:
	ComObjectBase() {
		TRACE1("(%p)\n", this);
		InterlockedIncrement(&MAIN_nObjects);
		ref_count_ = 1;
	}
	virtual ~ComObjectBase() {
		TRACE1("(%p)\n", this);
		InterlockedDecrement(&MAIN_nObjects);
	}

	// IUnknown methods:
	virtual ULONG STDMETHODCALLTYPE AddRef() {
		return InterlockedIncrement(&ref_count_);
	}

	virtual ULONG STDMETHODCALLTYPE Release() {
		ULONG ret = InterlockedDecrement(&ref_count_);
		if (ret == 0)
			delete this;
		return ret;
	}

protected:
	volatile ULONG ref_count_;
};

// Can't use e.g., auto_ptr<BYTE*> because it's using delete and not delete[].
class scoped_buffer {
public:
	scoped_buffer(SIZE_T size) { ptr_ = (BYTE*)CoTaskMemAlloc(size); }
	~scoped_buffer() { CoTaskMemFree(ptr_); }

	// Did the allocation succeed.
	bool alloc_failed() { return ptr_ == NULL; }
	BYTE* get() { assert(ptr_ != NULL); return ptr_; }
private:
	BYTE* ptr_;
};

