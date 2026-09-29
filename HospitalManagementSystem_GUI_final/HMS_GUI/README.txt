Hospital Management System - GUI edition
========================================
RUN (Windows):  double-click hms.exe   -> your browser opens the app.
                Keep the black window open while using it. Close it to stop.
                (Run it from inside this folder; data is saved in the data\ folder.)

REBUILD (if you change code):  double-click build.bat
        or:  g++ -std=c++17 -o hms.exe main.cpp -lws2_32
Linux/Mac:  g++ -std=c++17 -o hms main.cpp -lpthread && ./hms

If you edit web\index.html, run:  python web\make_page.py   then rebuild.

Logins:  admin01 / admin123   recep01 / recep123   dr.sharma / doc123   dr.rai / doc123
Reset all data: close the program and delete the data\ folder.
