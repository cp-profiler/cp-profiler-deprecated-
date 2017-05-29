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

PixelImage::PIXEL_COLOR PixelImage::BLACK = qRgb(0, 0, 0);
PixelImage::PIXEL_COLOR PixelImage::YELLOW = qRgb(255, 255, 255);
PixelImage::PIXEL_COLOR PixelImage::DARK_GRAY = qRgb(150, 150, 150);
PixelImage::PIXEL_COLOR PixelImage::LIGTH_GRAY = qRgb(200, 2000, 200);
PixelImage::PIXEL_COLOR PixelImage::BLACK_ALPHA = qRgba(255, 0, 0, 50);
PixelImage::PIXEL_COLOR PixelImage::GRID = qRgb(230, 230, 230);
PixelImage::PIXEL_COLOR PixelImage::WHITE = qRgb(255, 255, 255);

PixelImage::PixelImage() : image_(nullptr), width_(0), height_(0) {}

void PixelImage::resize(int width, int height) {
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

void PixelImage::clear() {
  /// set all pixels to white
  std::fill(buffer_.begin(), buffer_.end(), 0xFFFFFF);
  std::fill(guidlines_buffer_.begin(), guidlines_buffer_.end(), 0xFFFFFF);
}

void PixelImage::drawPixel(int x, int y, QRgb color) {
  if (y < 0) return;  /// TODO: fix later

  // assert(x >= 0);

  int x0 = x * pixel_width_;
  int y0 = y * pixel_height_;

  /// TODO(maxim): experiment with using std::fill to draw rows
  for (int column = 0; column < pixel_width_; ++column) {
    auto x = x0 + column;
    if (x >= width_) break;
    for (int row = 0; row < pixel_height_; ++row) {
      auto y = y0 + row;
      if (y >= height_) break;
      setPixel(buffer_, x, y, color);
    }
  }
}

void PixelImage::drawRect(int x, int width, int y, QRgb color) {
  // assert (x >= 0 && y >= 0);
  if (x + width < 0 || y < 0) return;

  int x_begin = x * pixel_width_;
  int y_begin = y * pixel_height_;
  int x_end = (x + width) * pixel_width_;
  int y_end = (y + 1) * pixel_height_;

  /// horizontal lines
  for (auto column = x_begin; column < x_end; ++column) {
    auto x = column;
    auto y_lowest = y_begin;
    auto y_highest = y_end - 1;

    setPixel(buffer_, x, y_lowest, BLACK);
    setPixel(buffer_, x, y_highest, BLACK);
  }

  /// vertical lines
  for (auto row = y_begin; row < y_end; ++row) {
    auto y = row;
    auto x_leftmost = x_begin;
    auto x_rightmost = x_end - 1;
    // assert((int)x_end - 1 >= 0);

    setPixel(buffer_, x_leftmost, y, BLACK);
    setPixel(buffer_, x_rightmost, y, BLACK);
  }

  /// fill the rect

  for (auto column = x_begin + 1; column < x_end - 1; ++column) {
    for (auto row = y_begin + 1; row < y_end - 1; ++row) {
      setPixel(buffer_, column, row, color);
    }
  }
}

void PixelImage::setPixel(std::vector<uint32>& buffer, int x, int y,
                          QRgb color) {
  // assert(x >= 0 && y >= 0);

  if (x >= width_ || x < 0 || y >= height_ || y < 0) {
    return;
  }

  uint32 r = qRed(color);
  uint32 g = qGreen(color);
  uint32 b = qBlue(color);

  // uint32 c = color;

  uint32 pixel_color = (0xFF << 24) + (r << 16) + (g << 8) + (b);

  buffer[y * width_ + x] = pixel_color;
}

static void addLayer(std::vector<uint32>& target_buf,
                     const std::vector<uint32>& source_buf) {
  for (unsigned i = 0; i < source_buf.size(); ++i) {
    if (source_buf[i] != 0xFFFFFF) {
      target_buf[i] = source_buf[i];
    }
  }
}

void PixelImage::update() {

  if (image_ != nullptr) delete image_;

  result_buffer_ = background_buffer_;

  addLayer(result_buffer_, buffer_);
  addLayer(result_buffer_, guidlines_buffer_);

  auto* buf = reinterpret_cast<unsigned char*>(&(result_buffer_[0]));

  image_ = new QImage(buf, width_, height_, QImage::Format_RGB32);

}

void PixelImage::drawHorizontalLine(int y, QRgb color) {
  drawHorizontalLine(buffer_, y, color);
}

void PixelImage::drawHorizontalLine(std::vector<uint32>& buffer, int y,
                                    QRgb color) {
  y = (y + 1) * pixel_height_;

  if (y < 0 || y > height_) return;

  for (int x = 0; x < width_; ++x) {
    setPixel(buffer, x, y, color);
  }
}

void PixelImage::drawVerticalLine(std::vector<uint32>& buffer, int x,
                                  QRgb color) {
  // assert(x >= 0);

  x = (x + 1) * pixel_width_;

  if (x < 0 || x > width_) return;

  for (int y = 0; y < height_; ++y) {
    setPixel(buffer, x, y, color);
  }
}

void PixelImage::drawMouseGuidelines(uint32 x, uint32 y) {
  std::fill(guidlines_buffer_.begin(), guidlines_buffer_.end(), 0xFFFFFF);

  drawHorizontalLine(guidlines_buffer_, y - 1, DARK_GRAY);
  drawHorizontalLine(guidlines_buffer_, y, DARK_GRAY);
  drawVerticalLine(guidlines_buffer_, x - 1, DARK_GRAY);
  drawVerticalLine(guidlines_buffer_, x, DARK_GRAY);
}

/// TODO: drawGrid only when resized / rescaled
void PixelImage::drawGrid() {
  std::fill(background_buffer_.begin(), background_buffer_.end(), 0xFFFFFF);

  /// draw cells gap_size squares wide
  const int gap = 1;
  const int pixel_size = pixel_height_;   /// arbitrary decision
  const int gap_size = gap * pixel_size;  /// actual gap size in pixels

  /// horizontal lines on level == j
  for (int j = gap_size; j < height_; j += gap_size) {
    /// one line
    for (int i = 0; i < width_ - pixel_size; ++i) {
      for (int k = 0; k < pixel_size; ++k)
        setPixel(background_buffer_, i + k, j, PixelImage::GRID);
    }
  }

  /// vertical lines on column == i
  for (int i = gap_size; i < width_; i += gap_size) {
    /// one line
    for (int j = 0; j < height_ - pixel_size; ++j) {
      for (int k = 0; k < pixel_size; ++k)
        setPixel(background_buffer_, i, j + k, PixelImage::GRID);
    }
  }
}

void PixelImage::setPixelSize(int width, int height) {
  bool changed = false;

  if (width > 0) {
    pixel_width_ = width;
    changed = true;
  }

  if (height > 0) {
    pixel_height_ = height;
    changed = true;
  }

  if (changed) drawGrid();
}

void PixelImage::setPixelWidth(int width) {
  setPixelSize(width, pixel_height_);
}

void PixelImage::setPixelHeight(int height) {
  setPixelSize(pixel_width_, height);
}

void PixelImage::scalePixelBy(int value) {
  setPixelSize(pixel_width_ + value, pixel_height_ + value);
}

void PixelImage::scaleDown() { scalePixelBy(-1); }

void PixelImage::scaleUp() { scalePixelBy(1); }

const QImage* PixelImage::image() { return image_; }

PixelImage::~PixelImage() { delete image_; }
