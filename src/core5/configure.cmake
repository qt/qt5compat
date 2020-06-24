

#### Inputs

# input iconv
set(INPUT_iconv "undefined" CACHE STRING "")
set_property(CACHE INPUT_iconv PROPERTY STRINGS undefined no yes posix sun gnu)



#### Libraries
if(NOT TARGET ICU::i18n)
    qt_find_package(ICU COMPONENTS i18n uc data PROVIDED_TARGETS ICU::i18n ICU::uc ICU::data MODULE_NAME qt5compat QMAKE_LIB icu)
endif()

#### Tests



#### Features

qt_feature("iconv" PUBLIC PRIVATE
    SECTION "Internationalization"
    LABEL "iconv"
    PURPOSE "Provides internationalization on Unix."
    CONDITION NOT QT_FEATURE_icu AND QT_FEATURE_textcodec AND ( TEST_posix_iconv OR TEST_sun_iconv )
)
qt_feature_definition("iconv" "QT_NO_ICONV" NEGATE VALUE "1")
qt_feature("posix-libiconv" PRIVATE
    LABEL "POSIX iconv"
    CONDITION NOT WIN32 AND NOT QNX AND NOT ANDROID AND NOT APPLE AND TEST_posix_iconv AND TEST_iconv_needlib
    ENABLE TEST_posix_iconv AND TEST_iconv_needlib
    DISABLE NOT TEST_posix_iconv OR NOT TEST_iconv_needlib
)
qt_feature("sun-libiconv"
    LABEL "SUN iconv"
    CONDITION NOT WIN32 AND NOT QNX AND NOT ANDROID AND NOT APPLE AND TEST_sun_iconv
    ENABLE TEST_sun_iconv
    DISABLE NOT TEST_sun_iconv
)
qt_feature("gnu-libiconv" PRIVATE
    LABEL "GNU iconv"
    CONDITION NOT WIN32 AND NOT QNX AND NOT ANDROID AND NOT APPLE AND TEST_posix_iconv AND NOT TEST_iconv_needlib
    ENABLE TEST_posix_iconv AND NOT TEST_iconv_needlib
    DISABLE NOT TEST_posix_iconv OR TEST_iconv_needlib
)
qt_feature("textcodec" PUBLIC
    SECTION "Internationalization"
    LABEL "QTextCodec"
    PURPOSE "Supports conversions between text encodings."
)
qt_feature_definition("textcodec" "QT_NO_TEXTCODEC" NEGATE VALUE "1")
qt_feature("codecs" PUBLIC
    SECTION "Internationalization"
    LABEL "Codecs"
    PURPOSE "Supports non-unicode text conversions."
    CONDITION QT_FEATURE_textcodec
)
qt_feature_definition("codecs" "QT_NO_CODECS" NEGATE VALUE "1")
qt_feature("big_codecs" PUBLIC
    SECTION "Internationalization"
    LABEL "Big Codecs"
    PURPOSE "Supports big codecs, e.g. CJK."
    CONDITION QT_FEATURE_textcodec
)
qt_feature_definition("big_codecs" "QT_NO_BIG_CODECS" NEGATE VALUE "1")
qt_configure_add_summary_section(NAME "Qt 5 Compatibility Libraries")
qt_configure_add_summary_entry(ARGS "iconv")
qt_configure_end_summary_section() # end of "Qt 5 Compatibility Libraries" section
