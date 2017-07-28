#include <iostream>
#include <iterator>
#include <algorithm>

class SplittableGroups;

class Group {

using T = ChildInfo;

public:
    SplittableGroups* container;
    int m_start;
    int m_end;
    int splitter;

    Group(SplittableGroups* sg, int start, int end)
    : container{sg}, m_start{start}, m_end{end}, splitter{0} {}

    // friend std::ostream& operator<<(std::ostream& o, const Group* g) {
    //     o << "[" << g->m_start << ", " << g->m_end << "): " << g->splitter;
    //     return o;
    // }

#ifdef MAXIM_DEBUG
    friend std::ostream& operator<<(std::ostream& o, const Group* g) {

        o << "[";
        for (auto i = g->m_start; i < g->m_end; ++i) {
            o << g->at(i - g->m_start).node->debug_id << " ";
        }
        o << "]";
        return o;
    }
#endif

    int size() {
        return m_end - m_start;
    }

    void increment_splitter() {
        assert(splitter < m_end - m_start);
        ++splitter;
    }

    T& at(int idx);
    const T& at(int idx) const;
    ChildInfo* begin();
    ChildInfo* end();
};

/// does not grow in size
/// uses contiguous memory
class SplittableGroups {

    friend class Group;

using T = ChildInfo;

    std::unordered_map<Group*, int> group2idx;
    std::vector<Group*> groups;
    std::vector<T> data;

public:


    SplittableGroups(const std::vector<std::vector<T>>& vecs) {

        /// TODO(maxim): make this unnecessary
        groups.reserve(10000000);
        /// get total size
        auto counter = 0;
        for (auto& v : vecs) {
            auto start = counter;
            for (auto& e : v) {
                data.push_back(e);
                ++counter;
            }

            auto new_g = new Group{this, start, counter}; 

            groups.push_back(new_g);
            group2idx[new_g] = groups.size() - 1;
        }
    }

    SplittableGroups(const SplittableGroups& other) = delete;

    Group* get_group(int g_idx) {
        assert((int)groups.size() > g_idx);
        return groups[g_idx];
    }

    Group* split_group(Group* old_g) {

        auto new_g = new Group{this, old_g->splitter + old_g->m_start, old_g->m_end};
        old_g->m_end = new_g->m_start;
        old_g->splitter = 0;

        auto g_idx = group2idx[old_g];
        // group2idx[new_g] = g_idx + 1;

        auto it_insert = std::begin(groups); std::advance(it_insert, g_idx + 1);
        groups.insert(it_insert, new_g);

        auto tmp_g_idx = g_idx + 1;
        for (auto it = it_insert; it < std::end(groups); ++it) {
            group2idx[*it] = tmp_g_idx;
            ++tmp_g_idx;
        }

        // groups.push_back(new_g);
        /// NOTE(maxim): for now use a stupid search
        // auto it = std::find(std::begin(groups), std::end(groups), old_g);
        // std::advance(it, 1);


        return new_g;
    }
#ifdef MAXIM_DEBUG
    friend std::ostream& operator<<(std::ostream& o, const SplittableGroups& sv) {
        for (const auto& ci : sv.data) {
            o << ci.node->debug_id << " ";
        }
        return o;
    }
#endif

#ifdef MAXIM_DEBUG
    friend void print(const SplittableGroups& sg) {

        for (const auto& ci : sg.data) {
            std::cout << ci.node->debug_id << " ";
        }
        std::cout << "\n";
    }
#endif

    friend void print_groups(const SplittableGroups& sg) {

        for (const auto& g : sg.groups) {
            std::cout << g << " ";
        }
        std::cout << "\n";
    }

    auto begin() -> decltype(groups.begin()) {
        return groups.begin();
    }

    auto end() -> decltype(groups.end())  {
        return groups.end();
    }

    Group* operator[](int idx) {
        return groups[idx];
    }

    auto size() -> decltype(groups.size()) {
        return groups.size();
    }

};

ChildInfo& Group::at(int idx) {
    assert(idx < m_end - m_start);
    return container->data[m_start + idx];
}

const ChildInfo& Group::at(int idx) const {
    assert(idx < m_end - m_start);
    return container->data[m_start + idx];
}

ChildInfo* Group::begin() {
    return &container->data[m_start];
}

ChildInfo* Group::end() {
    return &container->data[m_end - 1] + 1;
}