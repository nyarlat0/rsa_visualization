#pragma once
#include <QWidget>
class QLabel;
class QTextEdit;
class QLineEdit;
class QPushButton;
class ModCircleWidget;

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
  QPushButton *update_circle_button;
  ModCircleWidget *encrypt_circle;

private:
  QString encrypt();
  void import_pubkey();
  void export_encrypted();
  void shared_encrypt();
  void update_circle();

signals:
  void error_signal(const QString &message);
  void shared_result_signal(const QString &ctext);
};
