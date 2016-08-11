#ifndef _MYSMARTPTR_H_
#define _MYSMARTPTR_H_

#include <atomic>
#include <cassert>
#include <type_traits>
#include <utility>

namespace MyTR1 {

//********************************************** type traits ********************************************** 

	template <typename _Class, typename _Type>
	struct has_member_pointer {
		template <typename _T>
		static auto _Func(typename _T::pointer _arg) -> decltype(_arg);

		template <typename _T>
		static auto _Func(...)->_Type;

		typedef decltype(_Func<_Class>(nullptr)) type;
	};

	template <typename _T, _T v>
	struct integral_constant {
		static constexpr _T value = v;
		typedef _T value_type;
		typedef integral_constant<_T, v> type;
		constexpr operator _T() {
			return v;
		}
	};

	typedef integral_constant<bool, true> true_type;
	typedef integral_constant<bool, false> false_type;

	template <bool Cond, typename _T = void>
	struct enable_if 
	{
	};

	template <typename _T>
	struct enable_if<true, _T> {
		typedef _T type;
	};

	template <typename _T>
	struct is_void {
		static const bool value = false;
	};

	template <>
	struct is_void<void> {
		static const bool value = true;
	};

	template <typename _T1, typename _T2>
	struct is_same {
		static const bool value = false;
	};

	template <typename _T>
	struct is_same<_T, _T> {
		static const bool value = true;
	};

	template <typename _T>
	struct is_array {
		static const bool value = false;
	};

	template <typename _T>
	struct is_array<_T[]> {
		static const bool value = true;
	};

	template <typename _T, size_t _N>
	struct is_array<_T[_N]> {
		static const bool value = true;
	};

	template <typename _From, typename _To>
	struct is_convertible {
		static auto test(_To) {
			return true;
		}

		static auto test(...) {
			return false;
		}

		static _From _dummy;
		static const bool value = test(_dummy);
	};

	template <typename _T>
	struct is_function {
		static const bool value = !is_convertible<_T*, const volatile void*>::value;
	};

	template <bool Cond, typename _T, typename _F>
	struct conditional {
		typedef _T type;
	};

	template <typename _T, typename _F>
	struct conditional<false, _T, _F> {
		typedef _F type;
	};

	template <typename _T>
	struct is_lvalue_reference : integral_constant<bool, false> 
	{
	};

	template <typename _T>
	struct is_lvalue_reference<_T&> : integral_constant<bool, true> 
	{
	};

	template <typename _T>
	struct is_rvalue_reference : integral_constant<bool, false>
	{
	};

	template <typename _T>
	struct is_rvalue_reference<_T&&> : integral_constant<bool, true>
	{
	};

	template <typename _T>
	struct is_reference : integral_constant <bool,
		is_lvalue_reference<_T>::value || is_rvalue_reference<_T>::value > 
	{
	};

	template <typename _T>
	struct remove_reference {
		typedef _T type;
	};

	template <typename _T>
	struct remove_reference<_T&> {
		typedef _T type;
	};

	template <typename _T>
	struct remove_reference<_T&&> {
		typedef _T type;
	};

	template <typename _T>
	struct add_lvalue_reference {
		typedef _T& type;
	};

	template <typename _T>
	struct add_lvalue_reference<_T&> {
		typedef _T& type;
	};

	template <typename _T>
	struct add_lvalue_reference<_T&&> {
		typedef _T& type;
	};

	template <>
	struct add_lvalue_reference<void> {
		typedef void type;
	};

	template <typename _T>
	struct add_rvalue_reference {
		typedef _T&& type;
	};

	template <typename _T>
	struct add_rvalue_reference<_T&> {
		typedef _T&& type;
	};

	template <typename _T>
	struct add_rvalue_reference<_T&&> {
		typedef _T&& type;
	};

	template <>
	struct add_rvalue_reference<void> {
		typedef void type;
	};
//*********************************************************************************************************

//**************************************** definition of deleter ******************************************
	template <typename _T>
	class default_delete {
	public:
		constexpr default_delete() noexcept = default;

