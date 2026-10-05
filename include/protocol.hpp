#pragma once

#include <cstddef>
#include <cstdint>

#include "delivery.hpp"
#include "parser.hpp"
#include "packet.hpp"
#include "packet.hpp"
#include "connectivity_app_handler.hpp"
#include "project_log_config.hpp"

using namespace connectivity::parser;
using namespace connectivity::delivery;

namespace connectivity
{
  namespace protocol
  {

	enum class State : std::uint8_t
	{
	  Idle = 0U,
	  Sending,
	  WaitingAck,
	  RetryPending,
	  Failed
	};

	enum class ProtocolError : std::uint8_t
	{
	  Success = 0U,
	  Busy,
	  DeliveryError,
	  TransportError
	};

	template<typename Transport, typename AppHandler,
		typename Config = packet::DefaultConfig>
	  class Protocol
	  {
		public:
		  explicit Protocol(Transport &transport, AppHandler &appHandler);

		  void init();

		  void process();

		  ProtocolError send(packet::PacketType type, std::uint8_t mainCommand,
							 std::uint8_t subCommand, const std::uint8_t *data,
							 std::uint16_t length);

		  State state() const;

		private:
		  void onPacket(const packet::Packet<Config> &packet);

		  void onParserError(ParserError error);

		  void onTxComplete();

		  void retry();
		  static void sendCallback(void *context, packet::PacketType type,
							std::uint8_t mainCommand, std::uint8_t subCommand,
							const std::uint8_t *data, std::uint16_t length);

		private:
		  Transport &m_transport;
		  AppHandler &m_appHandler;

		  parser::Parser<Config> m_parser { };
		  delivery::Delivery<Config> m_delivery { };

		  State m_state = State::Idle;

		  std::uint8_t m_retryCount = 0U;
	  };

  } // namespace protocol
} // namespace connectivity

#include "protocol_impl.hpp"
