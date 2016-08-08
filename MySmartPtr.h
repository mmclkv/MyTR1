#ifndef _MYSMARTPTR_H_
#define _MYSMARTPTR_H_

#include <atomic>
#include <cassert>
#include <type_traits>
#include <utility>

template <typename _T>
class MySharedPtr;

template <typename _T>
class MyWeakPtr;

class Ref_Count {
public:
	Ref_Count() 
		: _use(1), _weak_use(0)
	{
	}

	void _Increment() { 
		++_use;
	}

	void _Decrement() {
		--_use;
		if (_use == 0) {
			_Destroy();
			if (_weak_use == 0)
				_Delete();
		}
	}

	void _Increment_Weak() {
		++_weak_use;
	}

	void _Decrement_Weak() {
		--_weak_use;
		if (_use == 0 && _weak_use == 0) {
			_Delete();
		}
	}

	unsigned long _Get_Use_Count() {
		return _use.load();
	}

	virtual ~Ref_Count() noexcept 
	{
	}

	// destroy the object pointed by the pointer 
	virtual void _Destroy() noexcept = 0;

	// destroy the reference count object
	virtual void _Delete() noexcept = 0;

private:
	std::atomic<unsigned long> _use;
	std::atomic<unsigned long> _weak_use;
};

template <typename _T>
class Ref_Count_Default : public Ref_Count {
public:
	Ref_Count_Default(_T* _rawPtr) 
		: Ref_Count(), _ptr(_rawPtr)
	{
	}

	virtual void _Destroy() noexcept {
		delete _ptr;
	}

	virtual void _Delete() noexcept {
		delete this;
	}

private:
	_T* _ptr;
};

template <typename _T, typename _D>
class Ref_Count_Del : public Ref_Count {
public:
	Ref_Count_Del(_T* _rawPtr, _D _deleter)
		: Ref_Count(), _myPair(_deleter, _rawPtr) 
	{
	}

	virtual void _Destroy() noexcept {
		_myPair.first(_myPair.second);
	}

	virtual void _Delete() noexcept {
		delete this;
	}

private:
	std::pair<_D, _T*> _myPair;
};

template <typename _T, typename _D, typename _Alloc>
class Ref_Count_Del_Alloc : public Ref_Count {
public:
	Ref_Count_Del_Alloc(_T* _rawPtr, _D _deleter, _Alloc _alloc) 
		: Ref_Count(), _myPair(std::pair<_D, _T*>(_deleter, _rawPtr), _alloc)
	{
	}

	virtual void _Destroy() noexcept {
		_myPair.first.first(_myPair.first.second);
	}

	virtual void _Delete() noexcept {
		typedef Ref_Count_Del_Alloc<_T, _D, _Alloc> _MyRefType;
		typename _Alloc::rebind<_MyRefType>::other _actual_Alloc(_myPair.second);
		_actual_Alloc.destroy(this);
		_actual_Alloc.deallocate(this, 1);
	}

private:
	std::pair<std::pair<_D, _T*>, _Alloc> _myPair;
};

template <typename _T>
class MySharedPtr {
	template <typename _Tx> 
	friend class MySharedPtr;

	template <typename _Tx>
	friend class MyWeakPtr;
public:
	typedef _T Element_type;

	constexpr MySharedPtr() noexcept
		// default constructor
		: _myPtr(nullptr), _myRefC(nullptr)
	{
	}

	constexpr MySharedPtr(std::nullptr_t) noexcept
		//null pointer constructor
		: _myPtr(nullptr), _myRefC(nullptr)
	{
	}

	template <typename _Y>
	explicit MySharedPtr(_Y* _rawPtr) 
		//raw pointer constructor
		: _myPtr(_rawPtr), _myRefC(new Ref_Count_Default<_Y>(_rawPtr))
	{
	}

	template <typename _D>
	MySharedPtr(std::nullptr_t, _D _deleter)
		//pointer and deleter constructor
		: _myPtr(nullptr), _myRefC(new Ref_Count_Del<_T, _D>(nullptr, _deleter))
	{
	}

