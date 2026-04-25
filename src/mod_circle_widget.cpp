#include "mod_circle_widget.h"
#include "rsa.h"
#include <thread>

#include <QChronoTimer>
#include <QEasingCurve>
#include <QPainter>
#include <QPen>
#include <QPointF>
#include <QVector>

#include <algorithm>
#include <cmath>
#include <random>

namespace {

constexpr double kMinRadiusPx = 20.0;
constexpr double kTickLengthPx = 8.0;
constexpr double kLabelGapPx = 2.0;

constexpr int kMaxSampleSize = 500;

constexpr int kAnimationFps = 60;
constexpr int kAnimationFramesPerMove = 15;

constexpr double kAnimationTrailSubstepPx = 2.5;
constexpr double kAnimationTrailMinWidthGlowRatio = 0.0;
constexpr double kAnimationTrailMaxWidthGlowRatio = 0.30;
constexpr double kAnimatedPointRadiusPx = 3.0;
constexpr double kAnimatedPointGlowRadiusPx = 20.0;

constexpr int kMarkCount = 8; // every 45 degrees

QPointF clock_point(const QPointF &center, double radius, long double angle) {
  return QPointF(center.x() + radius * std::cos(static_cast<double>(angle)),
                 center.y() - radius * std::sin(static_cast<double>(angle)));
}

QString format_big_number(const cpp_int &value) {
  std::string s = value.str();

  bool negative = false;
  if (!s.empty() && s[0] == '-') {
    negative = true;
    s.erase(s.begin());
  }

  if (s.size() <= 6)
    return negative ? "-" + QString::fromStdString(s)
                    : QString::fromStdString(s);

  QString out;
  if (negative)
    out += "-";

  out += QChar(s[0]);

  if (s.size() > 1) {
    out += ".";
    const std::size_t frac_digits = std::min<std::size_t>(1, s.size() - 1);
    for (std::size_t i = 1; i <= frac_digits; ++i) {
      out += QChar(s[i]);
    }
  }

  out += " * 10^";
  out += QString::number(static_cast<int>(s.size() - 1));

  return out;
}

} // namespace

