# Connectivity

A small, portable communication layer for embedded systems.

This project is being built around a simple idea: the communication layer should be useful on an STM32 bare-metal project today, and still fit naturally into a FreeRTOS or Zephyr project later.

The code is split into small modules so that framing, parsing, packet delivery, transport, and application handling each have a clear responsibility.

## Overview

The current design provides:

- A small binary packet format
- CRC16 protection
- Byte-by-byte packet parsing
- Packet generation for transmission
- A pluggable transport layer
- Application command handling using Main Command + Sub Command
- ACK/NAK based protocol handling
- Retry support in the protocol layer
- Static buffers and predictable memory usage
- A core that does not depend on an RTOS

The first target is an STM32 host communicating with an ESP32 used as a connectivity controller/NCP.

## Architecture

The main structure is:

```text
Application
     |
     v
App Handler
     |
     v
Protocol
  |     \
  |      \
Parser  Delivery
  |        |
  +----+---+
       |
       v
   Transport
       |
       v
      UART
```

Reception:

```text
UART RX interrupt
       |
       v
   RX RingBuffer
       |
       v
Transport::receive()
       |
       v
     Parser
       |
       v
     Protocol
       |
       v
   App Handler
```

Transmission:

```text
App Handler / Application
          |
          v
        Sender
          |
          v
       Protocol
          |
          v
       Delivery
          |
          v
       Transport
          |
          v
         UART
```

The protocol layer coordinates the communication. The transport moves bytes, while the application handler gives meaning to commands and payloads.

## Packet format

The current frame format is:

```text
+------+-----+------+----------+----------+----------+---------+---------+
| SYNC | SOF | TYPE | LEN LSB  | LEN MSB  | MAIN CMD | SUB CMD | PAYLOAD |
+------+-----+------+----------+----------+----------+---------+---------+
                                                               ...
+---------+---------+
| CRC LSB | CRC MSB |
+---------+---------+
```

Field layout:

```text
Byte 0       SYNC
Byte 1       SOF
Byte 2       TYPE
Byte 3       LENGTH LSB
Byte 4       LENGTH MSB
Byte 5       MAIN COMMAND
Byte 6       SUB COMMAND
Byte 7...    PAYLOAD
Last 2       CRC16
```

Current framing bytes:

```text
SYNC = 0xAA
SOF  = 0x55
```

The CRC covers:

```text
TYPE
LENGTH LSB
LENGTH MSB
MAIN COMMAND
SUB COMMAND
PAYLOAD
```

SYNC and SOF are not included in the CRC.

Current CRC settings:

```text
Initial value = 0x1D0F
Polynomial    = 0x1021
CRC on wire   = LSB first
```

## Packet types

Packet type is used for protocol-level message classification. The current design is:

```cpp
enum class PacketType : std::uint8_t
{
    Command = 0x01U,
    Event = 0x02U,
    Ack = 0x03U,
    Nak = 0x04U
};
```

The intended meaning is:

```text
Command  Application command/message
Event    Unsolicited notification
Ack      Positive protocol acknowledgement
Nak      Negative protocol acknowledgement
```

The application-specific meaning is carried by Main Command and Sub Command.

## Application command model

Commands are grouped by functional area.

```cpp
enum class MainCmd : std::uint8_t
{
    Wifi = 0x01U,
    Ble = 0x02U,
    General = 0x03U,
    Time = 0x04U
};
```

Each group has its own Sub Command values. For example:

```cpp
namespace wifi
{

enum class SubCmd : std::uint8_t
{
    VersionGet = 0x01U,
    VersionReport = 0x02U
};

} // namespace wifi
```

```cpp
namespace ble
{

enum class SubCmd : std::uint8_t
{
    VersionGet = 0x01U,
    VersionReport = 0x02U
};

} // namespace ble
```

```cpp
namespace general
{

enum class SubCmd : std::uint8_t
{
    VersionGet = 0x01U,
    VersionReport = 0x02U
};

} // namespace general
```

