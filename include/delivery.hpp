#pragma once

#include <array>
#include <cstdint>

#include "packet.hpp"

using namespace connectivity::packet;

namespace connectivity
{
  namespace delivery
  {
	enum class DeliveryError : std::uint8_t
	{
	  Success = 0U,
	  InvalidType,
	  InvalidData,
	  InvalidLength,
	  BufferOverflow
	};

	template<typename Config = DefaultConfig>
	  class Delivery
	  {
		public:
		  void init();

		  DeliveryError prepareHandshake(PacketType type);

		  DeliveryError prepareData(PacketType type, std::uint8_t mainCommand,
									std::uint8_t subCommand,
									const std::uint8_t *data,
									std::uint16_t length);

		  const std::uint8_t* data() const
		  {
			return m_buffer.data();
		  }

		  std::size_t size() const
		  {
			return m_size;
		  }

		private:
		  std::array<std::uint8_t, Config::kTxBufferSize> m_buffer { };
		  std::size_t m_size = 0U;
	  };
  }
}

#include "delivery_impl.hpp"
