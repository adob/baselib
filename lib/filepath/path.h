#pragma once

#include "lib/str.h"

namespace lib::filepath {
    const char Separator     = '/'; // OS-specific path separator
	const char ListSeparator = ':'; // OS-specific path list separator)

    // Base returns the last element of path.
    // Trailing path separators are removed before extracting the last element.
    // If the path is empty, Base returns ".".
    // If the path consists entirely of separators, Base returns a single separator.
    str base(str path);

    // VolumeName returns leading volume name.
    // Given "C:\foo\bar" it returns "C:" on Windows.
    // Given "\\host\share\foo" it returns "\\host\share".
    // On other platforms it returns "".
    str volume_name(str path);

    // FromSlash returns the result of replacing each slash ('/') character
    // in path with a separator character. Multiple slashes are replaced
    // by multiple separators.
    //
    // See also the Localize function, which converts a slash-separated path
    // as used by the io/fs package to an operating system path.
    String from_slash(str path);
}