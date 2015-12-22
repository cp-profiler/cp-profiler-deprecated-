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
 *
 */

#include "pixelImage.hh"
#include <QDebug>

PixelImage::PixelImage()
  : image_(nullptr), width_(0), height_(0) {

}

void
PixelImage::resize(int width, int height) {

  buffer_.clear();
  background_buffer_.clear();

  buffer_.resize(4 * width * height);
  background_buffer_.resize(4 * width * height);

  width_ = width;
  height_ = height;

  drawGrid();

  clear();

}

void
PixelImage::clear() {

  std::fill(buffer_.begin(), buffer_.end(), 255);

  /// initialize buffer with background (grid)
  buffer_ = background_buffer_;
}

void
PixelImage::drawPixel(int x, int y, QRgb color) {
  if (y < 0)
    return; /// TODO: fix later

  int x0 = x * step_;
  int y0 = y * step_;

  for (unsigned i = 0; i < step_; i++) {
    auto x = x0 + i;
    if (x >= width_) break;
    for (unsigned j = 0; j < step_; j++) {
      auto y = y0 + j;
      if (y >= height_) break;
      setPixel(buffer_, x, y, color);
    }
  }
}

void
PixelImage::setPixel(std::vector<unsigned char>& buffer, int x, int y, QRgb color) {

    char r = qRed(color);
    char g = qGreen(color);
    char b = qBlue(color);

    buffer[4 * (y * width_ + x)    ] = r;
    buffer[4 * (y * width_ + x) + 1] = g;
    buffer[4 * (y * width_ + x) + 2] = b;
}

void
PixelImage::update() {
  if (image_ != nullptr) delete image_;
  auto data_ptr = &(buffer_[0]);



  image_ = new QImage(data_ptr, width_, height_, QImage::Format_RGB32);
}

void
PixelImage::drawHorizontalLine(int y) {

  drawHorizontalLine(buffer_, y);
}

void
PixelImage::drawHorizontalLine(std::vector<unsigned char>& buffer, int y) {

  y = y * step_ + step_;

  if (y < 0 || y > height_) return;

  for (auto x = 0; x < width_; x++) {
    setPixel(buffer_, x, y, PixelImage::PIXEL_COLOR::DARK_GRAY);
  }
}

void
PixelImage::drawMouseGuidelines(unsigned x, unsigned y) {
  /// TODO
}

/// TODO: drawGrid only when resized / rescaled
void
PixelImage::drawGrid() {

  std::fill(background_buffer_.begin(), background_buffer_.end(), 255);

  /// draw cells 5 squares wide
  int gap =  1;
  int gap_size = gap * step_; /// actual gap size in pixels

  /// horizontal lines on level == j
  for (unsigned int j = gap_size; j < height_; j += gap_size) {

    /// one line
    for (unsigned int i = 0; i < width_ - step_; i++) {

      for (unsigned k = 0; k < step_; k++)
        setPixel(background_buffer_, i + k, j, PixelImage::PIXEL_COLOR::GRID);
    }
  }

  /// vertical lines on column == i
  for (unsigned int i = gap_size; i < width_; i += gap_size) {

    /// one line
    for (unsigned int j = 0; j < height_ - step_; j++) {

      for (unsigned k = 0; k < step_; k++)
        setPixel(background_buffer_, i, j + k, PixelImage::PIXEL_COLOR::GRID);

    }
  }
}

void
PixelImage::scaleDown() {
  if (step_ <= 1) return;
  step_--;

  drawGrid();
}

void
PixelImage::scaleUp() {
  step_++;

  drawGrid();
}

const QImage*
PixelImage::image() {
  return image_;
}

unsigned
PixelImage::scale() const {
  return step_;
}

PixelImage::~PixelImage() {
  delete image_;
  // delete[] buffer_;
}