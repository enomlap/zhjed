all:zhjed
#CFLAGS+=-D__z_DEBUG_ -O3 -Wall -g
CFLAGS+=-O3 -Wall -g
GTK_CFLAGS = -pthread -I/usr/include/gtk-2.0 -I/usr/lib/x86_64-linux-gnu/gtk-2.0/include -I/usr/include/gio-unix-2.0/ -I/usr/include/cairo -I/usr/include/pango-1.0 -I/usr/include/atk-1.0 -I/usr/include/cairo -I/usr/include/pixman-1 -I/usr/include/gdk-pixbuf-2.0 -I/usr/include/pango-1.0 -I/usr/include/harfbuzz -I/usr/include/pango-1.0 -I/usr/include/fribidi -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include -I/usr/include/uuid -I/usr/include/freetype2 -I/usr/include/libpng16
GTK_LIBS = -lgtk-x11-2.0 -lgdk-x11-2.0 -lpangocairo-1.0 -latk-1.0 -lcairo -lgdk_pixbuf-2.0 -lpangoft2-1.0 -lpango-1.0 -lgobject-2.0 -lglib-2.0 -lfontconfig -lfreetype
%.o: %.c
	cc ${CFLAGS} -c $< `pkg-config --cflags gtk+-2.0` ${GTK_CFLAGS}
zhjed.o: zhjed.c
	cc ${CFLAGS} -c zhjed.c `pkg-config --cflags gtk+-2.0` ${GTK_CFLAGS}
zhjed: zhjed.o
	cc ${CFLAGS} -o zhjed zhjed.o `pkg-config --libs gtk+-2.0` ${GTK_LIBS}
	strip $@

clean:
	rm -f zhjed.o zhjed
