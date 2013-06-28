# How to compile qtsparkle #

    mkdir bin
    cd bin
    cmake ..
    make


## Arguments to cmake ##

* -DCMAKE_BUILD_TYPE=[Release|Debug]
  A Release library will be smaller but won't contain any debugging symbols.
* -DBUILD_SHARED_LIBS=[ON|OFF]
  Set this to ON to produce a .dll or .so, OFF to produce a .a.  Defaults to
  OFF.
* -DCMAKE_TOOLCHAIN_FILE=path
  Useful when cross-compiling.  See below for a toolchain file for using
  mingw32 on Linux.
* -DQT_HEADERS_DIR=path
* -DQT_LIBRARY_DIR=path
  Useful when cross compiling or if cmake can't find your Qt installation.


## mingw32 CMake toolchain file ##

Put the following in a .cmake file and point to it with -DCMAKE_TOOLCHAIN_FILE
when compiling.

    # the name of the target operating system
    SET(CMAKE_SYSTEM_NAME Windows)

    # which compilers to use for C and C++
    SET(CMAKE_C_COMPILER i586-mingw32msvc-gcc)
    SET(CMAKE_CXX_COMPILER i586-mingw32msvc-g++)

    # here is the target environment located
    SET(CMAKE_FIND_ROOT_PATH  /target )


# How to use qtsparkle from your application #

See the exampleapp folder for a basic Qt application that uses qtsparkle.
You should create a qtsparkle::Updater in your application's main window, it
will check for updates as soon as your application returns to the event loop.

    #include <qtsparkle/Updater>

    MainWindow::MainWindow() {
        qApp->setApplicationName("ExampleApp");
        qApp->setApplicationVersion("1.2");

        qtsparkle::Updater* updater = new qtsparkle::Updater(
            QUrl("http://www.example.com/sparkle.xml"), this);

        connect(ui_->check_for_updates, SIGNAL(clicked()),
                updater, SLOT(CheckNow());
    }
