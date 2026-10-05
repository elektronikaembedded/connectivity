#pragma once

namespace connectivity
{
  namespace protocol
  {

	template<typename Transport, typename AppHandler, typename Config>
	  Protocol<Transport, AppHandler, Config>::Protocol(Transport &transport,
														AppHandler &appHandler) :
		  m_transport(transport), m_appHandler(appHandler)
	  {
	  }

	template<typename Transport, typename AppHandler, typename Config>
	  void Protocol<Transport, AppHandler, Config>::init()
	  {
		m_parser.init();
		m_delivery.init();

		m_state = State::Idle;
		m_retryCount = 0U;
		m_appHandler.sender().init(
			this, &Protocol<Transport, AppHandler, Config>::sendCallback);

		const auto error = m_transport.init();

		if(error != Transport::Error::Success)
		{
		  LOG::error("Connectivity transport init failed");
		  m_state = State::Failed;
		  return;
		}

		LOG::info("Connectivity protocol initialized");
	  }

	template<typename Transport, typename AppHandler, typename Config>
	  void Protocol<Transport, AppHandler, Config>::process()
	  {
		std::uint8_t rxByte = 0U;

		/*
		 * Process all bytes received by the transport.
		 */
		while (m_transport.receive(rxByte))
		{
		  const ParserError error = m_parser.process(rxByte);

		  if(error == ParserError::PacketReady)
		  {
			onPacket(m_parser.packet());
		  }
		  else if(error != ParserError::InProgress)
		  {
			onParserError(error);
		  }
		}

		/*
		 * Process pending transmission.
		 */
		if(m_state == State::Sending)
		{
		  const auto error = m_transport.transmit(m_delivery.data(),
												  m_delivery.size());

		  if(error == Transport::Error::Success)
		  {
			LOG::info("TX complete");

			onTxComplete();
		  }
		  else
		  {
			LOG::error("TX failed");
			m_state = State::RetryPending;
		  }
		}

		/*
		 * Process protocol state.
		 */
		switch (m_state)
		{
		  case State::Idle:
		  {
			break;
		  }

		  case State::Sending:
		  {
			/*
			 * TX is handled above.
			 */
			break;
		  }

		  case State::WaitingAck:
		  {
			/*
			 * ACK timeout will be handled by the timer
			 * mechanism that will be added later.
			 */
			break;
		  }

		  case State::RetryPending:
		  {
			retry();
			break;
		  }

		  case State::Failed:
		  {
			/*
			 * Transaction has failed.
			 */
			break;
		  }

		  default:
		  {
			LOG::error("Invalid protocol state");

			m_state = State::Idle;
			break;
		  }
		}
	  }

	template<typename Transport, typename AppHandler, typename Config>
	  ProtocolError Protocol<Transport, AppHandler, Config>::send(
		  packet::PacketType type, std::uint8_t mainCommand,
		  std::uint8_t subCommand, const std::uint8_t *data,
		  std::uint16_t length)
	  {
		if(m_state != State::Idle)
		{
		  LOG::error("TX request rejected: protocol busy");
		  return ProtocolError::Busy;
		}

		const DeliveryError error = m_delivery.prepareData(type, mainCommand,
														   subCommand, data,
														   length);

		if(error != DeliveryError::Success)
		{
		  LOG::error("TX packet preparation failed");
		  return ProtocolError::DeliveryError;
		}

		m_retryCount = 0U;
		m_state = State::Sending;

		LOG::info("TX packet queued");

		return ProtocolError::Success;
	  }

	template<typename Transport, typename AppHandler, typename Config>
	  void Protocol<Transport, AppHandler, Config>::onPacket(
		  const packet::Packet<Config> &rxPacket)
	  {
		switch (rxPacket.type)
		{
		  case packet::PacketType::Ack:
		  {
			if(m_state == State::WaitingAck)
			{
			  m_retryCount = 0U;
			  m_state = State::Idle;

			  LOG::info("RX ACK");
			}
			else
			{
			  LOG::error("Unexpected RX ACK");
			}

			break;
		  }

		  case packet::PacketType::Nak:
		  {
			if(m_state == State::WaitingAck)
			{
			  LOG::info("RX NAK");

			  m_state = State::RetryPending;
			}
			else
			{
			  LOG::error("Unexpected RX NAK");
			}

			break;
		  }

		  case packet::PacketType::Request:
		  {
			LOG::info("RX Request");

			/*
			 * Received Request from remote side.
			 *
			 * Application-specific command handling can be
			 * added here later.
			 */
			m_appHandler.onRequest(rxPacket.mainCommand, rxPacket.subCommand,
								   rxPacket.payload.data(),
								   rxPacket.payloadLength);
			break;
		  }

		  case packet::PacketType::Response:
		  {
			LOG::info("RX Response");

			/*
			 * Received response from remote side.
			 */
			m_appHandler.onResponse(rxPacket.mainCommand, rxPacket.subCommand,
									rxPacket.payload.data(),
									rxPacket.payloadLength);
			break;
		  }

		  case packet::PacketType::Event:
		  {
			LOG::info("RX Event");

			/*
			 * Received event from remote side.
			 */
			m_appHandler.onEvent(rxPacket.mainCommand, rxPacket.subCommand,
								 rxPacket.payload.data(),
								 rxPacket.payloadLength);
			break;
		  }

		  default:
		  {
			LOG::error("Unknown RX packet type");
			break;
		  }
		}
	  }

	template<typename Transport, typename AppHandler, typename Config>
	  void Protocol<Transport, AppHandler, Config>::onParserError(
		  ParserError error)
	  {
		switch (error)
		{
		  case ParserError::SofError:
		  {
			LOG::error("RX SOF error");
			break;
		  }

		  case ParserError::LengthError:
		  {
			LOG::error("RX length error");
			break;
		  }

		  case ParserError::CrcMismatch:
		  {
			LOG::error("RX CRC mismatch");
			break;
		  }

		  default:
		  {
			LOG::error("RX parser error");
			break;
		  }
		}
	  }

	template<typename Transport, typename AppHandler, typename Config>
	  void Protocol<Transport, AppHandler, Config>::onTxComplete()
	  {
		if(m_state == State::Sending)
		{
		  /*
		   * The complete frame has been successfully
		   * transmitted.
		   *
		   * Wait for ACK.
		   */
		  m_state = State::WaitingAck;

		  LOG::info("Waiting for ACK");
		}
		else
		{
		  LOG::error("Unexpected TX complete");
		}
	  }

	template<typename Transport, typename AppHandler, typename Config>
	  void Protocol<Transport, AppHandler, Config>::retry()
	  {
		/*
		 * Retry limit will be added when the timer/retry
		 * mechanism is implemented.
		 */

		++m_retryCount;

		LOG::info("Retrying TX");

		m_state = State::Sending;
	  }

	template<typename Transport, typename AppHandler, typename Config>
	  State Protocol<Transport, AppHandler, Config>::state() const
	  {
		return m_state;
	  }
	template<typename Transport, typename AppHandler, typename Config>
	  void Protocol<Transport, AppHandler, Config>::sendCallback(
		  void *context, packet::PacketType type, std::uint8_t mainCommand,
		  std::uint8_t subCommand, const std::uint8_t *data,
		  std::uint16_t length)
	  {
		auto *protocol = static_cast<Protocol*>(context);

		static_cast<void>(protocol->send(type, mainCommand, subCommand, data,
										 length));
	  }

  } // namespace protocol
} // namespace connectivity
