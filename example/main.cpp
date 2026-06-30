#include <QApplication>

#include "testWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    testWindow window;
    // window.resize(1024, 768);
    // window.show();
    window.showMaximized();
    return QApplication::exec();
}
