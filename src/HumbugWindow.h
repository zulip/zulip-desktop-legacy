#ifndef HUMBUGWINDOW_H
#define HUMBUGWINDOW_H

#include <QMainWindow>

namespace Ui {
class HumbugWindow;
}

class HumbugWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit HumbugWindow(QWidget *parent = 0);
    ~HumbugWindow();
    
private:
    Ui::HumbugWindow *ui;
};

#endif // HUMBUGWINDOW_H
