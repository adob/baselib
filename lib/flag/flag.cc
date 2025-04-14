#include "flag.h"

using namespace lib;
using namespace lib::flag;

std::function<void()> flag::usage = []{
    // fmt.Fprintf(CommandLine.Output(), "Usage of %s:\n", os.Args[0])
	// PrintDefaults()
};

bool flag::parsed() {
    return true;
}

void lib::flag::parse() {

}
