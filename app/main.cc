#include <QApplication>
#include <iostream>
#include <type_traits>

#include "bolo.h"
#include "mainwindow.h"

int main(int argc, char *argv[]) {
  QApplication a(argc, argv);
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <config>" << std::endl;
    return 0;
  }
  // 生成备份管理对象的智能指针
  auto res = bolo::Bolo::LoadFromJsonFile(argv[1]);
  if (res) {
    // 生成成功
    MainWindow w(std::move(res.value()));
    w.setWindowTitle("BackUp");
    w.setFixedSize(600, 640);
    w.show();

    return a.exec();
  } else {
    // 生成失败，输出错误信息
    std::cerr << res.error() << std::endl;
    return 0;
  }
}
