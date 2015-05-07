#ifndef WINDOW_H
#define WINDOW_H

#include <QDialog>
#include <QPlainTextEdit>

#include <QtGui>

class QComboBox;
class QDir;
class QLabel;
class QPushButton;

class Window : public QDialog
{
Q_OBJECT

public:
Window(QWidget *parent = 0);

private slots:

void closeWindow();

private:

QPushButton *acceptButton;
QPlainTextEdit *showProgress;

};

#endif
