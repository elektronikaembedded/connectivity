#include "connectivity_app_api_callback.hpp"
#include "connectivity_app_commands.hpp"
#include "connectivity_app_handler.hpp"
#include "project_log_config.hpp"

namespace connectivity
{
  namespace application
  {

	Handler::Handler(Sender &sender) :
		m_sender(sender)
	{
	}

	void Handler::onRequest(std::uint8_t mainCommand, std::uint8_t subCommand,
							const std::uint8_t *data, std::uint16_t length)
	{
	  LOG::info("Received command from remote side");
	  LOG::info("mainCmd = 0x%02X subCmd = 0x%02X", mainCommand, subCommand);

	  switch (static_cast<command::MainCmd>(mainCommand))
	  {
		case command::MainCmd::General:
		{
		  switch (static_cast<command::general::SubCmd>(subCommand))
		  {
			case command::general::SubCmd::VersionGet:
			{
			  (void) data;
			  (void) length;

			  std::uint8_t version[3] = { };
			  std::uint8_t versionLength = 0U;

			  const api::ApiError error =
				  api::api_callback_get_protocol_version(version,
														 &versionLength);

			  if(error != api::ApiError::Success)
			  {
				LOG::error("Failed to get protocol version");

				const std::uint8_t errorCode = static_cast<std::uint8_t>(error);

				m_sender.send(
					packet::PacketType::Response,
					static_cast<std::uint8_t>(command::MainCmd::Error),
					static_cast<std::uint8_t>(command::error::SubCmd::InternalError),
					&errorCode, sizeof(errorCode));

				break;
			  }

			  LOG::info("TX General VersionReport");

			  m_sender.send(
				  packet::PacketType::Response,
				  static_cast<std::uint8_t>(command::MainCmd::General),
				  static_cast<std::uint8_t>(command::general::SubCmd::VersionReport),
				  version, versionLength);

			  break;
			}

			case command::general::SubCmd::VersionReport:
			{
			  /*
			   * VersionReport received as a command.
			   * Handle if required.
			   */
			  break;
			}

			default:
			{
			  LOG::error("Unknown General sub command");
			  break;
			}
		  }

		  break;
		}

		default:
		{
		  LOG::error("Unknown main command");
		  break;
		}
	  }
	}
	void Handler::onResponse(std::uint8_t mainCommand, std::uint8_t subCommand,
					const std::uint8_t *data, std::uint16_t length)
	{

	}

	void Handler::onEvent(std::uint8_t mainCommand, std::uint8_t subCommand,
				 const std::uint8_t *data, std::uint16_t length)
	{

	}

  } // namespace application
} // namespace connectivity
