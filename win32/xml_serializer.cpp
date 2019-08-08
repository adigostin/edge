
#include "pch.h"
#include "xml_serializer.h"
#include "utility_functions.h"

namespace edge
{
	static const _bstr_t entry_elem_name = "Entry";
	static const _bstr_t index_attr_name = "index";
	static const _bstr_t value_attr_name = "Value";
	
	static com_ptr<IXMLDOMElement> serialize_internal (IXMLDOMDocument* doc, const object* obj, bool force_serialize_unchanged, size_t index_attribute);
	static void deserialize_to_internal (IXMLDOMElement* element, object* obj, bool ignore_index_attribute);

	static com_ptr<IXMLDOMElement> serialize_property (IXMLDOMDocument* doc, const object* obj, const object_collection_property* prop)
	{
		size_t size = prop->size(obj);
		if (size == 0)
			return nullptr;

		HRESULT hr;
		com_ptr<IXMLDOMElement> collection_element;
		auto ensure_collection_element_created = [doc, prop, &collection_element]()
		{
			if (collection_element == nullptr)
			{
				auto hr = doc->createElement(_bstr_t(prop->_name), &collection_element);
				assert(SUCCEEDED(hr));
			}
		};

		bool force_serialize_unchanged;
		bool add_index_attribute;
		if (prop->can_insert_remove())
		{
			// variable-size collection - always serialize all children
			force_serialize_unchanged = true;
			add_index_attribute = false;
		}
		else
		{
			// fixed-size collection allocated by the object in its constructor - serialize only changed children
			force_serialize_unchanged = false;
			add_index_attribute = true;
		}

		for (size_t i = 0; i < size; i++)
		{
			auto child = prop->child_at(obj, i);
			auto child_elem = serialize_internal(doc, child, force_serialize_unchanged, add_index_attribute ? i : -1);
			if (child_elem != nullptr)
			{
				ensure_collection_element_created();
				hr = collection_element->appendChild (child_elem, nullptr);  assert(SUCCEEDED(hr));
			}
		}

		return collection_element;
	}

	static com_ptr<IXMLDOMElement> serialize_property (IXMLDOMDocument* doc, const object* obj, const value_collection_property* prop)
	{
		size_t size = prop->size(obj);
		if (size == 0)
			return nullptr;
		
		if (!prop->changed(obj))
			return nullptr;

		com_ptr<IXMLDOMElement> collection_element;
		auto hr = doc->createElement (_bstr_t(prop->_name), &collection_element); assert(SUCCEEDED(hr));
		for (size_t i = 0; i < size; i++)
		{
			auto value = prop->get_value(obj, i);
			com_ptr<IXMLDOMElement> entry_element;
			hr = doc->createElement (entry_elem_name, &entry_element); assert(SUCCEEDED(hr));
			hr = entry_element->setAttribute (index_attr_name, _variant_t(i)); assert(SUCCEEDED(hr));
			hr = entry_element->setAttribute (value_attr_name, _variant_t(value.c_str())); assert(SUCCEEDED(hr));
			hr = collection_element->appendChild (entry_element, nullptr); assert(SUCCEEDED(hr));
		}

		return collection_element;
	}

	static com_ptr<IXMLDOMElement> serialize_internal (IXMLDOMDocument* doc, const object* obj, bool force_serialize_unchanged, size_t index_attribute)
	{
		com_ptr<IXMLDOMElement> object_element;
		auto ensure_object_element_created = [doc, obj, index_attribute, &object_element]()
		{
			if (object_element == nullptr)
			{
				auto hr = doc->createElement(_bstr_t(obj->type()->name), &object_element);
				assert(SUCCEEDED(hr));

				if (index_attribute != (size_t)-1)
				{
					hr = object_element->setAttribute (index_attr_name, _variant_t(index_attribute));
					assert(SUCCEEDED(hr));
				}
			}
		};
		
		if (force_serialize_unchanged)
			ensure_object_element_created();

		auto props = obj->type()->make_property_list();
		for (const property* prop : props)
		{
			if (auto value_prop = dynamic_cast<const value_property*>(prop))
			{
				bool is_factory_prop = std::any_of (obj->type()->factory_props().begin(), obj->type()->factory_props().end(), [value_prop](auto p) { return p == value_prop; });
				if (is_factory_prop || (value_prop->has_setter() && value_prop->changed_from_default(obj)))
				{
					ensure_object_element_created();
					auto value = value_prop->get_to_string(obj);
					auto hr = object_element->setAttribute(_bstr_t(value_prop->_name), _variant_t(value.c_str())); assert(SUCCEEDED(hr));
				}
			}
		}

		for (const property* prop : props)
		{
			if (dynamic_cast<const value_property*>(prop))
				continue;

			com_ptr<IXMLDOMElement> prop_element;
			if (auto oc_prop = dynamic_cast<const object_collection_property*>(prop))
			{
				prop_element = serialize_property (doc, obj, oc_prop);
			}
			else if (auto vc_prop = dynamic_cast<const value_collection_property*>(prop))
			{
				prop_element = serialize_property (doc, obj, vc_prop);
			}
			else
				assert(false); // not implemented

			if (prop_element)
			{
				ensure_object_element_created();
				auto hr = object_element->appendChild (prop_element, nullptr); assert(SUCCEEDED(hr));
			}
		}

		return object_element;
	}

