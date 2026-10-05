#pragma once

#include <array>
#include <cstdint>
#include "connectivity_app_sender.hpp"

namespace connectivity
{
  namespace application
  {
	class Handler
	{

	  public:
		explicit Handler(Sender &sender);
		void onRequest(std::uint8_t mainCommand, std::uint8_t subCommand,
					   const std::uint8_t *data, std::uint16_t length);

		void onResponse(std::uint8_t mainCommand, std::uint8_t subCommand,
						const std::uint8_t *data, std::uint16_t length);

		void onEvent(std::uint8_t mainCommand, std::uint8_t subCommand,
					 const std::uint8_t *data, std::uint16_t length);



		Sender& sender()
		{
		    return m_sender;
		}

	  private:
		Sender& m_sender;

	};
  }
}
