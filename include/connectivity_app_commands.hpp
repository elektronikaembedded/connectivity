#pragma once

#include <cstdint>

namespace connectivity
{
  namespace application
  {
	namespace command
	{

	  enum class MainCmd : std::uint8_t
	  {
		Wifi = 0x01U,
		Ble = 0x02U,
		General = 0x03U,
		Time = 0x04U,
		Error = 0xFFU
	  };

	  namespace wifi
	  {

		enum class SubCmd : std::uint8_t
		{
		  VersionGet = 0x01U,
		  VersionReport = 0x02U
		};

	  } // namespace wifi

	  namespace ble
	  {

		enum class SubCmd : std::uint8_t
		{
		  VersionGet = 0x01U,
		  VersionReport = 0x02U
		};

	  } // namespace ble

	  namespace general
	  {

		enum class SubCmd : std::uint8_t
		{
		  VersionGet = 0x01U,
		  VersionReport = 0x02U
		};

	  } // namespace general

	  namespace time
	  {

		enum class SubCmd : std::uint8_t
		{
		  TimeSet = 0x01U,
		  TimeGet = 0x02U,
		  TimeReport = 0x03U
		};

	  } // namespace time

	  namespace error
	  {

		enum class SubCmd : std::uint8_t
		{
		  NotHandled = 0x01U,
		  InvalidCommand = 0x02U,
		  InvalidData = 0x03U,
		  InternalError = 0x04U
		};

	  } // namespace error

	} // namespace command
  } // namespace application
} // namespace connectivity
