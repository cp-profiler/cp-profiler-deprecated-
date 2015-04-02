#include "treedialog.hh"

class Gist;

class CmpTreeDialog : public TreeDialog {

public:

  CmpTreeDialog(ReceiverThread* receiver, const TreeCanvas::CanvasType type, Gist* gist);

};
