#pragma once

#include <QMainWindow>

class QLayout;
class EncryptWidget;
class DecryptWidget;

class MainWidget : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWidget(QWidget *parent = nullptr);
  EncryptWidget *encrypt_widget;
  DecryptWidget *decrypt_widget;

  void show_error(const QString &text);
  void gen_keys();
};
