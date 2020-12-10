#include <iostream>
#include "FluxArc/FluxArc.hh"

struct Data
{
    int hello;
};

int main()
{
    auto arc = FluxArc::Archive("test.farc");
    Data d;

    d.hello = 1;
    arc.setFile("uncompressed", (char *)&d, sizeof(d));
    d.hello = 2;
    arc.setFile("compressed", (char *)&d, sizeof(d), true);
    d.hello = 3;
    arc.setFile("compressed_release", (char *)&d, sizeof(d), true, true);

    arc.setFile("lipsum", std::string("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum."), true, true);

    auto same_arc = FluxArc::Archive("test.farc");

    Data dg;
    same_arc.getFile("uncompressed", (char *)&dg);
    std::cout << dg.hello << std::endl;
    same_arc.getFile("compressed", (char *)&dg);
    std::cout << dg.hello << std::endl;
    same_arc.getFile("compressed_release", (char *)&dg);
    std::cout << dg.hello << std::endl;

    std::cout << same_arc.getFile("lipsum") << std::endl;
    return 0;
}