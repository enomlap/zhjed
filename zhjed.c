/* -----------------------------------------------------------------------------
 * 
 *  ZHJeditor	- A simple text editor.
 *  tt7fans@163.com
 *  zuo,Mar 09, 2020
 *
 *  A simple GTK+ text editor. It's a learning work. not suitable for real use!
 *  Thanks to : 
 *		leafpad				-		Tarot Osuji <tarot@sdf.lonestar.org>,
 *		ged						-   2005 Ben Good, James Hartzell
 *	I learned many from their works.
 *	License: BSD License.
 *	To compile:
 *		make
 *--Makefile--------------------------------------------------
all:zhjed
CFLAGS+=-O3 -Wall -g
GTK_CFLAGS = -pthread -I/usr/include/gtk-2.0 -I/usr/lib/x86_64-linux-gnu/gtk-2.0/include -I/usr/include/gio-unix-2.0/ -I/usr/include/cairo -I/usr/include/pango-1.0 -I/usr/include/atk-1.0 -I/usr/include/cairo -I/usr/include/pixman-1 -I/usr/include/gdk-pixbuf-2.0 -I/usr/include/pango-1.0 -I/usr/include/harfbuzz -I/usr/include/pango-1.0 -I/usr/include/fribidi -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include -I/usr/include/uuid -I/usr/include/freetype2 -I/usr/include/libpng16
GTK_LIBS = -lgtk-x11-2.0 -lgdk-x11-2.0 -lpangocairo-1.0 -latk-1.0 -lcairo -lgdk_pixbuf-2.0 -lgio-2.0 -lpangoft2-1.0 -lpango-1.0 -lgobject-2.0 -lglib-2.0 -lfontconfig -lfreetype
#%.o: %.c
	cc ${CFLAGS} -c $< `pkg-config --cflags gtk+-2.0` ${GTK_CFLAGS}
zhjed.o: zhjed.c
	cc ${CFLAGS} -c zhjed.c `pkg-config --cflags gtk+-2.0` ${GTK_CFLAGS}
zhjed: zhjed.o
	cc ${CFLAGS} -o zhjed zhjed.o `pkg-config --libs gtk+-2.0` ${GTK_LIBS}

clean:
	rm -f zhjed.o zhjed
 *----------------------------------------------------
 * -----------------------------------------------------------------------------*/
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h> 
//TODO:  fix conf->fontname,forecolor,backcolor buf[last]='0', otherwise,'color' will be anything!  # zuo,Sep 10, 2021 # zhjed.c
//TODO:  add 'fclose' in saveFile(), otherwise, the file is locked by this. zuo,Sep 10, 2021
//TODO:  set fonts,OK ,# zuo,Mar 09, 2020
//TODO:  set background color,OK,# zuo,Mar 09, 2020
//TODO:  /File/New command
#define PROGRAMNAME "ZHJeditor"
typedef struct _ZHJeditorWindow{
	GtkWidget *gwWindow;
	GtkWidget *gwCloseDialog;
	GtkWidget *gwFileSelector;
	GtkWidget *gwTextAreaView;
	GtkWidget *gwVbox,*gwMenuBar,*gtkScrollWindow;
	char *FileName;
	gboolean bClosing;
}ZHJeditorWindow;

