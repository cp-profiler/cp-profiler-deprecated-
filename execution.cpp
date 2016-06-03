#include "execution.hh"

void Execution::start(std::string label, bool isRestarts) {
    _data->setIsRestarts(isRestarts);

    std::time_t t = std::time(nullptr);
    string ts = std::asctime(std::localtime(&t));

    // asctime puts a newline at the end; remove it
    ts.pop_back();
    _data->setTitle(label + " (" + ts + ")");

    connect(this, SIGNAL(doneReceiving(void)), _data.get(), SLOT(setDoneReceiving(void)));
    connect(this, &Execution::doneReceiving, [this]() {
            _is_done = true;
            std::cerr << "execution " << this << " done receiving\n";
        });

    std::cerr << "Execution::start on " << this << "\n";

    builder = new TreeBuilder(this);

    connect(builder, &TreeBuilder::addedNode, this, &Execution::newNode);
    connect(builder, &TreeBuilder::addedRoot, this, &Execution::newRoot);

    builder->start();

    emit titleKnown();
}
