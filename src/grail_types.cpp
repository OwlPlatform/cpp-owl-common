#include <grail_types.hpp>

bool grail_types::operator<(const transmitter& a, const transmitter& b) {
  return a.phy < b.phy or
    ( (a.phy == b.phy) and a.id < b.id);
}

bool grail_types::operator==(const transmitter& a, const transmitter& b) {
  return a.phy == b.phy and a.id == b.id;
}

std::ostream& grail_types::operator<<(std::ostream& out, const transmitter& t) {
  return out << (unsigned int)t.phy << '.' << t.id;
}

grail_types::transmitter grail_types::readTransmitterFromBuffer(BuffReader& reader) {
  transmitter t;
  reader.convertPrimitive(t.phy);
  t.id.upper = reader.readPrimitive<uint64_t>();
  t.id.lower = reader.readPrimitive<uint64_t>();
  return t;
}

grail_types::transmitter grail_types::readTransmitter(const std::vector<unsigned char>& buff) {
  transmitter t;
  t.phy = readPrimitive<unsigned char>(buff, 0);
  t.id.upper = readPrimitive<uint64_t>(buff, 1);
  t.id.lower = readPrimitive<uint64_t>(buff, 1+sizeof(uint64_t));
  return t;
}

uint32_t grail_types::writeTransmitter(transmitter t, std::vector<unsigned char>& buff) {
  return pushBackVal<uint8_t>(t.phy, buff) +
    pushBackVal<uint64_t>(t.id.upper, buff) + pushBackVal<uint64_t>(t.id.lower, buff);
}

