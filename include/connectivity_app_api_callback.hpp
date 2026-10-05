#pragma once

#include <cstdint>

namespace connectivity
{
  namespace application
  {
	namespace api
	{

	  enum class ApiError : std::uint8_t
	  {
		Success = 0U,
		NullPointer,
		InvalidArg,
		LengthError
	  };

	  ApiError api_callback_get_protocol_version(std::uint8_t *pdata,
												 std::uint8_t *plength);

	} // namespace api
  } // namespace application
} // namespace connectivity