ModCircleWidget::ModCircleWidget(QWidget *parent) : QWidget(parent) {
  setMinimumSize(400, 400);

  animation_timer = new QChronoTimer(this);
  animation_timer->setTimerType(Qt::PreciseTimer);
  animation_timer->setInterval(
      std::chrono::nanoseconds(1'000'000'000 / kAnimationFps));

  connect(animation_timer, &QChronoTimer::timeout, this,
          [this]() { advance_animation(); });

  spinner_timer = new QChronoTimer(this);
  spinner_timer->setTimerType(Qt::PreciseTimer);
  spinner_timer->setInterval(
      std::chrono::nanoseconds(1'000'000'000 / kAnimationFps));
  connect(spinner_timer, &QChronoTimer::timeout, this, [this]() {
    spinner_angle = (spinner_angle + 6) % 360;
    update();
  });
}
void ModCircleWidget::set_loading(bool loading) {
  if (show_spinner == loading)
    return;

  show_spinner = loading;

  if (show_spinner)
    spinner_timer->start();
  else {
    spinner_timer->stop();
    spinner_angle = 0;
  }

  update();
}

void ModCircleWidget::draw_spinner(QPainter &p, const QPointF &center,
                                   double radius) {
  p.save();

  const double spinner_radius = radius * 0.22;
  const QRectF spinner_rect(center.x() - spinner_radius,
                            center.y() - spinner_radius, spinner_radius * 2.0,
                            spinner_radius * 2.0);

  QPen pen(QColor(255, 80, 200));
  pen.setWidthF(std::max(3.0, radius * 0.025));
  pen.setCapStyle(Qt::RoundCap);

  p.setPen(pen);
  p.setBrush(Qt::NoBrush);

  // Qt drawArc angles are in 1/16 degrees.
  // Positive angles go counterclockwise.
  const int start_angle = spinner_angle * 16;
  const int span_angle = 270 * 16;

  p.drawArc(spinner_rect, start_angle, span_angle);

  p.restore();
}

QFont ModCircleWidget::label_font() const {
  QFont f = font();
  f.setPointSizeF(f.pointSizeF() * 0.8);
  return f;
}

ModCircleWidget::CircleGeometry ModCircleWidget::circle_geometry() const {
  const QFontMetrics fm(label_font());
  const int side = std::min(width(), height());
  const QPointF center(width() / 2.0, height() / 2.0);

  int max_label_w = 0;
  int max_label_h = 0;

  if (mod > 0) {
    for (int k = 0; k < kMarkCount; ++k) {
      const cpp_int mark_value = (mod * k) / kMarkCount;
      const QString label = format_big_number(mark_value);

      QRect r = fm.boundingRect(label);
      r.adjust(-3, -2, 3, 2);

      max_label_w = std::max(max_label_w, r.width());
      max_label_h = std::max(max_label_h, r.height());
    }
  }

  const double horizontal_label_space =
      max_label_w + kTickLengthPx + kLabelGapPx;

  const double vertical_label_space = max_label_h + kTickLengthPx + kLabelGapPx;

  const double max_radius_x = width() / 2.0 - horizontal_label_space;
  const double max_radius_y = height() / 2.0 - vertical_label_space;

  const double radius = std::max(
      kMinRadiusPx, std::min({max_radius_x, max_radius_y, side * 0.45}));

  const double label_radius = radius + kTickLengthPx + kLabelGapPx;

  return {
      .center = center,
      .radius = radius,
      .label_radius = label_radius,
  };
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
  if (max_inclusive < 0)
    throw std::runtime_error("max_inclusive must be non-negative");

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

      if (rem_bits < 64)
        last &= ((uint64_t(1) << rem_bits) - 1);

      x <<= rem_bits;
      x |= cpp_int(last);
    }

    if (x <= max_inclusive)
      return x;
  }
}

QVector<cpp_int> random_unique_points(const cpp_int &long_num, int point_num) {
  if (long_num <= 0)
    throw std::runtime_error("long_num must be positive");

  if (point_num < 0)
    throw std::runtime_error("point_num must be non-negative");

  if (cpp_int(point_num) > long_num)
    throw std::runtime_error("point_num cannot exceed long_num");

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
  if (mod <= 0)
    return center;

  constexpr int frac_bits = 32;
  const long double pi = acosl(-1.0L);

  cpp_int scaled = (a << frac_bits) / mod;

  uint64_t q = scaled.convert_to<uint64_t>();
  long double frac = static_cast<long double>(q) /
                     static_cast<long double>(uint64_t(1) << frac_bits);

  long double angle = pi / 2.0L - 2.0L * pi * frac;

  double x = center.x() + radius * std::cos(static_cast<double>(angle));
  double y = center.y() - radius * std::sin(static_cast<double>(angle));

  return QPointF(x, y);
}

QVector<cpp_int> build_sample(const cpp_int &mod) {

  QVector<cpp_int> sample_vec;

  if (mod <= kMaxSampleSize) {
    int sample_size = static_cast<int>(mod);
    sample_vec.reserve(sample_size);

    for (int i = 0; i < sample_size; ++i) {
      sample_vec.push_back(i);
    }
  } else {
    sample_vec = random_unique_points(mod, kMaxSampleSize);
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

    if (begin >= end)
      break;

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
  if (mod <= 0)
    return QVector<QLineF>();

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

void ModCircleWidget::set_params(cpp_int new_mod, cpp_int new_exp) {
  mod = new_mod;
  exp = new_exp;
  reset_animation();
  update();
}

void ModCircleWidget::reset_animation() {
  animation_timer->stop();

  animation_segment_frames = kAnimationFramesPerMove;
  animation_frame = 0;

  show_animated_point = false;
  animation_trail.clear();

  animation_base = 0;
  animation_current_residue = 0;
  animation_next_residue = 0;

  animation_step = 0;
  animation_total_steps = 0;
}

void ModCircleWidget::clear_plot() {
  cached_lines.clear();
  reset_animation();
  update();
}

void ModCircleWidget::set_animations_enabled(bool enabled) {
  if (animations_enabled == enabled)
    return;

  animations_enabled = enabled;

  if (!animations_enabled) {
    reset_animation();
    update();
  }
}

void ModCircleWidget::set_animation_trail_max_points(int max_points) {
  animation_trail_max_points = std::max(1, max_points);

  while (animation_trail.size() > animation_trail_max_points) {
    animation_trail.pop_front();
  }

  update();
}

void ModCircleWidget::animate_point(cpp_int m) {
  reset_animation();

  if (!animations_enabled || mod <= 0 || exp <= 0) {
    update();
    return;
  }

  animation_base = m % mod;
  if (animation_base < 0)
    animation_base += mod;

  animation_current_residue = animation_base;
  animation_next_residue = (animation_current_residue * animation_base) % mod;
  animation_total_steps = exp;

  const auto [center, radius, _label_radius] = circle_geometry();

  animated_point =
      residue_to_point(animation_current_residue, mod, center, radius);
  append_animated_point_to_trail();

  show_animated_point = true;

  set_animation_segment();
  animation_timer->start();

  update();
}

void ModCircleWidget::advance_animation() {
  if (animation_step >= animation_total_steps) {
    animation_timer->stop();
    return;
  }

  QEasingCurve easing(QEasingCurve::InOutSine);
  const double progress =
      static_cast<double>(animation_frame + 1) / animation_segment_frames;
  const double eased = easing.valueForProgress(progress);

  animated_point = animation_start_point +
                   (animation_end_point - animation_start_point) * eased;
  append_animated_point_to_trail();
  update();

  ++animation_frame;
  if (animation_frame < animation_segment_frames)
    return;

  animation_frame = 0;
  ++animation_step;
  animated_point = animation_end_point;
  append_animated_point_to_trail();

  if (animation_step >= animation_total_steps) {
    animation_timer->stop();
    update();
    return;
  }

  animation_current_residue = animation_next_residue;
  animation_next_residue = (animation_current_residue * animation_base) % mod;
  set_animation_segment();
}

void ModCircleWidget::set_animation_segment() {

  const auto [center, radius, _label_radius] = circle_geometry();

  animation_start_point =
      residue_to_point(animation_current_residue, mod, center, radius);

  animation_end_point =
      residue_to_point(animation_next_residue, mod, center, radius);

  animation_segment_frames = kAnimationFramesPerMove;
}

void ModCircleWidget::append_animated_point_to_trail() {
  if (!animation_trail.isEmpty()) {
    const QPointF delta = animation_trail.back() - animated_point;

    if (delta.x() * delta.x() + delta.y() * delta.y() < 0.25)
      return;
  }

  animation_trail.push_back(animated_point);

  while (animation_trail.size() > animation_trail_max_points) {
    animation_trail.pop_front();
  }
}

void ModCircleWidget::draw_animated_point(QPainter &p) {
  p.save();

  if (animation_trail.size() > 1) {
    const double trail_min_width =
        kAnimatedPointGlowRadiusPx * kAnimationTrailMinWidthGlowRatio;
    const double trail_max_width =
        kAnimatedPointGlowRadiusPx * kAnimationTrailMaxWidthGlowRatio;

    for (int i = 1; i < animation_trail.size(); ++i) {
      const QPointF start = animation_trail[i - 1];
      const QPointF end = animation_trail[i];
      const QPointF delta = end - start;
      const double distance = std::hypot(static_cast<double>(delta.x()),
                                         static_cast<double>(delta.y()));
      const int substeps = std::max(
          1, static_cast<int>(std::ceil(distance / kAnimationTrailSubstepPx)));

      QPointF segment_start = start;
      for (int step = 1; step <= substeps; ++step) {
        const double local_age = static_cast<double>(step) / substeps;
        const double trail_age =
            (static_cast<double>(i - 1) + local_age) /
            static_cast<double>(animation_trail.size() - 1);
        const double eased_age = trail_age * trail_age;
        const QPointF segment_end = start + delta * local_age;

        QColor trail_color(
            80, 255, 120,
            static_cast<int>(std::clamp(eased_age * 185.0, 0.0, 185.0)));

        QPen pen(trail_color);
        pen.setWidthF(trail_min_width +
                      eased_age * (trail_max_width - trail_min_width));
        pen.setCapStyle(Qt::RoundCap);
        pen.setJoinStyle(Qt::RoundJoin);

        p.setPen(pen);
        p.drawLine(segment_start, segment_end);
        segment_start = segment_end;
      }
    }
  }

  p.setPen(Qt::NoPen);

  QRadialGradient glow(animated_point, kAnimatedPointGlowRadiusPx);
  glow.setColorAt(0.0, QColor(225, 255, 225, 245));
  glow.setColorAt(0.16, QColor(70, 255, 110, 235));
  glow.setColorAt(0.42, QColor(60, 255, 110, 120));
  glow.setColorAt(0.72, QColor(60, 255, 110, 38));
  glow.setColorAt(1.0, QColor(60, 255, 110, 0));

  p.setBrush(glow);
  p.drawEllipse(animated_point, kAnimatedPointGlowRadiusPx,
                kAnimatedPointGlowRadiusPx);

  QRadialGradient core(animated_point, kAnimatedPointRadiusPx);
  core.setColorAt(0.0, QColor(255, 255, 255, 255));
  core.setColorAt(0.55, QColor(160, 255, 175, 245));
  core.setColorAt(1.0, QColor(40, 255, 85, 220));

  p.setBrush(core);
  p.drawEllipse(animated_point, kAnimatedPointRadiusPx, kAnimatedPointRadiusPx);

  p.restore();
}

void ModCircleWidget::paintEvent(QPaintEvent *event) {
  Q_UNUSED(event);

  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing, true);

  const auto [center, radius, label_radius] = circle_geometry();
  const double tick_len = kTickLengthPx;

  // Outer circle
  p.drawEllipse(center, radius, radius);

  // Outer ticks + labels every 45 degrees
  if (mod > 0) {
    const long double pi = acosl(-1.0L);

    p.setFont(label_font());

    QFontMetrics fm(p.font());

    for (int k = 0; k < kMarkCount; ++k) {
      // top, then clockwise every 45 degrees
      long double angle = pi / 2.0L - static_cast<long double>(k) * (pi / 4.0L);

      QPointF tick_start = clock_point(center, radius, angle);
      QPointF tick_end = clock_point(center, radius + tick_len, angle);

      p.drawLine(tick_start, tick_end);

      // Value at this angular mark: 0, mod/8, 2mod/8, ..., 7mod/8
      cpp_int mark_value = (mod * k) / 8;
      QString label = format_big_number(mark_value);

      QPointF label_center = clock_point(center, label_radius, angle);

      QRect text_rect = fm.boundingRect(label);
      text_rect.adjust(-3, -2, 3, 2);

      const QPoint anchor = label_center.toPoint();

      const double dx = std::cos(static_cast<double>(angle));
      const double dy = -std::sin(static_cast<double>(angle)); // screen y

      constexpr double eps = 0.2;

      // horizontal placement
      if (dx > eps)
        text_rect.moveLeft(anchor.x());
      else if (dx < -eps)
        text_rect.moveRight(anchor.x());
      else
        text_rect.moveCenter(QPoint(anchor.x(), text_rect.center().y()));

      // vertical placement
      if (dy > eps)
        text_rect.moveTop(anchor.y());
      else if (dy < -eps)
        text_rect.moveBottom(anchor.y());
      else
        text_rect.moveCenter(QPoint(text_rect.center().x(), anchor.y()));

      p.drawText(text_rect, Qt::AlignCenter, label);
    }
  }

  // Chords
  for (const QLineF &cline : cached_lines) {
    QPointF a = cline.p1();
    QPointF b = cline.p2();

    auto line = QLineF(center + a * radius, center + b * radius);

    QLinearGradient grad(line.p1(), line.p2());
    grad.setColorAt(0.0, Qt::red);
    grad.setColorAt(0.15, Qt::red);

    grad.setColorAt(0.5, Qt::green);

    grad.setColorAt(0.85, Qt::blue);
    grad.setColorAt(1.0, Qt::blue);

    QPen pen(QBrush(grad), 1.0);
    p.setPen(pen);
    p.drawLine(line);
  }

  if (show_animated_point)
    draw_animated_point(p);

  if (show_spinner)
    draw_spinner(p, center, radius);
}
