#include "func.h" 

#include <fcntl.h>
#include <libelf.h>
#include <gelf.h>
#include <cxxabi.h>
#include <unistd.h>

using namespace lib;
using namespace lib::testing;
using namespace lib::testing::detail;

std::vector<FuncData> detail::get_all_funcs(error) {
    std::vector<FuncData> funcs;

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
        funcs.push_back(
            FuncData{.name = String(name), .ptr = (void *)sym.st_value});
        }
    }
    }

    elf_end(e);
    close(fd);

    return funcs;
}
String detail::demangle(str mangled) {
  int ok;
  String s;
  const char *demangled = abi::__cxa_demangle(mangled.c_str(), nil, nil, &ok);
  if (ok != 0) {
    return s;
  }

  s = str::from_c_str(demangled);
  free((void *)demangled);
  return s;
}