```cpp
namespace time
{

enum class SubCmd : std::uint8_t
{
    TimeSet = 0x01U,
    TimeGet = 0x02U,
    TimeReport = 0x03U
};

} // namespace time
```

This lets the same sub-command value be reused in different command groups while keeping the complete command unambiguous on the wire.

For example:

```text
01 / 01 = Wi-Fi / VersionGet
02 / 01 = BLE / VersionGet
03 / 01 = General / VersionGet
04 / 02 = Time / TimeGet
```

## Example: connectivity version

One of the first commands used for bring-up is querying the version of the connectivity protocol/module itself.

The request is:

```text
Command / General / VersionGet
```

The response is:

```text
Command / General / VersionReport / Major.Minor.Patch
```

The version is represented as three payload bytes:

```text
Major
Minor
Patch
```

For example, version `1.0.0` is:

```text
01 00 00
```

This version identifies the connectivity module/protocol, not the product firmware version.

## Application handling

The protocol layer does not contain product-specific command behaviour.

Once a valid application packet has been parsed, the protocol passes the command information to the application handler:

```cpp
onCommand(
    mainCommand,
    subCommand,
    data,
    length);
```

The handler dispatches by Main Command and Sub Command and can then call an application API for the actual operation.

For example:

```text
Protocol
   |
   v
App Handler
   |
   +--> General
          |
          +--> VersionGet
                 |
                 +--> Application API
```

This keeps the generic communication code separate from application behaviour.

## Application API

Application APIs are kept separate from command dispatch.

For example:

```cpp
ApiError api_callback_get_protocol_version(
    std::uint8_t* pdata,
    std::uint8_t* plength);
```

The handler asks the application API for the requested information and sends the result through the application sender.

That gives a clean boundary:

```text
Connectivity protocol
        |
        v
Application handler
        |
        v
Application API
        |
        v
Product / device implementation
```

## Sender

The application sender provides the path for application code to send a message back to the remote side without knowing anything about the UART or the protocol buffers.

```text
App Handler
    |
    v
  Sender
    |
    v
 Protocol::send()
    |
    v
 Delivery
    |
    v
 Transport
```

The protocol remains responsible for the actual frame generation, transmission and protocol state.

## Transport

The transport is supplied to the protocol as a template parameter:

```cpp
template <
    typename Transport,
    typename AppHandler,
    typename Config = packet::DefaultConfig>
class Protocol;
```

The current STM32 UART transport uses:

- TX: polling
- RX: interrupt
- RX buffering: ring buffer

The UART interrupt only receives the byte and stores it in the RX ring buffer. Packet parsing happens outside the interrupt.

That keeps the ISR small and makes the same protocol core usable with other transports.

## Usage

For the current STM32 bring-up, the connectivity objects can be created by wiring the transport, application sender, application handler, and protocol together.

A simple project configuration can look like this:

```cpp
#pragma once

#include "connectivity_transport_uart.hpp"
#include "protocol.hpp"
#include "connectivity_app_handler.hpp"
#include "connectivity_app_sender.hpp"

extern UART_HandleTypeDef huart1;

connectivity::transport::Uart uartTransport(huart1);

connectivity::application::Sender sender;

connectivity::application::Handler appHandler(sender);

connectivity::protocol::Protocol<
    connectivity::transport::Uart,
    connectivity::application::Handler> protocol(
        uartTransport,
        appHandler);
```

The objects are connected as follows:

```text
Uart Transport
      |
      v
    Protocol
      |
      v
 App Handler
      |
      v
    Sender
```

The protocol is initialized once during startup:

```cpp
protocol.init();
```

The application main loop only needs to run the protocol:

```cpp
while (1)
{
    protocol.process();
}
```

The UART receive interrupt is handled by the transport. The ISR stays small and the received byte is placed into the transport's RX ring buffer:

```cpp
void HAL_UART_RxCpltCallback(UART_HandleTypeDef* huart)
{
    uartTransport.onRxComplete(huart);
}
```

The flow is then:

```text
UART RX interrupt
       |
       v
  RX RingBuffer
       |
       v
protocol.process()
       |
       v
     Parser
       |
       v
  App Handler
```

