#include "decrypt_widget.h"
#include "helpers.h"
#include "mod_circle_widget.h"
#include "rsa.h"
#include <QApplication>
#include <QClipboard>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QTextEdit>
#include <QWidget>
#include <QtConcurrent>
#include <boost/multiprecision/cpp_int.hpp>
#include <qpushbutton.h>

using boost::multiprecision::cpp_int;

bool DecryptWidget::validate_fields(const QString &d_in, const QString &p_in,
                                    const QString &q_in, const QString &ctxt) {
  const auto error_style =
      QString("border: 2px solid red; border-radius: %1px;")
          .arg(mod_scale(this, -2));

  if (d_in.isEmpty() || !validate_num(d_in)) {
    d_edit->setStyleSheet(error_style);
    emit error_signal("Please enter d as single number!");
    return false;
  }
  d_edit->setStyleSheet("");

  if (p_in.isEmpty() || !validate_num(p_in)) {
    p_edit->setStyleSheet(error_style);
    emit error_signal("Please enter p as single number!");
    return false;
  }
  p_edit->setStyleSheet("");

  if (q_in.isEmpty() || !validate_num(q_in)) {
    q_edit->setStyleSheet(error_style);
    emit error_signal("Please enter q as single number!");
    return false;
  }
  q_edit->setStyleSheet("");

  if (ctxt.isEmpty() || !validate_num(ctxt)) {
    ciphertext_edit->setStyleSheet(error_style);
    emit error_signal("Please enter cipher text as single number!");
    return false;
  }
  ciphertext_edit->setStyleSheet("");

  return true;
}
bool DecryptWidget::validate_fields(const QString &d_in, const QString &p_in,
                                    const QString &q_in) {
  const auto error_style =
      QString("border: 2px solid red; border-radius: %1px;")
          .arg(mod_scale(this, -2));

  if (d_in.isEmpty() || !validate_num(d_in)) {
    d_edit->setStyleSheet(error_style);
    emit error_signal("Please enter d as single number!");
    return false;
  }
  d_edit->setStyleSheet("");

  if (p_in.isEmpty() || !validate_num(p_in)) {
    p_edit->setStyleSheet(error_style);
    emit error_signal("Please enter p as single number!");
    return false;
  }
  p_edit->setStyleSheet("");

  if (q_in.isEmpty() || !validate_num(q_in)) {
    q_edit->setStyleSheet(error_style);
    emit error_signal("Please enter q as single number!");
    return false;
  }
  q_edit->setStyleSheet("");

  return true;
}

QString DecryptWidget::decrypt() {
  auto d_in = d_edit->toPlainText().trimmed();
  auto p_in = p_edit->toPlainText().trimmed();
  auto q_in = q_edit->toPlainText().trimmed();
  auto ctxt = ciphertext_edit->toPlainText().trimmed();

  if (!validate_fields(d_in, p_in, q_in, ctxt))
    return "";

  auto d = cpp_int(d_in.toStdString());
  auto p = cpp_int(p_in.toStdString());
  auto q = cpp_int(q_in.toStdString());
  auto cnum = cpp_int(ctxt.toStdString());

  try {
    auto ptxt = rsa::decrypt(cnum, d, p, q);

    ciphertext_edit->clear();
    return ptxt;

  } catch (std::runtime_error &err) {
    emit error_signal(err.what());
    return "";
  }
}

void DecryptWidget::shared_decrypt() {
  const auto d_in = d_edit->toPlainText().trimmed();
  const auto p_in = p_edit->toPlainText().trimmed();
  const auto q_in = q_edit->toPlainText().trimmed();
  const auto ctxt = ciphertext_edit->toPlainText().trimmed();

  if (!validate_fields(d_in, p_in, q_in, ctxt))
    return;

  auto d = cpp_int(d_in.toStdString());
  auto p = cpp_int(p_in.toStdString());
  auto q = cpp_int(q_in.toStdString());
  auto cnum = cpp_int(ctxt.toStdString());

  auto ptxt = decrypt();

  if (!ptxt.isEmpty()) {
    decrypt_circle->animate_point(cnum);
    emit shared_result_signal(ptxt);
  }
}

