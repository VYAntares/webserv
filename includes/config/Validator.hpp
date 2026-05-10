#pragma once

#include "ConfigStruct.hpp"

class Validator {
	public:
		Validator(Config c);
		Config	validate();

	private:
		void	propogateServerToLocations(Server& s);
		void 	checkLocationConfig(Location& l);
		void	checkDuplicateIntraServer(Server& s, size_t& i);
		Config	_c;
};
 
