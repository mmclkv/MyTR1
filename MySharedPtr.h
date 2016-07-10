#ifndef _MYSHAREDPTR_H_
#define _MYSHAREDPTR_H_

#include <atomic>

class Ref_Count {
public:
	Ref_Count() 
	{
		_use.store(1);
	}

	void _Increment() { 
		_use.fetch_add(1);
	}

	void _Decrement() {
		_use.fetch_sub(1);
		if (_Get_Use_Count() == 0) {
			_Destroy();
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
	// use a atomic variable to record the reference count
	std::atomic_ulong _use;
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
public:
	typedef _T Element_type;
	typedef MySharedPtr<_T> _MyType;

	constexpr MySharedPtr() noexcept
		// default constructor
		: _myPtr(0), _myRefC(0)
	{
	}

	constexpr MySharedPtr(std::nullptr_t) noexcept
		//null pointer constructor
		: _myPtr(0), _myRefC(0)
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
		: _myPtr(0), _myRefC(new Ref_Count_Del<_T, _D>(0, _deleter))
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
		: _myPtr(0)
	{
		typedef Ref_Count_Del_Alloc<_T, _D, _Alloc> _MyRefType;
		typename _Alloc::rebind<_MyRefType>::other _actual_Alloc(_alloc);
		_myRefC = _actual_Alloc.allocate(1);
		_actual_Alloc.construct(_myRefC, 0, _del, _actual_Alloc);
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
		_actual_Alloc.construct(_refPtr, _rawPtr, _del, _actual_Alloc);
		_myRefC = _refPtr;
	}

	MySharedPtr(const _MyType& _other) noexcept
		: _myPtr(_other._myPtr), _myRefC(_other._myRefC)
	{ 
	}

	template <typename _U>
	MySharedPtr(const MySharedPtr<_U>& _other) noexcept
		: _myPtr(_other._myPtr), _myRefC(_other._myRefC)
	{
	}

	MySharedPtr& operator=(const _MyType& other) {

	}

	~MySharedPtr() {
		_myRefC->_Decrement();
	}

private:
	Element_type* _myPtr;
	Ref_Count *_myRefC;
};

#endif