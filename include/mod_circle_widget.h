#pragma once

#include <QWidget>
#include <boost/multiprecision/cpp_int.hpp>

using boost::multiprecision::cpp_int;

QVector<QLineF> build_circle_lines(const QVector<cpp_int> &shared_sample,
                                   const cpp_int &mod, const cpp_int &exp,
                                   int width, int height);

QVector<cpp_int> build_sample(const cpp_int &mod);

class ModCircleWidget : public QWidget {

public:
  explicit ModCircleWidget(QWidget *parent = nullptr);

  void set_lines(const QVector<QLineF> &lines);
  void clear_plot();

protected:
  void paintEvent(QPaintEvent *event) override;

private:
  QVector<QLineF> cached_lines;
};
