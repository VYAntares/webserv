#pragma once

#include "ConfigStruct.hpp"

class Validator {
	public:
		Validator(Config c);
		Config	validate();

	private:
		void	propogateServerToLocations(Server& s);
		Config	_c;
};
 