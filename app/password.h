#ifndef PASSWORD_H
#define PASSWORD_H

#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QWidget>

class PassWord : public QMessageBox {
  Q_OBJECT

 public:
  PassWord(QWidget *parent = 0);
  ~PassWord();

  QPushButton *option_ok;
  QPushButton *option_cancel;
  QLabel *plabel;
  QLineEdit password;
};

#endif  // PASSWORD_H
