#pragma once

#include <cstdint>

#include "packet.hpp"

namespace connectivity
{
  namespace application
  {

	class Sender
	{
	  public:
		using SendCallback = void (*)(
			void* context,
			packet::PacketType type,
			std::uint8_t mainCommand,
			std::uint8_t subCommand,
			const std::uint8_t* data,
			std::uint16_t length);

	  public:
		void init(void *context, SendCallback callback);

		void send(packet::PacketType type, std::uint8_t mainCommand,
				  std::uint8_t subCommand, const std::uint8_t *data,
				  std::uint16_t length);

	  private:
		void *m_context = nullptr;
		SendCallback m_sendCallback = nullptr;
	};

  } // namespace application
} // namespace connectivity