typedef struct {
	gint width;
	gint height;
	gchar *fontname;
	gchar *forecolor;
	gchar *backcolor;
	gboolean wordwrap;
	gboolean linenumbers;
	gboolean autoindent;
/*
---------------------new .ZHJeditorrc:
800
394
Mitra Mono 12
#a0f2a8
red
0
0
0
*/
/* --------------------- ~/.ZHJeditorrc : ( I just use the .leafpad file as my source file, first line removed.)
WenQuanYi Zen Hei Mono Medium 11
*/
} Conf;
static ZHJeditorWindow *ZHJeditorInstance;
static void load_config_file(Conf *conf);
void setFileName(const char *new);
void onQuit();
void doQuit();
void openFile(const char *FileName);
gchar *getBufferText();
void TonFileSave(void){printf("onFileSave invoked.\n");}
void onFileOpen(void);
void onHelpAbout(void);
void closeDialog(GtkDialog *dialog,gint id,gpointer data);
gboolean userQuit(GtkWidget *gwWindow,GdkEvent *event,gpointer user_data);
void onSelectAndOpen(); // Normally, callback function should has the form of 'theCallbackFunc(GtkItem *, gpointer *gp)', but this program only has one INSTANCE of gtkwindow, so ,..
void onFileSave();
void onSaveAs();
void onWrap(void);
void set_text_font_by_name(GtkWidget *widget, gchar *fontname);
void saveToSelect();
void saveFile();
void cut_item_clicked(GtkMenuItem *,gpointer);
void paste_item_clicked(GtkMenuItem *,gpointer);
void onFileOpen(void){
	GtkDialogFlags flags = GTK_DIALOG_DESTROY_WITH_PARENT;
	GtkWidget *dialog;
	GtkTextBuffer *buffer;
	buffer=gtk_text_view_get_buffer(GTK_TEXT_VIEW(ZHJeditorInstance->gwTextAreaView));
	if(gtk_text_buffer_get_modified(buffer)){ // modified, refuse to open another file.
		dialog = gtk_message_dialog_new (GTK_WINDOW(ZHJeditorInstance->gwWindow),
				flags,
				GTK_MESSAGE_WARNING,
				GTK_BUTTONS_CLOSE, "WARNING: Content modified!");
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		return;
	}
	ZHJeditorInstance->gwFileSelector=gtk_file_selection_new("Select file to open");
	g_signal_connect(GTK_FILE_SELECTION(ZHJeditorInstance->gwFileSelector)->ok_button,"clicked",G_CALLBACK(onSelectAndOpen),NULL); // Normally, 'NULL' should be ZHJeditorInstance->gwWindow here, but only 1 instance of ZHJeditorInstance
	g_signal_connect_swapped(GTK_FILE_SELECTION(ZHJeditorInstance->gwFileSelector)-> cancel_button,"clicked",G_CALLBACK(gtk_widget_destroy),ZHJeditorInstance->gwFileSelector);
	gtk_widget_show(ZHJeditorInstance->gwFileSelector);
}
static GtkItemFactoryEntry menu_items[]={
	{"/_File",NULL,NULL,0,"<Branch>"},
	{"/File/_Open","<control>O",onFileOpen,0,"<StockItem>",GTK_STOCK_OPEN},
	{"/File/_Save","<control>S",onFileSave,0,"<StockItem>",GTK_STOCK_SAVE},
	{"/File/Save_As","<shift><control>S",onSaveAs,0,"<StockItem>",GTK_STOCK_SAVE},
	{"/File/_Close","<control>Q",onQuit,0,"<StockItem>",GTK_STOCK_SAVE},
	{"/File/---",NULL,NULL,0,"<Separator>"},

	{"/_Edit",NULL,NULL,0,"<Branch>"},
	{ "/_Edit/_Word Wrap", NULL, onWrap, 0, "<CheckItem>" },
//	{ "/_Edit/_Font...", NULL,on_option_font, 0, "<StockItem>", GTK_STOCK_SELECT_FONT },
	/*
	 *  Text wrap  gtkenums.h
   *  typedef enum
   *  {
   *    GTK_WRAP_NONE,
   *    GTK_WRAP_CHAR,
   *    GTK_WRAP_WORD,
   *    GTK_WRAP_WORD_CHAR
   *  } GtkWrapMode;
   *  
	 * */
	{"/Edit/---",NULL,NULL,0,"<Separator>"},
	{"/_Help",NULL,NULL,0,"<Branch>"},
	{"/Help/_About",NULL,onHelpAbout,0,"<StockItem>","my-gtk-about"},
}; 
void onHelpAbout(void){
	const gchar *authors[]={"chawuciren@404.not.found",NULL};
	gtk_show_about_dialog(GTK_WINDOW(ZHJeditorInstance->gwWindow), 
			"version","0.001", 
			"copyright","Copyright \xc2\xa9 1q84 hj zuo", 
			"comments","GTK+ based simple text editor" "\nlearning lecture", 
			"authors",authors, NULL);
}
void onWrap(void)
{
  GtkItemFactory *ifactory;
  gboolean state;
  ifactory = gtk_item_factory_from_widget(ZHJeditorInstance->gwMenuBar);
  state = gtk_check_menu_item_get_active( GTK_CHECK_MENU_ITEM(gtk_item_factory_get_item(ifactory, "/Edit/Word Wrap")));
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(ZHJeditorInstance->gwTextAreaView), state ? GTK_WRAP_CHAR : GTK_WRAP_NONE);
}
GtkWidget *createMenuBar(GtkWidget *gwWindow){
	GtkAccelGroup *gaAccelerateGroup;
	GtkItemFactory *ifactory;
	gaAccelerateGroup=gtk_accel_group_new();
	ifactory=gtk_item_factory_new(GTK_TYPE_MENU_BAR,"<main>",gaAccelerateGroup); // set gaAccelerateGroup=NULL will cause CONTROL-O,... out of function.
	gtk_item_factory_create_items(ifactory,(sizeof(menu_items) / sizeof(GtkItemFactoryEntry)),menu_items,NULL);
	gtk_window_add_accel_group(GTK_WINDOW(gwWindow),gaAccelerateGroup);
	return gtk_item_factory_get_widget(ifactory,"<main>");
}
void setFileName(const char *newFileName){
	g_free(ZHJeditorInstance->FileName);
	if(NULL!=newFileName){
		ZHJeditorInstance->FileName=g_malloc(strlen(newFileName)+1);
		strcpy(ZHJeditorInstance->FileName,newFileName);
		gtk_window_set_title(GTK_WINDOW(ZHJeditorInstance->gwWindow),ZHJeditorInstance->FileName);
	}
	else ZHJeditorInstance->FileName=0;
}
void openFile(const char *FileName){
	FILE *infile;
	GtkTextBuffer *buffer;
	char *textdata=0;
	size_t data_len=0;
	size_t data_offset=0;
	size_t read_len=0;
	setFileName(FileName);
	if(!(infile=fopen(FileName,"r")))return;
	textdata=malloc(1);
	while(!feof(infile)){
		data_len+=512;
		textdata=g_realloc(textdata,data_len);
		read_len=fread(textdata+data_offset,1,512,infile);
		if(read_len!=512){
			data_len=data_len-512+read_len;
			textdata=g_realloc(textdata,data_len+1);
		}
		data_offset+=read_len;
	}
	fclose(infile);
	textdata[data_len]='\0';
	buffer=gtk_text_view_get_buffer(GTK_TEXT_VIEW(ZHJeditorInstance->gwTextAreaView));
	gtk_text_buffer_set_text(buffer,textdata,data_len);
	gtk_text_buffer_set_modified(buffer,0);
}
gchar *getBufferText(){
	GtkTextIter start,end;
	GtkTextBuffer *buffer;
	buffer=gtk_text_view_get_buffer(GTK_TEXT_VIEW(ZHJeditorInstance->gwTextAreaView));
	gtk_text_buffer_get_start_iter(buffer,&start);
	gtk_text_buffer_get_end_iter(buffer,&end);
	return gtk_text_buffer_get_text(buffer,&start,&end,0);
}
void saveFile(){
	//TODO: this assume ZHJeditorInstance->FileName is ok. but anyway, checking it. todo
	FILE *outfile;
	gchar *text;
	GtkTextBuffer *buf;
	outfile=fopen(ZHJeditorInstance->FileName,"w"); // check outfile is NULL?
	text=getBufferText();
	fprintf(outfile,"%s",text);
	g_free(text);
	fclose(outfile);
	buf=gtk_text_view_get_buffer(GTK_TEXT_VIEW(ZHJeditorInstance->gwTextAreaView));
	gtk_text_buffer_set_modified(buf,0);
}
int main(int argc,char **argv){
	Conf *conf;
	GdkColor color;
	gtk_init(&argc,&argv);
#ifdef __z_DEBUG_
	int i;
	printf("-------argc,argv:%d,",argc);for(i=0;i<argc;i++)printf("---%s",argv[i]);printf("\n");
#endif
	
	conf = g_malloc(sizeof(Conf));
	conf->width       = 600;
	conf->height      = 400;
	conf->fontname    = g_strdup("Monospace 12");
	conf->forecolor   = g_strdup("black");
	conf->backcolor   = g_strdup("#a0f2a8");
	conf->wordwrap    = FALSE;
	conf->linenumbers = FALSE;
	conf->autoindent  = FALSE;
	load_config_file(conf);
	//printf("--when aand who reached here?-------------in main : %d -- %d \n\n\n",conf->width,conf->height);
	ZHJeditorInstance=g_malloc(sizeof(ZHJeditorWindow));
	ZHJeditorInstance->FileName=0;
	ZHJeditorInstance->bClosing=0;
	ZHJeditorInstance->gwFileSelector=0;
	ZHJeditorInstance->gwCloseDialog=0;
	ZHJeditorInstance->gwWindow=gtk_window_new(GTK_WINDOW_TOPLEVEL);
	ZHJeditorInstance->gwVbox=gtk_vbox_new(0,0);
	ZHJeditorInstance->gwMenuBar=createMenuBar(ZHJeditorInstance->gwWindow);
	ZHJeditorInstance->gtkScrollWindow=gtk_scrolled_window_new(NULL,NULL);
	ZHJeditorInstance->gwTextAreaView=gtk_text_view_new();
	ZHJeditorInstance->gwVbox=gtk_vbox_new(0,0);
	gtk_widget_show(ZHJeditorInstance->gwMenuBar);
	//gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(ZHJeditorInstance->gwTextAreaView),GTK_WRAP_WORD);
	//printf("--when aand who reached here?-------------001\n\n\n");
	//set_text_font_by_name(GTK_TEXT_VIEW(ZHJeditorInstance->gwTextAreaView), "Cantarell Italic Light 15 @wght=200");
	//set_text_font_by_name((ZHJeditorInstance->gwTextAreaView), "Cantarell Italic Light 15 @wght=200");
	set_text_font_by_name((ZHJeditorInstance->gwTextAreaView), conf->fontname);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(ZHJeditorInstance->gwTextAreaView),GTK_WRAP_CHAR);
	// Sep.10/2021 removed :gdk_color_parse ("black", &color); // set the font color
	gdk_color_parse (conf->forecolor, &color); // set the font color
	gtk_widget_modify_text (ZHJeditorInstance->gwTextAreaView, GTK_STATE_NORMAL, &color);
	// Sep.10/2021 removed :gdk_color_parse ("#a0f2a8", &color); //set the background color, gdk_color_parse ("yellow", &color); //gdk_color_parse ("#666666", &color);
	gdk_color_parse (conf->backcolor, &color); //set the background color, gdk_color_parse ("yellow", &color); //gdk_color_parse ("#666666", &color);
	gtk_widget_modify_base(ZHJeditorInstance->gwTextAreaView, GTK_STATE_NORMAL, &color);
	gtk_widget_show(ZHJeditorInstance->gwTextAreaView);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(ZHJeditorInstance->gtkScrollWindow),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(ZHJeditorInstance->gtkScrollWindow),ZHJeditorInstance->gwTextAreaView);
	gtk_widget_show(ZHJeditorInstance->gtkScrollWindow);
	gtk_box_pack_start(GTK_BOX(ZHJeditorInstance->gwVbox),ZHJeditorInstance->gwMenuBar,0,0,0);
	gtk_box_pack_start(GTK_BOX(ZHJeditorInstance->gwVbox),ZHJeditorInstance->gtkScrollWindow,1,1,0);
	gtk_widget_show(ZHJeditorInstance->gwVbox);
	gtk_container_add(GTK_CONTAINER(ZHJeditorInstance->gwWindow),ZHJeditorInstance->gwVbox);
	gtk_window_set_title(GTK_WINDOW(ZHJeditorInstance->gwWindow),"ZHJeditor-Untitled");
	gtk_window_set_default_size( GTK_WINDOW(ZHJeditorInstance->gwWindow), conf->width, conf->height); //gtk_window_set_default_size(GTK_WINDOW(ZHJeditorInstance->gwWindow),600,400);
	//gtk_window_set_gravity(GTK_WINDOW(ZHJeditorInstance->gwWindow),GDK_GRAVITY_CENTER); // most window manager ignore the initial position for this function. lxde, yes.
	gtk_window_get_size(GTK_WINDOW(ZHJeditorInstance->gwWindow),&conf->width,&conf->height);
	//gtk_window_set_gravity(GTK_WINDOW(ZHJeditorInstance->gwWindow),GDK_GRAVITY_CENTER);//GDK_GRAVITY_SOUTH_EAST); // NOT WORK AT ALL!
	//gtk_window_move (GTK_WINDOW(ZHJeditorInstance->gwWindow), 0,0);
	//
	gtk_window_move (GTK_WINDOW(ZHJeditorInstance->gwWindow), (gdk_screen_width() - conf->width)/2, (gdk_screen_height() - conf->height)/2);
	g_signal_connect(ZHJeditorInstance->gwWindow,"delete_event",G_CALLBACK(onQuit),NULL);
	gtk_widget_show(ZHJeditorInstance->gwWindow);
	if(2==argc && NULL!=argv[1])openFile(argv[1]);
	gtk_main();
	return 0;
}
void doQuit(){
		gtk_widget_destroy(ZHJeditorInstance->gwWindow);
		gtk_main_quit();
		g_free(ZHJeditorInstance);
}
//gboolean userQuit(GtkWidget *gwWindow,GdkEvent *event,gpointer data){
void onQuit(){
	gboolean bModified;
	GtkTextBuffer *buffer;
	//if(ZHJeditorInstance->gwCloseDialog)return ;
	buffer=gtk_text_view_get_buffer(GTK_TEXT_VIEW(ZHJeditorInstance->gwTextAreaView));
	bModified=gtk_text_buffer_get_modified(buffer);
	if(!bModified){ // Not modified, simple quit.
		doQuit();
		return ;
	}//if modified:
	ZHJeditorInstance->gwCloseDialog=gtk_dialog_new_with_buttons("Save changes to file before"
			" bClosing?",
			GTK_WINDOW(ZHJeditorInstance->gwWindow),0,
			"Close without saving",0,
			GTK_STOCK_CANCEL,1,
			GTK_STOCK_SAVE,2,
			NULL);
	g_signal_connect(ZHJeditorInstance->gwCloseDialog,"response", G_CALLBACK(closeDialog), NULL);
	gtk_widget_show(ZHJeditorInstance->gwCloseDialog);
	return ;
}
void closeDialog(GtkDialog *dialog,gint id,gpointer data){
	gtk_widget_destroy(ZHJeditorInstance->gwCloseDialog);
	ZHJeditorInstance->bClosing=1;
	//printf("--Got id : %d -------------001\n\n\n",id);
	//printf("-- Got file name : %s-------------008\n\n\n",ZHJeditorInstance->FileName);
	switch(id){
		case 0:
			doQuit();
			break;
		case 2:
			onFileSave();
			//doQuit();
			break;
		case 1:
		default:
			ZHJeditorInstance->bClosing=0;
			ZHJeditorInstance->gwCloseDialog=0;
			break;
	}
}
void onSelectAndOpen(){
	const gchar *nm;
	nm=gtk_file_selection_get_filename(GTK_FILE_SELECTION(ZHJeditorInstance->gwFileSelector));
	openFile(nm);
	gtk_widget_destroy(ZHJeditorInstance->gwFileSelector);
	ZHJeditorInstance->gwFileSelector=0;
}
void onFileSave(){
	if(NULL==ZHJeditorInstance->FileName)
		onSaveAs();
	else{
		saveFile();
	}
}
void onSaveAs(){
	ZHJeditorInstance->gwFileSelector=gtk_file_selection_new("Select file to save:");
	g_signal_connect(GTK_FILE_SELECTION(ZHJeditorInstance->gwFileSelector)->ok_button, "clicked",G_CALLBACK(saveToSelect),NULL);
	g_signal_connect_swapped(GTK_FILE_SELECTION(ZHJeditorInstance->gwFileSelector)-> cancel_button,"clicked", G_CALLBACK(gtk_widget_destroy),ZHJeditorInstance->gwFileSelector);
	gtk_widget_show(ZHJeditorInstance->gwFileSelector);
}
void saveToSelect(){
	const gchar *nm;
	nm=gtk_file_selection_get_filename(GTK_FILE_SELECTION(ZHJeditorInstance->gwFileSelector));
	setFileName(nm);
	saveFile();
	gtk_widget_destroy(ZHJeditorInstance->gwFileSelector);
	ZHJeditorInstance->gwFileSelector=0;
}

