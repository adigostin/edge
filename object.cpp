
// This file is part of the "edge" library, available at https://github.com/adigostin/edge
// Copyright (c) 2011-2020 Adi Gostin, distributed under Apache License v2.0.

#include "object.h"

namespace edge
{
	void type::add_properties (std::vector<const property*>& properties) const
	{
		if (this->base_type != nullptr)
			this->base_type->add_properties (properties);

		for (auto p : props)
			properties.push_back (p);
	}

	std::vector<const property*> type::make_property_list() const
	{
		std::vector<const property*> props;
		this->add_properties (props);
		return props;
	}

	const property* type::find_property (const char* name) const
	{
		for (auto p : props)
		{
			if ((p->_name == name) || (strcmp(p->_name, name) == 0))
				return p;
		}

		if (base_type != nullptr)
			return base_type->find_property(name);
		else
			return nullptr;
	}

	bool type::has_property (const property* p) const
	{
		auto it = std::find(props.begin(), props.end(), p);
		if (it != props.end())
			return true;
		if (base_type)
			return base_type->has_property(p);
		return false;
	}

	bool type::is_derived_from (const type* t) const
	{
		return (base_type == t) || base_type->is_derived_from(t);
	}

	bool type::is_derived_from (const type& t) const { return is_derived_from(&t); }

	void object::on_property_changing (const property_change_args& args)
	{
		this->event_invoker<property_changing_e>()(this, args);
	}

	void object::on_property_changed (const property_change_args& args)
	{
		this->event_invoker<property_changed_e>()(this, args);
	}

	const type object::_type = { "object", nullptr, { } };
}
