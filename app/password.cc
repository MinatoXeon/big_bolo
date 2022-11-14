#include "password.h"

#include <QLayout>

PassWord::PassWord(QWidget *parent) : QMessageBox(parent) {
  this->setIcon(QMessageBox::NoIcon);
  this->setWindowTitle("Password");
  this->setStyleSheet("background-color:white");

  password.setParent(this);
  password.setReadOnly(false);
  password.setEchoMode(QLineEdit::Password);

  option_ok = this->addButton("Confirm", QMessageBox::AcceptRole);
  option_cancel = this->addButton("Cancel", QMessageBox::AcceptRole);
  plabel = new QLabel("Please input the password");

  dynamic_cast<QGridLayout *>(this->layout())->addWidget(plabel, 0, 1, 1, 4);
  dynamic_cast<QGridLayout *>(this->layout())->addWidget(&password, 2, 1, 1, 4);
  dynamic_cast<QGridLayout *>(this->layout())->addWidget(option_ok, 4, 1);
  dynamic_cast<QGridLayout *>(this->layout())->addWidget(option_cancel, 4, 2);

  this->setStyleSheet(
      "QLabel{"
      "min-width: 80px;"
      "min-height: 50px; "
      "}");
};

PassWord::~PassWord(){

};
