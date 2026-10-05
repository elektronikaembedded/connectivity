//packet.hpp
#pragma once

#include <cstdint>
#include <array>

namespace connectivity
{
  namespace packet
  /*
   * Byte 0     SYNC
   Byte 1     SOF
   Byte 2     TYPE
   Byte 3     LENGTH LSB
   Byte 4     LENGTH MSB
   Byte 5     MAIN CMD
   Byte 6     SUB CMD
   Byte 7...  PAYLOAD
   Byte N     CRC LSB
   Byte N+1   CRC MSB
   */
  {
	struct DefaultConfig
	{
		static constexpr bool kEnable = true;

		static constexpr std::size_t kRxBufferSize = 128U;
		static constexpr std::size_t kTxBufferSize = 128U;
		static constexpr std::size_t kMaxPayloadSize = 64U;

		static constexpr std::uint8_t kMaxRetries = 3U;

		// Depends on your scheduler/timer implementation.
		static constexpr std::uint32_t kAckTimeoutMs = 100U;
	};

	static constexpr std::uint8_t kSyncByte = 0xAAU;
	static constexpr std::uint8_t kSofByte = 0x55U;

	static constexpr std::size_t kSyncByteIndex = 0U;
	static constexpr std::size_t kSofByteIndex = 1U;
	static constexpr std::size_t kTypeByteIndex = 2U;
	static constexpr std::size_t kLengthByteIndex = 3U;
	static constexpr std::size_t kMainCmdIdIndex = 5U;
	static constexpr std::size_t kSubCmdIdIndex = 6U;
	static constexpr std::size_t kPayloadIndex = 7U;

	static constexpr std::uint16_t kCrcInitialValue = 0x1D0FU;

	enum class PacketType : std::uint8_t
	{
	  Request = 0x01U,
	  Response = 0x02U,
	  Event = 0x03U,
	  Ack = 0x05U,
	  Nak = 0x06U
	};

	enum class RxState : std::uint8_t
	{
	  WaitSync = 0,
	  WaitSof,
	  ReadType,
	  ReadLengthLsb,
	  ReadLengthMsb,
	  ReadMainCommand,
	  ReadSubCommand,
	  ReadPayload,
	  ReadCrcLsb,
	  ReadCrcMsb
	};

	template<typename Config = DefaultConfig>
	  struct Packet
	  {
		  static_assert(Config::kMaxPayloadSize > 0U);

		  PacketType type { PacketType::Request };

		  std::uint8_t mainCommand { 0U };
		  std::uint8_t subCommand { 0U };

		  std::uint16_t payloadLength { 0U };

		  std::array<std::uint8_t, Config::kMaxPayloadSize> payload { };

		  std::uint16_t crc { 0U };
	  };
  }
}

