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


class PixelImage {

private:

  std::vector<unsigned char> buffer_;
  std::vector<unsigned char> background_buffer_;
  // std::unique_ptr<QImage> image_;
  QImage* image_;

  unsigned int width_;
  unsigned int height_;

  QImage::Format image_format;

  unsigned step_ = 4; // size of a 'pixel' in pixels

  void setPixel(std::vector<unsigned char>& buffer, int x, int y, QRgb color);
  void drawHorizontalLine(std::vector<unsigned char>& buffer, int y);

public:
  PixelImage();
  // void drawPixel(int x, int y, unsigned int color);
  void drawPixel(int x, int y, QRgb color);

  // draw horizontal line (for zero level on histograms)
  // 1 pixel tall, on the bottom-most row of pixels: ______
  void drawHorizontalLine(int y);
  
  void drawMouseGuidelines(unsigned vline, unsigned depth);

  /// xoff and yoff account for scrolling
  void drawGrid();

  void resize(int width, int height);
  void scaleUp();
  void scaleDown();

  void update(); /// updates image_ from buffer_
  void clear(); /// fills the image with (white) color

  PixelImage(const PixelImage&) = delete;
  ~PixelImage();

  ///***** Getters *****
  unsigned int width() { return width_; }
  unsigned int height() { return height_; }

  const QImage* image();
  unsigned scale() const;

  enum PIXEL_COLOR {
    BLACK = qRgb(0, 0, 0),
    DARK_GRAY = qRgb(150, 150, 150),
    BLACK_ALPHA = qRgba(255, 0, 0, 50),
    GRID = qRgb(230, 230, 230),
    WHITE = qRgb(255, 255, 255)
  };

};

#endif