static void load_config_file(Conf *conf)
{
	FILE *fp;
	gchar *path;
	gchar buf[BUFSIZ];
	path = g_build_filename(g_get_home_dir(), "." PROGRAMNAME "rc", NULL);
	fp = fopen(path, "r");
	g_free(path);
	if (!fp) return;
	fgets(buf, sizeof(buf), fp); conf->width = atoi(buf);//width
	fgets(buf, sizeof(buf), fp); conf->height = atoi(buf);//height
	fgets(buf, sizeof(buf), fp); buf[strlen(buf)-1]='\0';g_free(conf->fontname);conf->fontname=g_strdup(buf);//fontname
	fgets(buf, sizeof(buf), fp); buf[strlen(buf)-1]='\0';g_free(conf->forecolor);conf->forecolor=g_strdup(buf);//forecolor
	fgets(buf, sizeof(buf), fp); buf[strlen(buf)-1]='\0';g_free(conf->backcolor);conf->backcolor=g_strdup(buf);//fontname
	fgets(buf, sizeof(buf), fp); conf->wordwrap = atoi(buf); //is-word-wrap:0=disable,1=enable
	fgets(buf, sizeof(buf), fp); conf->linenumbers = atoi(buf);//is-show-linenumber
	fgets(buf, sizeof(buf), fp); conf->autoindent = atoi(buf);//is-autoindent
#ifdef __z_DEBUG_
	printf("conf:\nconf->width=%d,conf->height=%d,conf->fontname=%s\n,conf->forecolor=%s\n,conf->backcolor=%s\n,conf->wordwrap=%d\n,conf->linenumbers=%d\n,conf->autoindent=%d\n",
			conf->width,conf->height,conf->fontname,conf->forecolor,conf->backcolor,conf->wordwrap,conf->linenumbers,conf->autoindent);
#endif
	fclose(fp);
}
void set_text_font_by_name(GtkWidget *widget, gchar *fontname)
{
	PangoFontDescription *font_desc; // "[FAMILY-LIST] [STYLE-OPTIONS] [SIZE] [VARIATIONS]"
	font_desc = pango_font_description_from_string(fontname);
	gtk_widget_modify_font(widget, font_desc);
	pango_font_description_free(font_desc);
}

/* for ubuntu distribution, to install :
------------   ~/.local/share/applications/zhjed.desktop ---------
[Desktop Entry]
Encoding=UTF-8
Exec=/home/zuo/bin/zhjed %f
Name=ZHJeditor
#Terminal=true
Terminal=false
Type=Application
NoDisplay=false
*/