For transmission, the application can send through the `Sender` without knowing anything about the UART transport:

```cpp
sender.send(
    packet::PacketType::Command,
    static_cast<std::uint8_t>(
        connectivity::application::command::MainCmd::General),
    static_cast<std::uint8_t>(
        connectivity::application::command::general::SubCmd::VersionReport),
    data,
    length);
```

In normal use, the application handler owns the sender reference and uses it when a received command requires a response or when the application needs to send an asynchronous message.

For a simple bare-metal test, this keeps `main.cpp` free from parser and transport details. In an RTOS project, the same `protocol.process()` call can be placed in a dedicated connectivity task.

> **Note:** if the objects above are placed in a header such as `project_config.hpp`, make sure that header is included in only one translation unit for the object definitions, or use `extern` declarations in the header and define the objects in a `.cpp` file. This avoids multiple-definition linker errors.

## Bare-metal

The intended bare-metal application loop is deliberately small:

```cpp
while (1)
{
    protocol.process();
}
```

The protocol internally handles received bytes, packet parsing, application dispatch, pending transmission, and protocol state.

## RTOS

The same protocol can run inside a single connectivity task.

A typical arrangement is:

```text
UART RX ISR
     |
     v
RX RingBuffer
     |
     v
Connectivity Task
     |
     +--> Parser
     +--> Protocol state machine
     +--> App Handler
     +--> TX
```

There is normally no need for separate RX, TX and protocol tasks unless the application needs more throughput or more complicated scheduling.

The protocol itself remains RTOS independent.

## ACK / NAK and retry

The protocol uses a small state machine:

```text
Idle
  |
  v
Sending
  |
  v
WaitingAck
  |       \
 ACK      NAK
  |         |
  v         v
Idle    RetryPending
             |
             v
          Sending
```

The serialized frame is retained by the Delivery object so the same frame can be sent again during a retry.

Timeout handling is planned as a separate reusable timer abstraction. This keeps the protocol independent of SysTick, hardware timers, FreeRTOS timers, or another RTOS API.

## Design principles

A few rules guide the implementation:

- Keep the protocol independent of the transport.
- Keep application behaviour out of the protocol core.
- Keep the parser focused on framing and CRC validation.
- Keep transport code focused on moving bytes.
- Prefer static storage and predictable memory usage.
- Avoid dynamic allocation in the core.
- Keep RTOS dependencies out of reusable protocol code.
- Make the wire format easy to inspect during bring-up.
- Keep modules small enough to reuse in other embedded projects.

## Project layout

The structure is still evolving, but the intended layout is roughly:

```text
Connectivity/
├── Modules/
│   └── connectivity/
│       └── include/
│           ├── packet.hpp
│           ├── parser.hpp
│           ├── parser_impl.hpp
│           ├── delivery.hpp
│           ├── delivery_impl.hpp
│           ├── protocol.hpp
│           ├── protocol_impl.hpp
│           ├── connectivity_app_commands.hpp
│           ├── connectivity_app_handler.hpp
│           ├── connectivity_app_sender.hpp
│           ├── connectivity_app_api_callback.hpp
│           └── connectivity_version.hpp
│
├── Ports/
│   ├── connectivity_transport_uart.hpp
│   ├── connectivity_transport_uart.cpp
│   └── project_config.hpp
│
└── ...
```

The exact layout may change as more transports and application features are added.

## Current status

The project is currently being brought up on STM32 using UART.

The first end-to-end application test is:

```text
General / VersionGet
        |
        v
Application API
        |
        v
General / VersionReport
```

The current focus is getting the basic communication path working cleanly before adding the reusable timer abstraction and more application commands.

## Why this project exists

This is not intended to be a full networking stack.

It is a lightweight communication layer for embedded projects where it is useful to have a predictable packet format and a clear separation between:

- bytes on the wire
- protocol state
- transport implementation
- application commands
- application data

The goal is to make the core easy to debug, easy to port, and simple enough to reuse in another embedded project without redesigning the whole communication stack.
