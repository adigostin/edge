
// This file is part of the "edge" library, available at https://github.com/adigostin/edge
// Copyright (c) 2011-2020 Adi Gostin, distributed under Apache License v2.0.

#pragma once
#include <typeindex>
#include <unordered_map>
#include <array>

namespace edge
{
	// In most cases it's easier to use this class as hierarchy root rather than as a member variable.
	// When it is a member variable, it might be destroyed before subscribers have a chance to unsubscribe themselves.
	class event_manager
	{
		template<typename event_t, typename... args_t>
		friend struct event;

		struct handler
		{
			void* callback;
			void* callback_arg;
		};

		// TODO: Make this a pointer to save RAM.
		std::unordered_multimap<std::type_index, handler> _events;

	protected:
		~event_manager()
		{
			assert(_events.empty());
		}

		template<typename event_t>
		typename event_t::invoker event_invoker() const
		{
			return typename event_t::invoker(this);
		}
	};

	// Note that this currently works only with a single thread;
	// don't try to do something with events in more than one thread.
	template<typename event_t, typename... args_t>
	struct event abstract
	{
		using callback_t = void(*)(void* callback_arg, args_t... args);

		event() = delete; // this class and classes derived from it are not meant to be instantiated.

		class subscriber
		{
			event_manager* const _em;

		public:
			subscriber (event_manager* em)
				: _em(em)
			{ }

			void add_handler (void(*callback)(void* callback_arg, args_t... args), void* callback_arg)
			{
				auto type = std::type_index(typeid(event_t));
				_em->_events.insert({ type, { callback, callback_arg } });
			}

			void remove_handler (void(*callback)(void* callback_arg, args_t... args), void* callback_arg)
			{
				auto type = std::type_index(typeid(event_t));
				auto range = _em->_events.equal_range(type);

				auto it = std::find_if(range.first, range.second, [=](const std::pair<std::type_index, event_manager::handler>& p)
					{
						return (p.second.callback == callback) && (p.second.callback_arg == callback_arg);
					});

				assert(it != range.second); // handler to remove not found

				_em->_events.erase(it);
			}

		private:
			template<typename T>
			struct extract_class;

			template<typename R, typename C, class... A>
			struct extract_class<R(C::*)(A...)>
			{
				using class_type = C;
			};

			template<typename R, typename C, class... A>
			struct extract_class<R(C::*)(A...) const>
			{
				using class_type = C;
			};

			template<auto member_callback>
			static void proxy (void* arg, args_t... args)
			{
				using member_callback_t = decltype(member_callback);
				using class_type = typename extract_class<member_callback_t>::class_type;
				auto c = static_cast<class_type*>(arg);
				(c->*member_callback)(args...);
			}

		public:
			template<auto member_callback>
			std::enable_if_t<std::is_member_function_pointer_v<decltype(member_callback)>>
			add_handler (typename extract_class<decltype(member_callback)>::class_type* target)
			{
				add_handler (&subscriber::proxy<member_callback>, target);
			}

			template<auto member_callback>
			std::enable_if_t<std::is_member_function_pointer_v<decltype(member_callback)>>
			remove_handler (typename extract_class<decltype(member_callback)>::class_type* target)
			{
				remove_handler (&subscriber::proxy<member_callback>, target);
			}
		};

	private:
		friend class event_manager;

		class invoker
		{
			const event_manager* const _em;

		public:
			invoker (const event_manager* em)
				: _em(em)
			{ }

			bool has_handlers() const { return _em->_events.find(std::type_index(typeid(event_t))) != _em->_events.end(); }

			void operator()(args_t... args)
			{
				const size_t count = _em->_events.count(std::type_index(typeid(event_t)));
				if (count)
				{
					auto range = _em->_events.equal_range(std::type_index(typeid(event_t)));

					// Note that this function must be reentrant (one event handler can invoke
					// another event, or add/remove events), that's why these stack copies.
					std::vector<event_manager::handler> long_list;
					event_manager::handler short_list[8];

					std::span<const event_manager::handler> handlers;
					if (count <= std::size(short_list))
					{
						size_t size = 0;
						for (auto it = range.first; it != range.second; it++)
							short_list[size++] = it->second;
						handlers = { &short_list[0], &short_list[size] };
					}
					else
					{
						for (auto it = range.first; it != range.second; it++)
							long_list.push_back(it->second);
						handlers = long_list;
					}

					for (auto& h : handlers)
					{
						auto callback = (callback_t)h.callback;
						auto arg = h.callback_arg;
						callback (arg, std::forward<args_t>(args)...);
					}
				}
			}
		};
	};
}
