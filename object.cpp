
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

	void object::on_property_changing (const property_change_args& args)
	{
		#ifdef _WIN32
		this->event_invoker<property_changing_e>()(this, args);
		#endif
	}

	void object::on_property_changed (const property_change_args& args)
	{
		#ifdef _WIN32
		this->event_invoker<property_changed_e>()(this, args);
		#endif
	}

	const xtype<> object::_type = { "object", nullptr, { }, [] { return std::make_unique<object>(); }, };

}
