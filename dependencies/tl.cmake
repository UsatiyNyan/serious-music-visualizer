set(OPTIONAL_BUILD_TESTS OFF)
set(OPTIONAL_BUILD_PACKAGE OFF)
cpmaddpackage(
        NAME tl-optional
        GIT_REPOSITORY "git@github.com:TartanLlama/optional.git"
        GIT_TAG v1.1.0)
unset(OPTIONAL_BUILD_TESTS)
unset(OPTIONAL_BUILD_PACKAGE)

set(EXPECTED_BUILD_TESTS OFF)
set(EXPECTED_BUILD_PACKAGE OFF)
cpmaddpackage(
        NAME tl-expected
        GIT_REPOSITORY "git@github.com:TartanLlama/expected.git"
        GIT_TAG v1.1.0)
unset(EXPECTED_BUILD_TESTS)
unset(EXPECTED_BUILD_PACKAGE)
