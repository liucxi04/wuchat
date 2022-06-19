#ifndef LUWU_LIBRARY_H
#define LUWU_LIBRARY_H

#include <memory>
#include "module.h"

namespace liucxi {

    class Library {
    public:
        static Module::ptr GetModule(const std::string &path);
    };

}

#endif
