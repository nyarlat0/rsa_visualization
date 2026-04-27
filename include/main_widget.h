#pragma once

#include <QFutureWatcher>
#include <QMainWindow>
#include <QVector>

class QLayout;
class QLineF;
class EncryptWidget;
class DecryptWidget;

struct CirclesLines {
  QVector<QLineF> enc_lines;
  QVector<QLineF> dec_lines;
};

class MainWidget : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWidget(QWidget *parent = nullptr);
  EncryptWidget *encrypt_widget;
  DecryptWidget *decrypt_widget;

  void show_error(const QString &text);
  void gen_keys();

  QFutureWatcher<CirclesLines> *circles_watcher = nullptr;
};
