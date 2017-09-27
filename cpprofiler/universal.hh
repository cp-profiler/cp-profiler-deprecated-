#pragma once

#include <sstream>
#include "cpprofiler/utils/string_utils.hh"

struct NodeUID {
    int32_t nid;
    int32_t rid;
    int32_t tid;
};

inline std::string to_string(const NodeUID& uid) {
    std::stringstream ss;
    ss << "{" << uid.nid << ", " << uid.rid << ", " << uid.tid << "}";
    return ss.str();
}

inline NodeUID string_to_NodeUID(const std::string& str) {
    auto elements = utils::split(str.substr(1,str.size()-2), ',');
    return NodeUID {
        stoi(elements[0]),
        stoi(elements[1]),
        stoi(elements[2])
    };
}

inline bool operator==(const NodeUID& lhs, const NodeUID& rhs) {
    return (lhs.nid == rhs.nid) && (lhs.tid == rhs.tid) && (lhs.rid == rhs.rid);
}

inline bool operator<(const NodeUID& lhs, const NodeUID& rhs) {
    if (lhs.rid < rhs.rid) return true;
    if (lhs.rid == rhs.rid) {
        if (lhs.nid < rhs.nid) return true;
        if (lhs.nid == rhs.nid) return (lhs.tid < rhs.tid);
    }

    return false;
}

struct NodeUIDHash
{
    std::size_t operator()(NodeUID const& a) const 
    {
        size_t const h1 ( std::hash<int>{}(a.nid) );
        size_t const h2 ( std::hash<int>{}(a.tid) );
        size_t const h3 ( std::hash<int>{}(a.rid) );
        return h1 ^ (h2 << 1) ^ (h3 << 1); // or use boost::hash_combine (see Discussion)
    }
};

namespace std
{
    template<> struct hash<NodeUID>
    {
        size_t operator()(NodeUID const& a) const
        {
            size_t const h1 ( std::hash<int>{}(a.nid) );
            size_t const h2 ( std::hash<int>{}(a.tid) );
            size_t const h3 ( std::hash<int>{}(a.rid) );
            return h1 ^ (h2 << 1) ^ (h3 << 1); // or use boost::hash_combine (see Discussion)
        }
    };
}
