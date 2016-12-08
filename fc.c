//    HDS OPE file Editor
//      fc.c  --- Finding Chart
//   
//                                           2010.3.15  A.Tajitsu


#include"main.h"    // 設定ヘッダ
#include"version.h"

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#include <signal.h>


void fc_re_dl_draw();
void fc_objtree_item();
void fc_dl_draw ();
void fc_dl_draw_all ();
static gboolean progress_timeout();
void do_fc();
void create_fc_dialog();
void create_fc_all_dialog();
void close_fc();
#ifndef USE_WIN32
static void cancel_fc();
static void cancel_fc_all();
#endif
void draw_fc_obj();
gboolean draw_fc_cairo();
static void refresh_fc();
void cc_get_fc_mode();
void pdf_fc();

#ifndef USE_WIN32
void dss_signal();
void dssall_signal();
#endif
gboolean check_dssall();

glong get_file_size();

extern int  get_dss();
extern gboolean my_main_iteration();
extern void cc_get_toggle();
extern void cc_get_adj();
extern void cc_get_combo_box();
extern GtkWidget* gtkut_button_new_from_stock();
extern void do_save_fc_pdf();



gboolean flagFC=FALSE, flag_getDSS=FALSE;
GdkPixbuf *pixbuf_fc=NULL, *pixbuf2_fc=NULL;

#ifndef USE_WIN32
pid_t fc_pid;
#endif
gboolean  flag_dssall_finish=FALSE;
gboolean  flag_dssall_kill=FALSE;

void fc_re_dl_draw (GtkWidget *widget, gpointer data)
{
  typHOE *hg = (typHOE *)data;

  if(flag_getDSS) return;
  flag_getDSS=TRUE;

  fc_dl_draw(hg);

  flag_getDSS=FALSE;
}


void fc_objtree_item (GtkWidget *widget, gpointer data)
{
  typHOE *hg = (typHOE *)data;
  GtkTreeIter iter;
  GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(hg->objtree));
  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(hg->objtree));
  

  if(flag_getDSS) return;
  flag_getDSS=TRUE;
  
  
  if (gtk_tree_selection_get_selected (selection, NULL, &iter)){
    gint i, i_list;
    GtkTreePath *path;
    
    path = gtk_tree_model_get_path (model, &iter);
    //i = gtk_tree_path_get_indices (path)[0];
    gtk_tree_model_get (model, &iter, COLUMN_OBJTREE_NUMBER, &i, -1);
    i--;
    
    if((int)hg->obj[i].epoch!=2000){
#ifdef GTK_MSG
      popup_message(GTK_STOCK_DIALOG_WARNING, POPUP_TIMEOUT*2,
		    "Error: Object Epoch should be J2000",
		    " ",
		    "       for DSS Quick View.",
		    NULL);
#else
      fprintf(stderr, "Error: Object Epoch should be J2000 for DSS Quick View.");
#endif
      
      flag_getDSS=FALSE;
    
      return;
    }

    hg->dss_i=i;

    fc_dl_draw(hg);
    gtk_tree_path_free (path);
  }

  flag_getDSS=FALSE;
}

void fc_dl_draw (typHOE *hg)
{
  GtkWidget *dialog, *vbox, *label, *button;
#ifndef USE_WIN32
  static struct sigaction act;
#endif
  guint timer;

  dialog = gtk_dialog_new();
  
  gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
  gtk_container_set_border_width(GTK_CONTAINER(dialog),5);
  gtk_container_set_border_width(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox),5);
  gtk_window_set_title(GTK_WINDOW(dialog),"HOE : Message");
  gtk_window_set_decorated(GTK_WINDOW(dialog),TRUE);
  
#ifdef USE_GTK2  
  gtk_dialog_set_has_separator(GTK_DIALOG(dialog),TRUE);
#endif
  
  switch(hg->fc_mode){
  case FC_STSCI_DSS1R:
    label=gtk_label_new("Retrieving DSS (POSS1 Red) image from \"" FC_HOST_STSCI "\" ...");
    break;
    
  case FC_STSCI_DSS1B:
    label=gtk_label_new("Retrieving DSS (POSS1 Blue) image from \"" FC_HOST_STSCI "\" ...");
    break;
    
  case FC_STSCI_DSS2R:
    label=gtk_label_new("Retrieving DSS (POSS2 Red) image from \"" FC_HOST_STSCI "\" ...");
    break;
    
  case FC_STSCI_DSS2B:
    label=gtk_label_new("Retrieving DSS (POSS2 Blue) image from \"" FC_HOST_STSCI "\" ...");
    break;
    
  case FC_STSCI_DSS2IR:
    label=gtk_label_new("Retrieving DSS (POSS2 IR) image from \"" FC_HOST_STSCI "\" ...");
    break;
    
  case FC_ESO_DSS2R:
    label=gtk_label_new("Retrieving DSS (POSS2 Red) image from \"" FC_HOST_ESO "\" ...");
    break;
    
  case FC_SKYVIEW_DSS1R:
    label=gtk_label_new("Retrieving DSS (POSS1 Red) image from \"" FC_HOST_SKYVIEW "\" ...");
    break;
    
  case FC_SKYVIEW_DSS1B:
    label=gtk_label_new("Retrieving DSS (POSS1 Blue) image from \"" FC_HOST_SKYVIEW "\" ...");
    break;
    
  case FC_SKYVIEW_DSS2R:
    label=gtk_label_new("Retrieving DSS (POSS2 Red) image from \"" FC_HOST_SKYVIEW "\" ...");
    break;
    
  case FC_SKYVIEW_DSS2B:
    label=gtk_label_new("Retrieving DSS (POSS2 Blue) image from \"" FC_HOST_SKYVIEW "\" ...");
    break;
    
  case FC_SKYVIEW_DSS2IR:
    label=gtk_label_new("Retrieving DSS (POSS2 IR) image from \"" FC_HOST_SKYVIEW "\" ...");
    break;
    
  case FC_SKYVIEW_2MASSJ:
    label=gtk_label_new("Retrieving 2MASS (J-Band) image from \"" FC_HOST_SKYVIEW "\" ...");
    break;
    
  case FC_SKYVIEW_2MASSH:
    label=gtk_label_new("Retrieving 2MASS (H-Band) image from \"" FC_HOST_SKYVIEW "\" ...");
    break;
    
  case FC_SKYVIEW_2MASSK:
    label=gtk_label_new("Retrieving 2MASS (K-Band) image from \"" FC_HOST_SKYVIEW "\" ...");
    break;
    
  case FC_SDSS:
    label=gtk_label_new("Retrieving SDSS (DR7) image from \"" FC_HOST_SDSS "\" ...");
    break;
    
  case FC_SDSS12:
    label=gtk_label_new("Retrieving SDSS (DR12) image from \"" FC_HOST_SDSS12 "\" ...");
    break;
    
  }
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),label,TRUE,TRUE,0);
  gtk_widget_show(label);
  
  hg->pbar=gtk_progress_bar_new();
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),hg->pbar,TRUE,TRUE,0);
  gtk_progress_bar_pulse(GTK_PROGRESS_BAR(hg->pbar));
  gtk_progress_bar_set_orientation (GTK_PROGRESS_BAR (hg->pbar), 
				    GTK_PROGRESS_RIGHT_TO_LEFT);
  gtk_progress_bar_set_pulse_step(GTK_PROGRESS_BAR(hg->pbar),0.05);
  gtk_widget_show(hg->pbar);
  
  unlink(hg->dss_file);
  
  timer=g_timeout_add(100, 
		      (GSourceFunc)progress_timeout,
		      (gpointer)hg);
  
  hg->plabel=gtk_label_new("Retrieving image from website ...");
  gtk_misc_set_alignment (GTK_MISC (hg->plabel), 0.0, 0.5);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area),
		       hg->plabel,FALSE,FALSE,0);

#ifndef USE_WIN32
#ifdef __GTK_STOCK_H__
  button=gtkut_button_new_from_stock("Cancel",GTK_STOCK_CANCEL);
#else
  button=gtk_button_new_with_label("Cancel");
#endif
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area),
		     button,FALSE,FALSE,0);
  my_signal_connect(button,"pressed",
		    cancel_fc, 
		    (gpointer)hg);
#endif
  
  gtk_widget_show_all(dialog);
  
  //#ifdef USE_WIN32
  //while (my_main_iteration(FALSE));
  //#else
#ifndef USE_WIN32
  act.sa_handler=dss_signal;
  sigemptyset(&act.sa_mask);
  act.sa_flags=0;
  sigaction(SIGUSR1, &act,0);
#endif

  gtk_window_set_modal(GTK_WINDOW(dialog),TRUE);
  
  get_dss(hg);
  //#ifndef USE_WIN32  
  gtk_main();
  //#endif
  gtk_timeout_remove(timer);
  gtk_widget_destroy(dialog);
  
  hg->dss_arcmin_ip=hg->dss_arcmin;
#ifndef USE_WIN32
  if(fc_pid){
#endif
    if(pixbuf_fc)  g_object_unref(G_OBJECT(pixbuf_fc));
    pixbuf_fc = gdk_pixbuf_new_from_file(hg->dss_file, NULL);
    
    do_fc(hg);
#ifndef USE_WIN32
  }
#endif
    
}



void fc_dl_draw_all (typHOE *hg)
{
  GtkTreeIter iter;
  gchar tmp[128];
  GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(hg->objtree));
  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(hg->objtree));
  GtkWidget *dialog, *vbox, *label, *button;
#ifndef USE_WIN32
  static struct sigaction act;
#endif
  guint timer;
  gint i_list;
  gint dssall_check_timer;
  cairo_t *cr;
  cairo_surface_t *surface=NULL;
  gint total_check=0;
  gboolean get_all=FALSE;

  if(hg->i_max<=0) return;
  if(flag_getDSS) return;
  flag_getDSS=TRUE;
  

  dialog = gtk_dialog_new();
  
  gtk_window_set_modal(GTK_WINDOW(dialog),TRUE);

  gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
  gtk_container_set_border_width(GTK_CONTAINER(dialog),5);
  gtk_container_set_border_width(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox),5);
  gtk_window_set_title(GTK_WINDOW(dialog),"HOE : Creating Finding Charts");
  gtk_window_set_decorated(GTK_WINDOW(dialog),TRUE);
  
#ifdef USE_GTK2  
  gtk_dialog_set_has_separator(GTK_DIALOG(dialog),TRUE);
