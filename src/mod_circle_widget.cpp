#include "mod_circle_widget.h"
#include "rsa.h"

#include <QPainter>
#include <QPen>
#include <QPointF>
#include <QVector>

#include <algorithm>
#include <cmath>
#include <random>

ModCircleWidget::ModCircleWidget(QWidget *parent) : QWidget(parent) {
  setMinimumSize(400, 400);
}

void ModCircleWidget::set_params(const cpp_int &n, const cpp_int &e) {
  if (n <= 0) {
    return;
  }

  mod = n;
  exp = e;
  update();
}

size_t bit_length(const cpp_int &n) {
  if (n == 0)
    return 1;

  cpp_int x = n;
  size_t bits = 0;

  while (x > 0) {
    x >>= 1;
    ++bits;
  }
  return bits;
}

cpp_int random_cpp_int_uniform(const cpp_int &max_inclusive,
                               std::mt19937_64 &rng) {
  if (max_inclusive < 0) {
    throw std::runtime_error("max_inclusive must be non-negative");
  }
  const size_t bits = bit_length(max_inclusive);

  std::uniform_int_distribution<uint64_t> dist(0, UINT64_MAX);

  while (true) {
    cpp_int x = 0;

    size_t full_words = bits / 64;
    size_t rem_bits = bits % 64;

    for (size_t i = 0; i < full_words; ++i) {
      x <<= 64;
      x |= cpp_int(dist(rng));
    }

    if (rem_bits > 0) {
      uint64_t last = dist(rng);

      if (rem_bits < 64) {
        last &= ((uint64_t(1) << rem_bits) - 1);
      }

      x <<= rem_bits;
      x |= cpp_int(last);
    }

    if (x <= max_inclusive) {
      return x;
    }
  }
}

QVector<cpp_int> random_unique_points(const cpp_int &long_num, int point_num) {
  if (long_num <= 0) {
    throw std::runtime_error("long_num must be positive");
  }
  if (point_num < 0) {
    throw std::runtime_error("point_num must be non-negative");
  }
  if (cpp_int(point_num) > long_num) {
    throw std::runtime_error("point_num cannot exceed long_num");
  }

  std::random_device rd;
  std::mt19937_64 rng(rd());

  std::set<cpp_int> uniq;

  while ((int)uniq.size() < point_num) {
    uniq.insert(random_cpp_int_uniform(long_num - 1, rng));
  }

  return QVector<cpp_int>(uniq.begin(), uniq.end());
}

QPointF residue_to_point(const cpp_int &a, const cpp_int &mod,
                         const QPointF &center, double radius) {
  if (mod <= 0) {
    return center;
  }

  constexpr int frac_bits = 32;
  const long double pi = acosl(-1.0L);

  cpp_int scaled = (a << frac_bits) / mod;

  uint64_t q = scaled.convert_to<uint64_t>();
  long double frac = static_cast<long double>(q) /
                     static_cast<long double>(uint64_t(1) << frac_bits);

  long double angle = 2.0L * pi * frac;

  double x = center.x() + radius * std::cos(static_cast<double>(angle));
  double y = center.y() - radius * std::sin(static_cast<double>(angle));

  return QPointF(x, y);
}

void ModCircleWidget::paintEvent(QPaintEvent *event) {
  Q_UNUSED(event);

  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing, true);

  if (mod <= 0) {
    p.setPen(Qt::black);
    p.drawText(rect(), Qt::AlignCenter, "n must be positive");
    return;
  }

  const int side = std::min(width(), height());
  const QPointF center(width() / 2.0, height() / 2.0);
  const double radius = side * 0.42;

  //
  // Build cpp_int sample
  //

  constexpr int max_sample_size = 500;

  QVector<cpp_int> sample_vec;
  int sample_size;
  if (mod <= max_sample_size) {
    sample_size = static_cast<int>(mod);
    sample_vec.reserve(sample_size);

    for (int i = 0; i < sample_size; i++) {
      sample_vec.push_back(i);
    }

  } else {
    sample_size = max_sample_size;
    sample_vec = random_unique_points(mod, sample_size);
  }

  //
  // Turn sample int into points on circle
  // and draw chords
  //

  // Outer circle
  p.drawEllipse(center, radius, radius);

  for (const cpp_int &a : sample_vec) {
    cpp_int b = rsa::mod_pow(a, exp, mod);

    QPointF pa = residue_to_point(a, mod, center, radius);
    QPointF pb = residue_to_point(b, mod, center, radius);

    p.drawLine(pa, pb);
  }
}
