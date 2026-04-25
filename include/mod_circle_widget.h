#pragma once

#include <QChronoTimer>
#include <QPointF>
#include <QVector>
#include <QWidget>
#include <boost/multiprecision/cpp_int.hpp>

using boost::multiprecision::cpp_int;

QVector<cpp_int> build_sample(const cpp_int &mod);

QVector<cpp_int> compute_b(const QVector<cpp_int> &a_vec, const cpp_int &mod,
                           const cpp_int &exp);

QVector<QLineF> build_circle_lines(const QVector<cpp_int> a_vec,
                                   const QVector<cpp_int> b_vec,
                                   const cpp_int &mod);

class ModCircleWidget : public QWidget {

public:
  explicit ModCircleWidget(QWidget *parent = nullptr);

  void set_lines(const QVector<QLineF> &lines);
  void clear_plot();
  void reset_animation();
  void animate_point(cpp_int m);
  void set_params(cpp_int new_mod, cpp_int new_exp);
  void set_loading(bool loading);

protected:
  void paintEvent(QPaintEvent *event) override;

private:
  cpp_int mod = 0;
  cpp_int exp = 0;

  struct CircleGeometry {
    QPointF center;
    double radius = 0.0;
    double label_radius = 0.0;
  };

  CircleGeometry circle_geometry() const;
  QFont label_font() const;

  bool show_spinner = false;
  int spinner_angle = 0;
  QChronoTimer *spinner_timer = nullptr;
  void draw_spinner(QPainter &p, const QPointF &center, double radius);

  void advance_animation();
  void set_animation_segment();

  QVector<QLineF> cached_lines;
  QChronoTimer *animation_timer = nullptr;
  QPointF animation_start_point;
  QPointF animation_end_point;
  QPointF animated_point;
  int animation_segment_frames = 1;
  int animation_frame = 0;
  bool show_animated_point = false;
  cpp_int animation_base = 0;
  cpp_int animation_current_residue = 0;
  cpp_int animation_next_residue = 0;
  cpp_int animation_step = 0;
  cpp_int animation_total_steps = 0;
};
