#include "helpers.h"
#include <QWidget>

int rem(QWidget *w, double factor) {
  return qRound(w->fontMetrics().height() * factor);
}

int mod_scale(QWidget *w, int scale, double ratio) {
  return rem(w, pow(1.5, scale));
}

bool validate_num(const QString &txt) {
  for (const auto &ch : txt) {
    if (!ch.isDigit())
      return false;
  }
  return true;
}