#endif
  
  switch(hg->fc_mode){
  case FC_STSCI_DSS1R:
    label=gtk_label_new("Retrieving DSS (POSS1 Red) image from \"" FC_HOST_STSCI "\" ...");
    break;
    
  case FC_STSCI_DSS1B:
    label=gtk_label_new("Retrieving DSS (POSS1 Blue) image from \"" FC_HOST_STSCI "\" ...");
    break;
    
  case FC_STSCI_DSS2R:
    label=gtk_label_new("Retrieving DSS (POSS2 Red) image from \"" FC_HOST_STSCI "\" ...");
    break;
    
  case FC_STSCI_DSS2B:
    label=gtk_label_new("Retrieving DSS (POSS2 Blue) image from \"" FC_HOST_STSCI "\" ...");
    break;
    
  case FC_STSCI_DSS2IR:
    label=gtk_label_new("Retrieving DSS (POSS2 IR) image from \"" FC_HOST_STSCI "\" ...");
    break;
    
  case FC_ESO_DSS2R:
    label=gtk_label_new("Retrieving DSS (POSS2 Red) image from \"" FC_HOST_ESO "\" ...");
    break;
    
  case FC_SKYVIEW_DSS1R:
    label=gtk_label_new("Retrieving DSS (POSS1 Red) image from \"" FC_HOST_SKYVIEW "\" ...");
    break;
    
  case FC_SKYVIEW_DSS1B:
    label=gtk_label_new("Retrieving DSS (POSS1 Blue) image from \"" FC_HOST_SKYVIEW "\" ...");
    break;
    
  case FC_SKYVIEW_DSS2R:
    label=gtk_label_new("Retrieving DSS (POSS2 Red) image from \"" FC_HOST_SKYVIEW "\" ...");
    break;
    
  case FC_SKYVIEW_DSS2B:
    label=gtk_label_new("Retrieving DSS (POSS2 Blue) image from \"" FC_HOST_SKYVIEW "\" ...");
    break;
    
  case FC_SKYVIEW_DSS2IR:
    label=gtk_label_new("Retrieving DSS (POSS2 IR) image from \"" FC_HOST_SKYVIEW "\" ...");
    break;
    
  case FC_SKYVIEW_2MASSJ:
    label=gtk_label_new("Retrieving 2MASS (J-Band) image from \"" FC_HOST_SKYVIEW "\" ...");
    break;
    
  case FC_SKYVIEW_2MASSH:
    label=gtk_label_new("Retrieving 2MASS (H-Band) image from \"" FC_HOST_SKYVIEW "\" ...");
    break;
    
  case FC_SKYVIEW_2MASSK:
    label=gtk_label_new("Retrieving 2MASS (K-Band) image from \"" FC_HOST_SKYVIEW "\" ...");
    break;
    
  case FC_SDSS:
    label=gtk_label_new("Retrieving SDSS (DR7) image from \"" FC_HOST_SDSS "\" ...");
    break;
    
  case FC_SDSS12:
    label=gtk_label_new("Retrieving SDSS (DR12) image from \"" FC_HOST_SDSS12 "\" ...");
    break;
    
  }
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),label,TRUE,TRUE,0);
  gtk_widget_show(label);
  
  hg->pbar=gtk_progress_bar_new();
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),hg->pbar,TRUE,TRUE,0);
  gtk_progress_bar_pulse(GTK_PROGRESS_BAR(hg->pbar));
  gtk_progress_bar_set_orientation (GTK_PROGRESS_BAR (hg->pbar), 
				    GTK_PROGRESS_RIGHT_TO_LEFT);
  gtk_progress_bar_set_pulse_step(GTK_PROGRESS_BAR(hg->pbar),0.05);
  gtk_widget_show(hg->pbar);

  hg->pbar2=gtk_progress_bar_new();
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),hg->pbar2,TRUE,TRUE,0);
  gtk_progress_bar_set_orientation (GTK_PROGRESS_BAR (hg->pbar2), 
				    GTK_PROGRESS_LEFT_TO_RIGHT);
  gtk_widget_show(hg->pbar2);
  

  hg->plabel=gtk_label_new("Retrieving image from website ...");
  gtk_misc_set_alignment (GTK_MISC (hg->plabel), 0.0, 0.5);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area),
		     hg->plabel,TRUE,TRUE,0);

#ifndef USE_WIN32
#ifdef __GTK_STOCK_H__
  button=gtkut_button_new_from_stock("Cancel",GTK_STOCK_CANCEL);
#else
  button=gtk_button_new_with_label("Cancel");
#endif
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area),
		     button,FALSE,FALSE,0);
  my_signal_connect(button,"pressed",
		    cancel_fc_all, 
		    (gpointer)hg);
#endif
    
  gtk_widget_show_all(dialog);
  

  hg->fc_output=FC_OUTPUT_PDF_ALL;

  surface = cairo_pdf_surface_create(hg->filename_pdf, PLOT_HEIGHT, PLOT_HEIGHT);
  cr = cairo_create(surface); 

  cairo_set_source_rgb(cr, 1, 1, 1);

  dssall_check_timer=g_timeout_add(1000, 
				   (GSourceFunc)check_dssall,
				   (gpointer)hg);

  for(i_list=0;i_list<hg->i_max;i_list++){
    if(hg->obj[i_list].check_sm) total_check++;
  }

  if(total_check==0){
    get_all=TRUE;
  }

  for(i_list=0;i_list<hg->i_max;i_list++){
    if((int)hg->obj[i_list].epoch!=2000){
#ifdef GTK_MSG
      popup_message(GTK_STOCK_DIALOG_WARNING, POPUP_TIMEOUT*2,
		    "Error: Object Epoch should be J2000",
		    "       for DSS Quick View.",
		    " ",
		    "       Skip this object.",
		    NULL);
#else
      fprintf(stderr, "Error: Object Epoch should be J2000 for DSS Quick View.");
#endif
      
    }
    else if((get_all) || (hg->obj[i_list].check_sm)){
      hg->dss_i=i_list;

#ifndef USE_WIN32
      act.sa_handler=dssall_signal;
      sigemptyset(&act.sa_mask);
      act.sa_flags=0;
      sigaction(SIGUSR1, &act,0);
#endif
      
      hg->dss_arcmin_ip=hg->dss_arcmin;
      
      timer=g_timeout_add(100, 
			  (GSourceFunc)progress_timeout,
			  (gpointer)hg);
      
      gtk_progress_bar_set_text(GTK_PROGRESS_BAR(hg->pbar),"Downloading ...");

      get_dss(hg);
      
      gtk_main();
      gtk_timeout_remove(timer);

      if(flag_dssall_kill){
	flag_dssall_kill=FALSE;
	flag_dssall_finish=FALSE;
	break;
      }
      else{
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(hg->pbar),"Creating PDF ...");
	while (my_main_iteration(FALSE));
      
#ifndef USE_WIN32
	if(fc_pid){
#endif
	  if(pixbuf_fc)  g_object_unref(G_OBJECT(pixbuf_fc));
	  pixbuf_fc = gdk_pixbuf_new_from_file(hg->dss_file, NULL);
	  
	  draw_fc_obj(hg,cr,PLOT_HEIGHT,PLOT_HEIGHT);
#ifndef USE_WIN32
	}
#endif
	
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(hg->pbar2),
				      (get_all) ? 
				      (gdouble)(hg->dss_i+1)/(gdouble)(hg->i_max) :
				      (gdouble)(hg->dss_i+1)/(gdouble)(total_check));
	sprintf(tmp,"Finished [ %d / %d ] Objects",i_list+1,
		(get_all) ? hg->i_max : total_check);
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(hg->pbar2),tmp);
				
      
	flag_dssall_finish=FALSE;
      }
    }
  }

  gtk_timeout_remove(dssall_check_timer);
  gtk_widget_destroy(dialog);

  cairo_surface_destroy(surface);
  cairo_destroy(cr);

  hg->fc_output=FC_OUTPUT_WINDOW;
  flag_getDSS=FALSE;
}

static gboolean progress_timeout( gpointer data ){
  typHOE *hg=(typHOE *)data;
  glong sz;
  gchar *tmp;

  if(GTK_WIDGET_REALIZED(hg->pbar)){
    gtk_progress_bar_pulse(GTK_PROGRESS_BAR(hg->pbar));
    
    sz=get_file_size(hg->dss_file);
    if(sz>1024){
      sz=sz/1024;
      if(sz>1024){
	tmp=g_strdup_printf("Downloaded %.2f MB",(gfloat)sz/1024.);
      }
      else{
	tmp=g_strdup_printf("Downloaded %d kB",(gint)sz);
      }
    }
    else if (sz>0){
      tmp=g_strdup_printf("Downloaded %d bytes",(gint)sz);
    }
    else{
      tmp=g_strdup_printf("Waiting for WWW responce ...");
    }
    gtk_label_set_text(GTK_LABEL(hg->plabel), tmp);
    g_free(tmp);
    
    return TRUE;
  }
  else{
    return FALSE;
  }
}


void do_fc(typHOE *hg){
  if(flagFC){
    gdk_window_raise(hg->fc_main->window);
    hg->fc_output=FC_OUTPUT_WINDOW;
    draw_fc_cairo(hg->fc_dw,NULL,
		  (gpointer)hg);
    return;
  }
  else{
    flagFC=TRUE;
  }
  
  create_fc_dialog(hg);
}

void create_fc_dialog(typHOE *hg)
{
  GtkWidget *vbox, *hbox, *hbox2, *table;
  GtkWidget *frame, *check, *label, *button, *spinner;
  GtkAdjustment *adj;
  GtkWidget *menubar;


  // Win構築は重いので先にExposeイベント等をすべて処理してから
  while (my_main_iteration(FALSE));

  hg->fc_main = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(hg->fc_main), "HOE : Finding Chart");
  //gtk_widget_set_usize(hg->skymon_main, SKYMON_SIZE, SKYMON_SIZE);
  
  my_signal_connect(hg->fc_main,
		    "destroy",
		    close_fc, 
		    (gpointer)hg);

  gtk_widget_set_app_paintable(hg->fc_main, TRUE);
  
  vbox = gtk_vbox_new(FALSE,0);
  gtk_container_add (GTK_CONTAINER (hg->fc_main), vbox);


  hbox = gtk_hbox_new(FALSE,0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

  frame = gtk_frame_new ("Act.");
  gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 5);

  table = gtk_table_new(2,2,FALSE);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_container_set_border_width (GTK_CONTAINER (table), 0);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 3);

#ifdef __GTK_STOCK_H__
  button=gtkut_button_new_from_stock(NULL,GTK_STOCK_CANCEL);
