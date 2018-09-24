#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""The setup script."""

import sys
from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext
from setuptools import distutils


# pybind11 related.
class Pybind11IncludePath(object):
    """Helper class to determine the pybind11 include path
    The purpose of this class is to postpone importing pybind11
    until it is actually installed, so that the ``get_include()``
    method can be invoked. """

    def __init__(self, user=False):
        self.user = user

    def __str__(self):
        import pybind11
        return pybind11.get_include(self.user)


ext_modules = [
        Extension(
                'cnt.sam._sam_impl',
                [
                        'cnt/sam/cpp11/sam.cc',
                        'cnt/sam/cpp11/main.cc',
                ],
                include_dirs=[
                        # Path to pybind11 headers
                        Pybind11IncludePath(),
                        Pybind11IncludePath(user=True),
                        'cnt/sam',
                ],
                language='c++'),
]


# As of Python 3.6, CCompiler has a `has_flag` method.
# cf http://bugs.python.org/issue26689
def has_flag(compiler, flagname):
    """Return a boolean indicating whether a flag name is supported on
    the specified compiler.
    """
    import tempfile
    with tempfile.NamedTemporaryFile('w', suffix='.cpp') as f:
        f.write('int main (int argc, char **argv) { return 0; }')
        try:
            compiler.compile([f.name], extra_postargs=[flagname])
        except distutils.errors.CompileError:
            return False
    return True


def cpp_flag(compiler):
    """Return the -std=c++[11/14] compiler flag.
    The c++14 is prefered over c++11 (when it is available).
    """
    if has_flag(compiler, '-std=c++14'):
        return '-std=c++14'
    elif has_flag(compiler, '-std=c++11'):
        return '-std=c++11'
    else:
        raise RuntimeError('Unsupported compiler -- at least C++11 support ' 'is needed!')


class BuildExt(build_ext):
    """A custom build extension for adding compiler-specific options."""
    c_opts = {
            'msvc': ['/EHsc'],
            'unix': [],
    }

    if sys.platform == 'darwin':
        c_opts['unix'] += ['-stdlib=libc++', '-mmacosx-version-min=10.7']

    def build_extensions(self):
        ct = self.compiler.compiler_type
        opts = self.c_opts.get(ct, [])
        if ct == 'unix':
            opts.append('-DVERSION_INFO="%s"' % self.distribution.get_version())
            opts.append(cpp_flag(self.compiler))
            if has_flag(self.compiler, '-fvisibility=hidden'):
                opts.append('-fvisibility=hidden')
        elif ct == 'msvc':
            opts.append('/DVERSION_INFO=\\"%s\\"' % self.distribution.get_version())
        for ext in self.extensions:
            ext.extra_compile_args = opts
        build_ext.build_extensions(self)


def load_requirements(path):
    with open(path) as fin:
        return [
                line for line in map(lambda l: l.strip(), fin.readlines())
                if line and not line.startswith('#')
        ]


with open('README.rst') as readme_file:
    readme = readme_file.read()

with open('HISTORY.rst') as history_file:
    history = history_file.read()

requirements = load_requirements('requirements_prod.txt')
test_requirements = load_requirements('requirements_dev.txt')

setup(
        author="Hunt Zhan",
        author_email='huntzhan.dev@gmail.com',
        classifiers=[
                'Development Status :: 2 - Pre-Alpha',
                'Intended Audience :: Developers',
                'License :: OSI Approved :: MIT License',
                'Natural Language :: English',
                'Programming Language :: Python :: 3',
                'Programming Language :: Python :: 3.6',
                'Programming Language :: Python :: 3.7',
        ],
        description="None",
        install_requires=requirements,
        license="MIT license",
        long_description=readme + '\n\n' + history,
        include_package_data=True,
        keywords='cnt.sam',
        name='cnt.sam',
        packages=['cnt.sam'],
        test_suite='tests',
        tests_require=test_requirements,
        url='https://github.com/cnt-dev/cnt.sam',
        version='0.1.2',
        zip_safe=False,
        # pybind11.
        ext_modules=ext_modules,
        cmdclass={'build_ext': BuildExt},
)
