//
// Created by liucxi on 2022/4/18.
//
#include "luwu/macro.h"
#include "luwu/utils.h"
#include <cassert>
#include <iostream>

void test_assert() {
//    std::cout << liucxi::BacktraceToString(10, 2, "    ") << std::endl;
    LUWU_ASSERT2(false, "abc");
}
int main(int argc, char *argv[]) {
    test_assert();
    return 0;
}
