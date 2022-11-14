#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QDebug>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QListView>
#include <QMainWindow>
#include <QProgressBar>
#include <QPushButton>
#include <QStandardItemModel>
#include <QVBoxLayout>
#include <QWidget>
#include <memory>

#include "bolo.h"
#include "itemdelegate.h"
#include "listview.h"
#include "password.h"

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  MainWindow(std::unique_ptr<bolo::Bolo> &&, QWidget *parent = 0);
  ~MainWindow();

 private:
  std::unique_ptr<bolo::Bolo> mybolo;
  QStandardItemModel *standard_item_model;
  ListView *list_view;
  ItemDelegate *itemdelegate;
  QHBoxLayout *sub_layout;
  QVBoxLayout *main_layout;

  QPushButton new_file;
  QLabel title;
  QFileDialog file_window;
  QProgressBar progressbar;
  PassWord password_window;

  void set_progressbar();

 public slots:
  void Show_FileWindow();
  void Add_NewFile(QString file_path);
  void Show_FileDetail(const QModelIndex &index);

 public:
  void dropEvent(QDropEvent *event);
  void dragEnterEvent(QDragEnterEvent *event);
  void InitData();

 signals:
  void Get_NewFile(QString);
};

#endif  // MAINWINDOW_H