	template <typename _Y, typename _D> 
	MySharedPtr(_Y* _rawPtr, _D _deleter) 
		//pointer and deleter constructor
		: _myPtr(_rawPtr), _myRefC(new Ref_Count_Del<_Y, _D>(_rawPtr, _deleter))
	{
	}

	template <typename _D, typename _Alloc>
	MySharedPtr(std::nullptr_t, _D _deleter, _Alloc _alloc) 
		: _myPtr(nullptr)
	{
		typedef Ref_Count_Del_Alloc<_T, _D, _Alloc> _MyRefType;
		typename _Alloc::rebind<_MyRefType>::other _actual_Alloc(_alloc);
		_MyRefType* _refPtr = _actual_Alloc.allocate(1);
		_actual_Alloc.construct(_refPtr, _myPtr, _del, _alloc);
		_myRefC = _refPtr;
	}

	template <typename _Y, typename _D, typename _Alloc>
	MySharedPtr(_Y* _rawPtr, _D _del, _Alloc _alloc)
		: _myPtr(_rawPtr)
	{
		//allocator of another type is allowed here, so rebind is needed

		//a temporary pointer is needed here for construction because the 
		//member pointer of MySharedPtr is a pointer to the base class of 
		//reference count object

		typedef Ref_Count_Del_Alloc<_T, _D, _Alloc> _MyRefType;
		typename _Alloc::rebind<_MyRefType>::other _actual_Alloc(_alloc);
		_MyRefType* _refPtr = _actual_Alloc.allocate(1);
		_actual_Alloc.construct(_refPtr, _myPtr, _del, _alloc);
		_myRefC = _refPtr;
	}

	MySharedPtr(const MySharedPtr& _other) noexcept
		: _myPtr(_other._myPtr), _myRefC(_other._myRefC)
	{
		if (_myRefC)
			_myRefC->_Increment();
	}

	template <typename _U>
	MySharedPtr(const MySharedPtr<_U>& _other) noexcept
		: _myPtr(_other._myPtr), _myRefC(_other._myRefC)
	{
		if (_myRefC)
			_myRefC->_Increment();
	}

	template <class _U>
	MySharedPtr(const MyWeakPtr<_U>& _other) noexcept
	{
		if (!_other.expired()) {
			_myPtr = _other._myPtr;
			_myRefC = _other._myRefC;
			_myRefC->_Increment();
		}
		else {
			_myPtr = nullptr;
			_myRefC = nullptr;
		}
	}

	MySharedPtr(MySharedPtr&& _other) noexcept 
		: _myPtr(std::move(_other._myPtr)), _myRefC(std::move(_other._myRefC))
	{
		_other._myPtr = nullptr;
		_other._myRefC = nullptr;
	}

	template <typename _U>
	MySharedPtr(MySharedPtr<_U>&& _other) noexcept
		: _myPtr(std::move(_other._myPtr)), _myRefC(std::move(_other._myRefC))
	{
		_other._myPtr = nullptr;
		_other._myRefC = nullptr;
	}

	template <typename _U>
	MySharedPtr(const MySharedPtr<_U>& _other, _T* _rawPtr)
		: _myPtr(_rawPtr), _myRefC(_other._myRefC)
	{
		if (_myRefC)
			_myRefC->_Increment();
	}

	~MySharedPtr() {
		if (_myRefC)
			_myRefC->_Decrement();
	}

	MySharedPtr& operator=(const MySharedPtr& _other) noexcept {
		MySharedPtr(_other).swap(*this);
		return *this;
	}

	template<typename _U>
	MySharedPtr& operator=(const MySharedPtr<_U>& _other) noexcept {
		MySharedPtr(_other).swap(*this);
		return *this;
	}

	MySharedPtr& operator=(MySharedPtr&& _other) noexcept {
		MySharedPtr(std::move(_other)).swap(*this);
		return *this;
	}

	template<typename _U>
	MySharedPtr& operator=(MySharedPtr<_U>&& _other) noexcept {
		MySharedPtr(std::move(_other)).swap(*this);
		return *this;
	}


