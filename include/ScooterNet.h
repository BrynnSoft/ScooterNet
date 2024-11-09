#pragma once

#include <stdint.h>

#include <ArduinoRS485.h>
#include <CRC.h>

const uint8_t BROADCAST_ID = 255;
const uint16_t MAX_PACKET_SIZE = 500; // TODO

struct PacketHeader {
  uint8_t src_id;
  uint8_t dst_id;
  uint16_t size;
  uint16_t crc;
};

enum NetStatus {
  NotInitialized,
  Ready,
  AllocationFailure,
  PacketOverSize,
  CrcError,
  BadHeader,
  BadPacket
};

class ScooterNetClass {
public:
#ifdef ARDUINO
  ScooterNetClass(RS485Class* rs485, uint8_t id);
#endif

  void begin();
  bool has_error();
  NetStatus get_error();

  void loop();

  void setPacketHandler(void (*packet_handler)(PacketHeader* header, uint8_t* buffer));
  void callPacketHandler(PacketHeader* header, uint8_t* buffer);

  void sendPacket(uint8_t dst, uint8_t* buffer, uint16_t size);
  uint16_t receivePacket(PacketHeader* header, uint8_t* buffer, uint16_t buffer_size);

  [[deprecated("Use calcCRC8() instead")]]
  uint16_t receivePacket(PacketHeader* header, uint8_t** buffer);

private:
  uint8_t m_id;
#ifdef ARDUINO
  RS485Class* m_rs485;
#endif
  NetStatus m_netstatus;

  void (*m_packet_handler)(PacketHeader* header, uint8_t* buffer) = nullptr;
};