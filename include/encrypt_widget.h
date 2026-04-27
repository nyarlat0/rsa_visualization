#pragma once
#include <QFutureWatcher>
#include <QVector>
#include <QWidget>
class QLabel;
class QLineF;
class QTextEdit;
class QLineEdit;
class QPushButton;
class ModCircleWidget;
#include <boost/multiprecision/cpp_int.hpp>

using boost::multiprecision::cpp_int;

class EncryptWidget : public QWidget {
  Q_OBJECT

public:
  explicit EncryptWidget(QWidget *parent);
  QLabel *e_label;
  QLineEdit *e_edit;

  QLabel *n_label;
  QTextEdit *n_edit;

  QLabel *plaintext_label;
  QTextEdit *plaintext_edit;

  QPushButton *encrypt_button;
  QPushButton *export_encrypted_button;
  QPushButton *import_pubkey_button;
  ModCircleWidget *encrypt_circle;
  void update_circle();

private:
  QFutureWatcher<QVector<QLineF>> *circle_watcher = nullptr;

  bool validate_fields(const QString &e_in, const QString &n_in,
                       const QString &ptxt = "default text");

  QString encrypt();
  void import_pubkey();
  void export_encrypted();
  void shared_encrypt();

signals:
  void error_signal(const QString &message);
  void shared_result_signal(const QString &ctext);
};