void DecryptWidget::export_pubkey() {
  auto d_in = d_edit->toPlainText().trimmed();
  auto p_in = p_edit->toPlainText().trimmed();
  auto q_in = q_edit->toPlainText().trimmed();

  if (!validate_fields(d_in, p_in, q_in))
    return;

  auto d = cpp_int(d_in.toStdString());
  auto p = cpp_int(p_in.toStdString());
  auto q = cpp_int(q_in.toStdString());

  cpp_int n = p * q;
  cpp_int lambda = rsa::lcm(p - 1, q - 1);
  auto e = rsa::mod_inverse(d, lambda);

  auto export_txt = QString("e=%1\nn=%2").arg(e.str(), n.str());
  QApplication::clipboard()->setText(export_txt);
}

void DecryptWidget::update_circle() {
  auto d_in = d_edit->toPlainText().trimmed();
  auto p_in = p_edit->toPlainText().trimmed();
  auto q_in = q_edit->toPlainText().trimmed();

  if (!validate_fields(d_in, p_in, q_in))
    return;

  auto d = cpp_int(d_in.toStdString());
  auto p = cpp_int(p_in.toStdString());
  auto q = cpp_int(q_in.toStdString());

  cpp_int n = p * q;
  decrypt_circle->set_params(n, d);
  decrypt_circle->set_loading(true);

  auto future = QtConcurrent::run([n, d] {
    auto a_vec = build_sample(n);
    auto b_vec = compute_b(a_vec, n, d);
    return build_circle_lines(a_vec, b_vec, n);
  });

  circle_watcher->setFuture(future);
}

DecryptWidget::DecryptWidget(QWidget *parent) : QWidget(parent) {
  //
  // Right Half - Decrypt plaintext
  //

  circle_watcher = new QFutureWatcher<QVector<QLineF>>(this);
  connect(circle_watcher, &QFutureWatcher<QVector<QLineF>>::finished, this,
          [this]() {
            decrypt_circle->set_loading(false);
            decrypt_circle->set_lines(circle_watcher->result());
          });

  auto *num_validator =
      new QRegularExpressionValidator(QRegularExpression("[0-9]+"), this);

  d_label = new QLabel("Anti-exponent d:");
  d_edit = new QTextEdit(this);
  d_edit->setPlaceholderText("Enter private d");

  p_label = new QLabel("Prime p:");
  p_edit = new QTextEdit(this);
  p_edit->setPlaceholderText("Enter private p");

  q_label = new QLabel("Prime q:");
  q_edit = new QTextEdit(this);
  q_edit->setPlaceholderText("Enter private q");

  ciphertext_label = new QLabel("Ciphertext = P ^ e mod n:");
  ciphertext_edit = new QTextEdit(this);
  ciphertext_edit->setPlaceholderText("Enter cyphertext");

  auto input_palette = ciphertext_edit->palette();
  input_palette.setColor(QPalette::PlaceholderText, QColor(140, 140, 140));
  p_edit->setPalette(input_palette);
  q_edit->setPalette(input_palette);
  d_edit->setPalette(input_palette);
  ciphertext_edit->setPalette(input_palette);

  decrypt_button = new QPushButton("Decrypt", this);
  export_pubkey_button = new QPushButton("Export PubKey", this);
  decrypt_circle = new ModCircleWidget(this);

  connect(decrypt_button, &QPushButton::clicked, this,
          &DecryptWidget::shared_decrypt);
  connect(export_pubkey_button, &QPushButton::clicked, this,
          &DecryptWidget::export_pubkey);
  connect(p_edit, &QTextEdit::textChanged, this, &DecryptWidget::update_circle);
  connect(q_edit, &QTextEdit::textChanged, this, &DecryptWidget::update_circle);
  connect(d_edit, &QTextEdit::textChanged, this, &DecryptWidget::update_circle);
}
