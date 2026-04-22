#include "encrypt_widget.h"
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
#include <QVBoxLayout>
#include <QWidget>
#include <QtConcurrent>
#include <boost/multiprecision/cpp_int.hpp>
#include <qlineedit.h>

using boost::multiprecision::cpp_int;

QString EncryptWidget::encrypt() {
  auto e_in = e_edit->text().trimmed();
  auto n_in = n_edit->toPlainText().trimmed();
  auto ptxt = plaintext_edit->toPlainText().trimmed();

  const auto error_style =
      QString("border: 2px solid red; border-radius: %1px;")
          .arg(mod_scale(this, -2));

  if (e_in.isEmpty() || !validate_num(e_in)) {
    e_edit->setStyleSheet(error_style);
    emit error_signal("Please enter e!");
    return "";
  }
  e_edit->setStyleSheet("");

  if (n_in.isEmpty() || !validate_num(n_in)) {
    n_edit->setStyleSheet(error_style);
    emit error_signal("Please enter n as single number!");
    return "";
  }
  n_edit->setStyleSheet("");

  if (ptxt.isEmpty()) {
    plaintext_edit->viewport()->setStyleSheet(error_style);
    emit error_signal("Please enter plain text!");
    return "";
  }
  plaintext_edit->setStyleSheet("");

  auto e = cpp_int(e_in.toStdString());
  auto n = cpp_int(n_in.toStdString());

  try {
    auto ctxt = QString::fromStdString(rsa::encrypt(ptxt, e, n).str());

    plaintext_edit->clear();
    return ctxt;

  } catch (std::runtime_error &err) {
    emit error_signal(err.what());
    return "";
  }
}

void EncryptWidget::shared_encrypt() {
  auto ctxt = encrypt();
  if (!ctxt.isEmpty()) {
    emit shared_result_signal(ctxt);
  }
}

void EncryptWidget::import_pubkey() {
  auto cl_txt = QApplication::clipboard()->text();
  const QString err_message =
      "Invalid public key in clipboard!\nIt should be in the "
      "form:\ne=[number]\nn=[number]";

  auto lines = cl_txt.split('\n', Qt::SkipEmptyParts);
  if (lines.size() != 2) {
    emit error_signal(err_message);
    return;
  }
  auto e_line = lines[0];
  auto n_line = lines[1];

  if (!e_line.startsWith("e=") || !n_line.startsWith("n=")) {
    emit error_signal(err_message);
    return;
  }

  auto e_str = e_line.mid(2).trimmed();
  auto n_str = n_line.mid(2).trimmed();

  if (e_str.isEmpty() || n_str.isEmpty() || !validate_num(e_str) ||
      !validate_num(n_str)) {
    emit error_signal(err_message);
    return;
  }

  e_edit->setText(e_str);
  n_edit->setText(n_str);
}

void EncryptWidget::export_encrypted() {
  auto ctxt = encrypt();
  if (!ctxt.isEmpty()) {
    QApplication::clipboard()->setText(ctxt);
  }
}

void EncryptWidget::update_circle() {
  auto e_in = e_edit->text().trimmed();
  auto n_in = n_edit->toPlainText().trimmed();

  const auto error_style =
      QString("border: 2px solid red; border-radius: %1px;")
          .arg(mod_scale(this, -2));

  if (e_in.isEmpty() || !validate_num(e_in)) {
    e_edit->setStyleSheet(error_style);
    emit error_signal("Please enter e!");
    return;
  }
  e_edit->setStyleSheet("");

  if (n_in.isEmpty() || !validate_num(n_in)) {
    n_edit->setStyleSheet(error_style);
    emit error_signal("Please enter n as single number!");
    return;
  }
  n_edit->setStyleSheet("");

  auto e = cpp_int(e_in.toStdString());
  auto n = cpp_int(n_in.toStdString());

  auto future = QtConcurrent::run([n, e] {
    auto a_vec = build_sample(n);
    auto b_vec = compute_b(a_vec, n, e);
    return build_circle_lines(a_vec, b_vec, n);
  });

  circle_watcher->setFuture(future);
}

EncryptWidget::EncryptWidget(QWidget *parent) : QWidget(parent) {
  //
  // Left Half - Encrypt plaintext
  //

  circle_watcher = new QFutureWatcher<QVector<QLineF>>(this);
  connect(circle_watcher, &QFutureWatcher<QVector<QLineF>>::finished, this,
          [this]() { encrypt_circle->set_lines(circle_watcher->result()); });

  auto *num_validator =
      new QRegularExpressionValidator(QRegularExpression("[0-9]+"), this);

  e_label = new QLabel("Exponent e:");
  e_edit = new QLineEdit(this);
  e_edit->setPlaceholderText("Enter public e");
  e_edit->setValidator(num_validator);

  n_label = new QLabel("Modulus n = p * q:");
  n_edit = new QTextEdit(this);
  n_edit->setPlaceholderText("Enter public n");

  plaintext_label = new QLabel("Plaintext = C ^ d mod n:");
  plaintext_edit = new QTextEdit(this);
  plaintext_edit->setPlaceholderText("Enter plaintext");

  auto intput_palette = plaintext_edit->palette();
  intput_palette.setColor(QPalette::PlaceholderText, QColor(140, 140, 140));
  plaintext_edit->setPalette(intput_palette);
  n_edit->setPalette(intput_palette);
  e_edit->setPalette(intput_palette);

  encrypt_button = new QPushButton("Encrypt", this);
  import_pubkey_button = new QPushButton("Import PubKey", this);
  export_encrypted_button = new QPushButton("Encrypt to clipboard", this);
  encrypt_circle = new ModCircleWidget(this);

  connect(encrypt_button, &QPushButton::clicked, this,
          &EncryptWidget::shared_encrypt);
  connect(import_pubkey_button, &QPushButton::clicked, this,
          &EncryptWidget::import_pubkey);
  connect(export_encrypted_button, &QPushButton::clicked, this,
          &EncryptWidget::export_encrypted);
  connect(n_edit, &QTextEdit::textChanged, this, &EncryptWidget::update_circle);
  connect(e_edit, &QLineEdit::textChanged, this, &EncryptWidget::update_circle);
}
