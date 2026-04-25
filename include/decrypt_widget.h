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
  void update_circle();

private:
  QFutureWatcher<QVector<QLineF>> *circle_watcher = nullptr;

  bool validate_fields(const QString &d_in, const QString &p_in,
                       const QString &q_in, const QString &ctxt);
  bool validate_fields(const QString &d_in, const QString &p_in,
                       const QString &q_in);

  QString decrypt();
  void export_pubkey();
  void shared_decrypt();

signals:
  void error_signal(const QString &message);
  void shared_result_signal(const QString &ptext);
};
