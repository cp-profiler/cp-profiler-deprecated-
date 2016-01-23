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
#include <cassert>

PixelImage::PixelImage()
  : image_(nullptr), width_(0), height_(0) {

}

void
PixelImage::resize(int width, int height) {

  buffer_.clear();
  background_buffer_.clear();
  guidlines_buffer_.clear();

  buffer_.resize(4 * width * height);
  background_buffer_.resize(4 * width * height);
  guidlines_buffer_.resize(4 * width * height);

  width_ = width;
  height_ = height;

  drawGrid();

  clear();

}

void
PixelImage::clear() {

  std::fill(buffer_.begin(), buffer_.end(), 255);
  std::fill(guidlines_buffer_.begin(), guidlines_buffer_.end(), 255);

}

void
PixelImage::drawPixel(int x, int y, QRgb color) {
  if (y < 0)
    return; /// TODO: fix later

  int x0 = x * scale_;
  int y0 = y * scale_;

  for (unsigned i = 0; i < scale_; i++) {
    auto x = x0 + i;
    if (x >= width_) break;
    for (unsigned j = 0; j < scale_; j++) {
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

  result_buffer_ = background_buffer_;

  /// TODO(maxim): make this a function
  /// TODO(maxim): make this much faster!
  for (int i = 0; i < buffer_.size(); ++i) {
    if (buffer_[i] != 255) {
      result_buffer_[i] = buffer_[i];
    }
  }

  for (int i = 0; i < guidlines_buffer_.size(); ++i) {
    if (guidlines_buffer_[i] != 255) {
      result_buffer_[i] = guidlines_buffer_[i];
    }
  }

  image_ = new QImage(&(result_buffer_[0]), width_, height_, QImage::Format_RGB32);
}

void
PixelImage::drawHorizontalLine(int y, QRgb color) {

  drawHorizontalLine(buffer_, y, color);
}

void
PixelImage::drawHorizontalLine(std::vector<unsigned char>& buffer, int y, QRgb color) {

  y = y * scale_ + scale_;

  if (y < 0 || y > height_) return;

  for (auto x = 0; x < width_; x++) {
    setPixel(buffer, x, y, color);
  }
}

void
PixelImage::drawVerticalLine(std::vector<unsigned char>& buffer, int x, QRgb color) {

  x = x * scale_ + scale_;

  if (x < 0 || x > width_) return;

  for (auto y = 0; y < height_; y++) {
    setPixel(buffer, x, y, color);
  }
}

void
PixelImage::drawMouseGuidelines(unsigned x, unsigned y) {

  std::fill(guidlines_buffer_.begin(), guidlines_buffer_.end(), 255);

  drawHorizontalLine(guidlines_buffer_, y - 1, PixelImage::PIXEL_COLOR::DARK_GRAY);
  drawHorizontalLine(guidlines_buffer_, y, PixelImage::PIXEL_COLOR::DARK_GRAY);
  drawVerticalLine(guidlines_buffer_, x - 1, PixelImage::PIXEL_COLOR::DARK_GRAY);
  drawVerticalLine(guidlines_buffer_, x, PixelImage::PIXEL_COLOR::DARK_GRAY);

}

/// TODO: drawGrid only when resized / rescaled
void
PixelImage::drawGrid() {

  std::fill(background_buffer_.begin(), background_buffer_.end(), 255);

  /// draw cells 5 squares wide
  int gap =  1;
  int gap_size = gap * scale_; /// actual gap size in pixels

  /// horizontal lines on level == j
  for (unsigned int j = gap_size; j < height_; j += gap_size) {

    /// one line
    for (unsigned int i = 0; i < width_ - scale_; i++) {

      for (unsigned k = 0; k < scale_; k++)
        setPixel(background_buffer_, i + k, j, PixelImage::PIXEL_COLOR::GRID);
    }
  }

  /// vertical lines on column == i
  for (unsigned int i = gap_size; i < width_; i += gap_size) {

    /// one line
    for (unsigned int j = 0; j < height_ - scale_; j++) {

      for (unsigned k = 0; k < scale_; k++)
        setPixel(background_buffer_, i, j + k, PixelImage::PIXEL_COLOR::GRID);

    }
  }
}

void
PixelImage::scaleDown() {
  if (scale_ <= 1) return;
  scale_--;

  drawGrid();
}

void
PixelImage::scaleUp() {
  scale_++;

  drawGrid();
}

const QImage*
PixelImage::image() {
  return image_;
}


PixelImage::~PixelImage() {
  delete image_;
}