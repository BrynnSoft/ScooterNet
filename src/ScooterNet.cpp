#include "ScooterNet.h"


ScooterNetClass::ScooterNetClass(RS485Class* rs485, uint8_t id) {
  m_rs485 = rs485;
  m_id = id;
  m_netstatus = NetStatus::NotInitialized;
}

void ScooterNetClass::begin() {
  m_rs485->begin(115200);
  m_netstatus = NetStatus::Ready;
}

bool ScooterNetClass::has_error() {
  return m_netstatus != NetStatus::Ready;
}

NetStatus ScooterNetClass::get_error() {
  return m_netstatus;
}

void ScooterNetClass::loop() {
  PacketHeader header;
  if (m_rs485->available()) {
    auto headerReadLength = m_rs485->readBytes((uint8_t*)&header, sizeof(PacketHeader));
    if (headerReadLength < sizeof(PacketHeader)) {
      m_netstatus = NetStatus::BadHeader;
      return;
    }
    if (header.size > MAX_PACKET_SIZE) {
      m_netstatus = NetStatus::PacketOverSize;
      for (uint16_t i = 0; i < header.size; i++) {
        m_rs485->read();
      }
      return;
    }
    auto* buffer = static_cast<uint8_t *>(malloc(header.size));
    if (buffer == nullptr) {
      m_netstatus = NetStatus::AllocationFailure;
      return;
    }
    auto bodyReadLength = m_rs485->readBytes(buffer, header.size);
    if (bodyReadLength < header.size) {
      free(buffer);
      m_netstatus = NetStatus::BadPacket;
      return;
    }
    if (header.dst_id == m_id || header.dst_id == BROADCAST_ID) {
      callPacketHandler(&header, buffer);
    }
    free(buffer);
  }
}

void ScooterNetClass::sendPacket(uint8_t dst, uint8_t *buffer, uint16_t size) {
  PacketHeader header {m_id, dst, size, 0};
  header.crc = calcCRC16(buffer, size);
  m_rs485->beginTransmission();
  m_rs485->write((const uint8_t*)&header, sizeof(PacketHeader));
  m_rs485->write(buffer, size);
  m_rs485->endTransmission();
}

uint16_t ScooterNetClass::receivePacket(PacketHeader *header, uint8_t *buffer,
                                    uint16_t buffer_size) {
  auto headerReadSize = m_rs485->readBytes((uint8_t*)header, sizeof(PacketHeader));
  if (headerReadSize < sizeof(PacketHeader)) {
    m_netstatus = NetStatus::BadHeader;
    return 0;
  }
  if (header->size > MAX_PACKET_SIZE || header->size > buffer_size) {
    m_netstatus = NetStatus::PacketOverSize;
    for (uint16_t i = 0; i < header->size; i++) {
      m_rs485->read();
    }
    return 0;
  }
  auto bodyReadSize = m_rs485->readBytes(buffer, header->size);
  if (bodyReadSize < header->size) {
    m_netstatus = NetStatus::BadPacket;
    return 0;
  }
  auto crc = calcCRC16(buffer, header->size);
  if (crc == header->crc) {
    return header->size;
  }
  m_netstatus = NetStatus::CrcError;
  return 0;
}

uint16_t ScooterNetClass::receivePacket(PacketHeader *header, uint8_t **buffer) {
  m_rs485->readBytes((uint8_t*)header, sizeof(PacketHeader));
  if (header->size > MAX_PACKET_SIZE) {
    m_netstatus = NetStatus::PacketOverSize;
    for (uint16_t i = 0; i < header->size; i++) {
      m_rs485->read();
    }
    return 0;
  }
  *buffer = static_cast<uint8_t *>(malloc(header->size));
  m_rs485->readBytes(*buffer, header->size);
  auto crc = calcCRC16(*buffer, header->size);
  if (crc == header->crc) {
    return header->size;
  }
  m_netstatus = NetStatus::CrcError;
  return 0;
}

void ScooterNetClass::setPacketHandler(void (*packet_handler)(PacketHeader *, uint8_t *)) {
  m_packet_handler = packet_handler;
}

void ScooterNetClass::callPacketHandler(PacketHeader *header, uint8_t *buffer) {
  if (m_packet_handler) {
    m_packet_handler(header, buffer);
  }
}