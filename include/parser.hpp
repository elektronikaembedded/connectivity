// parser.hpp
#pragma once

#include <cstddef>
#include <cstdint>

using namespace connectivity::packet;

#include "packet.hpp"

namespace connectivity
{
  namespace parser
  {
	enum class ParserError : std::uint8_t
	{
	  InProgress,
	  PacketReady,
	  SofError,
	  LengthError,
	  CrcMismatch
	};

	template<typename Config = DefaultConfig>
	  class Parser
	  {
		public:
		  void init();

		  ParserError process(std::uint8_t rxByte);

		  const Packet<Config>& packet() const
		  {
			return m_packet;
		  }

		private:
		  void reset();

		private:
		  RxState m_rxState = RxState::WaitSync;

		  packet::Packet<Config> m_packet { };

		  std::size_t m_payloadIndex = 0U;

		  std::uint16_t m_runningCrc = kCrcInitialValue;

	  };
  } // namespace protocol

} // namespace connectivity
#include "parser_impl.hpp"
