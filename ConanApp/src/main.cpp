#include <QtGui/QApplication>
#include "Conan.h"
#include "TestClasses.h"

int main (int argc, char *argv [])
{
    QApplication app (argc, argv);
    app.connect (&app, SIGNAL (lastWindowClosed ()), &app, SLOT (quit ()));

    //Q_INIT_RESOURCE (Conan);

    bool demo = true;

    if (demo) {
        conan::ConanWidget conan;
        conan.AddRootObject (&conan);
        conan.show ();
        return app.exec();
    }
    else /*test*/ {
        conan::AboutDialog about;
        about.show();

        conan::ConanWidget conan;
        conan::test::Hierarchy test ("test");
        conan.AddRootObject (&test);
        conan.show ();
        {
            conan::test::Hierarchy deleted ("deleted");
            conan.AddRootObject (&deleted);
        }
        return app.exec();
    }
}
