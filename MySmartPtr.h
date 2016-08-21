#ifndef _MYSMARTPTR_H_
#define _MYSMARTPTR_H_

#include <atomic>
#include <cassert>
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
	struct is_void : false_type 
	{
	};

	template <>
	struct is_void<void> : true_type 
	{
	};

	template <typename _T1, typename _T2>
	struct is_same : false_type 
	{
	};

	template <typename _T>
	struct is_same<_T, _T> : true_type 
	{
	};

	template <typename _T>
	struct is_array : false_type
	{
	};

	template <typename _T>
	struct is_array<_T[]> : true_type {
	};

	template <typename _T, size_t _N>
	struct is_array<_T[_N]> : true_type {
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
	struct is_class {
		template <typename _U>
		static char helper(int _U::*) { return true; }

		template <typename _U>
		static int helper(...) { return false; }

		static constexpr bool value = sizeof(helper<_T>(0)) == sizeof(char);
	};

	template <typename _Base, typename _Derived>
	struct is_base_of {
		struct _convert {
			operator _Base*() const;
			operator _Derived*();
		};

		template <typename _T>
		static char helper(_Derived*, _T);

		static int helper(_Base*, int);

		static constexpr bool value = sizeof(helper(_convert(), 0)) == sizeof(char);
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
	class shared_ptr;

	template <typename _T>
	class weak_ptr;

	template <typename _T, typename _D>
	class unique_ptr;

	template <typename _T>
	class enable_shared_from_this {
		friend class shared_ptr<_T>;
	protected:
		shared_ptr<_T> shared_from_this() {
			shared_ptr<_T> p;
			p = _weak_this;
			assert(p);
			return p;
		}

		shared_ptr<_T const> shared_from_this() const {
			shared_ptr<_T> p;
			p = _weak_this;
			assert(p);
			return p;
		}

		enable_shared_from_this<_T>& operator=(const enable_shared_from_this<_T> &obj) {
			return *this;
		}

	private:
		weak_ptr<_T> _weak_this;
	};

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
	class shared_ptr {
		template <typename _Tx>
		friend class shared_ptr;

		template <typename _Tx>
		friend class weak_ptr;

		template <typename _Tx, typename _Dx>
		friend class unique_ptr;

	public:
		typedef _T element_type;

		constexpr shared_ptr() noexcept
			// default constructor
			: _myPtr(nullptr), _myRefC(nullptr)
		{
		}

		constexpr shared_ptr(::std::nullptr_t) noexcept
			//null pointer constructor
			: _myPtr(nullptr), _myRefC(nullptr)
		{
		}

		template <typename _Y>
		explicit shared_ptr(_Y* _rawPtr)
			//raw pointer constructor
			: _myPtr(_rawPtr), _myRefC(new Ref_Count_Default<_Y>(_rawPtr))
		{
			_enable_shared_from_this(*this, _rawPtr);
		}

		template <typename _D>
		shared_ptr(::std::nullptr_t, _D _deleter)
			//pointer and deleter constructor
			: _myPtr(nullptr), _myRefC(new Ref_Count_Del<_T, _D>(nullptr, _deleter))
		{
		}

		template <typename _Y, typename _D>
		shared_ptr(_Y* _rawPtr, _D _deleter)
			//pointer and deleter constructor
			: _myPtr(_rawPtr), _myRefC(new Ref_Count_Del<_Y, _D>(_rawPtr, _deleter))
		{
			_enable_shared_from_this(*this, _rawPtr);
		}

		template <typename _D, typename _Alloc>
		shared_ptr(::std::nullptr_t, _D _deleter, _Alloc _alloc)
			: _myPtr(nullptr)
		{
			typedef Ref_Count_Del_Alloc<_T, _D, _Alloc> _MyRefType;
			typename _Alloc::rebind<_MyRefType>::other _actual_Alloc(_alloc);
			_MyRefType* _refPtr = _actual_Alloc.allocate(1);
			_actual_Alloc.construct(_refPtr, _myPtr, _del, _alloc);
			_myRefC = _refPtr;
		}

		template <typename _Y, typename _D, typename _Alloc>
		shared_ptr(_Y* _rawPtr, _D _del, _Alloc _alloc)
			: _myPtr(_rawPtr)
		{
			//allocator of another type is allowed here, so rebind is needed

			//a temporary pointer is needed here for construction because the 
			//member pointer of shared_ptr is a pointer to the base class of 
			//reference count object

			typedef Ref_Count_Del_Alloc<_T, _D, _Alloc> _MyRefType;
			typename _Alloc::rebind<_MyRefType>::other _actual_Alloc(_alloc);
			_MyRefType* _refPtr = _actual_Alloc.allocate(1);
			_actual_Alloc.construct(_refPtr, _myPtr, _del, _alloc);
			_myRefC = _refPtr;
			_enable_shared_from_this(*this, _rawPtr);
		}

		shared_ptr(const shared_ptr& _other) noexcept
			: _myPtr(_other._myPtr), _myRefC(_other._myRefC)
		{
			if (_myRefC)
				_myRefC->_Increment();
		}

		template <typename _U>
		shared_ptr(const shared_ptr<_U>& _other) noexcept
			: _myPtr(_other._myPtr), _myRefC(_other._myRefC)
		{
			if (_myRefC)
				_myRefC->_Increment();
		}

		template <class _U>
		shared_ptr(const weak_ptr<_U>& _other) noexcept
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

		shared_ptr(shared_ptr&& _other) noexcept
			: _myPtr(_other._myPtr), _myRefC(_other._myRefC)
		{
			_other._myPtr = nullptr;
			_other._myRefC = nullptr;
		}

		template <typename _U>
		shared_ptr(shared_ptr<_U>&& _other) noexcept
			: _myPtr(::std::move(_other._myPtr)), _myRefC(::std::move(_other._myRefC))
		{
			_other._myPtr = nullptr;
			_other._myRefC = nullptr;
		}

		template <typename _U>
		shared_ptr(const shared_ptr<_U>& _other, _T* _rawPtr)
			: _myPtr(_rawPtr), _myRefC(_other._myRefC)
		{
			if (_myRefC)
				_myRefC->_Increment();
			_enable_shared_from_this(*this, _rawPtr);
		}

		~shared_ptr() {
			if (_myRefC)
				_myRefC->_Decrement();
		}

		shared_ptr& operator=(const shared_ptr& _other) noexcept {
			shared_ptr(_other).swap(*this);
			return *this;
		}

		template<typename _U>
		shared_ptr& operator=(const shared_ptr<_U>& _other) noexcept {
			shared_ptr(_other).swap(*this);
			return *this;
		}

		shared_ptr& operator=(shared_ptr&& _other) noexcept {
			shared_ptr(::std::move(_other)).swap(*this);
			return *this;
		}

		template<typename _U>
		shared_ptr& operator=(shared_ptr<_U>&& _other) noexcept {
			shared_ptr(::std::move(_other)).swap(*this);
			return *this;
		}


		void swap(shared_ptr& _other) noexcept {

			//if we haven't defined our own version of swap() here, the built-in swap() 
			//may execute the copy constructor and the copy assignment of shared_ptr, causing redundant increment
			//and decrement

			::std::swap(_myPtr, _other._myPtr);
			::std::swap(_myRefC, _other._myRefC);
		}

		void reset() noexcept {
			shared_ptr().swap(*this);
		}

		template <typename _U>
		void reset(_U* _rawPtr) noexcept {
			shared_ptr(_rawPtr).swap(*this);
		}

		template <typename _U, typename _D>
		void reset(_U* _rawPtr, _D _deleter) {
			shared_ptr(_rawPtr, _deleter).swap(*this);
		}

		template <typename _U, typename _D, typename _Alloc>
		void reset(_U* _rawPtr, _D _deleter, _Alloc _alloc) {
			shared_ptr(_rawPtr, _deleter, _alloc).swap(*this);
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
		template <typename _T, typename _Y>
		void _enable_shared_from_this(const shared_ptr<_T>& _sp, enable_shared_from_this<_Y>* _rawPtr) {
			_rawPtr->_weak_this = _sp;
		}

		void _enable_shared_from_this(...) 
		{
		}

		element_type* _myPtr;
		Ref_Count* _myRefC;
	};

	template <class _T>
	class weak_ptr {
		template <class _Tx>
		friend class weak_ptr;

		template <class _Tx>
		friend class shared_ptr;

		template <typename _Tx, typename _Dx>
		friend class unique_ptr;

	public:
		typedef _T element_type;

		constexpr weak_ptr() noexcept
			: _myPtr(nullptr), _myRefC(nullptr)
		{
		}

		weak_ptr(const weak_ptr& _other) noexcept
			: _myPtr(_other._myPtr), _myRefC(_other._myRefC)
		{
			if (_myRefC)
				_myRefC->_Increment_Weak();
		}

		template <class _U>
		weak_ptr(const weak_ptr<_U>& _other) noexcept
			: _myPtr(_other._myPtr), _myRefC(_other._myRefC)
		{
			if (_myRefC)
				_myRefC->_Increment_Weak();
		}

		template <class _U>
		weak_ptr(const shared_ptr<_U>& _other) noexcept
			: _myPtr(_other._myPtr), _myRefC(_other._myRefC)
		{
			if (_myRefC)
				_myRefC->_Increment_Weak();
		}

		~weak_ptr() {
			if (_myRefC)
				_myRefC->_Decrement_Weak();
		}

		weak_ptr& operator= (const weak_ptr& _other) noexcept {
			weak_ptr(_other).swap(*this);
			return *this;
		}

		template <class _U>
		weak_ptr& operator= (const weak_ptr<_U>& _other) noexcept {
			weak_ptr(_other).swap(*this);
			return *this;
		}

		template <class _U>
		weak_ptr& operator= (const shared_ptr<_U>& _other) noexcept {
			weak_ptr(_other).swap(*this);
			return *this;

		}

		void swap(weak_ptr& _other) noexcept {
			::std::swap(_myPtr, _other._myPtr);
			::std::swap(_myRefC, _other._myRefC);
		}

		void reset() noexcept {
			weak_ptr().swap(*this);
		}

		long use_count() const noexcept {
			if (!_myRefC)
				return 0;
			return _myRefC->_Get_Use_Count();
		}

		bool expired() const noexcept {
			return (use_count() == 0);
		}

		shared_ptr<_T> lock() const noexcept {
			return shared_ptr<_T>(*this);
		}

	private:
		element_type* _myPtr;
		Ref_Count* _myRefC;
	};

	template <typename _T, typename _D = default_delete<_T>>
	class unique_ptr {
		template <typename _Tx, typename _Dx>
		friend class unique_ptr;
	public:
		typedef _T element_type;
		typedef _D deleter_type;
		typedef typename has_member_pointer<typename remove_reference<_D>::type, _T*>::type pointer;

		constexpr unique_ptr() noexcept
			: _myPtr(nullptr), _myDeleter(_D())
		{
		}

		constexpr unique_ptr(::std::nullptr_t) noexcept
			: unique_ptr()
		{
		}

		explicit unique_ptr(pointer _ptr) noexcept
			: _myPtr(_ptr), _myDeleter(_D())
		{
		}

		unique_ptr(pointer _ptr,
			typename conditional<is_reference<_D>::value, _D, const _D&>::type _del) noexcept
			: _myPtr(_ptr), _myDeleter(_del)
		{
		}

		unique_ptr(pointer _ptr,
			typename remove_reference<_D>::type&& _del) noexcept
			: _myPtr(_ptr), _myDeleter(::std::move(_del))
		{
		}

		unique_ptr(unique_ptr&& _other) noexcept
			: _myPtr(_other.release()), _myDeleter(::std::forward<_D>(_other.get_deleter()))
		{
		}

		template <typename _Tx, typename _Dx, typename =
			      typename enable_if<
			               is_convertible<typename unique_ptr<_Tx, _Dx>::pointer, 
			                              pointer>::value
			               && !(is_array<_Tx>::value)
			               && (is_reference<_Dx>::value 
							   ? is_same<_D, _Dx>::value 
							   : is_convertible<_Dx, _D>::value) 
			               >::type>
		unique_ptr(unique_ptr<_Tx, _Dx>&& _other) noexcept 
			: _myPtr(_other.release()), _myDeleter(::std::forward<_Dx>(_other.get_deleter()))
		{
		}

		unique_ptr(const unique_ptr&) = delete;

		~unique_ptr() {
			if (get()) {
				get_deleter()(get());
			}
		}

		unique_ptr& operator= (unique_ptr&& _other) noexcept {
			reset(_other.release());
			get_deleter() = ::std::forward<_D>(_other.get_deleter());
			return *this;
		}

		unique_ptr& operator= (::std::nullptr_t) noexcept {
			reset();
			return *this;
		}

		template <class _Tx, class _Dx, typename =
				  typename enable_if<
			               is_convertible<typename unique_ptr<_Tx, _Dx>::pointer,
			                              pointer>::value
			               && !(is_array<_Tx>::value)
			               && (is_reference<_Dx>::value
				               ? is_same<_D, _Dx>::value
				               : is_convertible<_Dx, _D>::value)
		                   >::type>
		unique_ptr& operator= (unique_ptr<_Tx, _Dx>&& _other) noexcept {
			reset(_other.release());
			get_deleter() = ::std::forward<_Dx>(_other.get_deleter());
			return *this;
		}
	
		unique_ptr& operator= (const unique_ptr&) = delete;

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

		void swap(unique_ptr& _other) noexcept {
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

	template <typename _T, typename _D>
	class unique_ptr<_T[], _D> {
	public:
		typedef _T element_type;
		typedef _D deleter_type;
		typedef typename has_member_pointer<typename remove_reference<_D>::type, _T*>::type pointer;

		constexpr unique_ptr() noexcept
			: _myPtr(nullptr), _myDeleter(_D())
		{
		}

		constexpr unique_ptr(::std::nullptr_t) noexcept
			: unique_ptr()
		{
		}

		explicit unique_ptr(pointer _ptr) noexcept
			: _myPtr(_ptr), _myDeleter(_D())
		{
		}

		unique_ptr(pointer _ptr,
			typename conditional<is_reference<_D>::value, _D, const _D&>::type _del) noexcept
			: _myPtr(_ptr), _myDeleter(_del)
		{
		}

		unique_ptr(pointer _ptr,
			typename remove_reference<_D>::type&& _del) noexcept
			: _myPtr(_ptr), _myDeleter(::std::move(_del))
		{
		}

		unique_ptr(unique_ptr&& _other) noexcept
			: _myPtr(_other.release()), _myDeleter(::std::forward<_D>(_other.get_deleter()))
		{
		}

		unique_ptr(const unique_ptr&) = delete;

		~unique_ptr() {
			if (get()) {
				get_deleter()(get());
			}
		}

		unique_ptr& operator= (unique_ptr&& _other) noexcept {
			reset(_other.release());
			get_deleter() = ::std::forward<_D>(_other.get_deleter());
			return *this;
		}

		unique_ptr& operator= (::std::nullptr_t) noexcept {
			reset();
			return *this;
		}

		unique_ptr& operator= (const unique_ptr&) = delete;

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

		void swap(unique_ptr& _other) noexcept {
			::std::swap(_myPtr, _other._myPtr);
			::std::swap(_myDeleter, _other._myDeleter);
		}

		element_type& operator[](size_t _index) const {
			return get()[_index];
		}

	private:
		pointer _myPtr;
		deleter_type _myDeleter;
	};
}

#endif