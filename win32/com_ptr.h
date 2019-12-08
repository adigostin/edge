#pragma once

namespace edge
{
	template<typename I> class com_ptr
	{
		I* _ptr = nullptr;

	public:
		com_ptr() = default;

		com_ptr (const com_ptr<I>& from)
		{
			static_assert(false); // use move constructor instead
		}

		com_ptr<I>& operator= (const com_ptr<I>& from)
		{
			static_assert(false); // use move-assignment instead
		}

		com_ptr (com_ptr<I>&& from) noexcept
		{
			static_assert (std::is_base_of_v<IUnknown, I>);
			if (from._ptr != nullptr)
				std::swap (this->_ptr, from._ptr);
		}

		com_ptr<I>& operator= (com_ptr<I>&& from) noexcept
		{
			static_assert (std::is_base_of_v<IUnknown, I>);
			if (_ptr != nullptr)
			{
				_ptr->Release();
				_ptr = nullptr;
			}

			std::swap (this->_ptr, from._ptr);
			return *this;
		}

		com_ptr(I* from)
			: _ptr(from)
		{
			static_assert (std::is_base_of_v<IUnknown, I>);
			if (_ptr != nullptr)
				_ptr->AddRef();
		}

		com_ptr(I* from, bool add_ref)
			: _ptr(from)
		{
			static_assert (std::is_base_of_v<IUnknown, I>);
			if (add_ref && (_ptr != nullptr))
				_ptr->AddRef();
		}

		template<typename IFrom>
		com_ptr (const com_ptr<IFrom>& from)
		{
			if (from != nullptr)
			{
				auto hr = from->QueryInterface(&_ptr);
				if (FAILED(hr))
					throw _com_error(hr);
			}
		}

		com_ptr(nullptr_t np)
		{ }

		com_ptr<I>& operator=(nullptr_t np)
		{
			if (_ptr)
			{
				_ptr->Release();
				_ptr = nullptr;
			}
			return *this;
		}

		~com_ptr()
		{
			static_assert (std::is_base_of_v<IUnknown, I>);

			if (_ptr != nullptr)
			{
				auto p = _ptr;
				_ptr = nullptr;
				p->Release();
			}
		}

		operator I*() const { return _ptr; }

		I* get() const { return _ptr; }

		I* operator->() const
		{
			assert(_ptr != nullptr);
			return _ptr;
		}

		I** operator&()
		{
			if (_ptr != nullptr)
			{
				auto p = _ptr;
				_ptr = nullptr;
				p->Release();
			}

			return &_ptr;
		}
	};

	template<typename T, typename... Args>
	com_ptr<T> make_com(Args&&... args)
	{
		return com_ptr<T>(new T(std::forward<Args>(args)...), false);
	}

	template<typename T> class co_task_mem_ptr
	{
		T* _ptr = nullptr;

	public:
		co_task_mem_ptr() = default;

		co_task_mem_ptr (const co_task_mem_ptr&) = delete;
		co_task_mem_ptr& operator= (const co_task_mem_ptr&) = delete;

		co_task_mem_ptr (co_task_mem_ptr&& from)
		{
			if (from._ptr != nullptr)
				std::swap (this->_ptr, from._ptr);
		}

		co_task_mem_ptr& operator= (co_task_mem_ptr&& from)
		{
			if (_ptr != nullptr)
			{
				_ptr->Release();
				_ptr = nullptr;
			}

			std::swap (this->_ptr, from._ptr);
			return *this;
		}

		co_task_mem_ptr (size_t s)
		{
			_ptr = (T*) CoTaskMemAlloc (s * sizeof(T));
			if (!_ptr)
				throw _com_error(E_OUTOFMEMORY);
		}

		~co_task_mem_ptr()
		{
			if (_ptr != nullptr)
			{
				auto p = _ptr;
				_ptr = nullptr;
				::CoTaskMemFree(p);
			}
		}

		T* get() const { return _ptr; }

		T* operator->() const { return _ptr; }

		T** operator&()
		{
			if (_ptr != nullptr)
			{
				auto p = _ptr;
				_ptr = nullptr;
				::CoTaskMemFree(p);
			}

			return &_ptr;
		}

		T* release()
		{
			auto res = _ptr;
			_ptr = nullptr;
			return res;
		}
	};

	inline co_task_mem_ptr<wchar_t> co_task_mem_alloc (const wchar_t* null_terminated)
	{
		auto len = wcslen(null_terminated);
		auto res = co_task_mem_ptr<wchar_t>(len + 1);
		wcscpy_s (res.get(), len + 1, null_terminated);
		return res;
	}
}
