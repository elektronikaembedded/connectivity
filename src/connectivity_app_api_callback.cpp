#include <cstdint>

#include "connectivity_app_api_callback.hpp"
#include "connectivity_version.hpp"

namespace connectivity
{
  namespace application
  {
	namespace api
	{

	  ApiError api_callback_get_protocol_version(std::uint8_t *pdata,
												 std::uint8_t *plength)
	  {
		if((pdata == nullptr) || (plength == nullptr))
		{
		  return ApiError::NullPointer;
		}

		pdata[0] = connectivity::version::kMajor;
		pdata[1] = connectivity::version::kMinor;
		pdata[2] = connectivity::version::kPatch;

		*plength = 3U;

		return ApiError::Success;
	  }

	} // namespace api
  } // namespace application
} // namespace connectivity
