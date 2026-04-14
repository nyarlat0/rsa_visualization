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

static double angle01(const QPointF &v) {
  double a = std::atan2(-v.y(), v.x());
  constexpr double tau = 2.0 * std::numbers::pi_v<double>;

  if (a < 0.0)
    a += tau;

  return a / tau;
}

static double circular_delta(double from, double to) {
  double d = to - from;

  if (d > 0.5)
    d -= 1.0;
  if (d < -0.5)
    d += 1.0;

  return d; // -0.5 .. +0.5
}

QVector<cpp_int> build_sample(const cpp_int &mod) {
  constexpr int max_sample_size = 5000;

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

QVector<QLineF> build_circle_lines(const QVector<cpp_int> &shared_sample,
                                   const cpp_int &mod, const cpp_int &exp) {
  if (mod <= 0) {
    return QVector<QLineF>();
  }

  const QPointF center(0.0, 0.0);
  const double radius = 1.0;

  QVector<cpp_int> sample_vec =
      shared_sample.isEmpty() ? build_sample(mod) : shared_sample;
  sample_vec.detach();

  int sample_size = sample_vec.size();

  QVector<QLineF> lines(sample_size);

  unsigned int hw = std::thread::hardware_concurrency();
  int thread_count = (hw > 1) ? static_cast<int>(hw - 1) : 1;

  if (thread_count == 1) {
    for (int i = 0; i < sample_size; i++) {
      const cpp_int &a = sample_vec[i];
      cpp_int b = rsa::mod_pow(a, exp, mod);

      QPointF pa = residue_to_point(a, mod, center, radius);
      QPointF pb = residue_to_point(b, mod, center, radius);

      lines[i] = QLineF(pa, pb);
    }

    return lines;
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
        const cpp_int &a = sample_vec[i];
        cpp_int b = rsa::mod_pow(a, exp, mod);

        QPointF pa = residue_to_point(a, mod, center, radius);
        QPointF pb = residue_to_point(b, mod, center, radius);

        lines[i] = QLineF(pa, pb);
      }
    });
  }

  for (auto &th : workers) {
    th.join();
  }

  return lines;
}

static double point_hue(const QPointF &v) {
  constexpr double pi = std::numbers::pi;

  double angle = std::atan2(-v.y(), v.x());
  if (angle < 0.0) {
    angle += 2.0 * pi;
  }

  return angle / (2.0 * pi);
}

static double alpha_scale_for_line_count(int line_count) {
  constexpr double reference_line_count = 400.0;
  constexpr double exponent = 0.6;

  double safe_count = std::max(1, line_count);
  double scale = std::pow(reference_line_count / safe_count, exponent);

  return std::clamp(scale, 1.5, 2.0);
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

  QImage heat(size(), QImage::Format_ARGB32_Premultiplied);
  heat.fill(Qt::transparent);

  QPainter hp(&heat);
  hp.setRenderHint(QPainter::Antialiasing);

  // Overlaps ADD color instead of replacing it
  hp.setCompositionMode(QPainter::CompositionMode_Plus);

  const int side = std::min(width(), height());
  const QPointF center(width() / 2.0, height() / 2.0);
  const double radius = side * 0.45;

  // Outer circle
  p.drawEllipse(center, radius, radius);

  const double alpha_scale = alpha_scale_for_line_count(cached_lines.size());

  // Chords
  for (const QLineF &cline : cached_lines) {
    QPointF a = cline.p1();
    QPointF b = cline.p2();

    auto line = QLineF(center + a * radius, center + b * radius);

    double h1 = angle01(cline.p1());
    double h2 = angle01(cline.p2());

    double jump = circular_delta(h1, h2);

    // negative jump = blue/cyan, positive jump = red/yellow
    double hue = jump < 0.0 ? 0.58 : 0.04;

    // Keep HSV channels in range even if the angular jump overshoots.
    double strength = std::clamp(std::abs(jump) * 2.0, 0.0, 1.0);

    double start_alpha =
        std::clamp((0.020 + 0.030 * strength) * alpha_scale, 0.004, 0.18);
    double end_alpha =
        std::clamp((0.060 + 0.120 * strength) * alpha_scale, 0.010, 0.42);

    QColor start =
        QColor::fromHsvF(hue, 0.95, 0.45 + 0.35 * strength, start_alpha);
    QColor end = QColor::fromHsvF(hue, 0.95, 0.75 + 0.25 * strength, end_alpha);

    QLinearGradient grad(line.p1(), line.p2());
    grad.setColorAt(0.0, start);
    grad.setColorAt(1.0, end);

    QPen pen(QBrush(grad), 1.1);
    pen.setCapStyle(Qt::RoundCap);
    hp.setPen(pen);
    hp.drawLine(line);
  }

  hp.end();
  p.drawImage(0, 0, heat);
}
