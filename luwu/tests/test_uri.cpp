

#include "luwu/uri.h"
#include <iostream>

int main(int argc, char * argv[]) {
    auto uri = liucxi::URI::Create("http://a:b@host.com/p/a/t/h?query=string#hash");
    std::cout << uri->toString() << std::endl;
    auto addr = uri->createAddress();
    std::cout << *addr << std::endl;
    return 0;
}