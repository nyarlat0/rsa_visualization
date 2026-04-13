#pragma once

#include <QWidget>
#include <boost/multiprecision/cpp_int.hpp>

using boost::multiprecision::cpp_int;

class ModCircleWidget : public QWidget {

public:
  explicit ModCircleWidget(QWidget *parent = nullptr);

  void set_params(const cpp_int &n, const cpp_int &e);

protected:
  void paintEvent(QPaintEvent *event) override;

private:
  cpp_int mod = 899;
  cpp_int exp = 17;
};
