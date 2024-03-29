TARGET  = qmlwebkitplugin
TARGETPATH = org/webkit

TEMPLATE = lib
CONFIG += qt plugin

win32|mac:!wince*:!win32-msvc:!macx-xcode:CONFIG += debug_and_release

QMLDIRFILE = $${_PRO_FILE_PWD_}/qmldir
copy2build.input = QMLDIRFILE
CONFIG(QTDIR_build) {
    copy2build.output = $$QT_BUILD_TREE/imports/$$TARGETPATH/qmldir
} else {
    copy2build.output = $$OUTPUT_DIR/imports/$$TARGETPATH/qmldir
}
!contains(TEMPLATE_PREFIX, vc):copy2build.variable_out = PRE_TARGETDEPS
copy2build.commands = $$QMAKE_COPY ${QMAKE_FILE_IN} ${QMAKE_FILE_OUT}
copy2build.name = COPY ${QMAKE_FILE_IN}
copy2build.CONFIG += no_link
QMAKE_EXTRA_COMPILERS += copy2build

TARGET = $$qtLibraryTarget($$TARGET)
contains(QT_CONFIG, reduce_exports):CONFIG += hide_symbols

wince*:LIBS += $$QMAKE_LIBS_GUI

symbian: {
    TARGET.EPOCALLOWDLLDATA=1
    TARGET.CAPABILITY = All -Tcb
    load(armcc_warnings)
}

include(../../../WebKit.pri)

QT += declarative

linux-* {
    # From Creator's src/rpath.pri:
    # Do the rpath by hand since it's not possible to use ORIGIN in QMAKE_RPATHDIR
    # this expands to $ORIGIN (after qmake and make), it does NOT read a qmake var.
    QMAKE_RPATHDIR = $$OUTPUT_DIR/lib $$QMAKE_RPATHDIR
    MY_RPATH = $$join(QMAKE_RPATHDIR, ":")

    QMAKE_LFLAGS += -Wl,-z,origin \'-Wl,-rpath,$${MY_RPATH}\'
    QMAKE_RPATHDIR =
} else {
    QMAKE_RPATHDIR = $$OUTPUT_DIR/lib $$QMAKE_RPATHDIR
}

SOURCES += qdeclarativewebview.cpp plugin.cpp
HEADERS += qdeclarativewebview_p.h

CONFIG(QTDIR_build) {
    DESTDIR = $$QT_BUILD_TREE/imports/$$TARGETPATH
} else {
    DESTDIR = $$OUTPUT_DIR/imports/$$TARGETPATH
}
target.path = $$[QT_INSTALL_IMPORTS]/$$TARGETPATH


qmldir.files += $$PWD/qmldir
qmldir.path +=  $$[QT_INSTALL_IMPORTS]/$$TARGETPATH

symbian:{
    load(data_caging_paths)
    include($$QT_SOURCE_TREE/demos/symbianpkgrules.pri)

    importFiles.sources = qmlwebkitplugin.dll qmldir
    importFiles.path = $$QT_IMPORTS_BASE_DIR/$$TARGETPATH

    DEPLOYMENT = importFiles
}

INSTALLS += target qmldir
