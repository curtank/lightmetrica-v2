
Lightmetrica
====================

Lightmetrica : A modern, research-oriented renderer

Project site: http://lightmetrica.org

Build
--------------------

> QTDIR="D:\Qt\Qt5.4.2\5.4\msvc2013_64_opengl" BOOST_ROOT="" BOOST_INCLUDEDIR="D:\boost\boost_1_59_0" BOOST_LIBRARYDIR="D:\boost\boost_1_59_0\lib64-msvc-14.0" TB B_ROOT="D:\tbb\tbb43_20150611oss" TBB_ARCH_PLATFORM="intel64" cmake -G "Visual Studio 14 2015 Win64" ..

> BOOST_ROOT="" BOOST_INCLUDEDIR="D:\boost\boost_1_59_0" BOOST_LIBRARYDIR="D:\boost\boost_1_59_0\lib64-msvc-14.0" cmake -G "Visual Studio 14 2015 Win64" ..



Recommended practices (C++)
--------------------

This section explains some coding practices specific to this project.
We introcues some coding practices necessary to implement
robust application suitable to the *research-oriented* application.
Some topics says the coding convension (as [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html), which basically depends on)
and some says useful coding practices for extending or reading the implementation.

The practices are based on our experience of developing research-oriented applications. 
Some practices might be uncommon for usual software development,
however we find them effective for constructing research-oriented applications.
Although they are not mandatory, 
we recommend to follow the practices described in this section.

### File name

- The file name should be separated by underbar (_).
    + e.g., `some_header_name.h`

### Project name

- The project name should be separated by hyphen (-).
    + e.g., `lightmetrica-test`

### File headers

- All header and implementation file should include the license header.
- The header file should begin with `#pragma region` and should *not* include  addtional include guards.
    + Rationale
        * Most modern compilers support `#pragma region`.
        * Lack of `#endif` in traditional include guards causes tricky bugs.

### Macro

- All macros should begin with `LM_` prefix
    + e.g., `LM_SAFE_DELETE`

### Namespace

- All application specic implementation should be wrapped with `LM_NAMESPACE_BEGIN` and `LM_NAMESPACE_END` macros

### Header files

- Headers files must not be order-dependent, so that the user of the header do not have to care about the include order.
    + That is, include all dependencies in the header files

### Commenting

- Use `#pragma region` as possible
    + To explain the code structure hierarchically
    + cf. [Literate programming](https://en.wikipedia.org/wiki/Literate_programming).

### Code organization

This section contains some *uncommon* practices.
The main motto for the following practice to increase the *locality*
of the implementation.

- Avoid to create a function that is called only once
    + Instead, use scope wrapped by `#pragma region` and `#pragma endregion`
- Prefer *local lambda function* to *private member function*

### Optimization

This is a topic specific to the `research-oriented` application.

- The implementation should not be optimized from the first time
- If you are to optimize the application, be sure to implement 2 versions and check consistency between two results, and measure performance improvement.
- Minimize the number of states or member variables
    + You should not introduce unnecessary state variables for a small optimization
    + Specially in the non-optimized version, the number of cached values should be zero.




Recommended practices (CMake)
--------------------

We also have the recommneded practice for writing CMake scripts.

### Variables

- The local variable should begin with underbar (_).
    + e.g., ``_SOURCE_FILES``

### Project and library/executable name

- The project and library/executable names should be accessed via variables
    + For project name, use `${PROJECT_NAME}`
    + For library or executable name, define and use local variable `${_PROJECT_NAME}`
        * e.g.,
            - `set(_PROJECT_NAME "lightmetrica-test")`
            - `pch_add_executable(${_PROJECT_NAME} ...)`