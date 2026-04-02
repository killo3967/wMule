#pragma once

#include <muleunit/test.h>

#include <common/Path.h>

namespace muleunit {

template <>
inline wxString StringFrom(const EInternalPathError& value)
{
	return InternalPathErrorToString(value);
}

}