#else
  button = gtk_button_new_with_label ("Quit");
#endif
  g_signal_connect (button, "clicked",
		    G_CALLBACK (close_fc), (gpointer)hg);
  gtk_table_attach (GTK_TABLE(table), button, 0, 1, 1, 2,
		    GTK_SHRINK,GTK_SHRINK,0,0);
  //gtk_box_pack_start(GTK_BOX(hbox2),button,FALSE,FALSE,0);
#ifdef __GTK_TOOLTIP_H__
  gtk_widget_set_tooltip_text(button,
			      "Quit");
#endif

#ifdef __GTK_STOCK_H__
  button=gtkut_button_new_from_stock(NULL,GTK_STOCK_SAVE);
#else
  button = gtk_button_new_with_label ("PDF");
#endif
  g_signal_connect (button, "clicked",
		    G_CALLBACK (do_save_fc_pdf), (gpointer)hg);
  gtk_table_attach (GTK_TABLE(table), button, 1, 2, 1, 2,
		    GTK_SHRINK,GTK_SHRINK,0,0);
  //gtk_box_pack_start(GTK_BOX(hbox2),button,FALSE,FALSE,0);
#ifdef __GTK_TOOLTIP_H__
  gtk_widget_set_tooltip_text(button,
			      "Save as PDF");
#endif


  frame = gtk_frame_new ("Source");
  gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 5);

  table = gtk_table_new(5,2,FALSE);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_container_set_border_width (GTK_CONTAINER (table), 0);
  gtk_table_set_row_spacings (GTK_TABLE (table), 0);
  gtk_table_set_col_spacings (GTK_TABLE (table), 3);

#ifdef __GTK_STOCK_H__
  button=gtkut_button_new_from_stock(NULL,GTK_STOCK_NETWORK);
#else
  button = gtk_button_new_with_label ("Download & Redraw");
#endif
  g_signal_connect (button, "clicked",
		    G_CALLBACK (fc_re_dl_draw), (gpointer)hg);
  gtk_table_attach (GTK_TABLE(table), button, 0, 1, 1, 2,
		    GTK_SHRINK,GTK_SHRINK,0,0);
  //gtk_box_pack_start(GTK_BOX(hbox2),button,FALSE,FALSE,0);
#ifdef __GTK_TOOLTIP_H__
  gtk_widget_set_tooltip_text(button,
			      "Download & Redraw");
#endif

  {
    GtkWidget *combo;
    GtkListStore *store;
    GtkTreeIter iter, iter_set;	  
    GtkCellRenderer *renderer;
    
    store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
    
    /*
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "DSS1 (Red) via STSCI",
		       1, FC_STSCI_DSS1R, -1);
    if(hg->fc_mode==FC_STSCI_DSS1R) iter_set=iter;
	
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "DSS1 (Blue) via STSCI",
		       1, FC_STSCI_DSS1B, -1);
    if(hg->fc_mode==FC_STSCI_DSS1B) iter_set=iter;
	
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "DSS2 (Red) via STSCI",
		       1, FC_STSCI_DSS2R, -1);
    if(hg->fc_mode==FC_STSCI_DSS2R) iter_set=iter;
	
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "DSS2 (Blue) via STSCI",
		       1, FC_STSCI_DSS2B, -1);
    if(hg->fc_mode==FC_STSCI_DSS2B) iter_set=iter;
	
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "DSS2 (IR) via STSCI",
		       1, FC_STSCI_DSS2IR, -1);
    if(hg->fc_mode==FC_STSCI_DSS2IR) iter_set=iter;
	
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "DSS2 (Red) via ESO",
		       1, FC_ESO_DSS2R, -1);
    if(hg->fc_mode==FC_ESO_DSS2R) iter_set=iter;
    */
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "DSS1 (Red) via SkiView",
		       1, FC_SKYVIEW_DSS1R, -1);
    if(hg->fc_mode==FC_SKYVIEW_DSS1R) iter_set=iter;
	
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "DSS1 (Blue) via SkiView",
		       1, FC_SKYVIEW_DSS1B, -1);
    if(hg->fc_mode==FC_SKYVIEW_DSS1B) iter_set=iter;
	
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "DSS2 (Red) via SkiView",
		       1, FC_SKYVIEW_DSS2R, -1);
    if(hg->fc_mode==FC_SKYVIEW_DSS2R) iter_set=iter;
	
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "DSS2 (Blue) via SkiView",
		       1, FC_SKYVIEW_DSS2B, -1);
    if(hg->fc_mode==FC_SKYVIEW_DSS2B) iter_set=iter;
	
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "DSS2 (IR) via SkiView",
		       1, FC_SKYVIEW_DSS2IR, -1);
    if(hg->fc_mode==FC_SKYVIEW_DSS2IR) iter_set=iter;
	
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "2MASS (J) via SkiView",
		       1, FC_SKYVIEW_2MASSJ, -1);
    if(hg->fc_mode==FC_SKYVIEW_2MASSJ) iter_set=iter;
	
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "2MASS (H) via SkiView",
		       1, FC_SKYVIEW_2MASSH, -1);
    if(hg->fc_mode==FC_SKYVIEW_2MASSH) iter_set=iter;
	
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "2MASS (K) via SkiView",
		       1, FC_SKYVIEW_2MASSK, -1);
    if(hg->fc_mode==FC_SKYVIEW_2MASSK) iter_set=iter;
	
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "SDSS DR7 quick view",
		       1, FC_SDSS, -1);
    if(hg->fc_mode==FC_SDSS) iter_set=iter;
	
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "SDSS DR12 quick view",
		       1, FC_SDSS12, -1);
    if(hg->fc_mode==FC_SDSS12) iter_set=iter;
	
    combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));
    gtk_table_attach (GTK_TABLE(table), combo, 1, 2, 1, 2,
		      GTK_SHRINK,GTK_SHRINK,0,0);
    //gtk_container_add (GTK_CONTAINER (hbox2), combo);
    g_object_unref(store);
	
    renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo),renderer, TRUE);
    gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT(combo), renderer, "text",0,NULL);
	
    gtk_combo_box_set_active_iter(GTK_COMBO_BOX(combo),&iter_set);
    gtk_widget_show(combo);
    my_signal_connect (combo,"changed",cc_get_fc_mode,
		       (gpointer)hg);
  }

  frame = gtk_frame_new ("Size [\']");
  gtk_table_attach (GTK_TABLE(table), frame, 2, 3, 0, 2,
		    GTK_SHRINK,GTK_SHRINK,0,0);
  //gtk_box_pack_start(GTK_BOX(hbox1), frame, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 0);

  hbox2 = gtk_hbox_new(FALSE,0);
  gtk_container_add (GTK_CONTAINER (frame), hbox2);

  adj = (GtkAdjustment *)gtk_adjustment_new(hg->dss_arcmin,
					    DSS_ARCMIN_MIN, DSS_ARCMIN_MAX,
					    1.0, 1.0, 0);
  spinner =  gtk_spin_button_new (adj, 0, 0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), FALSE);
  gtk_entry_set_editable(GTK_ENTRY(&GTK_SPIN_BUTTON(spinner)->entry),
			 TRUE);
  gtk_box_pack_start(GTK_BOX(hbox2),spinner,FALSE,FALSE,0);
  my_entry_set_width_chars(GTK_ENTRY(&GTK_SPIN_BUTTON(spinner)->entry),2);
  my_signal_connect (adj, "value_changed",
		     cc_get_adj,
		     &hg->dss_arcmin);


  frame = gtk_frame_new ("Scale");
  gtk_table_attach (GTK_TABLE(table), frame, 3, 4, 0, 2,
		    GTK_SHRINK,GTK_SHRINK,0,0);
  //gtk_box_pack_start(GTK_BOX(hbox1), frame, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 0);

  hbox2 = gtk_hbox_new(FALSE,0);
  gtk_container_add (GTK_CONTAINER (frame), hbox2);

  button=gtk_check_button_new_with_label("HistEq");
  gtk_container_set_border_width (GTK_CONTAINER (button), 0);
  gtk_box_pack_start(GTK_BOX(hbox2),button,FALSE,FALSE,0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),hg->dss_hist);
  my_signal_connect(button,"toggled",
		    G_CALLBACK (cc_get_toggle), 
		    &hg->dss_hist);


  frame = gtk_frame_new ("SDSS mark");
  gtk_table_attach (GTK_TABLE(table), frame, 4, 5, 0, 2,
		    GTK_SHRINK,GTK_SHRINK,0,0);
  //gtk_box_pack_start(GTK_BOX(hbox1), frame, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 0);

  hbox2 = gtk_hbox_new(FALSE,0);
  gtk_container_add (GTK_CONTAINER (frame), hbox2);

  button=gtk_check_button_new_with_label("Ph.");
  gtk_container_set_border_width (GTK_CONTAINER (button), 0);
  gtk_box_pack_start(GTK_BOX(hbox2),button,FALSE,FALSE,0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),hg->sdss_photo);
  my_signal_connect(button,"toggled",
		    G_CALLBACK (cc_get_toggle), 
		    &hg->sdss_photo);

  button=gtk_check_button_new_with_label("Sp.");
  gtk_container_set_border_width (GTK_CONTAINER (button), 0);
  gtk_box_pack_start(GTK_BOX(hbox2),button,FALSE,FALSE,0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),hg->sdss_spec);
  my_signal_connect(button,"toggled",
		    G_CALLBACK (cc_get_toggle), 
		    &hg->sdss_spec);


  frame = gtk_frame_new ("Instrument");
  gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 5);

  table = gtk_table_new(4,2,FALSE);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_container_set_border_width (GTK_CONTAINER (table), 0);
  gtk_table_set_row_spacings (GTK_TABLE (table), 0);
  gtk_table_set_col_spacings (GTK_TABLE (table), 3);


#ifdef __GTK_STOCK_H__
  button=gtkut_button_new_from_stock(NULL,GTK_STOCK_REFRESH);
#else
  button = gtk_button_new_with_label ("Redraw");
#endif
  g_signal_connect (button, "clicked",
		    G_CALLBACK (refresh_fc), (gpointer)hg);
  gtk_table_attach (GTK_TABLE(table), button, 0, 1, 1, 2,
		    GTK_SHRINK,GTK_SHRINK,0,0);
  //gtk_box_pack_start(GTK_BOX(hbox2),button,FALSE,FALSE,0);
#ifdef __GTK_TOOLTIP_H__
  gtk_widget_set_tooltip_text(button,
			      "Redraw");
