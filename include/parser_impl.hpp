#pragma once

#include <cstdint>

#include "crc.hpp"
#include "parser.hpp"
#include "packet.hpp"
#include "project_config.hpp"

namespace connectivity
{
  namespace parser
  {

	template<typename Config>
	  void Parser<Config>::init()
	  {
		m_rxState = packet::RxState::WaitSync;
		m_payloadIndex = 0U;
		m_runningCrc = packet::kCrcInitialValue;
		m_packet = { };
	  }

	template<typename Config>
	  void Parser<Config>::reset()
	  {
		init();
	  }

	template<typename Config>
	  ParserError Parser<Config>::process(const std::uint8_t rxByte)
	  {
		ParserError status = ParserError::InProgress;

		switch (m_rxState)
		{
		  case packet::RxState::WaitSync:
		  {
			if(rxByte == packet::kSyncByte)
			{
			  m_payloadIndex = 0U;
			  m_runningCrc = packet::kCrcInitialValue;
			  m_packet = { };
			  m_rxState = packet::RxState::WaitSof;
			}

			break;
		  }

		  case packet::RxState::WaitSof:
		  {
			if(rxByte == packet::kSofByte)
			{
			  m_rxState = packet::RxState::ReadType;
			}
			else
			{
			  m_rxState = packet::RxState::WaitSync;
			  status = ParserError::SofError;

			  //LOG::error("Connectivity RX SOF error");
			}

			break;
		  }

		  case packet::RxState::ReadType:
		  {
			m_packet.type = static_cast<packet::PacketType>(rxByte);

			m_runningCrc = crc::crc16::update(rxByte, m_runningCrc);

			m_rxState = packet::RxState::ReadLengthLsb;

			break;
		  }

		  case packet::RxState::ReadLengthLsb:
		  {
			m_packet.payloadLength = static_cast<std::uint16_t>(rxByte);

			m_runningCrc = crc::crc16::update(rxByte, m_runningCrc);

			m_rxState = packet::RxState::ReadLengthMsb;

			break;
		  }

		  case packet::RxState::ReadLengthMsb:
		  {
			m_packet.payloadLength |= static_cast<std::uint16_t>(rxByte) << 8U;

			m_runningCrc = crc::crc16::update(rxByte, m_runningCrc);

			if(m_packet.payloadLength > Config::kMaxPayloadSize)
			{
			  //LOG::error("Connectivity RX length error");

			  reset();
			  status = ParserError::LengthError;
			}
			else
			{
			  m_rxState = packet::RxState::ReadMainCommand;
			}

			break;
		  }

		  case packet::RxState::ReadMainCommand:
		  {
			m_packet.mainCommand = rxByte;

			m_runningCrc = crc::crc16::update(rxByte, m_runningCrc);

			m_rxState = packet::RxState::ReadSubCommand;

			break;
		  }

		  case packet::RxState::ReadSubCommand:
		  {
			m_packet.subCommand = rxByte;

			m_runningCrc = crc::crc16::update(rxByte, m_runningCrc);

			if(m_packet.payloadLength == 0U)
			{
			  m_rxState = packet::RxState::ReadCrcLsb;
			}
			else
			{
			  m_rxState = packet::RxState::ReadPayload;
			}

			break;
		  }

		  case packet::RxState::ReadPayload:
		  {
			m_packet.payload[m_payloadIndex] = rxByte;
			++m_payloadIndex;

			m_runningCrc = crc::crc16::update(rxByte, m_runningCrc);

			if(m_payloadIndex >= m_packet.payloadLength)
			{
			  m_rxState = packet::RxState::ReadCrcLsb;
			}

			break;
		  }

		  case packet::RxState::ReadCrcLsb:
		  {
			m_packet.crc = static_cast<std::uint16_t>(rxByte);

			m_rxState = packet::RxState::ReadCrcMsb;

			break;
		  }

		  case packet::RxState::ReadCrcMsb:
		  {
			m_packet.crc |= static_cast<std::uint16_t>(rxByte) << 8U;

			if(m_runningCrc != m_packet.crc)
			{
			  //LOG::error("Connectivity RX CRC mismatch");

			  status = ParserError::CrcMismatch;
			}
			else
			{
			  //LOG::info("Connectivity RX packet ready");

			  status = ParserError::PacketReady;
			}

			m_rxState = packet::RxState::WaitSync;
			m_payloadIndex = 0U;
			m_runningCrc = packet::kCrcInitialValue;

			break;
		  }

		  default:
		  {
			//LOG::error("Connectivity RX parser state error");

			reset();

			break;
		  }
		}

		return status;
	  }

  } // namespace protocol
} // namespace connectivity
