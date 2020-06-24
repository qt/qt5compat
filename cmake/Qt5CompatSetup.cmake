function(qt_run_config_test_posix_iconv)
    set(source "#include <iconv.h>

int main(int, char **)
{
    iconv_t x = iconv_open(\"\", \"\");
    char *inp;
    char *outp;
    size_t inbytes, outbytes;
    iconv(x, &inp, &inbytes, &outp, &outbytes);
    iconv_close(x);
    return 0;
}")
    check_cxx_source_compiles("${source}" HAVE_POSIX_ICONV)

    if(NOT HAVE_POSIX_ICONV)
        set(_req_libraries "${CMAKE_REQUIRE_LIBRARIES}")
        set(CMAKE_REQUIRE_LIBRARIES "iconv")
        check_cxx_source_compiles("${source}" HAVE_POSIX_ICONV)
        set(CMAKE_REQUIRE_LIBRARIES "${_req_libraries}")
        if(HAVE_POSIX_ICONV)
            set(TEST_iconv_needlib 1 CACHE INTERNAL "Need to link against libiconv")
        endif()
    endif()

    set(TEST_posix_iconv "${HAVE_POSIX_ICONV}" CACHE INTERNAL "POSIX iconv")
endfunction()


function(qt_run_config_test_sun_iconv)
    set(source "#include <iconv.h>

int main(int, char **)
{
    iconv_t x = iconv_open(\"\", \"\");
    const char *inp;
    char *outp;
    size_t inbytes, outbytes;
    iconv(x, &inp, &inbytes, &outp, &outbytes);
    iconv_close(x);
    return 0;
}")
    if(DARWIN)
        # as per !config.darwin in configure.json
        set(HAVE_SUN_ICONV OFF)
    else()
        check_cxx_source_compiles("${source}" HAVE_SUN_ICONV)
    endif()

    set(TEST_sun_iconv "${HAVE_SUN_ICONV}" CACHE INTERNAL "SUN libiconv")
endfunction()

function(qt_run_qt5compat_config_tests)
    qt_run_config_test_posix_iconv()

    add_library(Iconv INTERFACE)
    if(TEST_iconv_needlib)
       target_link_libraries(Iconv PUBLIC iconv)
    endif()

    if(NOT TEST_posix_iconv)
        qt_run_config_test_sun_iconv()
    endif()
endfunction()
qt_run_qt5compat_config_tests()
