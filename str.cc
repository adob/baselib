#include "str.h"

using namespace lib;

std::string str::std_string() const {
    return std::string(data, len);
}

std::string String::std_string() const {
    return std::string((char*) buffer.data, length);
}

CString str::c_str() const {
    return CString(*this);
}

CString::CString(str s) {
    len = s.len;
    data = (char *) malloc(len+1);
    if (data == nil) {
        exceptions::out_of_memory();
    }
    memmove(data, s.data, s.len);
    data[len] = '\0';
}

CString::~CString() {
    free(data);
}