	void swap(MySharedPtr& _other) noexcept {

		//if we haven't defined our own version of swap() here, the built-in swap() 
		//may execute the copy constructor and the copy assignment of MySharedPtr, causing redundant increment
		//and decrement

		std::swap(_myPtr, _other._myPtr);
		std::swap(_myRefC, _other._myRefC);
	}

	void reset() noexcept {
		MySharedPtr().swap(*this);
	}

	template <typename _U>
	void reset(_U* _rawPtr) noexcept {
		MySharedPtr(_rawPtr).swap(*this);
	}

	template <typename _U, typename _D>
	void reset(_U* _rawPtr, _D _deleter) {
		MySharedPtr(_rawPtr, _deleter).swap(*this);
	}

	template <typename _U, typename _D, typename _Alloc>
	void reset(_U* _rawPtr, _D _deleter, _Alloc _alloc) {
		MySharedPtr(_rawPtr, _deleter, _alloc).swap(*this);
	}

	Element_type* get() const noexcept {
		return _myPtr;
	}

	typename std::add_lvalue_reference<_T>::type operator*() const noexcept {
		return (*this->get());
	}

	Element_type* operator->() const noexcept {
		return (this->get());
	}

	long use_count() const noexcept {
		if (!_myRefC)
			return 0;
		return _myRefC->_Get_Use_Count();
	}

	bool unique() const noexcept {
		return (use_count == 1);
	}

	explicit operator bool() const noexcept {
		return (get() != nullptr);
	}

private:
	Element_type* _myPtr;
	Ref_Count* _myRefC;
};

template <class _T>
class MyWeakPtr {
	template <class _Tx>
	friend class MyWeakPtr;

	template <class _Tx>
	friend class MySharedPtr;
public:
	typedef _T Element_type;

	constexpr MyWeakPtr() noexcept
		: _myPtr(nullptr), _myRefC(nullptr)
	{
	}

	MyWeakPtr(const MyWeakPtr& _other) noexcept
		: _myPtr(_other._myPtr), _myRefC(_other._myRefC)
	{
		if (_myRefC)
			_myRefC->_Increment_Weak();
	}

	template <class _U> 
	MyWeakPtr(const MyWeakPtr<_U>& _other) noexcept
		: _myPtr(_other._myPtr), _myRefC(_other._myRefC)
	{
		if (_myRefC)
			_myRefC->_Increment_Weak();
	}

	template <class _U> 
	MyWeakPtr(const MySharedPtr<_U>& _other) noexcept
		: _myPtr(_other._myPtr), _myRefC(_other._myRefC)
	{
		if (_myRefC)
			_myRefC->_Increment_Weak();
	}

	~MyWeakPtr() {
		if (_myRefC)
			_myRefC->_Decrement_Weak();
	}

	MyWeakPtr& operator= (const MyWeakPtr& _other) noexcept {
		MyWeakPtr(_other).swap(*this);
		return *this;
	}

	template <class _U> 
	MyWeakPtr& operator= (const MyWeakPtr<_U>& _other) noexcept {
		MyWeakPtr(_other).swap(*this);
		return *this;
	}

	template <class _U> 
	MyWeakPtr& operator= (const MySharedPtr<_U>& _other) noexcept {
		if (_myRefC)
			_myRefC->_Decrement_Weak();
		_myPtr = _other._myPtr;
		_myRefC = _other._myRefC;
		if (_myRefC)
			_myRefC->_Increment_Weak();

	}

	void swap(MyWeakPtr& _other) noexcept {
		std::swap(_myPtr, _other._myPtr);
		std::swap(_myRefC, _other._myRefC);
	}

	void reset() noexcept {
		MyWeakPtr().swap(*this);
	}

	long use_count() const noexcept {
		if (!_myRefC)
			return 0;
		return _myRefC->_Get_Use_Count();
	}

	bool expired() const noexcept {
		return (use_count() == 0);
	}

	MySharedPtr<_T> lock() const noexcept {
		return MySharedPtr<_T>(*this);
	}

private:
	Element_type* _myPtr;
	Ref_Count* _myRefC;
};

#endif