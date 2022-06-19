//
// Created by liucxi on 2022/6/16.
//

#include "luwu/application.h"
#include "luwu/config.h"

int main(int argc, char **argv) {
    liucxi::Application app;
    if (app.init(argc, argv)) {
        return app.run();
    }
    return 0;
}