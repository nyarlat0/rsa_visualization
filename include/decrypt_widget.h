#pragma once
#include <QWidget>
class QLabel;
class QTextEdit;
class QLineEdit;
class QPushButton;
class ModCircleWidget;

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
  QPushButton *update_circle_button;
  ModCircleWidget *decrypt_circle;

private:
  QString decrypt();
  void export_pubkey();
  void shared_decrypt();
  void update_circle();

signals:
  void error_signal(const QString &message);
  void shared_result_signal(const QString &ptext);
};
