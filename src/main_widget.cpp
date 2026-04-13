#include "main_widget.h"
#include "decrypt_widget.h"
#include "encrypt_widget.h"
#include "helpers.h"
#include "mod_circle_widget.h"
#include "rsa.h"
#include <QGraphicsOpacityEffect>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMessageBox>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QStatusBar>
#include <QTextEdit>
#include <QTimer>
#include <QWidget>
#include <boost/multiprecision/cpp_int.hpp>

using boost::multiprecision::cpp_int;

void MainWidget::show_error(const QString &text) {
  auto *toast = new QLabel(text, this);
  toast->setStyleSheet("background-color: rgba(180, 40, 40, 230);"
                       "color: white;"
                       "padding: 8px 12px;"
                       "border-radius: 8px;"
                       "font-weight: bold;");
  toast->adjustSize();

  const int margin = 16;
  const int rise_px = 30;
  const int anim_ms = 250;
  const int visible_ms = 1800;

  const int x = width() - toast->width() - margin;
  const int end_y = height() - toast->height() - margin;
  const int start_y = end_y + rise_px;

  toast->move(x, start_y);
  toast->show();
  toast->raise();

  auto *opacity = new QGraphicsOpacityEffect(toast);
  opacity->setOpacity(0.0);
  toast->setGraphicsEffect(opacity);

  auto *slide_in = new QPropertyAnimation(toast, "pos");
  slide_in->setDuration(anim_ms);
  slide_in->setStartValue(QPoint(x, start_y));
  slide_in->setEndValue(QPoint(x, end_y));

  auto *fade_in = new QPropertyAnimation(opacity, "opacity");
  fade_in->setDuration(anim_ms);
  fade_in->setStartValue(0.0);
  fade_in->setEndValue(1.0);

  auto *show_group = new QParallelAnimationGroup(toast);
  show_group->addAnimation(slide_in);
  show_group->addAnimation(fade_in);

  connect(show_group, &QParallelAnimationGroup::finished, this, [=]() {
    QTimer::singleShot(visible_ms, toast, [=]() {
      auto *slide_out = new QPropertyAnimation(toast, "pos");
      slide_out->setDuration(anim_ms);
      slide_out->setStartValue(QPoint(x, end_y));
      slide_out->setEndValue(QPoint(x, end_y - rise_px));

      auto *fade_out = new QPropertyAnimation(opacity, "opacity");
      fade_out->setDuration(anim_ms);
      fade_out->setStartValue(1.0);
      fade_out->setEndValue(0.0);

      auto *hide_group = new QParallelAnimationGroup(toast);
      hide_group->addAnimation(slide_out);
      hide_group->addAnimation(fade_out);

      connect(hide_group, &QParallelAnimationGroup::finished, toast,
              &QLabel::deleteLater);

      hide_group->start(QAbstractAnimation::DeleteWhenStopped);
    });
  });

  show_group->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainWidget::gen_keys() {
  QString e, d, n, p, q;
  auto message = encrypt_widget->plaintext_edit->toPlainText().trimmed();
  try {
    rsa::gen_key(e, d, n, p, q);

    encrypt_widget->e_edit->setText(e);
    encrypt_widget->n_edit->setText(n);
    decrypt_widget->d_edit->setText(d);
    decrypt_widget->p_edit->setText(p);
    decrypt_widget->q_edit->setText(q);
  } catch (std::runtime_error &err) {
    show_error(err.what());
  }
}

MainWidget::MainWidget(QWidget *parent) : QMainWindow(parent) {

  setWindowTitle("RSA Visualization");
  resize(500, 260);

  encrypt_widget = new EncryptWidget(this);
  decrypt_widget = new DecryptWidget(this);

  connect(encrypt_widget, &EncryptWidget::error_signal, this,
          &MainWidget::show_error);
  connect(encrypt_widget, &EncryptWidget::shared_result_signal, this,
          [edit = decrypt_widget->ciphertext_edit](const QString &ctxt) {
            edit->setText(ctxt);
          });

  connect(decrypt_widget, &DecryptWidget::error_signal, this,
          &MainWidget::show_error);
  connect(decrypt_widget, &DecryptWidget::shared_result_signal, this,
          [edit = encrypt_widget->plaintext_edit](const QString &ptxt) {
            edit->setText(ptxt);
          });

  auto gen_keys_button = new QPushButton("Generate keys", this);
  connect(gen_keys_button, &QPushButton::clicked, this, &MainWidget::gen_keys);

  const int input_spacing = mod_scale(this, -2);
  const int grid_sapcing = mod_scale(this, 1);

  auto *e_lay = new QVBoxLayout;
  e_lay->addWidget(encrypt_widget->e_label);
  e_lay->addWidget(encrypt_widget->e_edit);
  e_lay->setSpacing(input_spacing);

  auto *n_lay = new QVBoxLayout;
  n_lay->addWidget(encrypt_widget->n_label);
  n_lay->addWidget(encrypt_widget->n_edit);
  n_lay->setSpacing(input_spacing);

  auto *l_key_lay = new QVBoxLayout;
  l_key_lay->addLayout(e_lay);
  l_key_lay->addLayout(n_lay);
  l_key_lay->addWidget(encrypt_widget->import_pubkey_button);
  l_key_lay->setSpacing(mod_scale(this, 0));

  auto *ptxt_lay = new QVBoxLayout;
  ptxt_lay->addWidget(encrypt_widget->plaintext_label);
  ptxt_lay->addWidget(encrypt_widget->plaintext_edit);
  ptxt_lay->setSpacing(input_spacing);

  auto *d_lay = new QVBoxLayout;
  d_lay->addWidget(decrypt_widget->d_label);
  d_lay->addWidget(decrypt_widget->d_edit);
  d_lay->setSpacing(input_spacing);

  auto *p_lay = new QVBoxLayout;
  p_lay->addWidget(decrypt_widget->p_label);
  p_lay->addWidget(decrypt_widget->p_edit);
  p_lay->setSpacing(input_spacing);

  auto *q_lay = new QVBoxLayout;
  q_lay->addWidget(decrypt_widget->q_label);
  q_lay->addWidget(decrypt_widget->q_edit);
  q_lay->setSpacing(input_spacing);

  auto *r_key_lay = new QVBoxLayout;
  r_key_lay->addLayout(d_lay);
  r_key_lay->addLayout(p_lay);
  r_key_lay->addLayout(q_lay);
  r_key_lay->addWidget(decrypt_widget->export_pubkey_button);
  r_key_lay->setSpacing(mod_scale(this, 0));

  auto *l_circle_lay = new QVBoxLayout;
  l_circle_lay->addWidget(encrypt_widget->encrypt_circle);
  l_circle_lay->addWidget(encrypt_widget->update_circle_button);

  auto *r_circle_lay = new QVBoxLayout;
  r_circle_lay->addWidget(decrypt_widget->decrypt_circle);
  r_circle_lay->addWidget(decrypt_widget->update_circle_button);

  auto *ctxt_lay = new QVBoxLayout;
  ctxt_lay->addWidget(decrypt_widget->ciphertext_label);
  ctxt_lay->addWidget(decrypt_widget->ciphertext_edit);
  ctxt_lay->setSpacing(input_spacing);

  auto *l_btn_lay = new QVBoxLayout;
  l_btn_lay->addWidget(encrypt_widget->encrypt_button);
  l_btn_lay->addWidget(encrypt_widget->export_encrypted_button);
  l_btn_lay->setSpacing(mod_scale(this, -1));

  auto *r_btn_lay = new QVBoxLayout;
  r_btn_lay->addWidget(decrypt_widget->decrypt_button);
  r_btn_lay->addWidget(gen_keys_button);
  r_btn_lay->setSpacing(mod_scale(this, -1));

  auto *central = new QWidget(this);
  auto *grid = new QGridLayout(central);

  grid->setSpacing(grid_sapcing);

  grid->addLayout(l_key_lay, 0, 0);
  grid->addLayout(r_key_lay, 0, 1);

  grid->addLayout(l_circle_lay, 2, 0);
  grid->addLayout(r_circle_lay, 2, 1);

  grid->addLayout(ptxt_lay, 3, 0);
  grid->addLayout(ctxt_lay, 3, 1);

  grid->addLayout(l_btn_lay, 4, 0);
  grid->addLayout(r_btn_lay, 4, 1);

  setCentralWidget(central);
}
