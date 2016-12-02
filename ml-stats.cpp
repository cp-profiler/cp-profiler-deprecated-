#include "ml-stats.hh"
#include "nodecursor.hh"
#include "nodevisitor.hh"
#include "data.hh"

#include <unordered_map>
#include <set>

// **************************************************
// StatsEntry
// **************************************************

using std::string;

class StatsEntry {
public:
    unsigned int nodeid;
    unsigned int gid;
    int parentid;
    NodeStatus status;
    int alternative;
    // int restartNumber;
    int depth;
    int decisionLevel;
    string label;
    int subtreeDepth;
    int subtreeSize;
    int subtreeSolutions;
    int nogoodStringLength;
    string nogoodString;
    int nogoodLength;
    int nogoodNumberVariables;
    int nogoodBLD;
    bool usesAssumptions;
    int backjumpDistance;
    int backjumpDestination;
    unsigned long long timestamp;
    string solutionString;
};

#define INCLUDE_NOGOOD_STRING 1

void printStatsHeader(std::ostream& out = std::cout) {
    out <<        "id"
        << "," << "gid"
        << "," << "parentId"
        << "," << "status"
        << "," << "alternative"
        // << "," << "restartNumber"
        << "," << "depth"
        << "," << "decisionLevel"
        << "," << "label"
        << "," << "subtreeDepth"
        << "," << "subtreeSize"
        << "," << "subtreeSolutions"
        << "," << "nogoodStringLength"
#if INCLUDE_NOGOOD_STRING
        << "," << "nogoodString"
#endif
        << "," << "nogoodLength"
        << "," << "nogoodNumberVariables"
        << "," << "nogoodBLD"
        << "," << "usesAssumptions"
        << "," << "backjumpDistance"
        << "," << "backjumpDestination"
        << "," << "timestamp"
        << "," << "solutionString"
        << "\n";
}

string
csvquote(const string& input) {
    std::stringstream out;
    out << '"';
    for (string::const_iterator it = input.begin() ; it != input.end() ; it++) {
        // CSV quoting is to replace " with "", e.g.
        // "one", "two", "I said, ""two,"" pay attention!"
        if (*it == '"')
            out << '"';
        out << *it;
    }
    out << '"';
    return out.str();
}

void printStatsEntry(const StatsEntry& se, std::ostream& out = std::cout) {
    out <<        se.nodeid
        << "," << se.gid
        << "," << se.parentid
        << "," << se.status
        << "," << se.alternative
        // << "," << se.restartNumber
        << "," << se.depth
        << "," << se.decisionLevel
        << "," << se.label
        << "," << se.subtreeDepth
        << "," << se.subtreeSize
        << "," << se.subtreeSolutions
        << "," << se.nogoodStringLength
#if INCLUDE_NOGOOD_STRING
        << "," << se.nogoodString
#endif
        << "," << se.nogoodLength
        << "," << se.nogoodNumberVariables
        << "," << se.nogoodBLD
        << "," << se.usesAssumptions
        << "," << se.backjumpDistance
        << "," << se.backjumpDestination
        << "," << se.timestamp
        << "," << csvquote(se.solutionString)
        << "\n";
}

// **************************************************
// StatsCursor
// **************************************************

class StatsCursor : public NodeCursor {
private:
    Execution* execution;
    int depth;
    std::vector<StatsEntry> stack;
    std::ostream& out;
public:
    StatsCursor(VisualNode* root, const NodeAllocator& na, Execution* execution_,
                std::ostream& out_)
        : NodeCursor(root, na)
        , execution(execution_)
        , depth(0)
        , out(out_)
    {
        printStatsHeader(out);
        enter();
    }

    int getNogoodStringLength(int sid) {
        std::unordered_map<int64_t, string>::const_iterator it = execution->getNogoods().find(sid);
        if (it != execution->getNogoods().end()) {
            return it->second.length();
        } else {
            return 0;
        }
    }

    string getNogoodString(int sid) {
        std::unordered_map<int64_t, string>::const_iterator it = execution->getNogoods().find(sid);
        if (it != execution->getNogoods().end()) {
            return it->second;
        } else {
            return "";
        }
    }

    string getSolutionString(int sid) {
        std::unordered_map<int64_t, string*>::const_iterator it = execution->getInfo().find(sid);
        if (it != execution->getInfo().end()) {
            return *it->second;
        } else {
            return "";
        }
    }

    int calculateNogoodLength(string nogood) {
        int count = 0;
        for (unsigned int i = 0 ; i < nogood.size() ; i++) {
            if (nogood[i] == ' ') {
                count++;
            }
        }
        return count;
    }

