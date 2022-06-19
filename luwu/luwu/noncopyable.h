//
// Created by liucxi on 2022/4/16.
//

#ifndef LUWU_NONCOPYABLE_H
#define LUWU_NONCOPYABLE_H

namespace liucxi {
    class Noncopyable {
    public:
        Noncopyable() = default;

        ~Noncopyable() = default;

        Noncopyable(const Noncopyable &) = delete;

        Noncopyable(const Noncopyable &&) = delete;

        Noncopyable &operator=(const Noncopyable &) = delete;
    };
}

#endif //LUWU_NONCOPYABLE_H