#endif


  {
    GtkWidget *combo;
    GtkListStore *store;
    GtkTreeIter iter, iter_set;	  
    GtkCellRenderer *renderer;
    
    store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
    
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "None",
		       1, FC_INST_NONE, -1);
    if(hg->fc_inst==FC_INST_NONE) iter_set=iter;
	
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "HDS",
		       1, FC_INST_HDS, -1);
    if(hg->fc_inst==FC_INST_HDS) iter_set=iter;
	
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "IRCS",
		       1, FC_INST_IRCS, -1);
    if(hg->fc_inst==FC_INST_IRCS) iter_set=iter;

    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "FOCAS",
		       1, FC_INST_FOCAS, -1);
    if(hg->fc_inst==FC_INST_FOCAS) iter_set=iter;

    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "MOIRCS",
		       1, FC_INST_MOIRCS, -1);
    if(hg->fc_inst==FC_INST_MOIRCS) iter_set=iter;

    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "SupCam",
		       1, FC_INST_SPCAM, -1);
    if(hg->fc_inst==FC_INST_SPCAM) iter_set=iter;

    combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));
    gtk_table_attach (GTK_TABLE(table), combo, 1, 2, 1, 2,
		      GTK_SHRINK,GTK_SHRINK,0,0);
    //gtk_container_add (GTK_CONTAINER (hbox2), combo);
    g_object_unref(store);
	
    renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo),renderer, TRUE);
    gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT(combo), renderer, "text",0,NULL);
	
    gtk_combo_box_set_active_iter(GTK_COMBO_BOX(combo),&iter_set);
    gtk_widget_show(combo);
    my_signal_connect (combo,"changed",cc_get_combo_box,
		       &hg->fc_inst);
  }

  button=gtk_check_button_new_with_label("Detail");
  gtk_container_set_border_width (GTK_CONTAINER (button), 0);
  gtk_table_attach (GTK_TABLE(table), button, 2, 3, 1, 2,
		    GTK_SHRINK,GTK_SHRINK,0,0);
  //gtk_box_pack_start(GTK_BOX(hbox2),button,FALSE,FALSE,0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),hg->dss_draw_slit);
  my_signal_connect(button,"toggled",
		    G_CALLBACK (cc_get_toggle), 
		    &hg->dss_draw_slit);



  frame = gtk_frame_new ("PA [deg]");
  gtk_table_attach (GTK_TABLE(table), frame, 3, 4, 0, 2,
		    GTK_SHRINK,GTK_SHRINK,0,0);
  //gtk_box_pack_start(GTK_BOX(hbox1), frame, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 0);

  hbox2 = gtk_hbox_new(FALSE,0);
  gtk_container_add (GTK_CONTAINER (frame), hbox2);

  adj = (GtkAdjustment *)gtk_adjustment_new(hg->dss_pa,
					    -360, 360,
					    1.0, 1.0, 0);
  spinner =  gtk_spin_button_new (adj, 0, 0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), TRUE);
  gtk_entry_set_editable(GTK_ENTRY(&GTK_SPIN_BUTTON(spinner)->entry),
			 TRUE);
  gtk_box_pack_start(GTK_BOX(hbox2),spinner,FALSE,FALSE,0);
  my_entry_set_width_chars(GTK_ENTRY(&GTK_SPIN_BUTTON(spinner)->entry),4);
  my_signal_connect (adj, "value_changed",
		     cc_get_adj,
		     &hg->dss_pa);


  button=gtk_check_button_new_with_label("Flip");
  gtk_container_set_border_width (GTK_CONTAINER (button), 0);
  gtk_box_pack_start(GTK_BOX(hbox2),button,FALSE,FALSE,0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),hg->dss_flip);
  my_signal_connect(button,"toggled",
		    G_CALLBACK (cc_get_toggle), 
		    &hg->dss_flip);
  
  // Drawing Area
  hg->fc_dw = gtk_drawing_area_new();
  gtk_widget_set_size_request (hg->fc_dw, FC_WIDTH, FC_HEIGHT);
  gtk_box_pack_start(GTK_BOX(vbox), hg->fc_dw, TRUE, TRUE, 0);
  gtk_widget_set_app_paintable(hg->fc_dw, TRUE);
  gtk_widget_show(hg->fc_dw);

  screen_changed(hg->fc_dw,NULL,NULL);
  
  my_signal_connect(hg->fc_dw, 
		    "expose-event", 
		    draw_fc_cairo,
		    (gpointer)hg);

  gtk_widget_show_all(hg->fc_main);

  gdk_window_raise(hg->fc_main->window);

  gdk_flush();
}


