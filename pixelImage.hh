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

#ifndef PIXEL_IMAGE_HH
#define PIXEL_IMAGE_HH

#include <QImage>
#include <memory>
#include <vector>
#include <cstdint>

typedef uint32_t uint32;

class PixelImage {

private:
  /// TODO(maxim): better name
  std::vector<uint32> buffer1_;
  std::vector<uint32> buffer_;
  std::vector<uint32> background_buffer_;
  std::vector<uint32> guidlines_buffer_;
  // std::unique_ptr<QImage> image_;

  std::vector<uint32> result_buffer_;
  QImage* image_;

  uint32 width_;
  uint32 height_;

  // int content_height_; // the sum of heights of the pt and histograms

  QImage::Format image_format;

  unsigned scale_ = 4; // size of a 'pixel' in pixels

  void setPixel(std::vector<uint32>& buffer, int x, int y, QRgb color);
  void drawHorizontalLine(std::vector<uint32>& buffer, int y, QRgb color);
  void drawVerticalLine(std::vector<uint32>& buffer, int x, QRgb color);

public:
  PixelImage();
  // void drawPixel(int x, int y, unsigned int color);
  void drawPixel(int x, int y, QRgb color);

  void drawRect(int x, int width, int y, QRgb color);

  // draw horizontal line (for zero level on histograms)
  // 1 pixel tall, on the bottom-most row of pixels: ______
  void drawHorizontalLine(int y, QRgb color);
  
  void drawMouseGuidelines(unsigned vline, unsigned depth);

  /// xoff and yoff account for scrolling
  void drawGrid();

  void resize(int width, int height);
  void scaleUp();
  void scaleDown();

  void update(); /// updates image_ from buffer_
  void clear(); /// fills the image with (white) color

  // void resetContentHeight() { content_height_ = 0; }
  // void addToContentHeight(int fake_pixels) { content_height_ += fake_pixels }

  PixelImage(const PixelImage&) = delete;
  ~PixelImage();

  ///***** Getters *****
  uint32 width_in_pixels() { return width_; }
  uint32 width() { return width_ / scale_; }
  uint32 height_in_pixels() { return height_; }
  uint32 height() { return height_ / scale_; }

  const QImage* image();
  uint32 scale() const { return scale_; };

  enum PIXEL_COLOR {
    BLACK = qRgb(0, 0, 0),
    // YELLOW = qRgb(255, 255, 255),
    DARK_GRAY = qRgb(150, 150, 150),
    LIGTH_GRAY = qRgb(200, 2000, 200),
    BLACK_ALPHA = qRgba(255, 0, 0, 50),
    GRID = qRgb(230, 230, 230),
    WHITE = qRgb(255, 255, 255)
  };

};

#endif