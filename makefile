.SILENT:



Cible:	Administrateur Client Serveur CreationBD BidonFichierPub Publicite Consultation Modification 


Administrateur:	mainAdmin.o windowadmin.o moc_windowadmin.o
	echo "Creation de Administrateur"
	g++  -o Administrateur mainAdmin.o windowadmin.o moc_windowadmin.o   /usr/lib64/libQt5Widgets.so /usr/lib64/libQt5Gui.so /usr/lib64/libQt5Core.so /usr/lib64/libGL.so -lpthread

mainAdmin.o:	mainAdmin.cpp
	echo "Creation de mainAdmin.o"
	g++ -c -pipe -g -std=gnu++11 -Wall -W -D_REENTRANT -fPIC -DQT_DEPRECATED_WARNINGS -DQT_QML_DEBUG -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB -I../Administrateur -I. -isystem /usr/include/qt5 -isystem /usr/include/qt5/QtWidgets -isystem /usr/include/qt5/QtGui -isystem /usr/include/qt5/QtCore -I. -I. -I/usr/lib64/qt5/mkspecs/linux-g++ -o mainAdmin.o mainAdmin.cpp

windowadmin.o:	windowadmin.cpp
	echo "Creation de windowadmin.o"
	g++ -c -pipe -g -std=gnu++11 -Wall -W -D_REENTRANT -fPIC -DQT_DEPRECATED_WARNINGS -DQT_QML_DEBUG -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB -I../Administrateur -I. -isystem /usr/include/qt5 -isystem /usr/include/qt5/QtWidgets -isystem /usr/include/qt5/QtGui -isystem /usr/include/qt5/QtCore -I. -I. -I/usr/lib64/qt5/mkspecs/linux-g++ -o windowadmin.o windowadmin.cpp

moc_windowadmin.o: moc_windowadmin.cpp
	echo "Creation de moc_windowadmin.o"
	g++ -c -pipe -g -std=gnu++11 -Wall -W -D_REENTRANT -fPIC -DQT_DEPRECATED_WARNINGS -DQT_QML_DEBUG -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB -I../Administrateur -I. -isystem /usr/include/qt5 -isystem /usr/include/qt5/QtWidgets -isystem /usr/include/qt5/QtGui -isystem /usr/include/qt5/QtCore -I. -I. -I/usr/lib64/qt5/mkspecs/linux-g++ -o moc_windowadmin.o moc_windowadmin.cpp


Client:	dialogmodification.o mainClient.o windowclient.o moc_dialogmodification.o moc_windowclient.o FichierUtilisateur.o
	echo "Creation de Client"
	g++  -o Client FichierUtilisateur.o dialogmodification.o mainClient.o windowclient.o moc_dialogmodification.o moc_windowclient.o   /usr/lib64/libQt5Widgets.so /usr/lib64/libQt5Gui.so /usr/lib64/libQt5Core.so /usr/lib64/libGL.so -lpthread

dialogmodification.o:	dialogmodification.cpp
	echo "Creation de dialogmodification.o"
	g++ -c -pipe -g -std=gnu++11 -Wall -W -D_REENTRANT -fPIC -DQT_DEPRECATED_WARNINGS -DQT_QML_DEBUG -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB -I../UNIX_DOSSIER_FINAL -I. -isystem /usr/include/qt5 -isystem /usr/include/qt5/QtWidgets -isystem /usr/include/qt5/QtGui -isystem /usr/include/qt5/QtCore -I. -I. -I/usr/lib64/qt5/mkspecs/linux-g++ -o dialogmodification.o dialogmodification.cpp

mainClient.o:	mainClient.cpp
	echo "Creation de mainClient.o"
	g++ -c -pipe -g -std=gnu++11 -Wall -W -D_REENTRANT -fPIC -DQT_DEPRECATED_WARNINGS -DQT_QML_DEBUG -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB -I../UNIX_DOSSIER_FINAL -I. -isystem /usr/include/qt5 -isystem /usr/include/qt5/QtWidgets -isystem /usr/include/qt5/QtGui -isystem /usr/include/qt5/QtCore -I. -I. -I/usr/lib64/qt5/mkspecs/linux-g++ -o mainClient.o mainClient.cpp

