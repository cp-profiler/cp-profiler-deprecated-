#include <unordered_map>

#include "ml-stats.hh"
#include "nodecursor.hh"
#include "nodevisitor.hh"

// **************************************************
// StatsEntry
// **************************************************

class StatsEntry {
public:
    unsigned int nodeid;
    int parentid;
    NodeStatus status;
    int depth;
    int decisionLevel;
    string label;
    int subtreeDepth;
    int subtreeSize;
    int subtreeSolutions;
    int nogoodStringLength;
    string nogoodString;
    int backjumpDistance;
    unsigned long long timestamp;
};

void printStatsHeader(std::ostream& out = std::cout) {
    out <<        "id"
        << "," << "parentId"
        << "," << "status"
        << "," << "depth"
        << "," << "decisionLevel"
        << "," << "label"
        << "," << "subtreeDepth"
        << "," << "subtreeSize"
        << "," << "subtreeSolutions"
        << "," << "nogoodStringLength"
        << "," << "nogoodString"
        << "," << "backjumpDistance"
        << "," << "timestamp"
              << "\n";
}

void printStatsEntry(const StatsEntry& se, std::ostream& out = std::cout) {
    out <<        se.nodeid
        << "," << se.parentid
        << "," << se.status
        << "," << se.depth
        << "," << se.decisionLevel
        << "," << se.label
        << "," << se.subtreeDepth
        << "," << se.subtreeSize
        << "," << se.subtreeSolutions
        << "," << se.nogoodStringLength
        << "," << se.nogoodString
        << "," << se.backjumpDistance
        << "," << se.timestamp
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
    const std::unordered_map<VisualNode*, int>& backjumpDistanceMap;
    std::ostream& out;
public:
    StatsCursor(VisualNode* root, const VisualNode::NodeAllocator& na, Execution* execution_,
                const std::unordered_map<VisualNode*, int>& backjumpDistanceMap_,
                std::ostream& out_)
        : NodeCursor(root, na)
        , execution(execution_)
        , depth(0)
        , backjumpDistanceMap(backjumpDistanceMap_)
        , out(out_)
    {
        printStatsHeader(out);
        enter();
    }

    int getNogoodStringLength(int sid) {
        std::unordered_map<unsigned long long, string>::const_iterator it = execution->getNogoods().find(sid);
        if (it != execution->getNogoods().end()) {
            return it->second.length();
        } else {
            return 0;
        }
    }

    string getNogoodString(int sid) {
        std::unordered_map<unsigned long long, string>::const_iterator it = execution->getNogoods().find(sid);
        if (it != execution->getNogoods().end()) {
            return it->second;
        } else {
            return "";
        }
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
        std::unordered_map<VisualNode*, int>::const_iterator it = backjumpDistanceMap.find(node());
        if (it != backjumpDistanceMap.end()) {
            se.backjumpDistance = it->second;
        } else {
            se.backjumpDistance = -1;
        }
        int gid = node()->getIndex(na);
        // Some nodes (e.g. undetermined nodes) do not have entries;
        // be careful with those.
        DbEntry* entry = execution->getEntry(gid);
        if (entry != nullptr) {
            unsigned int sid = entry->sid;
            se.nodeid = sid;
            se.parentid = entry->parent_sid;
            se.nogoodStringLength = getNogoodStringLength(sid);
            se.nogoodString = getNogoodString(sid);
            se.label = entry->label;
            se.decisionLevel = entry->decisionLevel;
            se.timestamp = entry->time_stamp;
        } else {
            se.nodeid = -1;
            se.parentid = -1;
            se.nogoodStringLength = 0;
            se.nogoodString = "";
            se.label = "";
            se.decisionLevel = -1;
            se.timestamp = 0;
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
        if (se.status != UNDETERMINED) {
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

// The BackjumpCursor fills in a map from node to backjump distance.
// Non-failure nodes are omitted.  The final failed node is also
// omitted, because there is no following node to compute its distance
// to.  (It could be assigned the distance to the root instead.)

class BackjumpCursor : public NodeCursor<VisualNode> {
private:
    const Execution* execution;
    std::unordered_map<VisualNode*, int>& backjumpDistance;
    VisualNode* mostRecentFailure;
    int mostRecentFailureDecisionLevel;
public:
    // Note that the map is passed in by reference so we can modify
    // it.
    BackjumpCursor(VisualNode* root,
                   const VisualNode::NodeAllocator& na,
                   const Execution* execution_,
                   std::unordered_map<VisualNode*, int>& bjd)
        : NodeCursor(root, na)
        , execution(execution_)
        , backjumpDistance(bjd)
    {}
    
    void processCurrentNode() {
        // Note that skipped/undetermined nodes do not do anything;
        // they are not the destination of a backjump.
        int gid;
        int decisionLevel;
        switch (node()->getStatus()) {
        case FAILED:
            gid = node()->getIndex(na);
            decisionLevel = execution->getEntry(gid)->decisionLevel;
            if (mostRecentFailure) {
                backjumpDistance[mostRecentFailure] =
                    mostRecentFailureDecisionLevel - decisionLevel;
                mostRecentFailure = NULL;
            }
            mostRecentFailure = node();
            mostRecentFailureDecisionLevel = decisionLevel;
            break;
        case BRANCH:
        case SOLVED:
            if (mostRecentFailure) {
                gid = node()->getIndex(na);
                decisionLevel = execution->getEntry(gid)->decisionLevel;
                backjumpDistance[mostRecentFailure] =
                    mostRecentFailureDecisionLevel - decisionLevel;
                mostRecentFailure = NULL;
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

// Collect the machine-learning statistics for a (sub)tree.  The first
// argument are the root of the subtree and the node-allocator for the
// tree, as are the usual arguments to a NodeCursor.  The third
// argument is the execution the subtree comes from, which is used to
// find the solver node id and branching/no-good information.
void collectMLStats(VisualNode* root, const VisualNode::NodeAllocator& na, Execution* execution, std::ostream& out) {
    // We traverse the tree twice.  In the first pass we calculate
    // backjump distance for failed nodes, using a BackjumpCursor.  In
    // the second pass we calculate all the remaining node data.

    // This map is the only thing shared between the passes.  Note
    // that we pass it by reference to the BackjumpCursor, which
    // writes to it.
    std::unordered_map<VisualNode*, int> backjumpDistance;

    // First pass: construct the backjumpDistance map.
    BackjumpCursor bjc(root, na, execution, backjumpDistance);
    PreorderNodeVisitor<BackjumpCursor> bjv(bjc);
    bjv.run();

    // Second pass: compute all the node statistics.
    StatsCursor c(root, na, execution, backjumpDistance, out);
    PostorderNodeVisitor<StatsCursor> v(c);
    v.run();
}
