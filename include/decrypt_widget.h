#pragma once
#include <QFutureWatcher>
#include <QLineF>
#include <QVector>
#include <QWidget>
class QLabel;
class QTextEdit;
class QLineEdit;
class QPushButton;
class ModCircleWidget;
#include <boost/multiprecision/cpp_int.hpp>

using boost::multiprecision::cpp_int;

class DecryptWidget : public QWidget {
  Q_OBJECT

public:
  explicit DecryptWidget(QWidget *parent);
  QLabel *d_label;
  QTextEdit *d_edit;

  QLabel *p_label;
  QTextEdit *p_edit;

  QLabel *q_label;
  QTextEdit *q_edit;

  QLabel *ciphertext_label;
  QTextEdit *ciphertext_edit;

  QPushButton *decrypt_button;
  QPushButton *export_pubkey_button;
  ModCircleWidget *decrypt_circle;
  void
  update_circle(const QVector<cpp_int> &shared_sample = QVector<cpp_int>());

private:
  QFutureWatcher<QVector<QLineF>> *circle_watcher = nullptr;

  QString decrypt();
  void export_pubkey();
  void shared_decrypt();

signals:
  void error_signal(const QString &message);
  void shared_result_signal(const QString &ptext);
};
