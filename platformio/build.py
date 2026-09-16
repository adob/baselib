"""Locate buildtool and register explicit application module requests."""

Import("env")

# Boost component repositories contain configuration probes and negative tests,
# not firmware sources. Keep these dependencies header-only in PlatformIO.
from pathlib import Path
from typing import Any
from platformio.package.manager.library import LibraryPackageManager
from SCons.Script import DefaultEnvironment

boost_roots = set()
fmt_roots = set()
for storage in env.GetLibSourceDirs():
    for package in LibraryPackageManager(storage).get_installed():
        if package.metadata.name == 'fmt':
            fmt_roots.add(Path(package.path).resolve())
        if package.metadata.name in {
            'boost_assert', 'boost_circular_buffer', 'boost_concept_check',
            'boost_config', 'boost_core', 'boost_move', 'boost_preprocessor',
            'boost_static_assert', 'boost_throw_exception', 'boost_type_traits',
        }:
            boost_roots.add(Path(package.path).resolve())


def filter_dependency_sources(buildenv: Any, node: Any) -> Any:
    """Filter a source node: Boost is header-only; fmt uses only format.cc."""
    path = Path(node.srcnode().abspath).resolve()
    # fmt's module interface and optional OS layer are not needed on the MCU.
    for root in fmt_roots:
        if path.is_relative_to(root):
            return node if path == root / 'src/format.cc' else None
    return None if any(path.is_relative_to(root) for root in boost_roots) else node


DefaultEnvironment().AddBuildMiddleware(filter_dependency_sources)

if env.GetProjectOption('custom_buildtool_modules', '').strip():
    Import("projenv")
    import sys

    from platformio.package.meta import PackageSpec

    # Resolve through the package manager, including symlink dependencies;
    # GetLibBuilders() is incomplete while this library script is loading.
    checkout = None
    requested = 'buildtool'
    for dependency in env.GetProjectOption('lib_deps', []):
        spec = PackageSpec(dependency)
        if spec.name == 'buildtool':
            requested = spec
            break
    for storage in env.GetLibSourceDirs():
        package = LibraryPackageManager(storage).get_package(requested)
        if package:
            checkout = Path(package.path)
            break
    if checkout is None:
        raise RuntimeError('buildtool dependency is missing; run pio pkg install')
    if not (checkout / 'platformio_adapter.py').is_file():
        raise RuntimeError(f'buildtool at {checkout} lacks the PlatformIO adapter; update the dependency')
    sys.path.insert(0, str(checkout))
    from platformio_adapter import configure
    configure(env, projenv, Path(env.Dir('.').srcnode().abspath).parent)
