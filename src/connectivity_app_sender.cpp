#include "connectivity_app_sender.hpp"

namespace connectivity
{
  namespace application
  {

	void Sender::init(void *context, SendCallback callback)
	{
	  m_context = context;
	  m_sendCallback = callback;
	}

	void Sender::send(packet::PacketType type, std::uint8_t mainCommand,
					  std::uint8_t subCommand, const std::uint8_t *data,
					  std::uint16_t length)
	{
	  if(m_sendCallback == nullptr)
	  {
		return;
	  }

	  m_sendCallback(m_context, type, mainCommand, subCommand, data, length);
	}

  } // namespace application
} // namespace connectivity
