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
    NodeStatus status;
    int depth;
    int subtreeDepth;
    int subtreeSize;
    int subtreeSolutions;
    int nogoodStringLength;
    int backjumpDistance;
};

void printStatsEntry(const StatsEntry& se) {
    std::cout << "StatsEntry:"
              << "\t" << se.nodeid
              << "\t" << se.status
              << "\t" << se.depth
              << "\t" << se.subtreeDepth
              << "\t" << se.subtreeSize
              << "\t" << se.subtreeSolutions
              << "\t" << se.nogoodStringLength
              << "\t" << se.backjumpDistance
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

public:
    StatsCursor(VisualNode* root, const VisualNode::NodeAllocator& na, Execution* execution_,
                const std::unordered_map<VisualNode*, int>& backjumpDistanceMap_)
        : NodeCursor(root, na)
        , execution(execution_)
        , depth(0)
        , backjumpDistanceMap(backjumpDistanceMap_) {

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
        unsigned int sid = execution->getEntry(gid)->sid;
        se.nodeid = sid;
        se.nogoodStringLength = getNogoodStringLength(sid);

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
        std::cout << "popped this: ";
        stack.pop_back();
        printStatsEntry(se);
        if (stack.size() > 0) {
            stack.back().subtreeDepth = std::max(stack.back().subtreeDepth, 1 + se.subtreeDepth);
            stack.back().subtreeSize += se.subtreeSize;
            stack.back().subtreeSolutions += se.subtreeSolutions;
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
    int depth;
    std::unordered_map<VisualNode*, int>& backjumpDistance;
    VisualNode* mostRecentFailure;
    int mostRecentFailureDepth;
public:
    // Note that the map is passed in by reference so we can modify
    // it.
    BackjumpCursor(VisualNode* root,
                   const VisualNode::NodeAllocator& na,
                   std::unordered_map<VisualNode*, int>& bjd)
        : NodeCursor(root, na)
        , depth(0)
        , backjumpDistance(bjd)
    {}
    
    void moveDownwards() { NodeCursor<VisualNode>::moveDownwards(); depth++; }
    void moveUpwards()   { NodeCursor<VisualNode>::moveUpwards();   depth--; }

    void processCurrentNode() {
        // Note that skipped/undetermined nodes do not do anything;
        // they are not the destination of a backjump.
        switch (node()->getStatus()) {
        case FAILED:
            if (mostRecentFailure) {
                backjumpDistance[mostRecentFailure] = mostRecentFailureDepth - depth;
                mostRecentFailure = NULL;
            }
            mostRecentFailure = node();
            mostRecentFailureDepth = depth;
            break;
        case BRANCH:
        case SOLVED:
            if (mostRecentFailure) {
                backjumpDistance[mostRecentFailure] = mostRecentFailureDepth - depth;
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
void collectMLStats(VisualNode* root, const VisualNode::NodeAllocator& na, Execution* execution) {
    // We traverse the tree twice.  In the first pass we calculate
    // backjump distance for failed nodes, using a BackjumpCursor.  In
    // the second pass we calculate all the remaining node data.

    // This map is the only thing shared between the passes.  Note
    // that we pass it by reference to the BackjumpCursor, which
    // writes to it.
    std::unordered_map<VisualNode*, int> backjumpDistance;

    // First pass: construct the backjumpDistance map.
    BackjumpCursor bjc(root, na, backjumpDistance);
    PreorderNodeVisitor<BackjumpCursor> bjv(bjc);
    bjv.run();

    // Second pass: compute all the node statistics.
    StatsCursor c(root, na, execution, backjumpDistance);
    PostorderNodeVisitor<StatsCursor> v(c);
    v.run();
}