    int calculateNogoodNumberVariables(string nogood) {
        std::set<string> variables;
        int start = 0;
        while (true) {
            size_t space = nogood.find(' ', start);
            string literalSubstring = nogood.substr(start, space - start);
            if (literalSubstring.size() > 0) {
                size_t punctuation = literalSubstring.find_first_of("<>=!");
                if (punctuation != string::npos) {
                    string variableSubstring = literalSubstring.substr(0, punctuation);
                    variables.insert(variableSubstring);
                }
            }
            if (space == string::npos)
                break;
            start = space+1;
        }
        return variables.size();
    }

    // What to do when we see a node for the first time: create a
    // StatsEntry for it and add it to the stack.
    void enter() {
        StatsEntry se;
        se.depth = depth;
        se.subtreeDepth = 1;
        se.status = node()->getStatus();
        switch (se.status) {
        case SKIPPED:
        case UNDETERMINED:
            se.subtreeSize = 0;
            break;
        default:
            se.subtreeSize = 1;
            break;
        }
        se.subtreeSolutions = se.status == SOLVED ? 1 : 0;
        int gid = node()->getIndex(na);
        // Some nodes (e.g. undetermined nodes) do not have entries;
        // be careful with those.
        se.gid = gid;
        DbEntry* entry = execution->getEntry(gid);
        if (entry != nullptr) {
            unsigned int sid = entry->s_node_id;
            se.nodeid = sid;
            se.parentid = entry->parent_sid;
            se.alternative = entry->alt;
            // se.restartNumber = entry->restart_id;
            se.nogoodStringLength = getNogoodStringLength(sid);
            se.nogoodString = getNogoodString(sid);
            se.nogoodLength = calculateNogoodLength(se.nogoodString);
            se.nogoodNumberVariables = calculateNogoodNumberVariables(se.nogoodString);
            se.nogoodBLD = entry->nogood_bld;
            se.usesAssumptions = entry->usesAssumptions;
            se.backjumpDistance = entry->backjump_distance;
            se.label = entry->label;
            se.decisionLevel = entry->decision_level;
            se.timestamp = entry->time_stamp;
            se.solutionString = getSolutionString(sid);

            se.backjumpDestination = se.decisionLevel - se.backjumpDistance;
        } else {
            se.nodeid = -1;
            se.parentid = -1;
            se.alternative = -1;
            // se.restartNumber = -1;
            se.nogoodStringLength = 0;
            se.nogoodString = "";
            se.nogoodLength = 0;
            se.nogoodNumberVariables = 0;
            se.label = "";
            se.decisionLevel = -1;
            se.timestamp = 0;
            se.solutionString = "";
            se.backjumpDestination = -1;
            se.backjumpDistance = -1;
        }

        stack.push_back(se);
    }


    void moveDownwards() {
        NodeCursor::moveDownwards();
        depth++;

        enter();
    }

    // What to do when we see a node for the second (last) time: pop
    // its StatsEntry from the stack and update its parent's subtree
    // information.  This means that a node's subtree information is
    // correct when its last child is done.
    void processCurrentNode() {
        StatsEntry se = stack.back();
        stack.pop_back();
        // Undetermined nodes are not real nodes (the solver never
        // visited them), so we don't do anything with those.
        // Also let's ignore the "super root".
        if (se.status != UNDETERMINED && se.depth >= 1) {
            printStatsEntry(se, out);
            if (stack.size() > 0) {
                stack.back().subtreeDepth = std::max(stack.back().subtreeDepth, 1 + se.subtreeDepth);
                stack.back().subtreeSize += se.subtreeSize;
                stack.back().subtreeSolutions += se.subtreeSolutions;
            }
        }
    }

    void moveUpwards() {
        NodeCursor::moveUpwards();
        depth--;
    }

    void moveSidewards() {
        NodeCursor::moveSidewards();
        enter();
    }
};

// **************************************************
// Module interface
// **************************************************

// Collect the machine-learning statistics for a (sub)tree.  The first
// argument are the root of the subtree and the node-allocator for the
// tree, as are the usual arguments to a NodeCursor.  The third
// argument is the execution the subtree comes from, which is used to
// find the solver node id and branching/no-good information.
void collectMLStats(VisualNode* root, const NodeAllocator& na, Execution* execution, std::ostream& out) {
    StatsCursor c(root, na, execution, out);
    PostorderNodeVisitor<StatsCursor> v(c);
    v.run();
}