	com_ptr<IXMLDOMElement> serialize (IXMLDOMDocument* doc, const object* obj, bool force_serialize_unchanged)
	{
		return serialize_internal (doc, obj, force_serialize_unchanged, -1);
	}

	// ========================================================================

	static std::unique_ptr<object> create_object (IXMLDOMElement* elem, const struct type* type)
	{
		std::vector<std::string> factory_param_strings;
		for (auto factory_prop : type->factory_props())
		{
			_variant_t value;
			elem->getAttribute(_bstr_t(factory_prop->_name), value.GetAddress());
			assert (value.vt == VT_BSTR);
			factory_param_strings.push_back(bstr_to_utf8(value.bstrVal));
		}

		std::vector<std::string_view> factory_params;
		for (auto& str : factory_param_strings)
			factory_params.push_back(str);
		auto obj = std::unique_ptr<object>(type->create({ factory_params.data(), factory_params.data() + factory_params.size() }));
		return obj;
	}

	std::unique_ptr<object> deserialize (IXMLDOMElement* elem)
	{
		_bstr_t namebstr;
		auto hr = elem->get_nodeName(namebstr.GetAddress()); assert(SUCCEEDED(hr));
		auto name = bstr_to_utf8(namebstr);
		auto type = type::find_type(name.c_str());
		if (type == nullptr)
			assert(false); // error handling for this not implemented
		auto obj = create_object(elem, type);
		deserialize_to_internal (elem, obj.get(), false);
		return obj;
	}

	static void deserialize_to_new_object_collection (IXMLDOMElement* collection_elem, object* o, const object_collection_property* prop)
	{
		size_t index = 0;
		com_ptr<IXMLDOMNode> child_node;
		auto hr = collection_elem->get_firstChild(&child_node); assert(SUCCEEDED(hr));
		while (child_node != nullptr)
		{
			com_ptr<IXMLDOMElement> child_elem = child_node.get();

			_bstr_t namebstr;
			auto hr = child_elem->get_nodeName(namebstr.GetAddress()); assert(SUCCEEDED(hr));
			auto name = bstr_to_utf8(namebstr);
			auto type = type::find_type(name.c_str());
			if (type == nullptr)
				assert(false); // error handling for this not implemented
			auto child = create_object(child_elem, type);
			auto child_raw = child.get();
			prop->insert_child (o, index, std::move(child));
			deserialize_to_internal (child_elem, child_raw, false);
			index++;
			hr = child_node->get_nextSibling(&child_node); assert(SUCCEEDED(hr));
		}
	}

	static void deserialize_to_existing_object_collection (IXMLDOMElement* collection_elem, object* obj, const object_collection_property* prop)
	{
		size_t child_node_index = 0;
		com_ptr<IXMLDOMNode> child_node;
		auto hr = collection_elem->get_firstChild(&child_node); assert(SUCCEEDED(hr));
		while (child_node != nullptr)
		{
			com_ptr<IXMLDOMElement> child_elem = child_node.get();
			size_t index = child_node_index;
			_variant_t index_attr_value;
			hr = child_elem->getAttribute(index_attr_name, index_attr_value.GetAddress());
			if (hr == S_OK)
			{
				bool converted = size_property_traits::from_string(bstr_to_utf8(index_attr_value.bstrVal), index);
				assert(converted);
			}

			auto child = prop->child_at(obj, index);
			deserialize_to_internal (child_elem, child, true);

			child_node_index++;
			hr = child_node->get_nextSibling(&child_node); assert(SUCCEEDED(hr));
		}
	}