		template <typename _U>
		default_delete(const default_delete<_U>& _del) noexcept
		{
		}

		void operator()(_T* _ptr) const {
			delete _ptr;
		}
	};

	template <typename _T>
	class default_delete<_T[]> {
	public:
		constexpr default_delete() noexcept = default;

		template <typename _U>
		default_delete(const default_delete<_U[]>& _del) noexcept
		{
		}

		void operator()(_T* _ptr) const {
			delete[] _ptr;
		}
	};

	template <typename _T>
	class MySharedPtr;

	template <typename _T>
	class MyWeakPtr;

	template <typename _T, typename _D>
	class MyUniquePtr;

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
		::std::atomic<unsigned long> _use;
		::std::atomic<unsigned long> _weak_use;
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
		::std::pair<_D, _T*> _myPair;
	};

	template <typename _T, typename _D, typename _Alloc>
	class Ref_Count_Del_Alloc : public Ref_Count {
	public:
		Ref_Count_Del_Alloc(_T* _rawPtr, _D _deleter, _Alloc _alloc)
			: Ref_Count(), _myPair(::std::pair<_D, _T*>(_deleter, _rawPtr), _alloc)
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
		::std::pair<::std::pair<_D, _T*>, _Alloc> _myPair;
	};

	template <typename _T>
	class MySharedPtr {
		template <typename _Tx>
		friend class MySharedPtr;

		template <typename _Tx>
		friend class MyWeakPtr;

		template <typename _Tx, typename _Dx>
		friend class MyUniquePtr;

	public:
		typedef _T element_type;

		constexpr MySharedPtr() noexcept
			// default constructor
			: _myPtr(nullptr), _myRefC(nullptr)
		{
		}

		constexpr MySharedPtr(::std::nullptr_t) noexcept
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
		MySharedPtr(::std::nullptr_t, _D _deleter)
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
		MySharedPtr(::std::nullptr_t, _D _deleter, _Alloc _alloc)
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
			: _myPtr(_other._myPtr), _myRefC(_other._myRefC)
		{
			_other._myPtr = nullptr;
			_other._myRefC = nullptr;
		}

		template <typename _U>
		MySharedPtr(MySharedPtr<_U>&& _other) noexcept
			: _myPtr(::std::move(_other._myPtr)), _myRefC(::std::move(_other._myRefC))
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
			MySharedPtr(::std::move(_other)).swap(*this);
			return *this;
		}

		template<typename _U>
		MySharedPtr& operator=(MySharedPtr<_U>&& _other) noexcept {
			MySharedPtr(::std::move(_other)).swap(*this);
			return *this;
		}


		void swap(MySharedPtr& _other) noexcept {

			//if we haven't defined our own version of swap() here, the built-in swap() 
			//may execute the copy constructor and the copy assignment of MySharedPtr, causing redundant increment
			//and decrement

			::std::swap(_myPtr, _other._myPtr);
			::std::swap(_myRefC, _other._myRefC);
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

		element_type* get() const noexcept {
			return _myPtr;
		}

		typename ::std::add_lvalue_reference<_T>::type operator*() const noexcept {
			return (*this->get());
		}

		element_type* operator->() const noexcept {
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
		element_type* _myPtr;
		Ref_Count* _myRefC;
	};

	template <class _T>
	class MyWeakPtr {
		template <class _Tx>
		friend class MyWeakPtr;

		template <class _Tx>
		friend class MySharedPtr;

		template <typename _Tx, typename _Dx>
		friend class MyUniquePtr;

	public:
		typedef _T element_type;

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
			return *this;

		}

		void swap(MyWeakPtr& _other) noexcept {
			::std::swap(_myPtr, _other._myPtr);
			::std::swap(_myRefC, _other._myRefC);
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
		element_type* _myPtr;
		Ref_Count* _myRefC;
	};

	template <typename _T, typename _D = default_delete<_T>>
	class MyUniquePtr {
		template <typename _Tx, typename _Dx>
		friend class MyUniquePtr;
	public:
		typedef _T element_type;
		typedef _D deleter_type;
		typedef typename has_member_pointer<typename remove_reference<_D>::type, _T*>::type pointer;

		constexpr MyUniquePtr() noexcept
			: _myPtr(nullptr), _myDeleter(_D())
		{
		}

		constexpr MyUniquePtr(::std::nullptr_t) noexcept
			: unique_ptr()
		{
		}

		explicit MyUniquePtr(pointer _ptr) noexcept
			: _myPtr(_ptr), _myDeleter(_D())
		{
		}

		MyUniquePtr(pointer _ptr,
			typename conditional<is_reference<_D>::value, _D, const _D&>::type _del) noexcept
			: _myPtr(_ptr), _myDeleter(_del)
		{
		}

		MyUniquePtr(pointer _ptr,
			typename remove_reference<_D>::type&& _del) noexcept
			: _myPtr(_ptr), _myDeleter(::std::move(_del))
		{
		}

		MyUniquePtr(MyUniquePtr&& _other) noexcept
			: _myPtr(_other.release()), _myDeleter(::std::forward<_D>(_other.get_deleter()))
		{
		}

		template <typename _Tx, typename _Dx, typename =
			      typename enable_if<
			               is_convertible<typename MyUniquePtr<_Tx, _Dx>::pointer, 
			                              pointer>::value
			               && !(is_array<_Tx>::value)
			               && (is_reference<_Dx>::value 
							   ? is_same<_D, _Dx>::value 
							   : is_convertible<_Dx, _D>::value) 
			               >::type>
		MyUniquePtr(MyUniquePtr<_Tx, _Dx>&& _other) noexcept 
			: _myPtr(_other.release()), _myDeleter(::std::forward<_Dx>(_other.get_deleter()))
		{
		}

		MyUniquePtr(const MyUniquePtr&) = delete;

		~MyUniquePtr() {
			if (get()) {
				get_deleter()(get());
			}
		}

		MyUniquePtr& operator= (MyUniquePtr&& _other) noexcept {
			reset(_other.release());
			get_deleter() = ::std::forward<_D>(_other.get_deleter());
			return *this;
		}

		MyUniquePtr& operator= (::std::nullptr_t) noexcept {
			reset();
			return *this;
		}

		template <class _Tx, class _Dx, typename =
				  typename enable_if<
			               is_convertible<typename MyUniquePtr<_Tx, _Dx>::pointer,
			                              pointer>::value
			               && !(is_array<_Tx>::value)
			               && (is_reference<_Dx>::value
				               ? is_same<_D, _Dx>::value
				               : is_convertible<_Dx, _D>::value)
		                   >::type>
		MyUniquePtr& operator= (MyUniquePtr<_Tx, _Dx>&& _other) noexcept {
			reset(_other.release());
			get_deleter() = ::std::forward<_Dx>(_other.get_deleter());
			return *this;
		}
	
		MyUniquePtr& operator= (const MyUniquePtr&) = delete;

		pointer get() const noexcept {
			return _myPtr;
		}

		deleter_type& get_deleter() noexcept {
			return _myDeleter;
		}

		const deleter_type& get_deleter() const noexcept {
			return _myDeleter;
		}

		explicit operator bool() const noexcept {
			return (get() != nullptr);
		}

		pointer release() noexcept {
			pointer _ptr = _myPtr;
			_myPtr = nullptr;
			return _ptr;
		}

		void reset(pointer _ptr = pointer()) noexcept {
			get_deleter()(get());
			_myPtr = _ptr;
		}

		void swap(MyUniquePtr& _other) noexcept {
			::std::swap(_myPtr, _other._myPtr);
			::std::swap(_myDeleter, _other._myDeleter);
		}

		typename add_lvalue_reference<element_type>::type operator*() const {
			return *get();
		}

		pointer operator->() const noexcept {
			return get();
		}

	private:
		pointer _myPtr;
		deleter_type _myDeleter;
	};
}

#endif