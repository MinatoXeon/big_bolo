#ifndef LISTVIEW_H
#define LISTVIEW_H

#include <QListView>
#include <QMouseEvent>

class ListView : public QListView {
  Q_OBJECT

 public:
  ListView(QWidget* parent = 0);
  ~ListView();

 private:
  void mousePressEvent(QMouseEvent* event);
};

#endif  // LISTVIEW_H
