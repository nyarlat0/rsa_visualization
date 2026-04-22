#include "mod_circle_widget.h"
#include "rsa.h"
#include <thread>

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

QVector<cpp_int> build_sample(const cpp_int &mod) {
  constexpr int max_sample_size = 500;

  QVector<cpp_int> sample_vec;

  if (mod <= max_sample_size) {
    int sample_size = static_cast<int>(mod);
    sample_vec.reserve(sample_size);

    for (int i = 0; i < sample_size; ++i) {
      sample_vec.push_back(i);
    }
  } else {
    sample_vec = random_unique_points(mod, max_sample_size);
  }

  return sample_vec;
}

QVector<cpp_int> compute_b(const QVector<cpp_int> &a_vec, const cpp_int &mod,
                           const cpp_int &exp) {
  const int sample_size = a_vec.size();
  auto b_vec = QVector<cpp_int>(sample_size);

  unsigned int hw = std::thread::hardware_concurrency();
  int thread_count = (hw > 1) ? static_cast<int>(hw - 1) : 1;

  if (thread_count == 1) {
    for (int i = 0; i < sample_size; i++) {
      const cpp_int &a = a_vec[i];
      b_vec[i] = rsa::mod_pow(a, exp, mod);
    }

    return b_vec;
  }

  std::vector<std::thread> workers;
  workers.reserve(thread_count);

  int chunk = (sample_size + thread_count - 1) / thread_count;

  for (int t = 0; t < thread_count; ++t) {
    int begin = t * chunk;
    int end = std::min(begin + chunk, sample_size);

    if (begin >= end) {
      break;
    }

    workers.emplace_back([&, begin, end]() {
      for (int i = begin; i < end; ++i) {
        const cpp_int &a = a_vec[i];
        b_vec[i] = rsa::mod_pow(a, exp, mod);
      }
    });
  }

  for (auto &th : workers) {
    th.join();
  }

  return b_vec;
}

QVector<QLineF> build_circle_lines(const QVector<cpp_int> a_vec,
                                   const QVector<cpp_int> b_vec,
                                   const cpp_int &mod) {
  if (mod <= 0) {
    return QVector<QLineF>();
  }

  const QPointF center(0.0, 0.0);
  const double radius = 1.0;

  int sample_size = a_vec.size();

  QVector<QLineF> lines(sample_size);

  for (int i = 0; i < sample_size; i++) {
    const cpp_int &a = a_vec[i];
    const cpp_int &b = b_vec[i];

    QPointF pa = residue_to_point(a, mod, center, radius);
    QPointF pb = residue_to_point(b, mod, center, radius);

    lines[i] = QLineF(pa, pb);
  }

  return lines;
}

void ModCircleWidget::set_lines(const QVector<QLineF> &lines) {
  cached_lines = lines;
  update();
}

void ModCircleWidget::clear_plot() {
  cached_lines.clear();
  update();
}

void ModCircleWidget::paintEvent(QPaintEvent *event) {
  Q_UNUSED(event);

  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing, true);

  const int side = std::min(width(), height());
  const QPointF center(width() / 2.0, height() / 2.0);
  const double radius = side * 0.45;

  // Outer circle
  p.drawEllipse(center, radius, radius);

  // Chords
  for (const QLineF &cline : cached_lines) {
    QPointF a = cline.p1();
    QPointF b = cline.p2();

    auto line = QLineF(center + a * radius, center + b * radius);
    p.drawLine(line);
  }
}
