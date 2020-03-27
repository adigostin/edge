
// This file is part of the "edge" library, available at https://github.com/adigostin/edge
// Copyright (c) 2011-2020 Adi Gostin, distributed under Apache License v2.0.

#pragma once
#include "object.h"

namespace edge
{
	struct collection_i
	{
	};

	class owned_object : public object
	{
		using base = object;

		template<typename child_type>
		friend struct typed_collection_i;

		collection_i* _parent = nullptr;

	public:
		collection_i* parent() const { return _parent; }

		#ifdef _WIN32
		struct added_to_parent_event : public event<added_to_parent_event> { };
		struct removing_from_parent_event : public event<removing_from_parent_event> { };

		added_to_parent_event::subscriber added_to_collection() { return added_to_parent_event::subscriber(this); }
		removing_from_parent_event::subscriber removing_from_collection() { return removing_from_parent_event::subscriber(this); }
		#endif

	protected:
		virtual void on_added_to_parent()
		{
			#ifdef _WIN32
			this->event_invoker<added_to_parent_event>()();
			#endif
		}

		virtual void on_removing_from_parent()
		{
			#ifdef _WIN32
			this->event_invoker<removing_from_parent_event>()();
			#endif
		}
	};

	template<typename child_t>
	struct typed_collection_i : collection_i
	{
		static_assert (std::is_base_of<owned_object, child_t>::value);

	private:
		virtual std::vector<std::unique_ptr<child_t>>& children_store() = 0;
		virtual object* as_object() = 0;

	protected:
		virtual void on_child_inserted (size_t index, child_t* child)
		{
		}

		virtual void on_child_removing (size_t index, child_t* child)
		{
		}

	public:
		const std::vector<std::unique_ptr<child_t>>& children() const
		{
			return const_cast<typed_collection_i*>(this)->children_store();
		}

		void insert (size_t index, std::unique_ptr<child_t>&& o)
		{
			auto& children = children_store();
			assert (index <= children.size());
			child_t* raw = o.get();
			children.insert (children.begin() + index, std::move(o));
			assert (raw->_parent == nullptr);
			raw->_parent = this;
			this->on_child_inserted (index, raw);
			static_cast<owned_object*>(raw)->on_added_to_parent();
		}

		void append (std::unique_ptr<child_t>&& o)
		{
			insert (children_store().size(), std::move(o));
		}

		std::unique_ptr<child_t> remove(size_t index)
		{
			auto& children = children_store();
			assert (index < children.size());
			child_t* raw = children[index].get();
			static_cast<owned_object*>(raw)->on_removing_from_parent();
			this->on_child_removing (index, raw);
			assert (raw->_parent == this);
			raw->_parent = nullptr;
			auto result = std::move (children[index]);
			children.erase (children.begin() + index);
			return result;
		}

		std::unique_ptr<child_t> remove_last()
		{
			return remove(children_store().size() - 1);
		}
	};
}
