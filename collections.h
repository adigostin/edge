
#pragma once
#include "object.h"

namespace edge
{
	struct object_collection_i : owner_i
	{
		virtual size_t size() const = 0;
		virtual object* child_at(size_t index) const = 0;
		virtual void insert (size_t index, std::unique_ptr<object>&& child) = 0;
		void append (std::unique_ptr<object>&& child) { insert(size(), std::move(child)); }
	};

	template<typename child_t>
	struct typed_object_collection_i : object_collection_i
	{
	private:
		virtual std::vector<std::unique_ptr<child_t>>& children_store() = 0;
		const std::vector<std::unique_ptr<child_t>>& children_store() const { return const_cast<typed_object_collection_i*>(this)->children_store(); }

	protected:
		virtual void on_child_inserting (size_t index, child_t* child) { }
		virtual void on_child_inserted (size_t index, child_t* child) { }
		virtual void on_child_removing (size_t index, child_t* child) { }
		virtual void on_child_removed (size_t index, child_t* child) { }

	public:
		const std::vector<std::unique_ptr<child_t>>& children() const { return children_store(); }

		virtual size_t size() const override final { return children_store().size(); }

		virtual child_t* child_at (size_t index) const override final { return children_store()[index].get(); }

		child_t* last() const { return children_store().back().get(); }

		virtual void insert (size_t index, std::unique_ptr<object>&& child) override final
		{
			auto typed_child_raw = static_cast<child_t*>(child.release());
			this->insert(index, std::unique_ptr<child_t>(typed_child_raw));
		}

		void insert (size_t index, std::unique_ptr<child_t>&& o)
		{
			static_assert (std::is_base_of<object, child_t>::value);

			auto& children = children_store();
			assert (index <= children.size());
			child_t* raw = o.get();
			assert (raw->_parent == nullptr);

			static_cast<object*>(raw)->on_inserting_to_parent();
			this->on_child_inserting(index, raw);
			children.insert (children.begin() + index, std::move(o));
			static_cast<object*>(raw)->_parent = this;
			this->on_child_inserted (index, raw);
			static_cast<object*>(raw)->on_inserted_to_parent();
		}

		void append (std::unique_ptr<child_t>&& o)
		{
			insert (children_store().size(), std::move(o));
		}

		std::unique_ptr<child_t> remove(size_t index)
		{
			static_assert (std::is_base_of<object, child_t>::value);

			auto& children = children_store();
			assert (index < children.size());
			auto it = children.begin() + index;
			child_t* raw = (*it).get();
			assert (raw->_parent == this);

			static_cast<object*>(raw)->on_removing_from_parent();
			this->on_child_removing (index, raw);
			static_cast<object*>(raw)->_parent = nullptr;
			auto result = std::move (children[index]);
			children.erase (children.begin() + index);
			this->on_child_removed (index, raw);
			static_cast<object*>(raw)->on_removed_from_parent();

			return result;
		}

		std::unique_ptr<child_t> remove_last()
		{
			return remove(children_store().size() - 1);
		}
	};

	// ========================================================================

	struct object_collection_property : property
	{
		using base = property;

		bool const preallocated;

		object_collection_property (const char* name, const property_group* group, const char* description, bool preallocated)
			: base(name, group, description), preallocated(preallocated)
		{ }

		// Purpose of this is to let the object that holds the property to cast a pointer-to-itself
		// to a ponter to the collection represented by the property. This eliminates the need to do
		// a dynamic_cast, which in turn eliminated the need to include RTTI.
		virtual object_collection_i* collection_cast(object* obj) const = 0;

		const object_collection_i* collection_cast (const object* obj) const { return collection_cast(const_cast<object*>(obj)); }
	};

	template<typename child_t>
	struct typed_object_collection_property : object_collection_property
	{
		using base = object_collection_property;

		using collection_getter_t = typed_object_collection_i<child_t>*(*)(object* obj);

		collection_getter_t const _collection_getter;

		constexpr typed_object_collection_property (const char* name, const property_group* group, const char* description,
			bool preallocated, collection_getter_t collection_getter)
			: base (name, group, description, preallocated)
			, _collection_getter(collection_getter)
		{
			static_assert (std::is_base_of<object, child_t>::value);
		}

		virtual typed_object_collection_i<child_t>* collection_cast(object* obj) const override
		{
			return _collection_getter(obj);
		}

		const typed_object_collection_i<child_t>* collection_cast(const object* obj) const
		{
			return _collection_getter(const_cast<object*>(obj));
		}
	};
}