windowclient.o:	windowclient.cpp
	echo "Creation de windowclient.o"
	g++ -c -pipe -g -std=gnu++11 -Wall -W -D_REENTRANT -fPIC -DQT_DEPRECATED_WARNINGS -DQT_QML_DEBUG -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB -I../UNIX_DOSSIER_FINAL -I. -isystem /usr/include/qt5 -isystem /usr/include/qt5/QtWidgets -isystem /usr/include/qt5/QtGui -isystem /usr/include/qt5/QtCore -I. -I. -I/usr/lib64/qt5/mkspecs/linux-g++ -o windowclient.o windowclient.cpp

moc_dialogmodification.o:	moc_dialogmodification.cpp
	echo "Creation de moc_dialogmodification.o"
	g++ -c -pipe -g -std=gnu++11 -Wall -W -D_REENTRANT -fPIC -DQT_DEPRECATED_WARNINGS -DQT_QML_DEBUG -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB -I../UNIX_DOSSIER_FINAL -I. -isystem /usr/include/qt5 -isystem /usr/include/qt5/QtWidgets -isystem /usr/include/qt5/QtGui -isystem /usr/include/qt5/QtCore -I. -I. -I/usr/lib64/qt5/mkspecs/linux-g++ -o moc_dialogmodification.o moc_dialogmodification.cpp

moc_windowclient.o:	moc_windowclient.cpp
	echo "Creation de moc_windowclient.o"
	g++ -c -pipe -g -std=gnu++11 -Wall -W -D_REENTRANT -fPIC -DQT_DEPRECATED_WARNINGS -DQT_QML_DEBUG -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB -I../UNIX_DOSSIER_FINAL -I. -isystem /usr/include/qt5 -isystem /usr/include/qt5/QtWidgets -isystem /usr/include/qt5/QtGui -isystem /usr/include/qt5/QtCore -I. -I. -I/usr/lib64/qt5/mkspecs/linux-g++ -o moc_windowclient.o moc_windowclient.cpp


Serveur:	Serveur.cpp FichierUtilisateur.cpp
	echo "Creation de Serveur"
	g++ Serveur.cpp -o Serveur FichierUtilisateur.o -I/usr/include/mysql -m64 -L/usr/lib64/mysql -lmysqlclient -lpthread -lz -lm -lrt -lssl -lcrypto -ldl


CreationBD:	CreationBD.cpp
	echo "Creation de CreationBD"
	g++ -o CreationBD CreationBD.cpp -I/usr/include/mysql -m64 -L/usr/lib64/mysql -lmysqlclient -lpthread -lz -lm -lrt -lssl -lcrypto -ldl


BidonFichierPub:	BidonFichierPub.cpp
	echo "Creation de BidonFichierPub"
	g++ -o BidonFichierPub BidonFichierPub.cpp


Publicite:	Publicite.cpp
	echo "Creation de Publicite"
	g++ -o Publicite Publicite.cpp


Consultation:	Consultation.cpp
	echo "Creation de Consultation"
	g++ Consultation.cpp -o Consultation -I/usr/include/mysql -m64 -L/usr/lib64/mysql -lmysqlclient -lpthread -lz -lm -lrt -lssl -lcrypto -ldl


Modification:	Modification.cpp FichierUtilisateur.cpp
	echo "Creation de Modification"
	g++ Modification.cpp -o Modification FichierUtilisateur.o -I/usr/include/mysql -m64 -L/usr/lib64/mysql -lmysqlclient -lpthread -lz -lm -lrt -lssl -lcrypto -ldl

FichierUtilisateur:	FichierUtilisateur.cpp
	echo "Creation de FichierUtilisateur"
	g++ FichierUtilisateur.cpp -o FichierUtilisateur -I/usr/include/mysql -m64 -L/usr/lib64/mysql -lmysqlclient -lpthread -lz -lm -lrt -lssl -lcrypto -ldl


clean2:
	clear
	ipcrm -Q 0x4d2

clean:	
	rm -f *.o core

clobber:	clean
	rm -f Administrateur Client
	ipcrm -Q 0x4d2