void create_fc_all_dialog (typHOE *hg)
{
  GtkWidget *dialog, *label, *button, *frame, *spinner;
  GtkWidget *hbox, *hbox2, *check, *table;
  GtkAdjustment *adj;
  
  dialog = gtk_dialog_new_with_buttons("HOE : Creating Finding Charts",
				       NULL,
				       GTK_DIALOG_MODAL,
				       GTK_STOCK_CANCEL,GTK_RESPONSE_CANCEL,
				       GTK_STOCK_OK,GTK_RESPONSE_OK,
				       NULL);

  gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK); 


  hbox = gtk_hbox_new(FALSE,2);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 0);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),
		     hbox,FALSE, FALSE, 0);

  frame = gtk_frame_new ("Source");
  gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 5);

  table = gtk_table_new(5,2,FALSE);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_container_set_border_width (GTK_CONTAINER (table), 0);
  gtk_table_set_row_spacings (GTK_TABLE (table), 0);
  gtk_table_set_col_spacings (GTK_TABLE (table), 3);

  {
    GtkWidget *combo;
    GtkListStore *store;
    GtkTreeIter iter, iter_set;	  
    GtkCellRenderer *renderer;
    
    store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
    
    /*
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "DSS1 (Red) via STSCI",
		       1, FC_STSCI_DSS1R, -1);
    if(hg->fc_mode==FC_STSCI_DSS1R) iter_set=iter;
	
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "DSS1 (Blue) via STSCI",
		       1, FC_STSCI_DSS1B, -1);
    if(hg->fc_mode==FC_STSCI_DSS1B) iter_set=iter;
	
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "DSS2 (Red) via STSCI",
		       1, FC_STSCI_DSS2R, -1);
    if(hg->fc_mode==FC_STSCI_DSS2R) iter_set=iter;
	
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "DSS2 (Blue) via STSCI",
		       1, FC_STSCI_DSS2B, -1);
    if(hg->fc_mode==FC_STSCI_DSS2B) iter_set=iter;
	
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "DSS2 (IR) via STSCI",
		       1, FC_STSCI_DSS2IR, -1);
    if(hg->fc_mode==FC_STSCI_DSS2IR) iter_set=iter;
	
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "DSS2 (Red) via ESO",
		       1, FC_ESO_DSS2R, -1);
    if(hg->fc_mode==FC_ESO_DSS2R) iter_set=iter;
    */
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "DSS1 (Red) via SkyView",
		       1, FC_SKYVIEW_DSS1R, -1);
    if(hg->fc_mode==FC_SKYVIEW_DSS1R) iter_set=iter;
	
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "DSS1 (Blue) via SkyView",
		       1, FC_SKYVIEW_DSS1B, -1);
    if(hg->fc_mode==FC_SKYVIEW_DSS1B) iter_set=iter;
	
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "DSS2 (Red) via SkyView",
		       1, FC_SKYVIEW_DSS2R, -1);
    if(hg->fc_mode==FC_SKYVIEW_DSS2R) iter_set=iter;
	
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "DSS2 (Blue) via SkyView",
		       1, FC_SKYVIEW_DSS2B, -1);
    if(hg->fc_mode==FC_SKYVIEW_DSS2B) iter_set=iter;
	
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "DSS2 (IR) via SkyView",
		       1, FC_SKYVIEW_DSS2IR, -1);
    if(hg->fc_mode==FC_SKYVIEW_DSS2IR) iter_set=iter;
	
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "2MASS (J) via SkyView",
		       1, FC_SKYVIEW_2MASSJ, -1);
    if(hg->fc_mode==FC_SKYVIEW_2MASSJ) iter_set=iter;
	
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "2MASS (H) via SkyView",
		       1, FC_SKYVIEW_2MASSH, -1);
    if(hg->fc_mode==FC_SKYVIEW_2MASSH) iter_set=iter;
	
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "2MASS (K) via SkyView",
		       1, FC_SKYVIEW_2MASSK, -1);
    if(hg->fc_mode==FC_SKYVIEW_2MASSK) iter_set=iter;
	
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "SDSS DR7 quick view",
		       1, FC_SDSS, -1);
    if(hg->fc_mode==FC_SDSS) iter_set=iter;
	
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "SDSS DR12 quick view",
		       1, FC_SDSS12, -1);
    if(hg->fc_mode==FC_SDSS12) iter_set=iter;
	
    combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));
    gtk_table_attach (GTK_TABLE(table), combo, 1, 2, 1, 2,
		      GTK_SHRINK,GTK_SHRINK,0,0);
    //gtk_container_add (GTK_CONTAINER (hbox2), combo);
    g_object_unref(store);
	
    renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo),renderer, TRUE);
    gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT(combo), renderer, "text",0,NULL);
	
    gtk_combo_box_set_active_iter(GTK_COMBO_BOX(combo),&iter_set);
    gtk_widget_show(combo);
    my_signal_connect (combo,"changed",cc_get_fc_mode,
		       (gpointer)hg);
  }

  frame = gtk_frame_new ("Size [\']");
  gtk_table_attach (GTK_TABLE(table), frame, 2, 3, 0, 2,
		    GTK_SHRINK,GTK_SHRINK,0,0);
  //gtk_box_pack_start(GTK_BOX(hbox1), frame, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 0);

  hbox2 = gtk_hbox_new(FALSE,0);
  gtk_container_add (GTK_CONTAINER (frame), hbox2);

  adj = (GtkAdjustment *)gtk_adjustment_new(hg->dss_arcmin,
					    DSS_ARCMIN_MIN, DSS_ARCMIN_MAX,
					    1.0, 1.0, 0);
  spinner =  gtk_spin_button_new (adj, 0, 0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), FALSE);
  gtk_entry_set_editable(GTK_ENTRY(&GTK_SPIN_BUTTON(spinner)->entry),
			 TRUE);
  gtk_box_pack_start(GTK_BOX(hbox2),spinner,FALSE,FALSE,0);
  my_entry_set_width_chars(GTK_ENTRY(&GTK_SPIN_BUTTON(spinner)->entry),2);
  my_signal_connect (adj, "value_changed",
		     cc_get_adj,
		     &hg->dss_arcmin);


  frame = gtk_frame_new ("Scale");
  gtk_table_attach (GTK_TABLE(table), frame, 3, 4, 0, 2,
		    GTK_SHRINK,GTK_SHRINK,0,0);
  //gtk_box_pack_start(GTK_BOX(hbox1), frame, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 0);

  hbox2 = gtk_hbox_new(FALSE,0);
  gtk_container_add (GTK_CONTAINER (frame), hbox2);

  button=gtk_check_button_new_with_label("HistEq");
  gtk_container_set_border_width (GTK_CONTAINER (button), 0);
  gtk_box_pack_start(GTK_BOX(hbox2),button,FALSE,FALSE,0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),hg->dss_hist);
  my_signal_connect(button,"toggled",
		    G_CALLBACK (cc_get_toggle), 
		    &hg->dss_hist);

  frame = gtk_frame_new ("SDSS mark");
  gtk_table_attach (GTK_TABLE(table), frame, 4, 5, 0, 2,
		    GTK_SHRINK,GTK_SHRINK,0,0);
  //gtk_box_pack_start(GTK_BOX(hbox1), frame, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 0);

  hbox2 = gtk_hbox_new(FALSE,0);
  gtk_container_add (GTK_CONTAINER (frame), hbox2);

  button=gtk_check_button_new_with_label("Ph.");
  gtk_container_set_border_width (GTK_CONTAINER (button), 0);
  gtk_box_pack_start(GTK_BOX(hbox2),button,FALSE,FALSE,0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),hg->sdss_photo);
  my_signal_connect(button,"toggled",
		    G_CALLBACK (cc_get_toggle), 
		    &hg->sdss_photo);

  button=gtk_check_button_new_with_label("Sp.");
  gtk_container_set_border_width (GTK_CONTAINER (button), 0);
  gtk_box_pack_start(GTK_BOX(hbox2),button,FALSE,FALSE,0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),hg->sdss_spec);
  my_signal_connect(button,"toggled",
		    G_CALLBACK (cc_get_toggle), 
		    &hg->sdss_spec);


  frame = gtk_frame_new ("Instrument");
  gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 5);

  table = gtk_table_new(4,2,FALSE);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_container_set_border_width (GTK_CONTAINER (table), 0);
  gtk_table_set_row_spacings (GTK_TABLE (table), 0);
  gtk_table_set_col_spacings (GTK_TABLE (table), 3);


  {
    GtkWidget *combo;
    GtkListStore *store;
    GtkTreeIter iter, iter_set;	  
    GtkCellRenderer *renderer;
    
    store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
    
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "None",
		       1, FC_INST_NONE, -1);
    if(hg->fc_inst==FC_INST_NONE) iter_set=iter;
	
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "HDS",
		       1, FC_INST_HDS, -1);
    if(hg->fc_inst==FC_INST_HDS) iter_set=iter;
	
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "IRCS",
		       1, FC_INST_IRCS, -1);
    if(hg->fc_inst==FC_INST_IRCS) iter_set=iter;

    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "FOCAS",
		       1, FC_INST_FOCAS, -1);
    if(hg->fc_inst==FC_INST_FOCAS) iter_set=iter;

    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "MOIRCS",
		       1, FC_INST_MOIRCS, -1);
    if(hg->fc_inst==FC_INST_MOIRCS) iter_set=iter;

    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "SupCam",
		       1, FC_INST_SPCAM, -1);
    if(hg->fc_inst==FC_INST_SPCAM) iter_set=iter;

    combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));
    gtk_table_attach (GTK_TABLE(table), combo, 1, 2, 1, 2,
		      GTK_SHRINK,GTK_SHRINK,0,0);
    //gtk_container_add (GTK_CONTAINER (hbox2), combo);
    g_object_unref(store);
	
    renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo),renderer, TRUE);
    gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT(combo), renderer, "text",0,NULL);
	
    gtk_combo_box_set_active_iter(GTK_COMBO_BOX(combo),&iter_set);
    gtk_widget_show(combo);
    my_signal_connect (combo,"changed",cc_get_combo_box,
		       &hg->fc_inst);
  }

  button=gtk_check_button_new_with_label("Detail");
  gtk_container_set_border_width (GTK_CONTAINER (button), 0);
  gtk_table_attach (GTK_TABLE(table), button, 2, 3, 1, 2,
		    GTK_SHRINK,GTK_SHRINK,0,0);
  //gtk_box_pack_start(GTK_BOX(hbox2),button,FALSE,FALSE,0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),hg->dss_draw_slit);
  my_signal_connect(button,"toggled",
		    G_CALLBACK (cc_get_toggle), 
		    &hg->dss_draw_slit);



  frame = gtk_frame_new ("PA [deg]");
  gtk_table_attach (GTK_TABLE(table), frame, 3, 4, 0, 2,
		    GTK_SHRINK,GTK_SHRINK,0,0);
  //gtk_box_pack_start(GTK_BOX(hbox1), frame, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 0);

  hbox2 = gtk_hbox_new(FALSE,0);
  gtk_container_add (GTK_CONTAINER (frame), hbox2);

  adj = (GtkAdjustment *)gtk_adjustment_new(hg->dss_pa,
					    -360, 360,
					    1.0, 1.0, 0);
  spinner =  gtk_spin_button_new (adj, 0, 0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), TRUE);
  gtk_entry_set_editable(GTK_ENTRY(&GTK_SPIN_BUTTON(spinner)->entry),
			 TRUE);
  gtk_box_pack_start(GTK_BOX(hbox2),spinner,FALSE,FALSE,0);
  my_entry_set_width_chars(GTK_ENTRY(&GTK_SPIN_BUTTON(spinner)->entry),4);
  my_signal_connect (adj, "value_changed",
		     cc_get_adj,
		     &hg->dss_pa);


  button=gtk_check_button_new_with_label("Flip");
  gtk_container_set_border_width (GTK_CONTAINER (button), 0);
  gtk_box_pack_start(GTK_BOX(hbox2),button,FALSE,FALSE,0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),hg->dss_flip);
  my_signal_connect(button,"toggled",
		    G_CALLBACK (cc_get_toggle), 
		    &hg->dss_flip);
  
  gtk_widget_show_all(dialog);

  if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
    gtk_widget_destroy(dialog);
    fc_dl_draw_all(hg);
  }
  else{
    gtk_widget_destroy(dialog);
  }
}



void close_fc(GtkWidget *w, gpointer gdata)
{
  typHOE *hg;
  hg=(typHOE *)gdata;


  gtk_widget_destroy(GTK_WIDGET(hg->fc_main));
  flagFC=FALSE;
}


#ifndef USE_WIN32
static void cancel_fc(GtkWidget *w, gpointer gdata)
{
  typHOE *hg;
  pid_t child_pid=0;
  hg=(typHOE *)gdata;

  if(fc_pid){
    kill(fc_pid, SIGKILL);
    gtk_main_quit();

    do{
      int child_ret;
      child_pid=waitpid(fc_pid, &child_ret,WNOHANG);
    } while((child_pid>0)||(child_pid!=-1));
    fc_pid=0;
  }
}

static void cancel_fc_all(GtkWidget *w, gpointer gdata)
{
  typHOE *hg;
  pid_t child_pid=0;
  hg=(typHOE *)gdata;

  flag_dssall_kill=TRUE;

  if(fc_pid){
    kill(fc_pid, SIGKILL);
    gtk_main_quit();

    do{
      int child_ret;
      child_pid=waitpid(fc_pid, &child_ret,WNOHANG);
    } while((child_pid>0)||(child_pid!=-1));
    fc_pid=0;
  }
  else{
    gtk_main_quit();
  }
}
#endif


