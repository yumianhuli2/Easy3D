#include "main_window.h"

#include <string>
#include <iostream>
#include <stdexcept>

#include <QDir>
#include <QTime>
#include <QApplication>
#include <QSplashScreen>
#include <QSurfaceFormat>
#include <QStyleFactory>

#include <easy3d/util/file.h>
#include <easy3d/util/logging.h>
#include <easy3d/viewer/setting.h>


int main(int argc, char *argv[])
{
    // Note: Calling QSurfaceFormat::setDefaultFormat() before constructing the
    //       QApplication instance is mandatory on some platforms(for example, macOS)
    //       when an OpenGL core profile context is requested. This is to ensure
    //       that resource sharing between contexts stays functional as all internal
    //       contexts are created using the correct version and profile.
    QSurfaceFormat format = QSurfaceFormat::defaultFormat();
    format.setVersion(4, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setSamples(4);
#ifndef NDEBUG
    format.setOption(QSurfaceFormat::DebugContext);
#endif
    QSurfaceFormat::setDefaultFormat(format);

    // Commented to let Qt choose the most suitable OpenGL implementation
	//QApplication::setAttribute(Qt::AA_UseDesktopOpenGL);

	QApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
	QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
#if (QT_VERSION >= QT_VERSION_CHECK(5, 7, 0))
//    QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
#endif

    QApplication app(argc, argv);
#ifdef _WIN32   // to have similar style as on macOS.
    app.setStyle(QStyleFactory::create("Fusion"));
#endif

     QDir workingDir = QCoreApplication::applicationDirPath();
#ifdef __APPLE__
    // This makes sure that our "working directory" is not within the application bundle
    if ( workingDir.dirName() == "MacOS" ) {
        workingDir.cdUp();
        workingDir.cdUp();
        workingDir.cdUp();
    }
#endif
    QDir::setCurrent(workingDir.absolutePath());

    // Initialize Google's logging library.
    const std::string dir = workingDir.absolutePath().toStdString();
    const std::string log_dir = dir + "/logs/";
    if (!easy3d::file::is_directory(log_dir))
        easy3d::file::create_directory(log_dir);
    google::SetLogDestination(google::GLOG_INFO, log_dir.c_str());
    google::SetLogFilenameExtension("Mapple_log-");
#ifndef NDEBUG
    FLAGS_stderrthreshold = google::GLOG_INFO;
#else
    FLAGS_stderrthreshold = google::GLOG_WARNING;
#endif
    FLAGS_colorlogtostderr = true;
    google::InitGoogleLogging(argv[0]);
    LOG(INFO) << "Current working directory: " << dir;

#ifdef NDEBUG
    // splash screen
    const std::string file = easy3d::setting::resource_directory() + "/images/splash.png";
    QPixmap pixmap(QString::fromStdString(file));
    QSplashScreen splash(pixmap, Qt::WindowStaysOnTopHint);
    QTime splashTimer;
    splashTimer.start();
    splash.show();
    splash.showMessage("  Starting Mapple...");
    QApplication::processEvents();

    //we want the splash screen to be visible for a minimum amount of time
    while (splashTimer.elapsed() < 200) {   // in milliseconds
        splash.raise();
        QApplication::processEvents(); //to let the system breath!
    }
#endif

    int status = 0;
    try {
        MainWindow win;
        win.show();

#ifdef NDEBUG
        splash.finish(&win);
        QApplication::processEvents();
#endif

        status = app.exec();
    }
    catch (const std::exception& e) {
        const std::string msg =
                std::string("Oh sorry, Mapple crashed.\n") +
                "Error message: " + e.what() + ".\n"
                "Please contact me (liangliang.nan@gmail.com) for more information.";
        LOG(ERROR) << msg;
    }

    return status;
}