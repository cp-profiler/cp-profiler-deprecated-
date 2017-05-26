#ifndef MESSAGE_HH
#define MESSAGE_HH

#include <vector>
#include <cstdint>
#include <string>
#include <cassert>

namespace cpprofiler {

  enum NodeStatus {
    SOLVED = 0,        ///< Node representing a solution
    FAILED = 1,        ///< Node representing failure
    BRANCH = 2,        ///< Node representing a branch
    UNDETERMINED = 3,  ///< Node that has not been explored yet
    STOP = 4,          ///< Node representing stop point
    UNSTOP = 5,        ///< Node representing ignored stop point
    SKIPPED = 6,       ///< Skipped by backjumping
    MERGING = 7
  };

  enum class MsgType {
    NODE = 0,
    DONE = 1,
    START = 2,
  };


template <typename T>
class Option {
  T value;
  bool present{false};

public:
  bool is_present() { return present; }
  void set(const T& t) { present = true; value = t; }
  void unset() { present = false; }
  T& get() { assert(present); return value; }
};

  class Message {

    MsgType _type;

    int32_t _id;
    int32_t _pid;
    int32_t _alt;
    int32_t _kids;
    NodeStatus _status;

    bool _have_restart_id{false};
    int32_t _restart_id;
    bool _have_thread_id{false};
    int32_t _thread_id;

    bool _have_label{false};
    std::string _label;

    bool _have_solution{false};
    std::string _solution;

    bool _have_nogood{false};
    std::string _nogood;

    bool _have_info{false};
    std::string _info;
  public:

    bool isNode(void) const { return _type == MsgType::NODE; }

    // required fields for node messages
    int32_t id() const { return _id; }
    void set_id(int32_t id) { _id = id; }

    int32_t pid() const { return _pid; }
    void set_pid(int32_t pid) { _pid = pid; }

    int32_t alt(void) const { return _alt; }
    void set_alt(int32_t alt) { _alt = alt; }

    int32_t kids(void) const { return _kids; }
    void set_kids(int32_t kids) { _kids = kids; }

    NodeStatus status(void) const { return _status; }
    void set_status(NodeStatus status) { _status = status; }

    // optional fields for node messages

    bool has_restart_id() const { return _have_restart_id; }
    int32_t restart_id() const { return _restart_id; }

    bool has_thread_id() const { return _have_thread_id; }
    int32_t thread_id() const { return _thread_id; }

    void set_label(const std::string& label);
    void set_info(const std::string& info);

    void set_nogood(const std::string& nogood);
    void set_solution(const std::string& solution);

    void set_rid(int32_t rid);
    void set_tid(int32_t tid);

    bool has_label() const { return _have_label; }
    const std::string& label() const { return _label; }

    bool has_solution(void) const { return _have_solution; }
    const std::string& solution(void) const { return _solution; }

    bool has_nogood(void) const { return _have_nogood; }
    const std::string& nogood(void) const { return _nogood; }

    bool isStart(void) const;
    bool isDone(void) const;

    // generic optional fields
    bool has_info() const { return _have_info; }
    const std::string& info(void) const { return _info; }

    void set_type(MsgType type);
    MsgType type() const { return _type; }

    void reset();
  };

}

class MessageMarshalling {

 private:
  enum Field {
    ID = 0,
    PID = 1,
    ALT = 2,
    KIDS = 3,
    STATUS = 4,
    RESTART_ID = 5,
    THREAD_ID = 6,
    LABEL = 7,
    SOLUTION = 8,
    NOGOOD = 9,
    INFO = 10
  };

  using NodeStatus = cpprofiler::NodeStatus;
  using MsgType = cpprofiler::MsgType;

  cpprofiler::Message msg;

  typedef char* iter;

  static void serializeType(std::vector<char>& data, MsgType f);
  static void serializeField(std::vector<char>& data, Field f);
  static void serialize(std::vector<char>& data, int32_t i);
  static void serialize(std::vector<char>& data, NodeStatus s);
  static void serialize(std::vector<char>& data, const std::string& s);

  static MsgType deserializeMsgType(iter& i);
  static Field deserializeField(iter& i);
  static int32_t deserializeInt(iter& i);
  static NodeStatus deserializeStatus(iter& i);
  static std::string deserializeString(iter& i);

 public:
  cpprofiler::Message& makeNode(int32_t id, int32_t pid, int32_t alt,
                                int32_t kids, NodeStatus status);

  void makeStart(int rid, const std::string& label, const std::string& info);
  void makeDone();

  const cpprofiler::Message& get_msg() { return msg; }

  std::vector<char> serialize() const;
  void deserialize(char* data, size_t size);
};

#endif  // MESSAGE_HH
