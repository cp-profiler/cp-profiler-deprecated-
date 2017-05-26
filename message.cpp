#include "message.hh"
#include <iostream>

void MessageMarshalling::serializeType(std::vector<char> &data,
                                       MessageMarshalling::MsgType f) {
  data.push_back(static_cast<char>(f));
}

void MessageMarshalling::serializeField(std::vector<char> &data, Field f) {
  data.push_back(static_cast<char>(f));
}

void MessageMarshalling::serialize(std::vector<char> &data,
                                   MessageMarshalling::NodeStatus s) {
  data.push_back(static_cast<char>(s));
}

void MessageMarshalling::serialize(std::vector<char> &data, int32_t i) {
  data.push_back(static_cast<char>((i & 0xFF000000) >> 24));
  data.push_back(static_cast<char>((i & 0xFF0000) >> 16));
  data.push_back(static_cast<char>((i & 0xFF00) >> 8));
  data.push_back(static_cast<char>((i & 0xFF)));
}

void MessageMarshalling::serialize(std::vector<char> &data,
                                   const std::string &s) {
  serialize(data, static_cast<int32_t>(s.size()));
  for (char c : s) {
    data.push_back(c);
  }
}

MessageMarshalling::MsgType MessageMarshalling::deserializeMsgType(iter &i) {
  auto m = static_cast<MessageMarshalling::MsgType>(*i);
  ++i;
  return m;
}

MessageMarshalling::Field MessageMarshalling::deserializeField(iter &i) {
  auto f = static_cast<MessageMarshalling::Field>(*i);
  ++i;
  return f;
}

MessageMarshalling::NodeStatus MessageMarshalling::deserializeStatus(iter &i) {
  auto f = static_cast<MessageMarshalling::NodeStatus>(*i);
  ++i;
  return f;
}

int32_t MessageMarshalling::deserializeInt(iter &it) {

  auto b1 = static_cast<uint32_t>(reinterpret_cast<uint8_t&>(*it++));
  auto b2 = static_cast<uint32_t>(reinterpret_cast<uint8_t&>(*it++));
  auto b3 = static_cast<uint32_t>(reinterpret_cast<uint8_t&>(*it++));
  auto b4 = static_cast<uint32_t>(reinterpret_cast<uint8_t&>(*it++));

  return static_cast<int32_t>(b1 << 24 | b2 << 16 | b3 << 8 | b4);
}

std::string MessageMarshalling::deserializeString(iter &it) {
  std::string result;
  // std::cerr << "string size:\n";
  int32_t size = deserializeInt(it);
  result.reserve(size);
  for (int32_t i = 0; i < size; i++) {
    result += *it;
    ++it;
  }
  return result;
}

std::vector<char> MessageMarshalling::serialize() const {
  std::vector<char> data;
  size_t dataSize = 1 + (msg.isNode() ? 4 * 4 + 1 : 0) +
                    (msg.has_restart_id() ? 1 + 4 : 0) +
                    (msg.has_thread_id() ? 1 + 4 : 0) +
                    (msg.has_label() ? 1 + 4 + msg.label().size() : 0) +
                    (msg.has_solution() ? 1 + 4 + msg.solution().size() : 0) +
                    (msg.has_nogood() ? 1 + 4 + msg.nogood().size() : 0) +
                    (msg.has_info() ? 1 + 4 + msg.info().size() : 0);
  data.reserve(dataSize);

  serializeType(data, msg.type());
  if (msg.isNode()) {
    serialize(data, msg.id());
    serialize(data, msg.pid());
    serialize(data, msg.alt());
    serialize(data, msg.kids());
    serialize(data, msg.status());
  }

  if (msg.has_restart_id()) {
    serializeField(data, RESTART_ID);
    serialize(data, msg.restart_id());
  }
  if (msg.has_thread_id()) {
    serializeField(data, THREAD_ID);
    serialize(data, msg.thread_id());
  }
  if (msg.has_label()) {
    serializeField(data, LABEL);
    serialize(data, msg.label());
  }
  if (msg.has_solution()) {
    serializeField(data, SOLUTION);
    serialize(data, msg.solution());
  }
  if (msg.has_nogood()) {
    serializeField(data, NOGOOD);
    serialize(data, msg.nogood());
  }
  if (msg.has_info()) {
    serializeField(data, INFO);
    serialize(data, msg.info());
  }
  return data;
}

void MessageMarshalling::deserialize(char *data, size_t size) {
  char *end = data + size;
  msg.set_type(deserializeMsgType(data));
  if (msg.isNode()) {
    msg.set_id(deserializeInt(data));
    msg.set_pid(deserializeInt(data));
    msg.set_alt(deserializeInt(data));
    msg.set_kids(deserializeInt(data));
    msg.set_status(deserializeStatus(data));
  }

  msg.reset();

  while (data != end) {
    MessageMarshalling::Field f = deserializeField(data);
    switch (f) {
      case RESTART_ID:
        msg.set_rid(deserializeInt(data)); break;
      case THREAD_ID:
        msg.set_tid(deserializeInt(data)); break;
      case LABEL:
        msg.set_label(deserializeString(data)); break;
      case SOLUTION:
        msg.set_solution(deserializeString(data)); break;
      case NOGOOD:
        msg.set_nogood(deserializeString(data)); break;
      case INFO:
        msg.set_info(deserializeString(data)); break;
      default:
        break;
    }
  }
}

cpprofiler::Message& MessageMarshalling::makeNode(int32_t id, int32_t pid, int32_t alt,
                                  int32_t kids, NodeStatus status) {
  msg.reset();
  msg.set_type(MsgType::NODE);

  msg.set_id(id);
  msg.set_pid(pid);
  msg.set_alt(alt);
  msg.set_kids(kids);
  msg.set_status(status);

  return msg;
}

void MessageMarshalling::makeStart(int rid, const std::string &label,
                                   const std::string &info) {
  msg.reset();
  msg.set_type(MsgType::START);
  msg.set_rid(rid);
  msg.set_label(label);
  msg.set_info(info);
}

void MessageMarshalling::makeDone() {
  msg.reset();
  msg.set_type(MsgType::DONE);
}

namespace cpprofiler {

void Message::set_label(const std::string &label) {
  _have_label = true;
  _label = label;
}

void Message::set_nogood(const std::string &nogood) {
  _have_nogood = true;
  _nogood = nogood;
}

void Message::set_info(const std::string &info) {
  _have_info = true;
  _info = info;
}

void Message::set_solution(const std::string &solution) {
  _have_solution = true;
  _solution = solution;
}

void Message::set_tid(int tid) {
  _have_thread_id = true;
  _thread_id = tid;
}

void Message::set_rid(int rid) {
  _have_restart_id = true;
  _restart_id = rid;
}

void Message::set_type(MsgType type) { _type = type; }

void Message::reset() {
  _have_restart_id = false;
  _have_thread_id = false;
  _have_label = false;
  _have_solution = false;
  _have_nogood = false;
  _have_info = false;
}

}
