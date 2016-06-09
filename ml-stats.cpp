#include "ml-stats.hh"
#include "nodecursor.hh"
#include "nodevisitor.hh"

#include <unordered_map>

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

#define INCLUDE_NOGOOD_STRING 0

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

class StatsCursor : public NodeCursor<VisualNode> {
private:
    Execution* execution;
    int depth;
    std::vector<StatsEntry> stack;
    const std::unordered_map<VisualNode*, int>& backjumpDestinationMap;
    std::ostream& out;
public:
    StatsCursor(VisualNode* root, const NodeAllocator& na, Execution* execution_,
                const std::unordered_map<VisualNode*, int>& backjumpDestinationMap_,
                std::ostream& out_)
        : NodeCursor(root, na)
        , execution(execution_)
        , depth(0)
        , backjumpDestinationMap(backjumpDestinationMap_)
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
        }
        std::unordered_map<VisualNode*, int>::const_iterator it2 = backjumpDestinationMap.find(node());
        if (it2 != backjumpDestinationMap.end()) {
            //            se.backjumpDestination = it2->second;
            if (se.backjumpDistance != se.decisionLevel - it2->second) {
                std::cerr << "BACKJUMP DISTANCE DISCREPANCY\n";
                printStatsEntry(se, std::cerr);
            }
            // se.backjumpDistance = se.decisionLevel - se.backjumpDestination;
        } else {
            se.backjumpDestination = -1;
            se.backjumpDistance = -1;
        }

        stack.push_back(se);
    }


    void moveDownwards() {
        NodeCursor<VisualNode>::moveDownwards();
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
        NodeCursor<VisualNode>::moveUpwards();
        depth--;
    }

    void moveSidewards() {
        NodeCursor<VisualNode>::moveSidewards();
        enter();
    }
};

// **************************************************
// BackjumpCursor
// **************************************************

// The BackjumpCursor fills in a map from node to backjump
// destination.  Non-failure nodes are omitted.  The final failed node
// is also omitted, because there is no following node that would be
// its destination.  (Decision level 0 could possibly be considered
// the destination instead.)

class BackjumpCursor : public NodeCursor<VisualNode> {
private:
    const Execution* execution;
    std::unordered_map<VisualNode*, int>& backjumpDestination;
    VisualNode* mostRecentFailure;
    int mostRecentFailureDecisionLevel;
public:
    // Note that the map is passed in by reference so we can modify
    // it.
    BackjumpCursor(VisualNode* root,
                   const NodeAllocator& na,
                   const Execution* execution_,
                   std::unordered_map<VisualNode*, int>& bjdest)
        : NodeCursor(root, na)
        , execution(execution_)
        , backjumpDestination(bjdest)
    {}

    void processCurrentNode() {
        // Note that skipped/undetermined nodes do not do anything;
        // they are not the destination of a backjump.
        int gid;
        int decisionLevel;
        switch (node()->getStatus()) {
        case FAILED:
            gid = node()->getIndex(na);
            decisionLevel = execution->getEntry(gid)->decision_level;
            if (mostRecentFailure) {
                backjumpDestination[mostRecentFailure] = decisionLevel;
                mostRecentFailure = nullptr;
            }
            mostRecentFailure = node();
            mostRecentFailureDecisionLevel = decisionLevel;
            break;
        case BRANCH:
        case SOLVED:
            if (mostRecentFailure) {
                gid = node()->getIndex(na);
                decisionLevel = execution->getEntry(gid)->decision_level;
                backjumpDestination[mostRecentFailure] = decisionLevel;
                mostRecentFailure = nullptr;
            }
            break;
        default:
            break;
        }
    }
};

// **************************************************
// Module interface
// **************************************************

// TODO: This can be simplified.  We now expect the solver to send the
// backjump distance with a failure, so we don't need to calculate it
// any more.  The way we did it was wrong anyway; it didn't account
// for restarts, so these appeared as very long backjumps.

// Collect the machine-learning statistics for a (sub)tree.  The first
// argument are the root of the subtree and the node-allocator for the
// tree, as are the usual arguments to a NodeCursor.  The third
// argument is the execution the subtree comes from, which is used to
// find the solver node id and branching/no-good information.
void collectMLStats(VisualNode* root, const NodeAllocator& na, Execution* execution, std::ostream& out) {
    // We traverse the tree twice.  In the first pass we calculate
    // backjump destination for failed nodes, using a BackjumpCursor.
    // In the second pass we calculate all the remaining node data.

    // This map is the only thing shared between the passes.  Note
    // that we pass it by reference to the BackjumpCursor, which
    // writes to it.
    std::unordered_map<VisualNode*, int> backjumpDestination;

    // TODO: remove this pass, and the backjump destination map
    // altogether

    // First pass: construct the backjumpDestination map.
    BackjumpCursor bjc(root, na, execution, backjumpDestination);
    PreorderNodeVisitor<BackjumpCursor> bjv(bjc);
    bjv.run();

    // Second pass: compute all the node statistics.
    StatsCursor c(root, na, execution, backjumpDestination, out);
    PostorderNodeVisitor<StatsCursor> v(c);
    v.run();
}
