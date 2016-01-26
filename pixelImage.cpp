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
#include <chrono>
#include <iostream>

using namespace std::chrono;

PixelImage::PixelImage()
  : image_(nullptr), width_(0), height_(0) {

}

void
PixelImage::resize(int width, int height) {

  buffer_.clear();
  background_buffer_.clear();
  guidlines_buffer_.clear();

  buffer_.resize(width * height);
  background_buffer_.resize(width * height);
  guidlines_buffer_.resize(width * height);

  width_ = width;
  height_ = height;

  drawGrid();

  clear();

}

void
PixelImage::clear() {

  std::fill(buffer_.begin(), buffer_.end(), 0xFFFFFF);
  std::fill(guidlines_buffer_.begin(), guidlines_buffer_.end(), 0xFFFFFF);

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
PixelImage::setPixel(std::vector<uint32>& buffer, int x, int y, QRgb color) {

    uint32 r = qRed(color);
    uint32 g = qGreen(color);
    uint32 b = qBlue(color);

    // uint32 c = color;

    uint32 pixel_color = (0xFF << 24) + (r << 16) + (g << 8) + (b);

    buffer[y * width_ + x] = pixel_color;
}

static void addLayer(std::vector<uint32>& target_buf,
               const std::vector<uint32>& source_buf)
{
  for (int i = 0; i < source_buf.size(); ++i) {
    if (source_buf[i] != 0xFFFFFF) {
      target_buf[i] = source_buf[i];
    }
  }
}

void
PixelImage::update() {
  auto time_begin = high_resolution_clock::now();

  if (image_ != nullptr) delete image_;

  result_buffer_ = background_buffer_;

  addLayer(result_buffer_, buffer_);
  addLayer(result_buffer_, guidlines_buffer_);

  auto* buf = reinterpret_cast<unsigned char*>(&(result_buffer_[0]));

  image_ = new QImage(buf, width_, height_, QImage::Format_RGB32);

  auto time_end = high_resolution_clock::now();
  auto time_span = duration_cast<duration<double>>(time_end - time_begin);
  // std::cout << "updating pixel image takes: " << time_span.count() * 1000 << "ms\n";
}

void
PixelImage::drawHorizontalLine(int y, QRgb color) {

  drawHorizontalLine(buffer_, y, color);
}

void
PixelImage::drawHorizontalLine(std::vector<uint32>& buffer, int y, QRgb color) {

  y = y * scale_ + scale_;

  if (y < 0 || y > height_) return;

  for (auto x = 0; x < width_; x++) {
    setPixel(buffer, x, y, color);
  }
}

void
PixelImage::drawVerticalLine(std::vector<uint32>& buffer, int x, QRgb color) {

  x = x * scale_ + scale_;

  if (x < 0 || x > width_) return;

  for (auto y = 0; y < height_; y++) {
    setPixel(buffer, x, y, color);
  }
}

void
PixelImage::drawMouseGuidelines(unsigned x, unsigned y) {

  std::fill(guidlines_buffer_.begin(), guidlines_buffer_.end(), 0xFFFFFF);

  drawHorizontalLine(guidlines_buffer_, y - 1, PixelImage::PIXEL_COLOR::DARK_GRAY);
  drawHorizontalLine(guidlines_buffer_, y, PixelImage::PIXEL_COLOR::DARK_GRAY);
  drawVerticalLine(guidlines_buffer_, x - 1, PixelImage::PIXEL_COLOR::DARK_GRAY);
  drawVerticalLine(guidlines_buffer_, x, PixelImage::PIXEL_COLOR::DARK_GRAY);

}

/// TODO: drawGrid only when resized / rescaled
void
PixelImage::drawGrid() {

  std::fill(background_buffer_.begin(), background_buffer_.end(), 0xFFFFFF);

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