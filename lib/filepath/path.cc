#include "path.h"
#include "lib/strings/strings.h"

using namespace lib;
using namespace filepath;

namespace lib::filepath {
    bool is_path_separator(char c);
    int volume_name_len(str path);
}

str filepath::base(str path) {
    if (path == "") {
		return ".";
	}

	// Strip trailing slashes.
	while (len(path) > 0 && is_path_separator(path[len(path)-1])) {
		path = path[0, len(path)-1];
	}
	// Throw away volume name
	path = path+len(volume_name(path));
	// Find the last element
	size i = len(path) - 1;
	while (i >= 0 && !is_path_separator(path[i])) {
		i--;
	}

	if (i >= 0) {
		path = path+(i+1);
	}
    
	// If empty now, it had only slashes.
	if (path == "") {
		return path[0,1];
	}

	return path;
}

bool filepath::is_path_separator(char c) {
    return Separator == c;
}

str filepath::volume_name(str path) {
    //return FromSlash(path[0,volume_name_len(path)]);
    return path[0,volume_name_len(path)];
}

int filepath::volume_name_len(str /*path*/) {
    return 0;
}


String filepath::from_slash(str path) {
    if (Separator == '/') {
		return path;
	}

	return strings::replace_all(path, '/', Separator);
}