	static void deserialize_value_collection (IXMLDOMElement* collection_elem, object* obj, const value_collection_property* prop)
	{
		com_ptr<IXMLDOMNode> entry_node;
		auto hr = collection_elem->get_firstChild(&entry_node); assert(SUCCEEDED(hr));
		while (entry_node != nullptr)
		{
			com_ptr<IXMLDOMElement> entry_elem = entry_node.get();
			_variant_t index_attr_value;
			hr = entry_elem->getAttribute(index_attr_name, index_attr_value.GetAddress()); assert(SUCCEEDED(hr));
			size_t index;
			bool converted = size_property_traits::from_string(bstr_to_utf8(index_attr_value.bstrVal), index); assert(converted);

			_variant_t value_attr_value;
			hr = entry_elem->getAttribute(value_attr_name, value_attr_value.GetAddress()); assert(SUCCEEDED(hr));
			
			auto value_utf8 = bstr_to_utf8(value_attr_value.bstrVal);
			if (prop->can_insert_remove())
			{
				bool converted = prop->insert_value (obj, index, value_utf8);
				assert(converted);
			}
			else
			{
				bool converted = prop->set_value(obj, index, value_utf8);
				assert(converted);
			}

			hr = entry_node->get_nextSibling(&entry_node); assert(SUCCEEDED(hr));
		}
	}

	static void deserialize_to_internal (IXMLDOMElement* element, object* obj, bool ignore_index_attribute)
	{
		auto deserializable = dynamic_cast<deserialize_i*>(obj);
		if (deserializable != nullptr)
			deserializable->on_deserializing();

		com_ptr<IXMLDOMNamedNodeMap> attrs;
		auto hr = element->get_attributes (&attrs); assert(SUCCEEDED(hr));
		long attr_count;
		hr = attrs->get_length(&attr_count); assert(SUCCEEDED(hr));
		for (long i = 0; i < attr_count; i++)
		{
			com_ptr<IXMLDOMNode> attr_node;
			hr = attrs->get_item(i, &attr_node); assert(SUCCEEDED(hr));
			com_ptr<IXMLDOMAttribute> attr = attr_node.get();
			_bstr_t namebstr;
			hr = attr->get_name(namebstr.GetAddress()); assert(SUCCEEDED(hr));

			if (ignore_index_attribute && (namebstr == index_attr_name))
				continue;

			auto name = bstr_to_utf8(namebstr);

			bool is_factory_property = std::any_of (obj->type()->factory_props().begin(),
				obj->type()->factory_props().end(),
				[&name](const value_property* fp) { return strcmp(name.c_str(), fp->_name) == 0; });
			if (is_factory_property)
				continue;

			_bstr_t valuebstr;
			hr = attr->get_text(valuebstr.GetAddress()); assert(SUCCEEDED(hr));
			auto value = bstr_to_utf8(valuebstr);

			auto prop = obj->type()->find_property(name.c_str());
			if (prop == nullptr)
				assert(false); // error handling for this not implemented
			auto value_prop = dynamic_cast<const value_property*>(prop);
			if (value_prop == nullptr)
				assert(false); // error handling for this not implemented
			bool set_successful = value_prop->try_set_from_string(obj, value.c_str());
			if (!set_successful)
				assert(false); // error handling for this not implemented
		}

		com_ptr<IXMLDOMNode> child_node;
		hr = element->get_firstChild(&child_node); assert(SUCCEEDED(hr));
		while (child_node != nullptr)
		{
			com_ptr<IXMLDOMElement> child_elem = child_node.get();
			_bstr_t namebstr;
			hr = child_elem->get_nodeName(namebstr.GetAddress()); assert(SUCCEEDED(hr));
			auto name = bstr_to_utf8(namebstr);
			
			auto prop = obj->type()->find_property(name.c_str());
			if (prop == nullptr)
				assert(false); // error handling for this not implemented

			if (auto vc_prop = dynamic_cast<const value_collection_property*>(prop))
			{
				deserialize_value_collection (child_elem, obj, vc_prop);
			}
			else if (auto oc_prop = dynamic_cast<const object_collection_property*>(prop))
			{
				if (oc_prop->can_insert_remove())
					deserialize_to_new_object_collection (child_elem, obj, oc_prop);
				else
					deserialize_to_existing_object_collection (child_elem, obj, oc_prop);
			}
			else
				assert(false); // error handling for this not implemented

			hr = child_node->get_nextSibling(&child_node); assert(SUCCEEDED(hr));
		}

		if (deserializable != nullptr)
			deserializable->on_deserialized();
	}

	void deserialize_to (IXMLDOMElement* element, object* obj)
	{
		return deserialize_to_internal (element, obj, false);
	}
}