void draw_fc_obj(typHOE *hg, cairo_t *cr, gint width, gint height){
  cairo_text_extents_t extents;
  double x,y;
  gint i_list;
  gint from_set, to_rise;
  int width_file, height_file;
  gfloat r_w,r_h, r;
  GdkPixbuf *pixbuf_flip=NULL;

  gdouble ra_0, dec_0;
  gchar tmp[2048];
  gfloat x_ccd, y_ccd, gap_ccd;
  struct ln_hms ra_hms;
  struct ln_dms dec_dms;


  /* draw the background */
  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
  cairo_paint (cr);
  
  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);



  if(pixbuf_fc){
    width_file = gdk_pixbuf_get_width(pixbuf_fc);
    height_file = gdk_pixbuf_get_height(pixbuf_fc);
    
    r_w =  (gfloat)width/(gfloat)width_file;
    r_h =  (gfloat)height/(gfloat)height_file;
    
    if(pixbuf2_fc) g_object_unref(G_OBJECT(pixbuf2_fc));
    
    if(r_w>r_h){
      r=r_h;
    }
    else{
      r=r_w;
    }
    
    if(hg->dss_flip){
      pixbuf_flip=gdk_pixbuf_flip(pixbuf_fc,TRUE);
      pixbuf2_fc=gdk_pixbuf_scale_simple(pixbuf_flip,
					 (gint)((gdouble)width_file*r),
					 (gint)((gdouble)height_file*r),
					 GDK_INTERP_BILINEAR);
      g_object_unref(G_OBJECT(pixbuf_flip));
    }
    else{
      pixbuf2_fc=gdk_pixbuf_scale_simple(pixbuf_fc,
					 (gint)((gdouble)width_file*r),
					 (gint)((gdouble)height_file*r),
					 GDK_INTERP_BILINEAR);
    }

      
    cairo_save (cr);
    cairo_translate (cr, (width-(gint)((gdouble)width_file*r))/2,
		     (height-(gint)((gdouble)height_file*r))/2);

    cairo_translate (cr, (gdouble)width_file*r/2,
		     (gdouble)height_file*r/2);

    switch(hg->fc_inst){
    case FC_INST_NONE:
    case FC_INST_HDS:
    case FC_INST_IRCS:
    case FC_INST_FOCAS:
    case FC_INST_MOIRCS:
      if(hg->dss_flip){
	cairo_rotate (cr,-M_PI*(gdouble)hg->dss_pa/180.);
      }
      else{
	cairo_rotate (cr,M_PI*(gdouble)hg->dss_pa/180.);
      }

      break;

    case FC_INST_SPCAM:
      if(hg->dss_flip){
	cairo_rotate (cr,-M_PI*(gdouble)(90-hg->dss_pa)/180.);
      }
      else{
	cairo_rotate (cr,M_PI*(gdouble)(90-hg->dss_pa)/180.);
      }

    }
    cairo_translate (cr, -(gdouble)width_file*r/2,
		     -(gdouble)height_file*r/2);
    gdk_cairo_set_source_pixbuf(cr, pixbuf2_fc, 0, 0);
    
    cairo_rectangle(cr, 0,0,
		    (gint)((gdouble)width_file*r),
		    (gint)((gdouble)height_file*r));
    cairo_fill(cr);

    cairo_restore(cr);

    cairo_save(cr);
    cairo_translate (cr, (width-(gint)((gdouble)width_file*r))/2,
		     (height-(gint)((gdouble)height_file*r))/2);

    switch(hg->fc_inst){
    case FC_INST_HDS:
      if(hg->dss_draw_slit){
	cairo_arc(cr,((gdouble)width_file*r)/2,((gdouble)height_file*r)/2,
		  ((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip/2.,
		  0,M_PI*2);
	cairo_clip(cr);
	cairo_new_path(cr);
	
	cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 0.3);
	cairo_set_line_width (cr, (gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip/60.*HDS_SLIT_MASK_ARCSEC);
	
	cairo_move_to(cr,((gdouble)width_file*r)/2,
		      ((gdouble)height_file*r)/2-((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip/2.);
	cairo_line_to(cr,((gdouble)width_file*r)/2,
		      ((gdouble)height_file*r)/2-(gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*(gdouble)hg->setup[0].slit_length/2./500./60.);
	
	cairo_move_to(cr,((gdouble)width_file*r)/2,
		      ((gdouble)height_file*r)/2+(gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*(gdouble)hg->setup[0].slit_length/2./500./60.);
	cairo_line_to(cr,((gdouble)width_file*r)/2,
		      ((gdouble)height_file*r)/2+((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip/2.);
	cairo_stroke(cr);
	
	cairo_set_line_width (cr, (gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip/60.*hg->setup[0].slit_width/500.);
	cairo_move_to(cr,((gdouble)width_file*r)/2,
		      ((gdouble)height_file*r)/2-(gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*(gdouble)hg->setup[0].slit_length/2./500./60.);
	cairo_line_to(cr,((gdouble)width_file*r)/2,
		      ((gdouble)height_file*r)/2+(gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*(gdouble)hg->setup[0].slit_length/2./500./60.);
	cairo_stroke(cr);
	
	cairo_reset_clip(cr);
      }
      
      cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 0.6);
      cairo_set_line_width (cr, 3.0);
      
      cairo_arc(cr,((gdouble)width_file*r)/2,((gdouble)height_file*r)/2,
		((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip/2.,
		0,M_PI*2);
      cairo_stroke(cr);
      
      cairo_move_to(cr,
		    ((gdouble)width_file*r)/2+((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip/2.*cos(M_PI/4),
		    ((gdouble)height_file*r)/2-((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip/2.*sin(M_PI/4));
      cairo_set_line_width (cr, 1.5);
      cairo_line_to(cr,
		    ((gdouble)width_file*r)/2+1.5*((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip/2.*cos(M_PI/4),
		    ((gdouble)height_file*r)/2-1.5*((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip/2.*sin(M_PI/4));
      cairo_stroke(cr);
      
      cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 1.0);
      cairo_select_font_face (cr, SKYMON_FONT, CAIRO_FONT_SLANT_NORMAL,
			      CAIRO_FONT_WEIGHT_NORMAL);
      cairo_set_font_size (cr, 9.0);
      cairo_move_to(cr,
		    ((gdouble)width_file*r)/2+1.5*((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip/2.*cos(M_PI/4),
		    ((gdouble)height_file*r)/2-1.5*((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip/2.*sin(M_PI/4));
      cairo_show_text(cr, "HDS SV FOV (1arcmin)");
      
      break;


    case FC_INST_IRCS:
      cairo_translate(cr,((gdouble)width_file*r)/2,((gdouble)height_file*r)/2);

      cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 0.6);
      cairo_set_line_width (cr, 3.0);

      cairo_rectangle(cr,
		      -((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*IRCS_X_ARCSEC/60.)/2.,
		      -((gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*IRCS_Y_ARCSEC/60.)/2.,
		      (gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*IRCS_X_ARCSEC/60.,
		      (gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*IRCS_Y_ARCSEC/60.);
      cairo_stroke(cr);

      cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 1.0);
      cairo_select_font_face (cr, SKYMON_FONT, CAIRO_FONT_SLANT_NORMAL,
			      CAIRO_FONT_WEIGHT_NORMAL);
      cairo_set_font_size (cr, 9.0);

      sprintf(tmp,"IRCS FOV (%dx%darcsec)",(gint)IRCS_X_ARCSEC, (gint)IRCS_Y_ARCSEC);
      cairo_text_extents (cr,tmp, &extents);
      cairo_move_to(cr,-extents.width/2,
		    -((gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*IRCS_Y_ARCSEC/60.)/2.-5);
      cairo_show_text(cr, tmp);

      break;


    case FC_INST_FOCAS:
      cairo_translate(cr,((gdouble)width_file*r)/2,((gdouble)height_file*r)/2);

      cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 0.6);
      cairo_set_line_width (cr, 3.0);
      
      cairo_arc(cr,0,0,
		((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip/2.*FOCAS_R_ARCMIN,
		0,M_PI*2);
      cairo_stroke(cr);

      cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 1.0);
      cairo_select_font_face (cr, SKYMON_FONT, CAIRO_FONT_SLANT_NORMAL,
			      CAIRO_FONT_WEIGHT_NORMAL);
      cairo_set_font_size (cr, 9.0);
	
      sprintf(tmp,"FOCAS FOV (%darcmin)",FOCAS_R_ARCMIN);
      cairo_text_extents (cr,tmp, &extents);
      cairo_move_to(cr,
		    -extents.width/2,
		    -FOCAS_R_ARCMIN/2.*((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip)-5);
      cairo_show_text(cr, tmp);

      if(hg->dss_draw_slit){
	cairo_new_path(cr);
	cairo_arc(cr,0,0,
		  ((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip/2.*FOCAS_R_ARCMIN,
		  0,M_PI*2);
	cairo_clip(cr);
	cairo_new_path(cr);
	
	cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 0.6);
	cairo_set_line_width (cr, FOCAS_GAP_ARCSEC/60.*((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip));
	cairo_move_to(cr,-(gdouble)width/2,0);
	cairo_line_to(cr,(gdouble)width/2,0);
	cairo_stroke(cr);
	
	cairo_reset_clip(cr);

	cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 1.0);
	cairo_select_font_face (cr, SKYMON_FONT, CAIRO_FONT_SLANT_NORMAL,
				CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size (cr, 9.0);
	cairo_text_extents (cr,"Chip 2", &extents);

	cairo_move_to(cr,
		      cos(M_PI/4)*((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip/2.*FOCAS_R_ARCMIN+5,
		      -sin(M_PI/4)*((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip/2.*FOCAS_R_ARCMIN-5);
	cairo_show_text(cr,"Chip 2");
	
	cairo_move_to(cr,
		      cos(M_PI/4)*((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip/2.*FOCAS_R_ARCMIN+5,
		      sin(M_PI/4)*((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip/2.*FOCAS_R_ARCMIN+extents.height+5);
	cairo_show_text(cr,"Chip 1");
      }

      break;


    case FC_INST_MOIRCS:
      cairo_translate(cr,((gdouble)width_file*r)/2,((gdouble)height_file*r)/2);

      cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 0.6);
      cairo_set_line_width (cr, 3.0);

      cairo_rectangle(cr,
		      -((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_X_ARCMIN)/2.,
		      -((gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_Y_ARCMIN)/2.,
		      (gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_X_ARCMIN,
		      (gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_Y_ARCMIN);
      cairo_stroke(cr);

      cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 1.0);
      cairo_select_font_face (cr, SKYMON_FONT, CAIRO_FONT_SLANT_NORMAL,
			      CAIRO_FONT_WEIGHT_NORMAL);
      cairo_set_font_size (cr, 9.0);

      sprintf(tmp,"MOIRCS FOV (%dx%darcmin)",(gint)MOIRCS_X_ARCMIN, (gint)MOIRCS_Y_ARCMIN);
      cairo_text_extents (cr,tmp, &extents);
      cairo_move_to(cr,-extents.width/2,
		    -((gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_Y_ARCMIN)/2.-5);
      cairo_show_text(cr, tmp);

      if(hg->dss_draw_slit){
	cairo_new_path(cr);
	cairo_rectangle(cr,
			-((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_X_ARCMIN)/2.,
			-((gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_Y_ARCMIN)/2.,
			(gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_X_ARCMIN,
			(gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_Y_ARCMIN);
	cairo_clip(cr);
	cairo_new_path(cr);
	
	cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 0.6);
	cairo_set_line_width (cr, MOIRCS_GAP_ARCSEC/60.*((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip));
	cairo_move_to(cr,-(gdouble)width/2,0);
	cairo_line_to(cr,(gdouble)width/2,0);
	cairo_stroke(cr);
	
	cairo_move_to(cr,
		      -((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_X_ARCMIN)/2.,
		      ((gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_Y_ARCMIN)/2.);
	cairo_line_to(cr,
		      -((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_X_ARCMIN)/2.+MOIRCS_VIG1X_ARCSEC/60.*((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip),
		      ((gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_Y_ARCMIN)/2.);
	cairo_line_to(cr,
		      -((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_X_ARCMIN)/2.,
		      ((gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_Y_ARCMIN)/2.-MOIRCS_VIG1Y_ARCSEC/60.*((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip));
	cairo_close_path(cr);
	cairo_fill_preserve(cr);

	cairo_move_to(cr,
		      -((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_X_ARCMIN)/2.,
		      -((gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_Y_ARCMIN)/2.);
	cairo_line_to(cr,
		      -((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_X_ARCMIN)/2.+MOIRCS_VIG2X_ARCSEC/60.*((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip),
		      -((gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_Y_ARCMIN)/2.);
	cairo_line_to(cr,
		      -((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_X_ARCMIN)/2.,
		      -((gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_Y_ARCMIN)/2.+MOIRCS_VIG2Y_ARCSEC/60.*((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip));
	cairo_close_path(cr);
	cairo_fill_preserve(cr);

	cairo_new_path(cr);

	cairo_reset_clip(cr);

	cairo_set_line_width(cr,1.5);
	cairo_arc(cr,0,0,
		  (gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_VIGR_ARCMIN/2.,
		  0,M_PI*2);
	cairo_stroke(cr);

	cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 1.0);
	cairo_select_font_face (cr, SKYMON_FONT, CAIRO_FONT_SLANT_NORMAL,
				CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size (cr, 9.0);
	cairo_text_extents (cr,"Detector 2", &extents);

	cairo_move_to(cr,
		      ((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_X_ARCMIN)/2.+5,
		      -((gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_Y_ARCMIN)/2.+extents.height);
	cairo_show_text(cr,"Detector 2");
	
	cairo_move_to(cr,
		      ((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_X_ARCMIN)/2.+5,
		      ((gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_Y_ARCMIN)/2.);
	cairo_show_text(cr,"Detector 1");

	cairo_rotate (cr,-M_PI/2);
	cairo_text_extents (cr,"6 arcmin from the center", &extents);
	cairo_move_to(cr,-extents.width/2.,
		      -(gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_VIGR_ARCMIN/2.-5);
	cairo_show_text(cr,"6 arcmin from the center");
      }


      break;


    case FC_INST_SPCAM:
      cairo_translate(cr,((gdouble)width_file*r)/2,((gdouble)height_file*r)/2);

      cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 0.6);
      cairo_set_line_width (cr, 3.0);

      cairo_rectangle(cr,
		      -((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*SPCAM_X_ARCMIN)/2.,
		      -((gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*SPCAM_Y_ARCMIN)/2.,
		      (gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*SPCAM_X_ARCMIN,
		      (gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*SPCAM_Y_ARCMIN);
      cairo_stroke(cr);

      cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 1.0);
      cairo_select_font_face (cr, SKYMON_FONT, CAIRO_FONT_SLANT_NORMAL,
			      CAIRO_FONT_WEIGHT_NORMAL);
      cairo_set_font_size (cr, 9.0);

      sprintf(tmp,"Suprime-Cam FOV (%dx%darcmin)",SPCAM_X_ARCMIN, SPCAM_Y_ARCMIN);
      cairo_text_extents (cr,tmp, &extents);
      cairo_move_to(cr,-extents.width/2,
		    -((gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*SPCAM_Y_ARCMIN)/2.-5);
      cairo_show_text(cr, tmp);

      if(hg->dss_draw_slit){
	cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 0.3);
	cairo_set_line_width (cr, 1.5);

	x_ccd=0.20/60.*2048.*((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip);
	y_ccd=0.20/60.*4096.*((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip);
	gap_ccd=SPCAM_GAP_ARCSEC/60.*((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip);
	//2 fio
	cairo_rectangle(cr,-x_ccd/2.,-y_ccd-gap_ccd/2.,
			x_ccd,y_ccd);
	//5 satsuki
	cairo_rectangle(cr,-x_ccd/2.,+gap_ccd/2.,
			x_ccd,y_ccd);

	//7 clarisse
	cairo_rectangle(cr,-x_ccd/2*3.-gap_ccd,-y_ccd-gap_ccd/2.,
			x_ccd,y_ccd);
	//9 san
	cairo_rectangle(cr,-x_ccd/2.*3.-gap_ccd,+gap_ccd/2.,
			x_ccd,y_ccd);

	//6 chihiro
	cairo_rectangle(cr,-x_ccd/2*5.-gap_ccd*2.,-y_ccd-gap_ccd/2.,
			x_ccd,y_ccd);
	//8 ponyo
	cairo_rectangle(cr,-x_ccd/2.*5.-gap_ccd*2.,+gap_ccd/2.,
			x_ccd,y_ccd);

	//2 fio
	cairo_rectangle(cr,x_ccd/2.+gap_ccd,-y_ccd-gap_ccd/2.,
			x_ccd,y_ccd);
	//5 satsuki
	cairo_rectangle(cr,x_ccd/2.+gap_ccd,+gap_ccd/2.,
			x_ccd,y_ccd);

	//0 nausicca
	cairo_rectangle(cr,x_ccd/2.*3.+gap_ccd*2.,-y_ccd-gap_ccd/2.,
			x_ccd,y_ccd);
	//3 sophie
	cairo_rectangle(cr,x_ccd/2.*3.+gap_ccd*2,+gap_ccd/2.,
			x_ccd,y_ccd);


	cairo_stroke(cr);

	cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 1.0);
	cairo_select_font_face (cr, SKYMON_FONT, CAIRO_FONT_SLANT_NORMAL,
				CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size (cr, 9.0);
	cairo_text_extents (cr,"2. fio", &extents);

	//2 fio
	cairo_move_to(cr,-x_ccd/2.+15,-y_ccd-gap_ccd/2.+15+extents.height);
	cairo_show_text(cr,"2. fio");

	//5 satsuki
	cairo_move_to(cr,-x_ccd/2.+15,+gap_ccd/2.+y_ccd-15);
	cairo_show_text(cr,"5. satsuki");

	//7 clarisse
	cairo_move_to(cr,-x_ccd/2*3.-gap_ccd+15,-y_ccd-gap_ccd/2.+15+extents.height);
	cairo_show_text(cr,"7. clarisse");

	//9 san
	cairo_move_to(cr,-x_ccd/2.*3.-gap_ccd+15,+gap_ccd/2.+y_ccd-15);
	cairo_show_text(cr,"9. san");

	//6 chihiro
	cairo_move_to(cr,-x_ccd/2*5.-gap_ccd*2.+15,-y_ccd-gap_ccd/2.+15+extents.height);
	cairo_show_text(cr,"6. chihiro");

	//8 ponyo
	cairo_move_to(cr,-x_ccd/2.*5.-gap_ccd*2.+15,+gap_ccd/2.+y_ccd-15);
	cairo_show_text(cr,"8. ponyo");

	//1 kiki
	cairo_move_to(cr,x_ccd/2.+gap_ccd+15,-y_ccd-gap_ccd/2.+15+extents.height);
	cairo_show_text(cr,"1. kiki");

	//4 sheeta
	cairo_move_to(cr,x_ccd/2.+gap_ccd+15,+gap_ccd/2.+y_ccd-15);
	cairo_show_text(cr,"4. sheeta");

	//0 nausicaa
	cairo_move_to(cr,x_ccd/2.*3.+gap_ccd*2.+15,-y_ccd-gap_ccd/2.+15+extents.height);
	cairo_show_text(cr,"0. nausicaa");

	//3 sophie
	cairo_move_to(cr,x_ccd/2.*3.+gap_ccd*2+15,+gap_ccd/2.+y_ccd-15);
	cairo_show_text(cr,"3. sophie");
      }

      break;
    }

    
    cairo_restore(cr);

    cairo_save(cr);
    cairo_translate (cr, (width-(gint)((gdouble)width_file*r))/2,
		     (height-(gint)((gdouble)height_file*r))/2);

    ra_0=hg->obj[hg->dss_i].ra;
    ra_hms.hours=(gint)(ra_0/10000);
    ra_0=ra_0-(gdouble)(ra_hms.hours)*10000;
    ra_hms.minutes=(gint)(ra_0/100);
    ra_hms.seconds=ra_0-(gdouble)(ra_hms.minutes)*100;
    
    if(hg->obj[hg->dss_i].dec<0){
      dec_dms.neg=1;
      dec_0=-hg->obj[hg->dss_i].dec;
    }
    else{
      dec_dms.neg=0;
      dec_0=hg->obj[hg->dss_i].dec;
    }
    dec_dms.degrees=(gint)(dec_0/10000);
    dec_0=dec_0-(gfloat)(dec_dms.degrees)*10000;
    dec_dms.minutes=(gint)(dec_0/100);
    dec_dms.seconds=dec_0-(gfloat)(dec_dms.minutes)*100;

    
    cairo_set_source_rgba(cr, 1.0, 1.0, 0.4, 1.0);
    cairo_select_font_face (cr, SKYMON_FONT, CAIRO_FONT_SLANT_NORMAL,
			  CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size (cr, 11.0);
    cairo_move_to(cr,5,(gdouble)height_file*r-5);
    sprintf(tmp,"RA=%02d:%02d:%05.2lf  Dec=%s%02d:%02d:%05.2lf",
	    ra_hms.hours,ra_hms.minutes,ra_hms.seconds,
	    (dec_dms.neg) ? "-" : "+", 
	    dec_dms.degrees, dec_dms.minutes,dec_dms.seconds);
    cairo_text_extents (cr, tmp, &extents);
    cairo_show_text(cr,tmp);

    cairo_select_font_face (cr, SKYMON_FONT, CAIRO_FONT_SLANT_NORMAL,
			  CAIRO_FONT_WEIGHT_BOLD);
    cairo_move_to(cr,5,(gdouble)height_file*r-5-extents.height-5);
    cairo_show_text(cr,hg->obj[hg->dss_i].name);



    cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 1.0);
    cairo_select_font_face (cr, SKYMON_FONT, CAIRO_FONT_SLANT_NORMAL,
			  CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size (cr, 10.0);
    sprintf(tmp,"%dx%d arcmin",
	    hg->dss_arcmin_ip,hg->dss_arcmin_ip);
    cairo_text_extents (cr, tmp, &extents);
    cairo_move_to(cr,
		  (gdouble)width_file*r-extents.width-5,
		  extents.height+5);
    cairo_show_text(cr,tmp);

    cairo_restore(cr);


    cairo_save (cr);

    cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 1.0);
    cairo_select_font_face (cr, SKYMON_FONT, CAIRO_FONT_SLANT_NORMAL,
			  CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size (cr, 11.0);
    cairo_text_extents (cr, "N", &extents);

    cairo_translate (cr, (width-(gint)((gdouble)width_file*r))/2,
		     (height-(gint)((gdouble)height_file*r))/2);
    cairo_translate (cr, 
		     5+(gdouble)width_file*r*0.05+extents.width*1.5,
		     5+(gdouble)width_file*r*0.05+extents.height*1.5);
    switch(hg->fc_inst){
    case FC_INST_NONE:
    case FC_INST_HDS:
    case FC_INST_IRCS:
    case FC_INST_FOCAS:
    case FC_INST_MOIRCS:
      if(hg->dss_flip){
	cairo_rotate (cr,-M_PI*(gdouble)hg->dss_pa/180.);
      }
      else{
	cairo_rotate (cr,M_PI*(gdouble)hg->dss_pa/180.);
      }
      break;

    case FC_INST_SPCAM:
      if(hg->dss_flip){
	cairo_rotate (cr,-M_PI*(gdouble)(90-hg->dss_pa)/180.);
      }
      else{
	cairo_rotate (cr,M_PI*(gdouble)(90-hg->dss_pa)/180.);
      }
      break;
    }

    
    cairo_move_to(cr,
		  -extents.width/2,
		  -(gdouble)width_file*r*0.05);
    cairo_show_text(cr,"N");
    cairo_move_to(cr,
		  -(gdouble)width_file*r*0.05-extents.width,
		  +extents.height/2);
    if(hg->dss_flip){
      cairo_show_text(cr,"W");
    }
    else{
      cairo_show_text(cr,"E");
    }

    cairo_set_line_width (cr, 1.5);
    cairo_move_to(cr,
		  0,
		  -(gdouble)width_file*r*0.05);
    cairo_line_to(cr, 0, 0);
    cairo_line_to(cr,
		  -(gdouble)width_file*r*0.05, 0);
    cairo_stroke(cr);

    if(hg->dss_flip){
      cairo_move_to(cr,0,0);
      cairo_text_extents (cr, "(flipped)", &extents);
      cairo_rel_move_to(cr,-extents.width/2.,extents.height+5);
      cairo_show_text(cr,"(flipped)");
    }

    
    cairo_restore(cr);

  }

  if(hg->fc_output!=FC_OUTPUT_WINDOW){
    cairo_show_page(cr); 
  }
}

gboolean draw_fc_cairo(GtkWidget *widget, 
		       GdkEventExpose *event, 
		       gpointer userdata){
  cairo_t *cr;
  GdkPixmap *pixmap_fc=NULL;
  cairo_surface_t *surface=NULL;
  typHOE *hg;
  int width, height;


  
  if(!flagFC) return (FALSE);

  hg=(typHOE *)userdata;

  if(hg->fc_output!=FC_OUTPUT_WINDOW){
    width= PLOT_HEIGHT;
    height= PLOT_HEIGHT;

    surface = cairo_pdf_surface_create(hg->filename_pdf, width, height);
    cr = cairo_create(surface); 

    cairo_set_source_rgb(cr, 1, 1, 1);
  }
  else{
    width= widget->allocation.width;
    height= widget->allocation.height;
    if(width<=1){
      gtk_window_get_size(GTK_WINDOW(hg->fc_main), &width, &height);
    }
    
    pixmap_fc = gdk_pixmap_new(widget->window,
			       width,
			       height,
			       -1);
  
    cr = gdk_cairo_create(pixmap_fc);

    cairo_set_source_rgba(cr, 0.3, 0.3, 0.3, 1.0);
  }

  draw_fc_obj(hg,cr,width,height);


  if(hg->fc_output!=FC_OUTPUT_WINDOW){
    cairo_surface_destroy(surface);
  }

  cairo_destroy(cr);

  if(hg->fc_output==FC_OUTPUT_WINDOW){
    gdk_draw_drawable(widget->window,
		      widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
		      pixmap_fc,
		      0,0,0,0,
		      width,
		      height);

    g_object_unref(G_OBJECT(pixmap_fc));
  }

  return TRUE;

}


static void refresh_fc (GtkWidget *widget, gpointer data)
{
  typHOE *hg = (typHOE *)data;

  if(flagFC){
    hg->fc_output=FC_OUTPUT_WINDOW;
    draw_fc_cairo(hg->fc_dw,NULL,
		  (gpointer)hg);
  }
}


void cc_get_fc_mode (GtkWidget *widget,  gpointer gdata)
{
  GtkTreeIter iter;
  typHOE *hg = (typHOE *)gdata;

  if(gtk_combo_box_get_active_iter(GTK_COMBO_BOX(widget), &iter)){
    gint n;
    GtkTreeModel *model;
    
    model=gtk_combo_box_get_model(GTK_COMBO_BOX(widget));
    gtk_tree_model_get (model, &iter, 1, &n, -1);

    hg->fc_mode=n;

    switch(hg->fc_mode){
    case FC_STSCI_DSS1R:
    case FC_STSCI_DSS1B:
    case FC_STSCI_DSS2R:
    case FC_STSCI_DSS2B:
    case FC_STSCI_DSS2IR:
      if(hg->dss_host) g_free(hg->dss_host);
      hg->dss_host             =g_strdup(FC_HOST_STSCI);
      if(hg->dss_file) g_free(hg->dss_file);
      hg->dss_file             =g_strdup(FC_FILE_GIF);
      if(hg->dss_path) g_free(hg->dss_path);
      hg->dss_path             =g_strdup(FC_PATH_STSCI);

      if(hg->dss_src) g_free(hg->dss_src);
      switch(hg->fc_mode){
      case FC_STSCI_DSS1R:
	hg->dss_src             =g_strdup(FC_SRC_STSCI_DSS1R);
	break;
      case FC_STSCI_DSS1B:
	hg->dss_src             =g_strdup(FC_SRC_STSCI_DSS1B);
	break;
      case FC_STSCI_DSS2R:
	hg->dss_src             =g_strdup(FC_SRC_STSCI_DSS2R);
	break;
      case FC_STSCI_DSS2B:
	hg->dss_src             =g_strdup(FC_SRC_STSCI_DSS2B);
	break;
      case FC_STSCI_DSS2IR:
	hg->dss_src             =g_strdup(FC_SRC_STSCI_DSS2IR);
	break;
      }
      break;

    case FC_ESO_DSS2R:
      if(hg->dss_host) g_free(hg->dss_host);
      hg->dss_host             =g_strdup(FC_HOST_ESO);
      if(hg->dss_path) g_free(hg->dss_path);
      hg->dss_path             =g_strdup(FC_PATH_ESO);
      if(hg->dss_file) g_free(hg->dss_file);
      hg->dss_file             =g_strdup(FC_FILE_GIF);
      if(hg->dss_tmp) g_free(hg->dss_tmp);
      hg->dss_tmp             =g_strdup(FC_FILE_HTML);
      if(hg->dss_src) g_free(hg->dss_src);
      hg->dss_src             =g_strdup(FC_SRC_ESO_DSS2R);
      break;

    case FC_SKYVIEW_DSS1R:
    case FC_SKYVIEW_DSS1B:
    case FC_SKYVIEW_DSS2R:
    case FC_SKYVIEW_DSS2B:
    case FC_SKYVIEW_DSS2IR:
    case FC_SKYVIEW_2MASSJ:
    case FC_SKYVIEW_2MASSH:
    case FC_SKYVIEW_2MASSK:
      if(hg->dss_host) g_free(hg->dss_host);
      hg->dss_host             =g_strdup(FC_HOST_SKYVIEW);
      if(hg->dss_path) g_free(hg->dss_path);
      hg->dss_path             =g_strdup(FC_PATH_SKYVIEW);
      if(hg->dss_file) g_free(hg->dss_file);
      hg->dss_file             =g_strdup(FC_FILE_JPEG);
      if(hg->dss_tmp) g_free(hg->dss_tmp);
      hg->dss_tmp             =g_strdup(FC_FILE_HTML);
      if(hg->dss_src) g_free(hg->dss_src);
      switch(hg->fc_mode){
      case FC_SKYVIEW_DSS1R:
	hg->dss_src             =g_strdup(FC_SRC_SKYVIEW_DSS1R);
	break;
      case FC_SKYVIEW_DSS1B:
	hg->dss_src             =g_strdup(FC_SRC_SKYVIEW_DSS1B);
	break;
      case FC_SKYVIEW_DSS2R:
	hg->dss_src             =g_strdup(FC_SRC_SKYVIEW_DSS2R);
	break;
      case FC_SKYVIEW_DSS2B:
	hg->dss_src             =g_strdup(FC_SRC_SKYVIEW_DSS2B);
	break;
      case FC_SKYVIEW_DSS2IR:
	hg->dss_src             =g_strdup(FC_SRC_SKYVIEW_DSS2IR);
	break;
      case FC_SKYVIEW_2MASSJ:
	hg->dss_src             =g_strdup(FC_SRC_SKYVIEW_2MASSJ);
	break;
      case FC_SKYVIEW_2MASSH:
	hg->dss_src             =g_strdup(FC_SRC_SKYVIEW_2MASSH);
	break;
      case FC_SKYVIEW_2MASSK:
	hg->dss_src             =g_strdup(FC_SRC_SKYVIEW_2MASSK);
	break;
      }
      break;

    case FC_SDSS:
      if(hg->dss_host) g_free(hg->dss_host);
      hg->dss_host             =g_strdup(FC_HOST_SDSS);
      if(hg->dss_path) g_free(hg->dss_path);
      hg->dss_path             =g_strdup(FC_PATH_SDSS);
      if(hg->dss_file) g_free(hg->dss_file);
      hg->dss_file             =g_strdup(FC_FILE_JPEG);
      break; 

    case FC_SDSS12:
      if(hg->dss_host) g_free(hg->dss_host);
      hg->dss_host             =g_strdup(FC_HOST_SDSS12);
      if(hg->dss_path) g_free(hg->dss_path);
      hg->dss_path             =g_strdup(FC_PATH_SDSS12);
      if(hg->dss_file) g_free(hg->dss_file);
      hg->dss_file             =g_strdup(FC_FILE_JPEG);
    break;
   }
  }
}

void pdf_fc (typHOE *hg)
{
  hg->fc_output=FC_OUTPUT_PDF;

  if(flagFC){
    draw_fc_cairo(hg->fc_dw,NULL,
		  (gpointer)hg);
  }

  hg->fc_output=FC_OUTPUT_WINDOW;
}

#ifndef USE_WIN32
void dss_signal(int sig){
  pid_t child_pid=0;
  
  gtk_main_quit();

  do{
    int child_ret;
    child_pid=waitpid(fc_pid, &child_ret,WNOHANG);
  } while((child_pid>0)||(child_pid!=-1));
  
}


void dssall_signal(int sig){
  pid_t child_pid=0;

  flag_dssall_finish=TRUE;

  do{
    int child_ret;
    child_pid=waitpid(fc_pid, &child_ret,WNOHANG);
  } while((child_pid>0)||(child_pid!=-1));
  
}

#endif

gboolean check_dssall (gpointer gdata){
  if(flag_dssall_finish){
    flag_dssall_finish=FALSE;
      gtk_main_quit();
  }
  return(TRUE);
}

glong get_file_size(gchar *fname)
{
  FILE *fp;
  long sz;

  fp = fopen( fname, "rb" );
  if( fp == NULL ){
    return -1;
  }

  fseek( fp, 0, SEEK_END );
  sz = ftell( fp );

  fclose( fp );
  return sz;
}
