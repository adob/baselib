#include <libelf.h>
#include <gelf.h>
#include <cxxabi.h>
#include <vector>
#include <dlfcn.h>

#include "lib/debug/debug.h"
#include "lib/fmt/fmt.h"
#include "lib/str.h"
#include "lib/strings/strings.h"
#include "testing.h"
#include "../strings.h"
#include "../base.h"
#include "../print.h"

using namespace lib;

struct TestFuncData {
    String name;
    void *ptr;
} ;

static std::vector<TestFuncData> get_all_funcs(error) {
    std::vector<TestFuncData> funcs;

    if (elf_version(EV_CURRENT) == EV_NONE) {
        fprintf(stderr, "ELF library initialization failed: %s\n", elf_errmsg(-1));
        exit(EXIT_FAILURE);
    }

    int fd = open("/proc/self/exe", O_RDONLY, 0);
    if (fd < 0) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    Elf *e = elf_begin(fd, ELF_C_READ, NULL);
    if (!e) {
        fprintf(stderr, "elf_begin failed: %s\n", elf_errmsg(-1));
        close(fd);
        exit(EXIT_FAILURE);
    }

    size_t shstrndx;
    if (elf_getshdrstrndx(e, &shstrndx) != 0) {
        fprintf(stderr, "elf_getshdrstrndx failed: %s\n", elf_errmsg(-1));
        elf_end(e);
        close(fd);
        exit(EXIT_FAILURE);
    }

    Elf_Scn *scn = NULL;
    GElf_Shdr shdr;
    while ((scn = elf_nextscn(e, scn)) != NULL) {
        if (gelf_getshdr(scn, &shdr) != &shdr) {
            fprintf(stderr, "gelf_getshdr failed: %s\n", elf_errmsg(-1));
            elf_end(e);
            close(fd);
            exit(EXIT_FAILURE);
        }

        if (shdr.sh_type == SHT_SYMTAB) {
            Elf_Data *data = elf_getdata(scn, NULL);
            if (!data) {
                fprintf(stderr, "elf_getdata failed: %s\n", elf_errmsg(-1));
                elf_end(e);
                close(fd);
                exit(EXIT_FAILURE);
            }

            size_t num_symbols = shdr.sh_size / shdr.sh_entsize;
            for (size_t i = 0; i < num_symbols; ++i) {
                GElf_Sym sym;
                if (gelf_getsym(data, i, &sym) != &sym) {
                    fprintf(stderr, "gelf_getsym failed: %s\n", elf_errmsg(-1));
                    continue;
                }

                const char *name = elf_strptr(e, shdr.sh_link, sym.st_name);
                funcs.push_back(TestFuncData {
                    .name = String(name), 
                    .ptr = (void*) sym.st_value}
                );
            }
        }
    }

    elf_end(e);
    close(fd);

    return funcs;
}

static String demangle(str mangled) {
    int ok;
    String s;
    const char *demangled = abi::__cxa_demangle(mangled.c_str(), nil, nil, &ok);
    if (ok != 0) {
        return s;
    }

    s = str::from_c_str(demangled);
    free((void*) demangled);
    return s;
}

static bool is_testing_func(str sym) {
    if (!strings::has_prefix(sym, "test_")) {
        return false;
    }

    //print "got", sym;

    // size paren_idx = strings::index(sym, '(');
    // if (paren_idx < 0) {
    //     return false;
    // }
    
    return strings::has_suffix(sym, "(lib::testing::T&)");
}

static std::vector<TestFuncData> get_test_funcs(error err) {
    std::vector<TestFuncData> test_funcs;

    std::vector<TestFuncData> all_funcs = get_all_funcs(err);
    if (err) return {};

    for (TestFuncData &data : all_funcs) {
        String demangled = demangle(data.name);

        if (is_testing_func(demangled)) {
            test_funcs.push_back(data);
        }
    }

    return test_funcs;
}


static void call_test_func(testing::T &t, TestFuncData &data) {
    using TestFunc = void (testing::T&);

    TestFunc *func = (TestFunc*) data.ptr;
    func(t);
}

static void print_fail(void *ptr) {
    debug::SymInfo info = debug::get_symbol_info(ptr);
    print "--- FAIL: %s in %s:%d" % info.demangled, info.filename, info.lineno;
}

int main() {
    debug::init();
    bool failed = false;
    
    std::vector<TestFuncData> test_funcs = get_test_funcs(error::panic);
    for (TestFuncData &data : test_funcs) {
        testing::T t;
        call_test_func(t, data);

        if (t.failed) {
            failed = true;
            print_fail(data.ptr);
            for (testing::T::ErrorInfo const& info : t.errors) {
                print "\t%s" % info.description;
                print info.backtrace;
            } 
        } else {
            print "ok";
        }
    }
    
    if (!failed) {
        print "PASS";
        return 1;
    }

    return 0;
}