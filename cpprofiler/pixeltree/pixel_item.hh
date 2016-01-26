/*  Permission is hereby granted, free of charge, to any person obtaining
 *  a copy of this software and associated documentation files (the
 *  "Software"), to deal in the Software without restriction, including
 *  without limitation the rights to use, copy, modify, merge, publish,
 *  distribute, sublicense, and/or sell copies of the Software, and to
 *  permit persons to whom the Software is furnished to do so, subject to
 *  the following conditions:
 *
 *  The above copyright notice and this permission notice shall be
 *  included in all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 *  LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 *  OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *  WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef CPPROFILER_PIXELTREE_PIXELITEM_HH
#define CPPROFILER_PIXELTREE_PIXELITEM_HH

namespace cpprofiler { namespace pixeltree {

  class PixelItem {
  private:
    int   _idx; /// TODO(maxim): find out what this is for (not gid)
    int   _depth;
    VisualNode* _node;
    bool  _selected;

  public:
    PixelItem(int idx, VisualNode* node, int depth)
    : _idx(idx), _depth(depth), _node(node), _selected(false) {};

    inline int idx() const { return _idx; }

    inline int gid(const NodeAllocator& na) const {
      return _node->getIndex(na);
    }

    inline int depth() const { return _depth; }

    inline VisualNode* node() { return _node; }

    inline bool isSelected() { return _selected; }

    inline void setSelected(bool value) {
      _node->setSelected(value);
      _selected = value;
    }
  };

}}

#endif