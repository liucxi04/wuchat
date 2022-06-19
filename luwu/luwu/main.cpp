#include "application.h"

int main(int argc, char** argv) {
    liucxi::Application app;
    if(app.init(argc, argv)) {
        std::cout << "init success" << std::endl;
        return app.run();
    }
    return 0;
}

/*
 * make -j
 * sh move.sh
 *
 * */