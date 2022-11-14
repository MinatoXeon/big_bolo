#include "listview.h"

ListView::ListView(QWidget* parent) : QListView(parent){};

ListView::~ListView(){};

void ListView::mousePressEvent(QMouseEvent* event) {
  // 重写鼠标事件，允许点击空白处已选组件状态失效
  QModelIndex index = this->indexAt(event->pos());
  if (!index.isValid()) {
    // 点击到空白处
    setCurrentIndex(index);
  } else {
    QListView::mousePressEvent(event);
  }
};
