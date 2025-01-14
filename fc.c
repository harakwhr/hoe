//    HDS OPE file Editor
//      fc.c : Finding Chart
//                                           2010.3.15  A.Tajitsu

#include"main.h"   
#include "fc.h"


static void fcdb_toggle (GtkWidget *widget, gpointer data)
{
  typHOE *hg = (typHOE *)data;

  hg->fcdb_flag=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

  if(flagFC)  draw_fc_cairo(hg->fc_dw, hg);
}


void fc_item_redl (GtkWidget *widget, gpointer data)
{
  typHOE *hg = (typHOE *)data;
  
  fc_item2(hg, FC_MODE_REDL);
}


void fc_item2 (typHOE *hg, gint mode_switch)
{
  fc_dl(hg, mode_switch);

  hg->dss_arcmin_ip=hg->dss_arcmin;
  hg->fc_mode_get=hg->fc_mode;
  //#ifndef USE_WIN32
  if(hg->fc_pid){
    //#endif
    if(pixbuf_fc)  g_object_unref(G_OBJECT(pixbuf_fc));
    pixbuf_fc = gdk_pixbuf_new_from_file(hg->dss_file, NULL);

    do_fc(hg);
    //#ifndef USE_WIN32
  }
  //#endif
  
  //fcdb_clear_tree(hg,FALSE);

  if(hg->fcdb_auto) fcdb_item(NULL, (gpointer)hg);
}


void fc_dl (typHOE *hg, gint mode_switch)
{
  GtkTreeIter iter;
  GtkWidget *button;
  gint timer=-1;
  gint mode;
  GtkTreeModel *model;
  GtkTreeSelection *selection;
  gchar *tmp;

  if(flag_getDSS) return;
  flag_getDSS=TRUE;
  
  switch(mode_switch){
  case FC_MODE_OBJ:
  case FC_MODE_TRDB:

    switch(mode_switch){
    case FC_MODE_OBJ:
      model = gtk_tree_view_get_model(GTK_TREE_VIEW(hg->objtree));
      selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(hg->objtree));
      break;

    case FC_MODE_TRDB:
      model = gtk_tree_view_get_model(GTK_TREE_VIEW(hg->trdb_tree));
      selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(hg->trdb_tree));
      break;
    }

    if (gtk_tree_selection_get_selected (selection, NULL, &iter)){
      gint i, i_list;
      GtkTreePath *path;
    
      path = gtk_tree_model_get_path (model, &iter);
      
      switch(mode_switch){
      case FC_MODE_OBJ:
	gtk_tree_model_get (model, &iter, COLUMN_OBJTREE_NUMBER, &i, -1);
	break;

      case FC_MODE_TRDB:
	gtk_tree_model_get (model, &iter, COLUMN_TRDB_NUMBER, &i, -1);
	break;
      }
      i--;

      if(i!=hg->dss_i) hg->fcdb_i_max=0;
      
      hg->dss_i=i;

      gtk_tree_path_free (path);
    }
    else{
      popup_message(hg->w_top, 
#ifdef USE_GTK3
		    "dialog-warning", 
#else
		    GTK_STOCK_DIALOG_WARNING,
#endif
		    POPUP_TIMEOUT,
		    "<b>Error</b>: Please select a target in the Object List.",
		    NULL);
      flag_getDSS=FALSE;
      return;
    }
    break;

  default:
    // Just use same hg->dss_i
    break;
  }

  tmp=g_strdup_printf("Retrieving <b>%s</b> image from \"<b>%s</b>\" ...",
		      FC_img[hg->fc_mode], FC_host[hg->fc_mode]);
  create_pdialog(hg,
		 (flagFC) ? hg->fc_main : hg->w_top,
		 "HOE : Downloading Finding Chart",
		 tmp,
		 FALSE, FALSE);
  g_free(tmp);
  my_signal_connect(hg->pdialog,"delete-event", delete_fc, (gpointer)hg);

  tmp=g_strdup_printf("Downloading Finding Charts ...");
  gtk_label_set_markup(GTK_LABEL(hg->plabel), tmp);
  g_free(tmp);

#ifdef USE_GTK3
  button=gtkut_button_new_from_icon_name("Cancel","process-stop");
#else
  button=gtkut_button_new_from_stock("Cancel",GTK_STOCK_CANCEL);
#endif
  gtk_dialog_add_action_widget(GTK_DIALOG(hg->pdialog),button,GTK_RESPONSE_CANCEL);
  my_signal_connect(button,"pressed",
		    thread_cancel_fc,
		    (gpointer)hg);
 
  unlink(hg->dss_file); 
  
  gtk_widget_show_all(hg->pdialog);

  timer=g_timeout_add(100, 
		      (GSourceFunc)progress_timeout,
		      (gpointer)hg);
  
  gtk_window_set_modal(GTK_WINDOW(hg->pdialog),TRUE);

  hg->ploop=g_main_loop_new(NULL, FALSE);
  hg->pcancel=g_cancellable_new();
  hg->pthread=g_thread_new("hoe_get_dss", thread_get_dss, (gpointer)hg);
  g_main_loop_run(hg->ploop);
  //g_thread_join(hg->pthread);
  g_main_loop_unref(hg->ploop);
  hg->ploop=NULL;

  gtk_window_set_modal(GTK_WINDOW(hg->pdialog),FALSE);
  if(timer!=-1) g_source_remove(timer);
  gtk_widget_destroy(hg->pdialog);
  
  flag_getDSS=FALSE;
}


void fc_dl_draw_all (typHOE *hg)
{
  GtkTreeIter iter;
  gchar *tmp;
  GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(hg->objtree));
  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(hg->objtree));
  GtkWidget *button;
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
  

  hg->fc_mode_get=hg->fc_mode;
  tmp=g_strdup_printf("Retrieving <b>%s</b> image from \"<b>%s</b>\" ...",
		      FC_img[hg->fc_mode], FC_host[hg->fc_mode]);
  create_pdialog(hg,
		 (flagFC) ? hg->fc_main : hg->w_top,
		 "HOE : Creating Finding Chart PDF",
		 tmp,
		 TRUE, FALSE);
  g_free(tmp);
  my_signal_connect(hg->pdialog,"delete-event", delete_fc, (gpointer)hg);

  gtk_label_set_markup(GTK_LABEL(hg->plabel),
		       "Retrieving image from website ...");

#ifdef USE_GTK3
  button=gtkut_button_new_from_icon_name("Cancel","process-stop");
#else
  button=gtkut_button_new_from_stock("Cancel",GTK_STOCK_CANCEL);
#endif
  gtk_dialog_add_action_widget(GTK_DIALOG(hg->pdialog),button,GTK_RESPONSE_CANCEL);
  my_signal_connect(button,"pressed",
		    thread_cancel_fc, 
		    (gpointer)hg);
    
  gtk_widget_show_all(hg->pdialog);
  

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
    if((get_all) || (hg->obj[i_list].check_sm)){
      if((!hg->fc_all_magskip)||
	 ((hg->obj[i_list].mag>10)&&(hg->obj[i_list].mag<99))){
	hg->dss_i=i_list;

	hg->dss_arcmin_ip=hg->dss_arcmin;
	
	timer=g_timeout_add(100, 
			    (GSourceFunc)progress_timeout,
			    (gpointer)hg);
	
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(hg->pbar),"Downloading ...");
	
	hg->ploop=g_main_loop_new(NULL, FALSE);
	hg->pcancel=g_cancellable_new();
	hg->pthread=g_thread_new("hoe_get_dss", thread_get_dss, (gpointer)hg);
	g_main_loop_run(hg->ploop);
	//g_thread_join(hg->pthread);
	g_main_loop_unref(hg->ploop);
	hg->ploop=NULL;

	flag_dssall_finish=TRUE;

	g_source_remove(timer);
	if(hg->pabort) flag_dssall_kill=TRUE;
	if(flag_dssall_kill){
	  flag_dssall_kill=FALSE;
	  flag_dssall_finish=FALSE;
	  break;
	}
	else{
	  gtk_progress_bar_set_text(GTK_PROGRESS_BAR(hg->pbar),"Creating PDF ...");
	  //#ifndef USE_WIN32
	  if(hg->fc_pid){
	    //#endif
	    if(pixbuf_fc)  g_object_unref(G_OBJECT(pixbuf_fc));
	    pixbuf_fc = gdk_pixbuf_new_from_file(hg->dss_file, NULL);
	    
	    draw_fc_obj(hg,cr,PLOT_HEIGHT,PLOT_HEIGHT);
	    //#ifndef USE_WIN32
	  }
	  //#endif
	  
	  gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(hg->pbar2),
					(get_all) ? 
					(gdouble)(hg->dss_i+1)/(gdouble)(hg->i_max) :
					(gdouble)(hg->dss_i+1)/(gdouble)(total_check));
	  tmp=g_strdup_printf("Finished [ %d / %d ] Objects",i_list+1,
			      (get_all) ? hg->i_max : total_check);
	  gtk_progress_bar_set_text(GTK_PROGRESS_BAR(hg->pbar2),tmp);
	  g_free(tmp);
	  
	  flag_dssall_finish=FALSE;
	}
      }
    }
  }

  g_source_remove(dssall_check_timer);
  gtk_widget_destroy(hg->pdialog);

  cairo_surface_destroy(surface);
  cairo_destroy(cr);

  hg->fc_output=FC_OUTPUT_WINDOW;
  flag_getDSS=FALSE;
}


void close_hsc_dither(GtkWidget *w, gpointer gdata)
{
  typHOE *hg=(typHOE *)gdata;
    
  gtk_widget_destroy(hg->hsc_show_main);
  flagHSCDialog=FALSE;
  hg->hsc_show_ol=FALSE;
  
  if(flagFC){
    hg->fc_output=FC_OUTPUT_WINDOW;
    draw_fc_cairo(hg->fc_dw,hg);
  }
}

void set_hsc_dither (GtkWidget *widget, gpointer gdata)
{
  GtkWidget *dialog, *label, *button, *hbox, *hbox2, *frame,
    *spinner, *table, *check;
  GtkAdjustment *adj;
  typHOE *hg;
  gint i;

  if(flagHSCDialog){
    return;
  }
  else{
    flagHSCDialog=TRUE;
  }

  hg=(typHOE *)gdata;
  hg->hsc_show_dith_p=HSC_DITH_NO;
  hg->hsc_show_dith_i=1;
  
  hg->hsc_show_main = gtk_dialog_new();
  gtk_container_set_border_width(GTK_CONTAINER(hg->hsc_show_main),5);
  gtk_window_set_title(GTK_WINDOW(hg->hsc_show_main),
		       "HOE : HSC Dithering Parameters");

  my_signal_connect(hg->hsc_show_main,"destroy",
		    close_hsc_dither, 
		    (gpointer)hg);

  hbox = gtkut_hbox_new(FALSE,0);
  gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(hg->hsc_show_main))),
		     hbox,FALSE, FALSE, 0);

  label=gtk_label_new("Dither Type");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
  
  
  {
    GtkWidget *combo;
    GtkListStore *store;
    GtkTreeIter iter, iter_set;	  
    GtkCellRenderer *renderer;
    
    store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
    
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "No dither",
		       1, HSC_DITH_NO, -1);
    if(hg->hsc_show_dith_p==HSC_DITH_NO) iter_set=iter;
    
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "5-shot",
		       1, HSC_DITH_5, -1);
    if(hg->hsc_show_dith_p==HSC_DITH_5) iter_set=iter;
    
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "N-shot",
		       1, HSC_DITH_N, -1);
    if(hg->hsc_show_dith_p==HSC_DITH_N) iter_set=iter;
    
    
    combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));
    gtk_box_pack_start(GTK_BOX(hbox),combo,FALSE,FALSE,0);
    g_object_unref(store);
    
    renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo),renderer, TRUE);
    gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT(combo), renderer, "text",0,NULL);
    
    gtk_combo_box_set_active_iter(GTK_COMBO_BOX(combo),&iter_set);
    gtk_widget_show(combo);
    my_signal_connect (combo,"changed",cc_get_hsc_show_dith, (gpointer)hg);
  }

  label=gtk_label_new(" ");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
#endif
  gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 5);

#ifdef USE_GTK3
  button=gtkut_button_new_from_icon_name(NULL,"go-previous");
#else
  button=gtkut_button_new_from_stock(NULL,GTK_STOCK_GO_BACK);
#endif
  gtk_box_pack_start(GTK_BOX(hbox),
		     button,FALSE,FALSE,0);
  my_signal_connect (button,"clicked",hsc_show_dith_back, (gpointer)hg);

  hg->hsc_label_dith=gtk_label_new(" 1/5 ");
#ifdef USE_GTK3
  gtk_widget_set_halign (hg->hsc_label_dith, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (hg->hsc_label_dith, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (hg->hsc_label_dith), 0.5, 0.5);
#endif
  gtk_box_pack_start(GTK_BOX(hbox), hg->hsc_label_dith, FALSE, FALSE, 5);
  set_hsc_show_dith_label(hg);
  
#ifdef USE_GTK3
  button=gtkut_button_new_from_icon_name(NULL,"go-next");
#else
  button=gtkut_button_new_from_stock(NULL,GTK_STOCK_GO_FORWARD);
#endif
  gtk_box_pack_start(GTK_BOX(hbox),
		     button,FALSE,FALSE,0);
  my_signal_connect (button,"clicked",hsc_show_dith_forward, (gpointer)hg);

  label=gtk_label_new(" ");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
#endif
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);

#ifdef USE_GTK3
  button=gtkut_button_new_from_icon_name(NULL,"view-refresh");
#else
  button=gtkut_button_new_from_stock(NULL,GTK_STOCK_REFRESH);
#endif
  gtk_box_pack_start(GTK_BOX(hbox), button,FALSE,FALSE,0);
  my_signal_connect (button, "clicked",
		     G_CALLBACK (refresh_fc), (gpointer)hg);


  check = gtk_check_button_new_with_label("Draw accumulated dithering area");
  gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(hg->hsc_show_main))),
		     check,FALSE, FALSE, 0);
  my_signal_connect (check, "toggled",
		     hsc_dith_ol_fc,
		     (gpointer)hg);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check),
			       hg->hsc_show_ol);
  
  frame = gtkut_frame_new ("<b>5-shot parameters</b>");
  gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(hg->hsc_show_main))),
		     frame,FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 3);

  table = gtkut_table_new(4, 1, FALSE, 5, 0, 5);
  gtk_container_add (GTK_CONTAINER (frame), table);

  label=gtk_label_new("DITH_RA [arcsec]");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_END);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
#endif
  gtkut_table_attach(table, label, 0, 1, 0, 1,
		     GTK_FILL,GTK_SHRINK,0,0);

  adj = (GtkAdjustment *)gtk_adjustment_new(hg->hsc_show_dith_ra,
					    60, 300,
					    1.0, 10.0, 0.0);
  spinner =  gtk_spin_button_new (adj, 0, 0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), FALSE);
#ifdef USE_GTK3      
  gtk_widget_set_halign(spinner,GTK_ALIGN_CENTER);
  gtk_widget_set_valign(spinner,GTK_ALIGN_CENTER);
#endif
  gtkut_table_attach(table, spinner, 1, 2, 0, 1,
		     GTK_SHRINK,GTK_SHRINK,0,0);
  my_entry_set_width_chars(GTK_ENTRY(&GTK_SPIN_BUTTON(spinner)->entry),4);
  my_signal_connect (adj, "value_changed",
		     cc_get_adj,
		     &hg->hsc_show_dith_ra);


  label=gtk_label_new("    DITH_DEC [arcsec]");
#ifdef USE_GTK3
  gtk_widget_set_halign(label,GTK_ALIGN_CENTER);
  gtk_widget_set_valign(label,GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
#endif
  gtkut_table_attach(table, label, 2, 3, 0, 1,
		     GTK_SHRINK,GTK_SHRINK,0,0);

  adj = (GtkAdjustment *)gtk_adjustment_new(hg->hsc_show_dith_dec,
					    60, 300,
					    1.0, 10.0, 0.0);
  spinner =  gtk_spin_button_new (adj, 0, 0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), FALSE);
#ifdef USE_GTK3      
  gtk_widget_set_halign(spinner,GTK_ALIGN_CENTER);
  gtk_widget_set_valign(spinner,GTK_ALIGN_CENTER);
#endif
  gtkut_table_attach(table, spinner, 3, 4, 0, 1,
		     GTK_SHRINK,GTK_SHRINK,0,0);
  my_entry_set_width_chars(GTK_ENTRY(&GTK_SPIN_BUTTON(spinner)->entry),4);
  my_signal_connect (adj, "value_changed",
		     cc_get_adj,
		     &hg->hsc_show_dith_dec);


  frame = gtkut_frame_new ("<b>N-shot parameters</b>");
  gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(hg->hsc_show_main))),
		     frame,FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 3);

  table = gtkut_table_new(6, 1, FALSE, 5, 0, 5);
  gtk_container_add (GTK_CONTAINER (frame), table);

  label=gtk_label_new("    NDITH");
#ifdef USE_GTK3
  gtk_widget_set_halign(label,GTK_ALIGN_CENTER);
  gtk_widget_set_valign(label,GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
#endif
  gtkut_table_attach(table, label, 0, 1, 0, 1,
		     GTK_SHRINK,GTK_SHRINK,0,0);

  adj = (GtkAdjustment *)gtk_adjustment_new(hg->hsc_show_dith_n,
					    3, 30,
					    1.0, 1.0, 0.0);
  spinner =  gtk_spin_button_new (adj, 0, 0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), FALSE);
#ifdef USE_GTK3      
  gtk_widget_set_halign(spinner,GTK_ALIGN_CENTER);
  gtk_widget_set_valign(spinner,GTK_ALIGN_CENTER);
#endif
  gtkut_table_attach(table, spinner, 1, 2, 0, 1,
		     GTK_SHRINK,GTK_SHRINK,0,0);
  my_entry_set_width_chars(GTK_ENTRY(&GTK_SPIN_BUTTON(spinner)->entry),3);
  my_signal_connect (adj, "value_changed",
		     cc_get_adj,
		     &hg->hsc_show_dith_n);


  label=gtk_label_new("    RDITH [arcsec]");
#ifdef USE_GTK3
  gtk_widget_set_halign(label,GTK_ALIGN_CENTER);
  gtk_widget_set_valign(label,GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
#endif
  gtkut_table_attach(table, label, 2, 3, 0, 1,
		     GTK_SHRINK,GTK_SHRINK,0,0);

  adj = (GtkAdjustment *)gtk_adjustment_new(hg->hsc_show_dith_r,
					    60, 300,
					    1.0, 10.0, 0.0);
  spinner =  gtk_spin_button_new (adj, 0, 0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), FALSE);
#ifdef USE_GTK3      
  gtk_widget_set_halign(spinner,GTK_ALIGN_CENTER);
  gtk_widget_set_valign(spinner,GTK_ALIGN_CENTER);
#endif
  gtkut_table_attach(table, spinner, 3, 4, 0, 1,
		     GTK_SHRINK,GTK_SHRINK,0,0);
  my_entry_set_width_chars(GTK_ENTRY(&GTK_SPIN_BUTTON(spinner)->entry),4);
  my_signal_connect (adj, "value_changed",
		     cc_get_adj,
		     &hg->hsc_show_dith_r);

  label=gtk_label_new("TDITH [deg]");
#ifdef USE_GTK3
  gtk_widget_set_halign(label,GTK_ALIGN_CENTER);
  gtk_widget_set_valign(label,GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
#endif
  gtkut_table_attach(table, label, 4, 5, 0, 1,
		     GTK_SHRINK,GTK_SHRINK,0,0);

  adj = (GtkAdjustment *)gtk_adjustment_new(hg->hsc_show_dith_t,
					    0, 90,
					    1.0, 10.0, 0.0);
  spinner =  gtk_spin_button_new (adj, 0, 0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), FALSE);
#ifdef USE_GTK3      
  gtk_widget_set_halign(spinner,GTK_ALIGN_CENTER);
  gtk_widget_set_valign(spinner,GTK_ALIGN_CENTER);
#endif
  gtkut_table_attach(table, spinner, 5, 6, 0, 1,
		     GTK_SHRINK,GTK_SHRINK,0,0);
  my_entry_set_width_chars(GTK_ENTRY(&GTK_SPIN_BUTTON(spinner)->entry),4);
  my_signal_connect (adj, "value_changed",
		     cc_get_adj,
		     &hg->hsc_show_dith_t);


  frame = gtkut_frame_new ("<b>Pointing Offset</b>");
  gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(hg->hsc_show_main))),
		     frame,FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 3);

  table = gtkut_table_new(4, 1, FALSE, 5, 0, 5);
  gtk_container_add (GTK_CONTAINER (frame), table);

  label=gtk_label_new("RA [arcsec]");
#ifdef USE_GTK3
  gtk_widget_set_halign(label,GTK_ALIGN_CENTER);
  gtk_widget_set_valign(label,GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
#endif
  gtkut_table_attach(table, label, 0, 1, 0, 1,
		     GTK_SHRINK,GTK_SHRINK,0,0);

  adj = (GtkAdjustment *)gtk_adjustment_new(hg->hsc_show_osra,
					    -3000, 3000,
					    1.0, 10.0, 0.0);
  spinner =  gtk_spin_button_new (adj, 0, 0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), FALSE);
#ifdef USE_GTK3      
  gtk_widget_set_halign(spinner,GTK_ALIGN_CENTER);
  gtk_widget_set_valign(spinner,GTK_ALIGN_CENTER);
#endif
  gtkut_table_attach(table, spinner, 1, 2, 0, 1,
		     GTK_SHRINK,GTK_SHRINK,0,0);
  my_entry_set_width_chars(GTK_ENTRY(&GTK_SPIN_BUTTON(spinner)->entry),5);
  my_signal_connect (adj, "value_changed",
		     cc_get_adj,
		     &hg->hsc_show_osra);


  label=gtk_label_new("    Dec [arcsec]");
#ifdef USE_GTK3
  gtk_widget_set_halign(label,GTK_ALIGN_CENTER);
  gtk_widget_set_valign(label,GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
#endif
  gtkut_table_attach(table, label, 2, 3, 0, 1,
		     GTK_SHRINK,GTK_SHRINK,0,0);

  adj = (GtkAdjustment *)gtk_adjustment_new(hg->hsc_show_osdec,
					    -3000, 3000,
					    1.0, 10.0, 0.0);
  spinner =  gtk_spin_button_new (adj, 0, 0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), FALSE);
#ifdef USE_GTK3      
  gtk_widget_set_halign(spinner,GTK_ALIGN_CENTER);
  gtk_widget_set_valign(spinner,GTK_ALIGN_CENTER);
#endif
  gtkut_table_attach(table, spinner, 3, 4, 0, 1,
		     GTK_SHRINK,GTK_SHRINK,0,0);
  my_entry_set_width_chars(GTK_ENTRY(&GTK_SPIN_BUTTON(spinner)->entry),5);
  my_signal_connect (adj, "value_changed",
		     cc_get_adj,
		     &hg->hsc_show_osdec);

#ifdef USE_GTK3
  button=gtkut_button_new_from_icon_name("Close","window-close");
#else
  button=gtkut_button_new_from_stock("Close",GTK_STOCK_CANCEL);
#endif
  gtk_dialog_add_action_widget(GTK_DIALOG(hg->hsc_show_main),
			       button,GTK_RESPONSE_CANCEL);
  my_signal_connect(button,"pressed",
		    close_hsc_dither, 
		    (gpointer)hg);

  gtk_widget_show_all(hg->hsc_show_main); 
}


void do_fc(typHOE *hg){
  if(flagFC){
    gdk_window_raise(gtk_widget_get_window(hg->fc_main));
    hg->fc_output=FC_OUTPUT_WINDOW;
    draw_fc_cairo(hg->fc_dw, hg);
    return;
  }
  else{
    flagFC=TRUE;
  }
  
  create_fc_dialog(hg);
}


void set_fc_frame_col(typHOE *hg){
  if((hg->fc_mode>FC_SEP2)&&(hg->fc_mode<FC_SEP3)){
    gtk_widget_set_sensitive(hg->fc_frame_col,TRUE);
  }
  else{
    gtk_widget_set_sensitive(hg->fc_frame_col,FALSE);
  }
}

void set_fc_frame_col_pdf(typHOE *hg){
  if((hg->fc_mode>FC_SEP2)&&(hg->fc_mode<FC_SEP3)){
    gtk_widget_set_sensitive(hg->fc_frame_col_pdf,TRUE);
  }
  else{
    gtk_widget_set_sensitive(hg->fc_frame_col_pdf,FALSE);
  }
}


void create_fc_dialog(typHOE *hg)
{
  GtkWidget *vbox, *vbox1, *hbox, *hbox1, *hbox2, *ebox, *table;
  GtkWidget *frame, *check, *label, *button, *spinner;
  GtkAdjustment *adj;
  GtkWidget *menubar;
  GdkPixbuf *icon;
  gchar *tmp;

  hg->fc_main = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(hg->fc_main), "HOE : Finding Chart");
  
  my_signal_connect(hg->fc_main,
		    "destroy",
		    close_fc, 
		    (gpointer)hg);

  vbox = gtkut_vbox_new(FALSE,0);
  gtk_container_add (GTK_CONTAINER (hg->fc_main), vbox);

  hbox = gtkut_hbox_new(FALSE,0);
  gtk_box_pack_start(GTK_BOX(vbox), 
		     hbox, FALSE, FALSE, 0);

  frame = gtkut_frame_new ("<b>Image Source</b>");
  gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 3);

  table = gtkut_table_new(5, 2, FALSE, 3, 0, 0);
  gtk_container_add (GTK_CONTAINER (frame), table);

  icon = gdk_pixbuf_new_from_resource ("/icons/dl_icon.png", NULL);
  button=gtkut_button_new_from_pixbuf(NULL, icon);
  g_object_unref(icon);
  my_signal_connect (button, "clicked",
		     G_CALLBACK (fc_item_redl), (gpointer)hg);
#ifdef USE_GTK3      
  gtk_widget_set_halign(button,GTK_ALIGN_CENTER);
  gtk_widget_set_valign(button,GTK_ALIGN_CENTER);
#endif
  gtkut_table_attach(table, button, 0, 1, 1, 2,
		      GTK_SHRINK,GTK_SHRINK,0,0);
#ifdef __GTK_TOOLTIP_H__
  gtk_widget_set_tooltip_text(button,
			      "Download & Redraw");
#endif

  {
    GtkWidget *combo;
    GtkListStore *store;
    GtkTreeIter iter, iter_set;	  
    GtkCellRenderer *renderer;
    GtkWidget *bar;
    gint i_fc;
    
    store = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_INT, G_TYPE_BOOLEAN);

    for(i_fc=0;i_fc<NUM_FC;i_fc++){
      if(FC_markup[i_fc]){
	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, 0, FC_markup[i_fc],
			   1, i_fc, 2, TRUE, -1);
	if(hg->fc_mode==i_fc) iter_set=iter;
      }
      else{
	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter,
			    0, NULL,
			    1, i_fc, 2, FALSE, -1);
      }
    }

    combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));
    gtkut_table_attach(table, combo, 1, 2, 1, 2,
			GTK_FILL|GTK_EXPAND,GTK_SHRINK,0,0);
    g_object_unref(store);
	
    renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo),renderer, TRUE);
    gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT(combo), renderer, "markup",0,NULL);
	
    gtk_combo_box_set_row_separator_func (GTK_COMBO_BOX (combo), 
					  is_separator, NULL, NULL);	

    gtk_combo_box_set_active_iter(GTK_COMBO_BOX(combo),&iter_set);
    gtk_widget_show(combo);
    my_signal_connect (combo,"changed",cc_get_fc_mode,
		       (gpointer)hg);
  }

  frame = gtk_frame_new ("Size [min]");
#ifdef USE_GTK3      
  gtk_widget_set_halign(frame,GTK_ALIGN_CENTER);
  gtk_widget_set_valign(frame,GTK_ALIGN_CENTER);
#endif
  gtkut_table_attach(table, frame, 2, 3, 0, 2,
		      GTK_SHRINK,GTK_SHRINK,0,0);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 0);

  hbox2 = gtkut_hbox_new(FALSE,0);
  gtk_container_add (GTK_CONTAINER (frame), hbox2);

  hg->fc_adj_dss_arcmin = (GtkAdjustment *)gtk_adjustment_new(hg->dss_arcmin,
							      DSS_ARCMIN_MIN, DSS_ARCMIN_MAX,
							      1.0, 1.0, 0);
  spinner =  gtk_spin_button_new (hg->fc_adj_dss_arcmin, 0, 0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), FALSE);
  gtk_box_pack_start(GTK_BOX(hbox2),spinner,FALSE,FALSE,0);
  my_entry_set_width_chars(GTK_ENTRY(&GTK_SPIN_BUTTON(spinner)->entry),5);
  my_signal_connect (hg->fc_adj_dss_arcmin, "value_changed",
		     cc_get_adj,
		     &hg->dss_arcmin);

  set_dss_arcmin_upper(hg);
  

  hg->fc_frame_col = gtk_frame_new ("Scale/Color");
#ifdef USE_GTK3      
  gtk_widget_set_halign(hg->fc_frame_col,GTK_ALIGN_CENTER);
  gtk_widget_set_valign(hg->fc_frame_col,GTK_ALIGN_CENTER);
#endif
  gtkut_table_attach(table, hg->fc_frame_col, 3, 4, 0, 2,
		      GTK_SHRINK,GTK_SHRINK,0,0);
  gtk_container_set_border_width (GTK_CONTAINER (hg->fc_frame_col), 0);

  hbox2 = gtkut_hbox_new(FALSE,0);
  gtk_container_add (GTK_CONTAINER (hg->fc_frame_col), hbox2);

  set_fc_frame_col(hg);

  {
    GtkWidget *combo;
    GtkListStore *store;
    GtkTreeIter iter, iter_set;	  
    GtkCellRenderer *renderer;
    
    store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
    
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "Linear",
		       1, FC_SCALE_LINEAR, -1);
    if(hg->dss_scale==FC_SCALE_LINEAR) iter_set=iter;
	
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "Log",
		       1, FC_SCALE_LOG, -1);
    if(hg->dss_scale==FC_SCALE_LOG) iter_set=iter;
	
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "Sqrt",
		       1, FC_SCALE_SQRT, -1);
    if(hg->dss_scale==FC_SCALE_SQRT) iter_set=iter;
	
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "HistEq",
		       1, FC_SCALE_HISTEQ, -1);
    if(hg->dss_scale==FC_SCALE_HISTEQ) iter_set=iter;
	
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "LogLog",
		       1, FC_SCALE_LOGLOG, -1);
    if(hg->dss_scale==FC_SCALE_LOGLOG) iter_set=iter;
	
	
    combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));
    gtk_box_pack_start(GTK_BOX(hbox2),combo,FALSE,FALSE,0);
    g_object_unref(store);
	
    renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo),renderer, TRUE);
    gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT(combo), renderer, "text",0,NULL);
	
    gtk_combo_box_set_active_iter(GTK_COMBO_BOX(combo),&iter_set);
    gtk_widget_show(combo);
    my_signal_connect (combo,"changed",cc_get_combo_box,
		       &hg->dss_scale);
  }

  button=gtk_check_button_new_with_label("Inverse");
  gtk_container_set_border_width (GTK_CONTAINER (button), 0);
  gtk_box_pack_start(GTK_BOX(hbox2),button,FALSE,FALSE,0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),hg->dss_invert);
  my_signal_connect(button,"toggled",
		    G_CALLBACK (cc_get_toggle), 
		    &hg->dss_invert);

  hbox = gtkut_hbox_new(FALSE,0);
  gtk_box_pack_start(GTK_BOX(vbox), 
		     hbox, FALSE, FALSE, 0);

  frame = gtkut_frame_new ("<b>Instrument</b>");
  gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 3);

  table = gtkut_table_new(4, 2, FALSE, 3, 0, 0);
  gtk_container_add (GTK_CONTAINER (frame), table);


#ifdef USE_GTK3
  button=gtkut_button_new_from_icon_name(NULL,"view-refresh");
  gtk_widget_set_halign(button,GTK_ALIGN_CENTER);
  gtk_widget_set_valign(button,GTK_ALIGN_CENTER);
#else
  button=gtkut_button_new_from_stock(NULL,GTK_STOCK_REFRESH);
#endif
  gtkut_table_attach(table, button, 0, 1, 1, 2,
		      GTK_SHRINK,GTK_SHRINK,0,0);
  my_signal_connect (button, "clicked",
		     G_CALLBACK (refresh_fc), (gpointer)hg);
#ifdef __GTK_TOOLTIP_H__
  gtk_widget_set_tooltip_text(button,
			      "Redraw");
#endif


  {
    GtkWidget *combo;
    GtkListStore *store;
    GtkTreeIter iter, iter_set;	  
    GtkCellRenderer *renderer;
    gint i_inst;
    
    store = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_INT, G_TYPE_BOOLEAN);
    
    for(i_inst=0;i_inst<NUM_FC_INST;i_inst++){
      if(FC_instname[i_inst]){
	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, 0, FC_instname[i_inst],
			   1, i_inst, 2, TRUE, -1);
	if(hg->fc_inst==i_inst) iter_set=iter;
      }
      else{
	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter,
			    0, NULL,
			    1, i_inst, 2, FALSE, -1);
      }
    }

    combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));
    gtkut_table_attach(table, combo, 1, 2, 1, 2,
			GTK_FILL|GTK_EXPAND,GTK_SHRINK,0,0);
    g_object_unref(store);
	
    renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo),renderer, TRUE);
    gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT(combo), renderer, "text",0,NULL);
	
    gtk_combo_box_set_row_separator_func (GTK_COMBO_BOX (combo), 
					  is_separator, NULL, NULL);	

    gtk_combo_box_set_active_iter(GTK_COMBO_BOX(combo),&iter_set);
    gtk_widget_show(combo);
    my_signal_connect (combo,"changed",cc_get_fc_inst, (gpointer)hg);
  }

  button=gtk_check_button_new_with_label("Detail");
  gtk_container_set_border_width (GTK_CONTAINER (button), 0);
#ifdef USE_GTK3      
  gtk_widget_set_halign(button,GTK_ALIGN_CENTER);
  gtk_widget_set_valign(button,GTK_ALIGN_CENTER);
#endif
  gtkut_table_attach(table, button, 2, 3, 1, 2,
		      GTK_SHRINK,GTK_SHRINK,0,0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),hg->dss_draw_slit);
  my_signal_connect(button,"toggled",
		    G_CALLBACK (cc_get_toggle), 
		    &hg->dss_draw_slit);

  hg->check_hsc_sat = gtk_check_button_new_with_label("Sat.Mask");
  gtkut_table_attach(table, hg->check_hsc_sat, 3, 4, 1, 2,
		     GTK_SHRINK,GTK_SHRINK,0,0);
  my_signal_connect (hg->check_hsc_sat, "toggled",
		     cc_get_toggle,
		     &hg->hsc_sat);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hg->check_hsc_sat),
			       hg->hsc_sat);
#ifdef __GTK_TOOLTIP_H__
  gtk_widget_set_tooltip_text(hg->check_hsc_sat,
			      "HSC masking area plot for bright stars (Coupon+ 2018, PASJ, 70, S7), working w/ GAIA or SIMBAD data");
#endif



  frame = gtk_frame_new ("PA [deg]");
#ifdef USE_GTK3      
  gtk_widget_set_halign(frame,GTK_ALIGN_CENTER);
  gtk_widget_set_valign(frame,GTK_ALIGN_CENTER);
#endif
  gtkut_table_attach(table, frame, 4, 5, 0, 2,
		      GTK_SHRINK,GTK_SHRINK,0,0);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 0);

  hbox2 = gtkut_hbox_new(FALSE,0);
  gtk_container_add (GTK_CONTAINER (frame), hbox2);

  hg->fc_adj_dss_pa = (GtkAdjustment *)gtk_adjustment_new(hg->dss_pa,
						       -360, 360,
						       1.0, 1.0, 0);
  spinner =  gtk_spin_button_new (hg->fc_adj_dss_pa, 0, 0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), TRUE);
  gtk_box_pack_start(GTK_BOX(hbox2),spinner,FALSE,FALSE,0);
  my_entry_set_width_chars(GTK_ENTRY(&GTK_SPIN_BUTTON(spinner)->entry),4);
  my_signal_connect (hg->fc_adj_dss_pa, "value_changed",
		     cc_get_adj,
		     &hg->dss_pa);


  hg->fc_button_flip=gtk_check_button_new_with_label("Flip");
  gtk_container_set_border_width (GTK_CONTAINER (hg->fc_button_flip), 0);
  gtk_box_pack_start(GTK_BOX(hbox2),hg->fc_button_flip,FALSE,FALSE,0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hg->fc_button_flip),hg->dss_flip);
  my_signal_connect(hg->fc_button_flip,"toggled",
		    G_CALLBACK (cc_get_toggle), 
		    &hg->dss_flip);

  tmp=g_strdup_printf("<b>%s</b>", db_name[hg->fcdb_type]);
  hg->fcdb_frame = gtkut_frame_new (tmp);
  g_free(tmp);

  gtk_box_pack_start(GTK_BOX(hbox), hg->fcdb_frame, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hg->fcdb_frame), 3);

  table = gtkut_table_new(3, 2, FALSE, 3, 0, 0);
  gtk_container_add (GTK_CONTAINER (hg->fcdb_frame), table);

  label=gtk_label_new("  ");
  gtkut_table_attach(table, label, 0, 1, 0, 1,
		      GTK_SHRINK,GTK_FILL,0,0);

#ifdef USE_GTK3
  button=gtkut_button_new_from_icon_name(NULL,"edit-find");
  gtk_widget_set_halign(button,GTK_ALIGN_CENTER);
  gtk_widget_set_valign(button,GTK_ALIGN_CENTER);
#else
  button=gtkut_button_new_from_stock(NULL,GTK_STOCK_FIND);
#endif
  gtkut_table_attach(table, button, 0, 1, 1, 2,
		      GTK_SHRINK,GTK_SHRINK,0,0);
  my_signal_connect (button, "clicked",
		     G_CALLBACK (fcdb_item), (gpointer)hg);
#ifdef __GTK_TOOLTIP_H__
  gtk_widget_set_tooltip_text(button,
			      "Query");
#endif

  vbox1 = gtkut_vbox_new(FALSE,0);
#ifdef USE_GTK3      
  gtk_widget_set_halign(vbox1,GTK_ALIGN_CENTER);
  gtk_widget_set_valign(vbox1,GTK_ALIGN_CENTER);
#endif
  gtkut_table_attach(table, vbox1, 1, 2, 0, 2,
		      GTK_SHRINK,GTK_SHRINK,0,0);

  hg->fcdb_button=gtk_check_button_new_with_label("Disp");
  gtk_container_set_border_width (GTK_CONTAINER (hg->fcdb_button), 0);
  gtk_box_pack_start(GTK_BOX(vbox1), hg->fcdb_button, FALSE, FALSE, 0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hg->fcdb_button),
			       hg->fcdb_flag);
  my_signal_connect(hg->fcdb_button,"toggled",
		    G_CALLBACK(fcdb_toggle), 
		    (gpointer)hg);

  button=gtk_check_button_new_with_label("Auto");
  gtk_container_set_border_width (GTK_CONTAINER (button), 0);
  gtk_box_pack_start(GTK_BOX(vbox1), button, FALSE, FALSE, 0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),hg->fcdb_auto);
  my_signal_connect(button,"toggled",
		    G_CALLBACK (cc_get_toggle), 
		    &hg->fcdb_auto);

  label=gtk_label_new("  ");
  gtkut_table_attach(table, label, 2, 3, 0, 1,
		      GTK_SHRINK,GTK_FILL,0,0);

#ifdef USE_GTK3
  button=gtkut_button_new_from_icon_name(NULL,"emblem-system");
  gtk_widget_set_halign(button,GTK_ALIGN_CENTER);
  gtk_widget_set_valign(button,GTK_ALIGN_CENTER);
#else
  button=gtkut_button_new_from_stock(NULL,GTK_STOCK_PROPERTIES);
#endif
  gtkut_table_attach(table, button, 2, 3, 1, 2,
		      GTK_SHRINK,GTK_SHRINK,0,0);
  my_signal_connect (button, "clicked",
		     fcdb_para_item, (gpointer)hg);
#ifdef __GTK_TOOLTIP_H__
  gtk_widget_set_tooltip_text(button,
			      "Search Param.");
#endif
  
  hbox = gtkut_hbox_new(FALSE,0);
  gtk_box_pack_start(GTK_BOX(vbox), 
		     hbox, TRUE, TRUE, 0);

  vbox1 = gtkut_vbox_new(FALSE,3);
  gtk_box_pack_start(GTK_BOX(hbox), vbox1, FALSE, FALSE, 3);



  icon = gdk_pixbuf_new_from_resource ("/icons/comet_icon.png", NULL);
  button=gtkut_toggle_button_new_from_pixbuf(NULL, icon);
  g_object_unref(icon);
  gtk_container_set_border_width (GTK_CONTAINER (button), 0);
  gtk_box_pack_start(GTK_BOX(vbox1), button, FALSE, FALSE, 0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),hg->orbit_flag);
  my_signal_connect (button, "clicked",
		     G_CALLBACK (orbit_fc), (gpointer)hg);
#ifdef __GTK_TOOLTIP_H__
  gtk_widget_set_tooltip_text(button,
			      "Draw Non-Sidereal Orbit");
#endif

  icon = gdk_pixbuf_new_from_resource ("/icons/hsc_icon.png", NULL);
  button=gtkut_button_new_from_pixbuf(NULL, icon);
  g_object_unref(icon);
  gtk_container_set_border_width (GTK_CONTAINER (button), 0);
  gtk_box_pack_start(GTK_BOX(vbox1), button, FALSE, FALSE, 0);
  my_signal_connect (button, "clicked",
		     G_CALLBACK (set_hsc_dither), (gpointer)hg);
#ifdef __GTK_TOOLTIP_H__
  gtk_widget_set_tooltip_text(button,
			      "HSC Dithering Parameter");
#endif


  icon = gdk_pixbuf_new_from_resource ("/icons/pdf_icon.png", NULL);
  button=gtkut_button_new_from_pixbuf(NULL, icon);
  g_object_unref(icon);
  my_signal_connect (button, "clicked",
		     G_CALLBACK (do_save_fc_pdf), (gpointer)hg);
  gtk_box_pack_start(GTK_BOX(vbox1), button, FALSE, FALSE, 0);
#ifdef __GTK_TOOLTIP_H__
  gtk_widget_set_tooltip_text(button,
			      "Save as PDF");
#endif

#ifdef USE_GTK3
  button=gtkut_button_new_from_icon_name(NULL,"document-print");
#else
  button=gtkut_button_new_from_stock(NULL,GTK_STOCK_PRINT);
#endif
  my_signal_connect (button, "clicked",
		     G_CALLBACK (do_print_fc), (gpointer)hg);
  gtk_box_pack_start(GTK_BOX(vbox1), button, FALSE, FALSE, 0);
#ifdef __GTK_TOOLTIP_H__
  gtk_widget_set_tooltip_text(button,
			      "Print out");
#endif

#ifdef USE_GTK3
  button=gtkut_button_new_from_icon_name(NULL,"help-browser");
#else
  button=gtkut_button_new_from_stock(NULL,GTK_STOCK_INFO);
#endif
  my_signal_connect (button, "clicked",
		     G_CALLBACK (show_fc_help), hg->fc_main);
  gtk_box_pack_start(GTK_BOX(vbox1), button, FALSE, FALSE, 0);
#ifdef __GTK_TOOLTIP_H__
  gtk_widget_set_tooltip_text(button,
			      "Show Help");
#endif
  gtk_widget_grab_focus (button);

#ifdef USE_GTK3
  button=gtkut_button_new_from_icon_name(NULL,"window-close");
#else
  button=gtkut_button_new_from_stock(NULL,GTK_STOCK_CANCEL);
#endif
  my_signal_connect (button, "clicked",
		     G_CALLBACK (close_fc), (gpointer)hg);
  gtk_box_pack_start(GTK_BOX(vbox1), button, FALSE, FALSE, 0);
#ifdef __GTK_TOOLTIP_H__
  gtk_widget_set_tooltip_text(button,
			      "Close");
#endif

  hg->fc_mag=1;
  hg->fc_magmode=0;
  hg->fc_ptn=0;

  // Drawing Area
  ebox=gtk_event_box_new();
  gtk_box_pack_start(GTK_BOX(hbox), ebox, TRUE, TRUE, 0);
  hg->fc_dw = gtk_drawing_area_new();
  gtk_widget_set_size_request (hg->fc_dw, hg->sz_fc, hg->sz_fc);
  gtk_container_add(GTK_CONTAINER(ebox), hg->fc_dw);
  gtk_widget_set_app_paintable(hg->fc_dw, TRUE);

  gtk_widget_set_events(hg->fc_dw, GDK_STRUCTURE_MASK | GDK_EXPOSURE_MASK);

#ifdef USE_GTK3
  my_signal_connect(hg->fc_dw, 
		    "draw", 
		    draw_fc_cb,
		    (gpointer)hg);
#else
  my_signal_connect(hg->fc_dw, 
		    "expose-event", 
		    expose_fc_cairo,
		    (gpointer)hg);
#endif
  my_signal_connect(hg->fc_dw, 
		    "configure-event", 
		    configure_fc_cb,
		    (gpointer)hg);
  
  gtk_widget_set_events(ebox, GDK_SCROLL_MASK |
                      GDK_BUTTON_PRESS_MASK);

  my_signal_connect(ebox,
		    "scroll-event", 
		    resize_draw_fc,
		    (gpointer)hg);
  my_signal_connect(ebox, 
		    "button-press-event", 
		    button_draw_fc,
		    (gpointer)hg);
		    
  gtk_widget_show(hg->fc_dw);

  gtk_widget_show_all(hg->fc_main);
  
  gdk_window_raise(gtk_widget_get_window(hg->fc_main));
  
  draw_fc_cairo(hg->fc_dw, hg);
}


void close_fc(GtkWidget *w, gpointer gdata)
{
  typHOE *hg;
  hg=(typHOE *)gdata;

  gtk_widget_destroy(GTK_WIDGET(hg->fc_main));
  flagFC=FALSE;
}


static void thread_cancel_fc(GtkWidget *w, gpointer gdata)
{
  typHOE *hg=(typHOE *)gdata;

  if(GTK_IS_WIDGET(hg->pdialog)) gtk_widget_unmap(hg->pdialog);

  g_cancellable_cancel(hg->pcancel);
  g_object_unref(hg->pcancel); 

  hg->pabort=TRUE;

  hg->fc_pid=0;

  if(hg->ploop) g_main_loop_quit(hg->ploop);
}

static gboolean delete_fc(GtkWidget *w, GdkEvent *event, gpointer gdata)
{
  thread_cancel_fc(w, gdata);

  return(TRUE);
}


void draw_fc_obj(typHOE *hg, cairo_t *cr, gint width, gint height){
  cairo_text_extents_t extents;
  double x,y;
  gint i_list;
  gint from_set, to_rise;
  int width_file, height_file;
  gdouble r_w,r_h, r;
  GdkPixbuf *pixbuf_flip=NULL;
  gdouble angle;

  gchar tmp[2048];
  gdouble x_ccd, y_ccd, gap_ccd;
  struct ln_hms ra_hms;
  struct ln_dms dec_dms;


  /* draw the background */
  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
  cairo_paint (cr);
  
  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);



  if(pixbuf_fc){
    width_file = gdk_pixbuf_get_width(pixbuf_fc);
    height_file = gdk_pixbuf_get_height(pixbuf_fc);
    
    r_w =  (gdouble)width/(gdouble)width_file;
    r_h =  (gdouble)height/(gdouble)height_file;
    
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
    case FC_INST_IRCS:
      angle=M_PI*(gdouble)(-90+hg->dss_pa)/180.;
      break;

    case FC_INST_SPCAM:
	angle=M_PI*(gdouble)(90-hg->dss_pa)/180.;
      break;
      
    case FC_INST_HSCDET:
    case FC_INST_HSCA:
      angle=M_PI*(gdouble)(270-hg->dss_pa)/180.;
      break;
      
    default:
      angle=M_PI*(gdouble)hg->dss_pa/180.;
      break;
    }
    cairo_rotate (cr,-(hg->dss_flip) ? -angle : angle);
    
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
      draw_hds(hg, cr, width, height, width_file, height_file, 1.0, r);
      break;

    case FC_INST_IRD:
      draw_ird(hg, cr, width, height, width_file, height_file, 1.0, r);
      break;

    case FC_INST_IRCS:
      draw_ircs(hg, cr, width, height, width_file, height_file, 1.0, r);
      break;

    case FC_INST_FOCAS:
      draw_focas(hg, cr, width, height, width_file, height_file, 1.0, r);
      break;

    case FC_INST_MOIRCS:
      draw_moircs(hg, cr, width, height, width_file, height_file, 1.0, r);
      break;

    case FC_INST_SWIMS:
      draw_swims(hg, cr, width, height, width_file, height_file, 1.0, r);
      break;

    case FC_INST_SPCAM:
      draw_spcam(hg, cr, width, height, width_file, height_file, 1.0, r);
      break;

    case FC_INST_HSCDET:
    case FC_INST_HSCA:
      draw_hsc(hg, cr, width, height, width_file, height_file, 1.0, r);
      break;

    case FC_INST_FMOS:
      draw_fmos(hg, cr, width, height, width_file, height_file, 1.0, r);
      break;

    case FC_INST_PFS:
      draw_pfs(hg, cr, width, height, width_file, height_file, 1.0, r);
      break;

    case FC_INST_KOOLS:
      draw_kools(hg, cr, width, height, width_file, height_file, 1.0, r);
      break;
      
    case FC_INST_TRICCS:
      draw_triccs(hg, cr, width, height, width_file, height_file, 1.0, r);
      break;
    }

    cairo_restore(cr);

    // Draw Labels
    cairo_save(cr);
    draw_fc_label(hg, cr, width, height, width_file, height_file, 1.0, r);
    cairo_restore(cr);

    // Draw PA
    cairo_save (cr);
    hg->fc_mag=1;
    draw_pa(hg, cr, width, height, width_file, height_file,
	    0, 0, 1.0, r);
    cairo_restore(cr);
 
    // Guide Star
    cairo_save(cr);
    draw_gs(hg, cr, width, height, width_file, height_file, 1.0, r);
    cairo_restore(cr);
    
    // Non-Sidereal Track
    cairo_save(cr);
    draw_nst(hg, cr, width, height, width_file, height_file, 1.0, r);
    cairo_restore(cr);
 }

  if(hg->fc_output!=FC_OUTPUT_WINDOW){
    cairo_show_page(cr); 
  }
}


void translate_to_center(typHOE *hg,
			 cairo_t *cr,
			 int width,
			 int height,
			 int width_file,
			 int height_file,
			 gdouble r)
{
  gdouble angle;
  
  cairo_translate (cr, (width-(gint)((gdouble)width_file*r))/2,
		   (height-(gint)((gdouble)height_file*r))/2);

  cairo_translate (cr, (gdouble)width_file*r/2,
		   (gdouble)height_file*r/2);

  switch(hg->fc_inst){
  case FC_INST_IRCS:
    angle=M_PI*(gdouble)(-90+hg->dss_pa)/180.;
    break;

  case FC_INST_SPCAM:
    angle=M_PI*(gdouble)(90-hg->dss_pa)/180.;
    break;

  case FC_INST_HSCDET:
  case FC_INST_HSCA:
    angle=M_PI*(gdouble)(270-hg->dss_pa)/180.;
    break;

  default:
    angle=M_PI*(gdouble)hg->dss_pa/180.;
    break;
  }

  cairo_rotate (cr, (hg->dss_flip) ? -angle : angle);
}

void set_pa(typHOE *hg){
  gdouble angle;
  
  switch(hg->fc_inst){
  case FC_INST_HDSAUTO:
    if(hg->skymon_mode==SKYMON_SET){
      hg->dss_pa=(int)hg->obj[hg->dss_i].s_hpa;
    }
    else{
      hg->dss_pa=(int)hg->obj[hg->dss_i].c_hpa;
    }
    break;

  case FC_INST_HDSZENITH:
    if(hg->skymon_mode==SKYMON_SET){
      hg->dss_pa=(int)hg->obj[hg->dss_i].s_pa;
    }
    else{
      hg->dss_pa=(int)hg->obj[hg->dss_i].c_pa;
    }
    break;

  default:
    hg->dss_pa=hg->obj[hg->dss_i].pa;
    break;
  }

  if(flagFC){
    gtk_adjustment_set_value(hg->fc_adj_dss_pa, 
			     (gdouble)hg->dss_pa);
  }

}

void rot_pa(typHOE *hg, cairo_t *cr){
  gdouble angle;
  
  switch(hg->fc_inst){
  case FC_INST_IRCS:
    angle=M_PI*(gdouble)(-90+hg->dss_pa)/180.;
    break;

  case FC_INST_SPCAM:
    angle=M_PI*(gdouble)(90-hg->dss_pa)/180.;
    break;

  case FC_INST_HSCDET:
  case FC_INST_HSCA:
    angle=M_PI*(gdouble)(270-hg->dss_pa)/180.;
    break;

  default:
    angle=M_PI*(gdouble)hg->dss_pa/180.;
    break;
  }

  cairo_rotate (cr, (hg->dss_flip) ? -angle : angle);
}

void translate_hsc_offset(cairo_t *cr, typHOE *hg, int width_file,
			  gdouble r){
  gdouble dra, ddec, theta;
  dra=((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip
    *(gdouble)hg->hsc_show_osra/60.;
  ddec=((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip
    *(gdouble)hg->hsc_show_osdec/60.;
  theta=M_PI*(gdouble)(270-hg->dss_pa)/180.;
  
  if(hg->dss_flip){
    cairo_translate(cr, 
		    -(dra*cos(-theta)-ddec*sin(-theta)),
		    -(dra*sin(-theta)+ddec*cos(-theta)));
  }
  else{
    cairo_translate(cr, 
		    -(dra*cos(theta)-ddec*sin(theta)),
		    -(dra*sin(theta)+ddec*cos(theta)));
  }
}

void translate_hsc_dith(cairo_t *cr, typHOE *hg, int width_file,
			gdouble r, gint i_dith, gboolean flag_back){
  gdouble dra, ddec, x0, y0, x1=0, y1=0, theta;
  
  switch(hg->hsc_show_dith_p){
  case HSC_DITH_5:
    dra=((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip
      *(gdouble)hg->hsc_show_dith_ra/60.;
    ddec=((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip
      *(gdouble)hg->hsc_show_dith_dec/60.;
    switch(i_dith){
    case 1:
      x0=0;
      y0=0;
      break;
      
    case 2:
      x0=+ddec*2;
      y0=+dra*1;
      break;
	
    case 3:
      x0=-ddec*1;
      y0=+dra*2;
      break;
      
    case 4:
      x0=-ddec*2;
      y0=-dra*1;
      break;
      
    case 5:
      x0=+ddec*1;
      y0=-dra*2;
	break;
	
    }
    
    x1=x0*cos(-(gdouble)hg->dss_pa*M_PI/180)
      -y0*sin(-(gdouble)hg->dss_pa*M_PI/180);
    y1=x0*sin(-(gdouble)hg->dss_pa*M_PI/180)
      +y0*cos(-(gdouble)hg->dss_pa*M_PI/180);
    break;
    
  case HSC_DITH_N:
    y0=((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip
      *(gdouble)hg->hsc_show_dith_r/60*
      cos(-(gdouble)hg->hsc_show_dith_t*M_PI/180
	  -2*M_PI/(gdouble)(hg->hsc_show_dith_n)*(gdouble)(i_dith-1));
    x0=((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip
      *(gdouble)hg->hsc_show_dith_r/60*
      sin(-(gdouble)hg->hsc_show_dith_t*M_PI/180
	  -2*M_PI/(gdouble)(hg->hsc_show_dith_n)*(gdouble)(i_dith-1));
    
    x1=x0*cos(-(gdouble)hg->dss_pa*M_PI/180)
      -y0*sin(-(gdouble)hg->dss_pa*M_PI/180);
    y1=x0*sin(-(gdouble)hg->dss_pa*M_PI/180)
      +y0*cos(-(gdouble)hg->dss_pa*M_PI/180);
    break;
  }
  
  if(flag_back){
    cairo_translate(cr, -x1, -y1);
  }
  else{
    cairo_translate(cr, x1, y1);
  }
}



#ifdef USE_GTK3
gboolean draw_fc_cb(GtkWidget *widget,
		    cairo_t *cr, 
		    gpointer userdata){
  typHOE *hg=(typHOE *)userdata;
  if(!pixbuf_fcbk) draw_fc_cairo(widget,hg);
  gdk_cairo_set_source_pixbuf(cr, pixbuf_fcbk, hg->fc_shift_x, hg->fc_shift_y);
  cairo_paint(cr);
  return(TRUE);
}
#else
gboolean expose_fc_cairo(GtkWidget *widget,
			 GdkEventExpose *event, 
			 gpointer userdata){
  typHOE *hg=(typHOE *)userdata;
  if(!pixmap_fcbk) draw_fc_cairo(hg->fc_dw,hg);
  {
    GtkAllocation *allocation=g_new(GtkAllocation, 1);
    GtkStyle *style=gtk_widget_get_style(widget);
    gtk_widget_get_allocation(widget,allocation);
    
    gdk_draw_drawable(gtk_widget_get_window(widget),
		      style->fg_gc[gtk_widget_get_state(widget)],
		      pixmap_fcbk,
		      0,0,hg->fc_shift_x,hg->fc_shift_y,
		      allocation->width*hg->fc_mag,
		      allocation->height*hg->fc_mag);
    g_free(allocation);
  }

  return (TRUE);
}
#endif

gboolean configure_fc_cb(GtkWidget *widget,
			 GdkEventConfigure *event, 
			 gpointer userdata){
  typHOE *hg=(typHOE *)userdata;
  draw_fc_cairo(widget,hg);
  return(TRUE);
}


gboolean resize_draw_fc(GtkWidget *widget,  
			GdkEventScroll *event, 
			gpointer userdata){
  typHOE *hg;
  GdkScrollDirection direction;
  gint x,y;
  gint magx0, magy0, mag0;
  gint width, height;

  direction = event->direction;
  hg=(typHOE *)userdata;

  if(flagFC){
    if(event->state & GDK_SHIFT_MASK){ 
      // Change Position Angle -- 5 deg step
      if(direction & GDK_SCROLL_DOWN){
	gtk_adjustment_set_value(hg->fc_adj_dss_pa, 
				 (gdouble)(hg->dss_pa-5));
      }
      else{
	gtk_adjustment_set_value(hg->fc_adj_dss_pa, 
				 (gdouble)(hg->dss_pa+5));
      }
      hg->fc_output=FC_OUTPUT_WINDOW;
      draw_fc_cairo(hg->fc_dw,hg);
    }
    else if(event->state & GDK_CONTROL_MASK){
      // Change Position Angle -- 1 deg step
      if(direction & GDK_SCROLL_DOWN){
	gtk_adjustment_set_value(hg->fc_adj_dss_pa, 
				 (gdouble)(hg->dss_pa-1));
      }
      else{
	gtk_adjustment_set_value(hg->fc_adj_dss_pa, 
				 (gdouble)(hg->dss_pa+1));
      }
      hg->fc_output=FC_OUTPUT_WINDOW;
      draw_fc_cairo(hg->fc_dw,hg);
    }
    else if(event->state & GDK_MOD1_MASK){
      // Change Magnification w/ fixed center
      GtkAllocation *allocation=g_new(GtkAllocation, 1);
      gtk_widget_get_allocation(widget,allocation);

      width= allocation->width;
      height=allocation->height;
      g_free(allocation);

      x=width/2;
      y=height/2;
	  
      mag0=hg->fc_mag;
      magx0=hg->fc_magx;
      magy0=hg->fc_magy;
      
      if(direction & GDK_SCROLL_DOWN){
	hg->fc_mag--;
	hg->fc_ptn=0;
      }
      else{
	hg->fc_mag++;
	hg->fc_ptn=0;
      }
      
      if(hg->fc_mag<1){
	hg->fc_mag=1;
	hg->fc_magmode=0;
      }
      else if(hg->fc_mag>FC_MAX_MAG){
	hg->fc_mag=FC_MAX_MAG;
      }
      else{
	if(mag0==1){
	  hg->fc_magmode=0;
	}
	else if(hg->fc_magmode==0){
	  if((magx0!=x)||(magy0!=y)){
	    hg->fc_magmode=1;
	  }
	}
	
	if(hg->fc_magmode==0){
	  hg->fc_magx=x;
	  hg->fc_magy=y;
	}
	else{
	  hg->fc_magx=magx0+(x-width/2)/mag0;
	  hg->fc_magy=magy0+(y-height/2)/mag0;
	}
	if(hg->fc_mag!=mag0){
	  draw_fc_cairo(hg->fc_dw,hg);
	}
      }
    }
    else{
      // Draw points at cursor position
#ifdef USE_GTK3
      gdk_window_get_device_position(gtk_widget_get_window(widget),
				     event->device, &x,&y,NULL);
#else
      gdk_window_get_pointer(gtk_widget_get_window(widget),&x,&y,NULL);
#endif
      
      mag0=hg->fc_mag;
      magx0=hg->fc_magx;
      magy0=hg->fc_magy;
      
      if(direction & GDK_SCROLL_DOWN){
	hg->fc_mag--;
	hg->fc_ptn=0;
      }
      else{
	hg->fc_mag++;
	hg->fc_ptn=0;
      }
      
      if(hg->fc_mag<1){
	hg->fc_mag=1;
	hg->fc_magmode=0;
      }
      else if(hg->fc_mag>FC_MAX_MAG){
	hg->fc_mag=FC_MAX_MAG;
      }
      else{

	if(mag0==1){
	  hg->fc_magmode=0;
	}
	else if(hg->fc_magmode==0){
	  if((magx0!=x)||(magy0!=y)){
	    hg->fc_magmode=1;
	  }
	}
	
	if(hg->fc_magmode==0){
	  hg->fc_magx=x;
	  hg->fc_magy=y;
	}
	else{
	  GtkAllocation *allocation=g_new(GtkAllocation, 1);
	  gtk_widget_get_allocation(widget,allocation);

	  width= allocation->width;
	  height=allocation->height;
	  g_free(allocation);
	  
	  hg->fc_magx=magx0+(x-width/2)/mag0;
	  hg->fc_magy=magy0+(y-height/2)/mag0;
	}

	if(hg->fc_mag!=mag0){
	  draw_fc_cairo(hg->fc_dw,hg);
	}
      }
    }
  }

  return(TRUE);
}


static gboolean button_draw_fc(GtkWidget *widget, 
			GdkEventButton *event, 
			gpointer userdata){
  typHOE *hg;
  gint x,y, width, height;

  hg=(typHOE *)userdata;

  if(flagFC){
#ifdef USE_GTK3
    gdk_window_get_device_position(gtk_widget_get_window(widget),
				   event->device, &x,&y,NULL);
#else
    gdk_window_get_pointer(gtk_widget_get_window(widget),&x,&y,NULL);
#endif

    if((event->button==1)&&(hg->fcdb_flag)&&(hg->fcdb_i==hg->dss_i)){
      hg->fc_ptn=-1;
      hg->fc_ptx1=x;
      hg->fc_pty1=y;
    }
    else if(event->button==2){
      GtkAllocation *allocation=g_new(GtkAllocation, 1);
      gtk_widget_get_allocation(widget,allocation);

      width= allocation->width;
      height=allocation->height;
      g_free(allocation);
	  
      hg->fc_magx+=(x-width/2)/hg->fc_mag;
      hg->fc_magy+=(y-height/2)/hg->fc_mag;
    }
    else{
      if(hg->fc_ptn==2){
	hg->fc_ptn=0;
      }
      else if(hg->fc_ptn==0){
	hg->fc_ptn=1;
	hg->fc_ptx1=x;
	hg->fc_pty1=y;
      }
      else if(hg->fc_ptn==1){
	hg->fc_ptn=2;
	hg->fc_ptx2=x;
	hg->fc_pty2=y;
      }
    }

    draw_fc_cairo(hg->fc_dw,hg);
  }

  return(TRUE);
}


static void refresh_fc (GtkWidget *widget, gpointer data)
{
  typHOE *hg = (typHOE *)data;

  if(flagFC){
    hg->fc_output=FC_OUTPUT_WINDOW;
    draw_fc_cairo(hg->fc_dw,hg);
  }
}

static void orbit_fc (GtkWidget *widget, gpointer data)
{
  typHOE *hg = (typHOE *)data;

  if(flagFC){
    hg->orbit_flag=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
  
    hg->fc_output=FC_OUTPUT_WINDOW;
    draw_fc_cairo(hg->fc_dw,hg);
  }
}


static void hsc_dith_ol_fc (GtkWidget *widget, gpointer data)
{
  typHOE *hg = (typHOE *)data;

  if(flagFC){
    hg->hsc_show_ol=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
  
    hg->fc_output=FC_OUTPUT_WINDOW;
    draw_fc_cairo(hg->fc_dw,hg);
  }
}


void set_hsc_show_dith_label (typHOE *hg){
  gchar *tmp;

  switch(hg->hsc_show_dith_p){
  case HSC_DITH_NO:
    tmp=g_strdup_printf(" %d/1 ",hg->hsc_show_dith_i);
    break;

  case HSC_DITH_5:
    tmp=g_strdup_printf(" %d/5 ",hg->hsc_show_dith_i);
    break;

  case HSC_DITH_N:
    tmp=g_strdup_printf(" %d/%d ",hg->hsc_show_dith_i,hg->hsc_show_dith_n);
    break;
  }

  gtk_label_set_text(GTK_LABEL(hg->hsc_label_dith),tmp);

  if(tmp) g_free(tmp);
}


static void hsc_show_dith_back (GtkWidget *widget,  gpointer * gdata)
{
  typHOE *hg;

  hg=(typHOE *)gdata;

  switch(hg->hsc_show_dith_p){
  case HSC_DITH_NO:
    return;
    break;

  case HSC_DITH_5:
  case HSC_DITH_N:
    if(hg->hsc_show_dith_i>1){
      hg->hsc_show_dith_i--;
      set_hsc_show_dith_label(hg);
      if(flagFC)  draw_fc_cairo(hg->fc_dw,hg);
    }
    break;
  }
}

static void hsc_show_dith_forward (GtkWidget *widget,  gpointer * gdata)
{
  typHOE *hg;

  hg=(typHOE *)gdata;

  switch(hg->hsc_show_dith_p){
  case HSC_DITH_NO:
    return;
    break;

  case HSC_DITH_5:
    if(hg->hsc_show_dith_i<5){
      hg->hsc_show_dith_i++;
      set_hsc_show_dith_label(hg);
      if(flagFC)  draw_fc_cairo(hg->fc_dw,hg);
    }
    break;

  case HSC_DITH_N:
    if(hg->hsc_show_dith_i<hg->hsc_show_dith_n){
      hg->hsc_show_dith_i++;
      set_hsc_show_dith_label(hg);
      if(flagFC)  draw_fc_cairo(hg->fc_dw,hg);
    }
    break;
  }
}

static void cc_get_hsc_show_dith (GtkWidget *widget,  gpointer * gdata)
{
  typHOE *hg;
  GtkTreeIter iter;

  hg=(typHOE *)gdata;

  if(gtk_combo_box_get_active_iter(GTK_COMBO_BOX(widget), &iter)){
    gint n;
    GtkTreeModel *model;
    
    model=gtk_combo_box_get_model(GTK_COMBO_BOX(widget));
    gtk_tree_model_get (model, &iter, 1, &n, -1);

    hg->hsc_show_dith_p=n;
    hg->hsc_show_dith_i=1;

    set_hsc_show_dith_label(hg);

    if(flagFC)  draw_fc_cairo(hg->fc_dw,hg);
  }
}

void set_dss_arcmin_upper(typHOE *hg){
  switch(hg->fc_mode){
  case FC_PANCOL:
  case FC_PANG:
  case FC_PANR:
  case FC_PANI:
  case FC_PANY:
  case FC_PANZ:
    if (hg->dss_arcmin>PANSTARRS_MAX_ARCMIN){
      hg->dss_arcmin=PANSTARRS_MAX_ARCMIN;      
    }
    if(flagFC){
      gtk_adjustment_set_upper(hg->fc_adj_dss_arcmin, 
			       (gdouble)(PANSTARRS_MAX_ARCMIN));
      gtk_adjustment_set_value(hg->fc_adj_dss_arcmin, 
			       (gdouble)hg->dss_arcmin);
    }
    break;
  default:
    if(flagFC){
      gtk_adjustment_set_upper(hg->fc_adj_dss_arcmin, 
			       (gdouble)(DSS_ARCMIN_MAX));
    }
    break;
  }
}

static void cc_get_fc_inst (GtkWidget *widget,  gpointer * gdata)
{
  typHOE *hg;
  GtkTreeIter iter;

  hg=(typHOE *)gdata;

  if(gtk_combo_box_get_active_iter(GTK_COMBO_BOX(widget), &iter)){
    gint n;
    GtkTreeModel *model;
    
    model=gtk_combo_box_get_model(GTK_COMBO_BOX(widget));
    gtk_tree_model_get (model, &iter, 1, &n, -1);

    hg->fc_inst=n;
  }

  switch(hg->fc_inst){
  case FC_INST_HDS:
  case FC_INST_HDSAUTO:
  case FC_INST_HDSZENITH:
    gtk_adjustment_set_value(hg->fc_adj_dss_arcmin, 
			     (gdouble)(HDS_SIZE));
    break;

  case FC_INST_IRD:
    gtk_adjustment_set_value(hg->fc_adj_dss_arcmin, 
			     (gdouble)(IRD_SIZE));
    break;

  case FC_INST_IRCS:
    gtk_adjustment_set_value(hg->fc_adj_dss_arcmin, 
			     (gdouble)(IRCS_SIZE));
    break;

  case FC_INST_COMICS:
    gtk_adjustment_set_value(hg->fc_adj_dss_arcmin, 
			     (gdouble)(COMICS_SIZE));
    break;

  case FC_INST_FOCAS:
    gtk_adjustment_set_value(hg->fc_adj_dss_arcmin, 
			     (gdouble)(FOCAS_SIZE));
    break;

  case FC_INST_MOIRCS:
    gtk_adjustment_set_value(hg->fc_adj_dss_arcmin, 
			     (gdouble)(MOIRCS_SIZE));
    break;

  case FC_INST_SWIMS:
    gtk_adjustment_set_value(hg->fc_adj_dss_arcmin, 
			     (gdouble)(SWIMS_SIZE));
    break;

  case FC_INST_FMOS:
    gtk_adjustment_set_value(hg->fc_adj_dss_arcmin, 
			     (gdouble)(FMOS_SIZE));
    break;
			     
  case FC_INST_SPCAM:
    gtk_adjustment_set_value(hg->fc_adj_dss_arcmin, 
			     (gdouble)(SPCAM_SIZE));
    break;
			     
  case FC_INST_HSCDET:
  case FC_INST_HSCA:
    gtk_adjustment_set_value(hg->fc_adj_dss_arcmin, 
			     (gdouble)(HSC_SIZE));
    break;

  case FC_INST_PFS:
    gtk_adjustment_set_value(hg->fc_adj_dss_arcmin, 
			     (gdouble)(HSC_SIZE));
    break;

  case FC_INST_KOOLS:
    gtk_adjustment_set_value(hg->fc_adj_dss_arcmin, 
			     (gdouble)(KOOLS_SIZE));
    break;

  case FC_INST_TRICCS:
    gtk_adjustment_set_value(hg->fc_adj_dss_arcmin, 
			     (gdouble)(TRICCS_SIZE));
    break;

  default:
    break;
  }

  if(hg->fc_inst==FC_INST_HDSAUTO){
    hg->dss_flip=FALSE;
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hg->fc_button_flip),
				 hg->dss_flip);
    gtk_widget_set_sensitive(hg->fc_button_flip,FALSE);
  }
  else if(hg->fc_inst==FC_INST_HDSZENITH){
    hg->dss_flip=TRUE;
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hg->fc_button_flip),
				 hg->dss_flip);
    gtk_widget_set_sensitive(hg->fc_button_flip,FALSE);
  }
  else{
    gtk_widget_set_sensitive(hg->fc_button_flip,TRUE);
  }

  switch(hg->fc_inst){
  case FC_INST_HSCDET:
  case FC_INST_HSCA:
    gtk_widget_set_sensitive(hg->check_hsc_sat, TRUE);
    break;
    
  default:
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hg->check_hsc_sat),
				 FALSE);
    gtk_widget_set_sensitive(hg->check_hsc_sat, FALSE);
    break;
  }
}

static void cc_get_fc_mode (GtkWidget *widget,  gpointer gdata)
{
  GtkTreeIter iter;
  typHOE *hg = (typHOE *)gdata;

  if(gtk_combo_box_get_active_iter(GTK_COMBO_BOX(widget), &iter)){
    gint n;
    GtkTreeModel *model;
    
    model=gtk_combo_box_get_model(GTK_COMBO_BOX(widget));
    gtk_tree_model_get (model, &iter, 1, &n, -1);

    hg->fc_mode=n;

    set_fc_frame_col(hg);
    set_fc_mode(hg);
  }
}

static void cc_get_fc_mode_pdf (GtkWidget *widget,  gpointer gdata)
{
  GtkTreeIter iter;
  typHOE *hg = (typHOE *)gdata;

  if(gtk_combo_box_get_active_iter(GTK_COMBO_BOX(widget), &iter)){
    gint n;
    GtkTreeModel *model;
    
    model=gtk_combo_box_get_model(GTK_COMBO_BOX(widget));
    gtk_tree_model_get (model, &iter, 1, &n, -1);

    hg->fc_mode=n;

    set_fc_frame_col_pdf(hg);
    set_fc_mode(hg);
  }
}

static void do_print_fc (GtkWidget *widget, gpointer gdata)
{
  typHOE *hg;
  GtkPrintOperation *op; 
  GtkPrintOperationResult res; 

  hg=(typHOE *)gdata;

  op = gtk_print_operation_new ();

  gtk_print_operation_set_n_pages (op, 1); 
  my_signal_connect (op, "draw_page", G_CALLBACK (draw_page), (gpointer)hg); 
  res = gtk_print_operation_run (op, GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG,
				 NULL,NULL);

  g_object_unref(G_OBJECT(op));
}


static void draw_page (GtkPrintOperation *operation, 
		       GtkPrintContext *context,
		       gint page_nr, gpointer gdata)
{
  typHOE *hg;
  hg=(typHOE *)gdata;

  hg->fc_output=FC_OUTPUT_PRINT;
  hg->context=context;
  if(flagFC){
    draw_fc_cairo(hg->fc_dw,hg);
  }

  hg->fc_output=FC_OUTPUT_WINDOW;
  hg->context=NULL;
} 



gboolean check_dssall (gpointer gdata){
  if(flag_dssall_finish){
    flag_dssall_finish=FALSE;
      gtk_main_quit();
  }
  return(TRUE);
}


static void show_fc_help (GtkWidget *widget, GtkWidget *parent)
{
  GtkWidget *dialog, *label, *button, *pixmap, *vbox, *hbox, *table;
  GdkPixbuf *icon, *pixbuf;
  gint w,h;

  gtk_icon_size_lookup(GTK_ICON_SIZE_LARGE_TOOLBAR,&w,&h);

  dialog = gtk_dialog_new();
  gtk_window_set_transient_for(GTK_WINDOW(dialog),GTK_WINDOW(parent));
  gtk_container_set_border_width(GTK_CONTAINER(dialog),5);
  gtk_window_set_title(GTK_WINDOW(dialog),"HOE : Help for Finding Chart");

  my_signal_connect(dialog,"destroy",
		    close_fc_help, 
		    GTK_WIDGET(dialog));
  
  table = gtkut_table_new(2, 11, FALSE, 5, 10, 5);
  gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
		     table,FALSE, FALSE, 0);

  icon = gdk_pixbuf_new_from_resource ("/icons/dl_icon.png", NULL);
  pixbuf=gdk_pixbuf_scale_simple(icon,w,h,GDK_INTERP_BILINEAR);

  pixmap = gtk_image_new_from_pixbuf(pixbuf);
  g_object_unref(icon);
  g_object_unref(pixbuf);
#ifdef USE_GTK3      
  gtk_widget_set_halign(pixmap,GTK_ALIGN_CENTER);
  gtk_widget_set_valign(pixmap,GTK_ALIGN_CENTER);
#endif
  gtkut_table_attach(table, pixmap, 0, 1, 0, 1,
		      GTK_SHRINK,GTK_SHRINK,0,0);
  gtk_widget_show(pixmap);
  //g_object_unref(pixmap);

  label = gtk_label_new ("  Download new image and redraw w/instrument");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
  gtk_widget_set_hexpand(label,TRUE);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtkut_table_attach(table, label, 1, 2, 0, 1,
		      GTK_FILL,GTK_SHRINK,0,0);
  

#ifdef USE_GTK3
  pixmap=gtk_image_new_from_icon_name ("view-refresh", 
				       GTK_ICON_SIZE_LARGE_TOOLBAR);
  gtk_widget_set_halign(pixmap,GTK_ALIGN_CENTER);
  gtk_widget_set_valign(pixmap,GTK_ALIGN_CENTER);
#else
  pixmap=gtk_image_new_from_stock (GTK_STOCK_REFRESH, 
				   GTK_ICON_SIZE_LARGE_TOOLBAR);
#endif
  gtkut_table_attach(table, pixmap, 0, 1, 1, 2,
		      GTK_SHRINK,GTK_SHRINK,0,0);
  gtk_widget_show(pixmap);
  //g_object_unref(pixmap);

  label = gtk_label_new ("  Redraw selected instrument and PA");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
  gtk_widget_set_hexpand(label,TRUE);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtkut_table_attach(table, label, 1, 2, 1, 2,
		      GTK_FILL,GTK_SHRINK,0,0);

#ifdef USE_GTK3
  pixmap=gtk_image_new_from_icon_name ("edit-find",
				       GTK_ICON_SIZE_LARGE_TOOLBAR);
  gtk_widget_set_halign(pixmap,GTK_ALIGN_CENTER);
  gtk_widget_set_valign(pixmap,GTK_ALIGN_CENTER);
#else
  pixmap=gtk_image_new_from_stock (GTK_STOCK_FIND,
				   GTK_ICON_SIZE_LARGE_TOOLBAR);
#endif
  gtkut_table_attach(table, pixmap, 0, 1, 2, 3,
		      GTK_SHRINK,GTK_SHRINK,0,0);
  gtk_widget_show(pixmap);
  //  g_object_unref(pixmap);

  label = gtk_label_new ("  Query objects in the finding chart via online database (SIMBAD/NED)");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
  gtk_widget_set_hexpand(label,TRUE);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtkut_table_attach(table, label, 1, 2, 2, 3,
		      GTK_FILL,GTK_SHRINK,0,0);

#ifdef USE_GTK3
  pixmap=gtk_image_new_from_icon_name ("emblem-system",
				       GTK_ICON_SIZE_LARGE_TOOLBAR);
  gtk_widget_set_halign(pixmap,GTK_ALIGN_CENTER);
  gtk_widget_set_valign(pixmap,GTK_ALIGN_CENTER);
#else
  pixmap=gtk_image_new_from_stock (GTK_STOCK_PROPERTIES, 
				   GTK_ICON_SIZE_LARGE_TOOLBAR);
#endif
  gtkut_table_attach(table, pixmap, 0, 1, 3, 4,
		      GTK_SHRINK,GTK_SHRINK,0,0);
  gtk_widget_show(pixmap);

  label = gtk_label_new ("  Change parameters for database query");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
  gtk_widget_set_hexpand(label,TRUE);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtkut_table_attach(table, label, 1, 2, 3, 4,
		      GTK_FILL,GTK_SHRINK,0,0);


  label = gtkut_label_new ("<b>wheel-scroll</b>");
#ifdef USE_GTK3
  gtk_widget_set_halign(label,GTK_ALIGN_CENTER);
  gtk_widget_set_valign(label,GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtkut_table_attach(table, label, 0, 1, 4, 5,
		      GTK_SHRINK,GTK_SHRINK,0,0);
  
  label = gtk_label_new ("  Enlarge view around cursor (upto x5)");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
  gtk_widget_set_hexpand(label,TRUE);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtkut_table_attach(table, label, 1, 2, 4, 5,
		      GTK_FILL,GTK_SHRINK,0,0);

#ifdef USE_OSX
  label = gtkut_label_new ("<b>option</b> + <b>wheel-scroll</b>");
#else  
  label = gtkut_label_new ("<b>alt</b> + <b>wheel-scroll</b>");
#endif
#ifdef USE_GTK3
  gtk_widget_set_halign(label,GTK_ALIGN_CENTER);
  gtk_widget_set_valign(label,GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtkut_table_attach(table, label, 0, 1, 5, 6,
		      GTK_SHRINK,GTK_SHRINK,0,0);
  
  label = gtk_label_new ("  Enlarge view w/o moving the center (upto x5)");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
  gtk_widget_set_hexpand(label,TRUE);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtkut_table_attach(table, label, 1, 2, 5, 6,
		      GTK_FILL,GTK_SHRINK,0,0);
  
  label = gtkut_label_new ("<b>ctrl</b> + <b>wheel-scroll</b>");
#ifdef USE_GTK3
  gtk_widget_set_halign(label,GTK_ALIGN_CENTER);
  gtk_widget_set_valign(label,GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtkut_table_attach(table, label, 0, 1, 6, 7,
		      GTK_SHRINK,GTK_SHRINK,0,0);
  
  label = gtk_label_new ("  Rotate position angle (w/1 deg step)");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
  gtk_widget_set_hexpand(label,TRUE);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtkut_table_attach(table, label, 1, 2, 6, 7,
		      GTK_FILL,GTK_SHRINK,0,0);
  
  label = gtkut_label_new ("<b>shift</b> + <b>wheel-scroll</b>");
#ifdef USE_GTK3
  gtk_widget_set_halign(label,GTK_ALIGN_CENTER);
  gtk_widget_set_valign(label,GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtkut_table_attach(table, label, 0, 1, 7, 8,
		      GTK_SHRINK,GTK_SHRINK,0,0);
  
  label = gtk_label_new ("  Rotate position angle (w/5 deg step)");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
  gtk_widget_set_hexpand(label,TRUE);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtkut_table_attach(table, label, 1, 2, 7, 8,
		      GTK_FILL,GTK_SHRINK,0,0);
  
  label = gtkut_label_new ("<b>left-click</b>");
#ifdef USE_GTK3
  gtk_widget_set_halign(label,GTK_ALIGN_CENTER);
  gtk_widget_set_valign(label,GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtkut_table_attach(table, label, 0, 1, 8, 9,
		      GTK_SHRINK,GTK_SHRINK,0,0);
  
  label = gtk_label_new ("  Focus on the identified object");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
  gtk_widget_set_hexpand(label,TRUE);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtkut_table_attach(table, label, 1, 2, 8, 9,
		      GTK_FILL,GTK_SHRINK,0,0);

  label = gtkut_label_new ("<b>middle-click</b>");
#ifdef USE_GTK3
  gtk_widget_set_halign(label,GTK_ALIGN_CENTER);
  gtk_widget_set_valign(label,GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtkut_table_attach(table, label, 0, 1, 9, 10,
		      GTK_SHRINK,GTK_SHRINK,0,0);
  
  label = gtk_label_new ("  Move the clicked point to the center");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
  gtk_widget_set_hexpand(label,TRUE);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtkut_table_attach(table, label, 1, 2, 9, 10,
		      GTK_FILL,GTK_SHRINK,0,0);

  label = gtkut_label_new ("<b>right-click</b>");
#ifdef USE_GTK3
  gtk_widget_set_halign(label,GTK_ALIGN_CENTER);
  gtk_widget_set_valign(label,GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtkut_table_attach(table, label, 0, 1, 10, 11,
		      GTK_SHRINK,GTK_SHRINK,0,0);
  
  label = gtk_label_new ("  Measure the distance between 2-points (The 3rd click to clear)");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
  gtk_widget_set_hexpand(label,TRUE);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtkut_table_attach(table, label, 1, 2, 10, 11,
		      GTK_FILL,GTK_SHRINK,0,0);
  

  label = gtk_label_new ("");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
  gtk_widget_set_hexpand(label,TRUE);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtkut_table_attach(table, label, 0, 2, 11, 12,
		      GTK_FILL,GTK_SHRINK,0,0);


  label = gtk_label_new ("Please use SkyView or SDSS for large FOV (> 60\') to save the traffic.");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
  gtk_widget_set_hexpand(label,TRUE);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtkut_table_attach(table, label, 0, 2, 12, 13,
		      GTK_FILL,GTK_SHRINK,0,0);

  label = gtk_label_new ("ESO and STSci cannot change their pixel scale.");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
  gtk_widget_set_hexpand(label,TRUE);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtkut_table_attach(table, label, 0, 2, 13, 14,
		      GTK_FILL,GTK_SHRINK,0,0);

  label = gtk_label_new ("Because the maximum pixel sizes for SkyView (1000pix) and SDSS (2000pix) are limited,");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
  gtk_widget_set_hexpand(label,TRUE);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtkut_table_attach(table, label, 0, 2, 14, 15,
		      GTK_FILL,GTK_SHRINK,0,0);

  label = gtk_label_new ("the downloaded FC image for large FOV (> 13\' for SDSS) should be degraded from the original.");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
  gtk_widget_set_hexpand(label,TRUE);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtkut_table_attach(table, label, 0, 2, 15, 16,
		     GTK_FILL,GTK_SHRINK,0,0);


#ifdef USE_GTK3
  button=gtkut_button_new_from_icon_name("Close","window-close");
#else
  button=gtkut_button_new_from_stock("OK",GTK_STOCK_OK);
#endif
  gtk_dialog_add_action_widget(GTK_DIALOG(dialog),button,GTK_RESPONSE_CANCEL);
  my_signal_connect(button,"pressed",
		    close_fc_help, 
		    GTK_WIDGET(dialog));

  gtk_widget_show_all(dialog);
}

static void close_fc_help(GtkWidget *w, GtkWidget *dialog)
{
  gtk_widget_destroy(dialog);
}

void create_fcdb_para_dialog (typHOE *hg)
{
  GtkWidget *dialog, *label, *button, *frame, *hbox, *vbox, 
    *spinner, *combo, *table, *check, *rb[18], 
    *table1, *hbox1, *vbox1;
  GtkAdjustment *adj;
  gint tmp_band, tmp_mag, tmp_otype, tmp_ned_otype, tmp_ned_diam, 
    tmp_gsc_mag, tmp_gsc_diam, tmp_ps1_mag, tmp_ps1_diam, tmp_ps1_mindet,
    tmp_ps1_mode, tmp_ps1_dr, tmp_sdss_search,
    tmp_sdss_magmax[NUM_SDSS_BAND], tmp_sdss_magmin[NUM_SDSS_BAND], 
    tmp_sdss_diam, tmp_lamost_dr, tmp_usno_mag, tmp_ucac_mag,
    tmp_gaia_mag, tmp_kepler_mag, 
    tmp_2mass_mag, tmp_2mass_diam,
    tmp_wise_mag;
  gboolean tmp_ned_ref, tmp_gsc_fil, tmp_ps1_fil, tmp_usno_fil, tmp_ucac_fil,
    tmp_sdss_fil[NUM_SDSS_BAND], 
    tmp_gaia_fil, tmp_kepler_fil, tmp_2mass_fil, tmp_wise_fil,
    tmp_smoka_shot,
    tmp_smoka_subaru[NUM_SMOKA_SUBARU],
    tmp_smoka_kiso[NUM_SMOKA_KISO],
    tmp_smoka_oao[NUM_SMOKA_OAO],
    tmp_smoka_mtm[NUM_SMOKA_MTM],
    tmp_smoka_kanata[NUM_SMOKA_KANATA],
    tmp_hst_image[NUM_HST_IMAGE],
    tmp_hst_spec[NUM_HST_SPEC],
    tmp_hst_other[NUM_HST_OTHER],
    tmp_eso_image[NUM_ESO_IMAGE],
    tmp_eso_spec[NUM_ESO_SPEC],
    tmp_eso_vlti[NUM_ESO_VLTI],
    tmp_eso_pola[NUM_ESO_POLA],
    tmp_eso_coro[NUM_ESO_CORO],
    tmp_eso_other[NUM_ESO_OTHER],
    tmp_eso_sam[NUM_ESO_SAM],
    tmp_gemini_inst;
  confPropFCDB *cdata;
  gboolean rebuild_flag=FALSE;
  gint i;
  gchar tmp[BUFFSIZE];
  gchar *tmp_str;

  cdata=g_malloc0(sizeof(confPropFCDB));
  cdata->mode=0;

  tmp_band=hg->fcdb_band;
  tmp_mag=hg->fcdb_mag;
  tmp_otype=hg->fcdb_otype;
  tmp_ned_diam=hg->fcdb_ned_diam;
  tmp_ned_otype=hg->fcdb_ned_otype;
  tmp_ned_ref=hg->fcdb_ned_ref;
  tmp_gsc_fil=hg->fcdb_gsc_fil;
  tmp_gsc_mag=hg->fcdb_gsc_mag;
  tmp_gsc_diam=hg->fcdb_gsc_diam;
  tmp_ps1_fil=hg->fcdb_ps1_fil;
  tmp_ps1_mag=hg->fcdb_ps1_mag;
  tmp_ps1_diam=hg->fcdb_ps1_diam;
  tmp_ps1_mindet=hg->fcdb_ps1_mindet;
  tmp_ps1_mode=hg->fcdb_ps1_mode;
  tmp_ps1_dr=hg->fcdb_ps1_dr;
  tmp_sdss_search=hg->fcdb_sdss_search;
  for(i=0;i<NUM_SDSS_BAND;i++){
    tmp_sdss_fil[i]=hg->fcdb_sdss_fil[i];
    tmp_sdss_magmax[i]=hg->fcdb_sdss_magmax[i];
    tmp_sdss_magmin[i]=hg->fcdb_sdss_magmin[i];
  }
  tmp_sdss_diam=hg->fcdb_sdss_diam;
  tmp_lamost_dr=hg->fcdb_lamost_dr;
  tmp_usno_fil=hg->fcdb_usno_fil;
  tmp_usno_mag=hg->fcdb_usno_mag;
  tmp_ucac_fil=hg->fcdb_ucac_fil;
  tmp_ucac_mag=hg->fcdb_ucac_mag;
  tmp_gaia_fil=hg->fcdb_gaia_fil;
  tmp_gaia_mag=hg->fcdb_gaia_mag;
  tmp_kepler_fil=hg->fcdb_kepler_fil;
  tmp_kepler_mag=hg->fcdb_kepler_mag;
  tmp_2mass_fil=hg->fcdb_2mass_fil;
  tmp_2mass_mag=hg->fcdb_2mass_mag;
  tmp_2mass_diam=hg->fcdb_2mass_diam;
  tmp_wise_fil=hg->fcdb_wise_fil;
  tmp_wise_mag=hg->fcdb_wise_mag;
  tmp_smoka_shot=hg->fcdb_smoka_shot;
  for(i=0;i<NUM_SMOKA_SUBARU;i++){
    tmp_smoka_subaru[i]=hg->fcdb_smoka_subaru[i];
  }
  for(i=0;i<NUM_SMOKA_KISO;i++){
    tmp_smoka_kiso[i]=hg->fcdb_smoka_kiso[i];
  }
  for(i=0;i<NUM_SMOKA_OAO;i++){
    tmp_smoka_oao[i]=hg->fcdb_smoka_oao[i];
  }
  for(i=0;i<NUM_SMOKA_MTM;i++){
    tmp_smoka_mtm[i]=hg->fcdb_smoka_mtm[i];
  }
  for(i=0;i<NUM_SMOKA_KANATA;i++){
    tmp_smoka_kanata[i]=hg->fcdb_smoka_kanata[i];
  }
  for(i=0;i<NUM_HST_IMAGE;i++){
    tmp_hst_image[i]=hg->fcdb_hst_image[i];
  }
  for(i=0;i<NUM_HST_SPEC;i++){
    tmp_hst_spec[i]=hg->fcdb_hst_spec[i];
  }
  for(i=0;i<NUM_HST_OTHER;i++){
    tmp_hst_other[i]=hg->fcdb_hst_other[i];
  }
  for(i=0;i<NUM_ESO_IMAGE;i++){
    tmp_eso_image[i]=hg->fcdb_eso_image[i];
  }
  for(i=0;i<NUM_ESO_SPEC;i++){
    tmp_eso_spec[i]=hg->fcdb_eso_spec[i];
  }
  for(i=0;i<NUM_ESO_VLTI;i++){
    tmp_eso_vlti[i]=hg->fcdb_eso_vlti[i];
  }
  for(i=0;i<NUM_ESO_POLA;i++){
    tmp_eso_pola[i]=hg->fcdb_eso_pola[i];
  }
  for(i=0;i<NUM_ESO_CORO;i++){
    tmp_eso_coro[i]=hg->fcdb_eso_coro[i];
  }
  for(i=0;i<NUM_ESO_OTHER;i++){
    tmp_eso_other[i]=hg->fcdb_eso_other[i];
  }
  for(i=0;i<NUM_ESO_SAM;i++){
    tmp_eso_sam[i]=hg->fcdb_eso_sam[i];
  }
  tmp_gemini_inst =hg->fcdb_gemini_inst;

  dialog = gtk_dialog_new();
  gtk_window_set_transient_for(GTK_WINDOW(dialog),
			       GTK_WINDOW((flagFC) ? hg->fc_main :hg->w_top));
  cdata->dialog=dialog;
  gtk_container_set_border_width(GTK_CONTAINER(dialog),5);
  gtk_window_set_title(GTK_WINDOW(dialog),"HOE : Change Parameters for database query");
  gtk_window_set_modal(GTK_WINDOW(dialog),TRUE);

  my_signal_connect(dialog,"delete-event",delete_disp_para,GTK_WIDGET(dialog));

  hbox1 = gtkut_hbox_new(FALSE,0);
  gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
		     hbox1,FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hbox1), 0);

  frame = gtkut_frame_new ("<b>Database</b>");
  gtk_container_add (GTK_CONTAINER (hbox1), frame);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 3);

  hbox = gtkut_hbox_new(FALSE,0);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 0);

  rb[0] = gtk_radio_button_new_with_label_from_widget (NULL, "SIMBAD");
  gtk_box_pack_start(GTK_BOX(hbox), rb[0], FALSE, FALSE, 0);
  my_signal_connect (rb[0], "toggled", radio_fcdb, (gpointer)hg);

  rb[1] = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(rb[0]), "NED");
  gtk_box_pack_start(GTK_BOX(hbox), rb[1], FALSE, FALSE, 0);
  gtk_widget_show (rb[1]);
  my_signal_connect (rb[1], "toggled", radio_fcdb, (gpointer)hg);

  frame = gtkut_frame_new ("<b>Optical</b>");
  gtk_container_add (GTK_CONTAINER (hbox1), frame);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 3);

  hbox = gtkut_hbox_new(FALSE,2);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);

  rb[2] = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(rb[0]), "GSC");
  gtk_box_pack_start(GTK_BOX(hbox), rb[2], FALSE, FALSE, 0);
  gtk_widget_show (rb[2]);
  my_signal_connect (rb[2], "toggled", radio_fcdb, (gpointer)hg);

  rb[3] = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(rb[0]), "PanSTARRS");
  gtk_box_pack_start(GTK_BOX(hbox), rb[3], FALSE, FALSE, 0);
  gtk_widget_show (rb[3]);
  my_signal_connect (rb[3], "toggled", radio_fcdb, (gpointer)hg);

  rb[4] = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(rb[0]), "SDSS");
  gtk_box_pack_start(GTK_BOX(hbox), rb[4], FALSE, FALSE, 0);
  gtk_widget_show (rb[4]);
  my_signal_connect (rb[4], "toggled", radio_fcdb, (gpointer)hg);

  rb[5] = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(rb[0]), "LAMOST");
  gtk_box_pack_start(GTK_BOX(hbox), rb[5], FALSE, FALSE, 0);
  gtk_widget_show (rb[5]);
  my_signal_connect (rb[5], "toggled", radio_fcdb, (gpointer)hg);

  rb[6] = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(rb[0]), "USNO-B");
  gtk_box_pack_start(GTK_BOX(hbox), rb[6], FALSE, FALSE, 0);
  gtk_widget_show (rb[6]);
  my_signal_connect (rb[6], "toggled", radio_fcdb, (gpointer)hg);

  rb[7] = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(rb[0]), "UCAC4");
  gtk_box_pack_start(GTK_BOX(hbox), rb[7], FALSE, FALSE, 0);
  gtk_widget_show (rb[7]);
  my_signal_connect (rb[7], "toggled", radio_fcdb, (gpointer)hg);

  rb[8] = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(rb[0]), "GAIA");
  gtk_box_pack_start(GTK_BOX(hbox), rb[8], FALSE, FALSE, 0);
  gtk_widget_show (rb[8]);
  my_signal_connect (rb[8], "toggled", radio_fcdb, (gpointer)hg);

  rb[9] = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(rb[0]), "Kepler");
  gtk_box_pack_start(GTK_BOX(hbox), rb[9], FALSE, FALSE, 0);
  gtk_widget_show (rb[9]);
  my_signal_connect (rb[9], "toggled", radio_fcdb, (gpointer)hg);

  hbox1 = gtkut_hbox_new(FALSE,0);
  gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
		     hbox1,FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hbox1), 0);

  frame = gtkut_frame_new ("<b>Infrared</b>");
  gtk_container_add (GTK_CONTAINER (hbox1), frame);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 3);

  hbox = gtkut_hbox_new(FALSE,0);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 0);

  rb[10] = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(rb[0]), "2MASS");
  gtk_box_pack_start(GTK_BOX(hbox), rb[10], FALSE, FALSE, 0);
  gtk_widget_show (rb[10]);
  my_signal_connect (rb[10], "toggled", radio_fcdb, (gpointer)hg);

  rb[11] = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(rb[0]), "WISE");
  gtk_box_pack_start(GTK_BOX(hbox), rb[11], FALSE, FALSE, 0);
  gtk_widget_show (rb[11]);
  my_signal_connect (rb[11], "toggled", radio_fcdb, (gpointer)hg);

  rb[12] = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(rb[0]), "AKARI/IRC");
  gtk_box_pack_start(GTK_BOX(hbox), rb[12], FALSE, FALSE, 0);
  gtk_widget_show (rb[12]);
  my_signal_connect (rb[12], "toggled", radio_fcdb, (gpointer)hg);

  rb[13] = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(rb[0]), "AKARI/FIS");
  gtk_box_pack_start(GTK_BOX(hbox), rb[13], FALSE, FALSE, 0);
  gtk_widget_show (rb[13]);
  my_signal_connect (rb[13], "toggled", radio_fcdb, (gpointer)hg);

  frame = gtkut_frame_new ("<b>Data Archive</b>");
  gtk_container_add (GTK_CONTAINER (hbox1), frame);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 3);

  hbox = gtkut_hbox_new(FALSE,2);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);

  rb[14] = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(rb[0]), "SMOKA");
  gtk_box_pack_start(GTK_BOX(hbox), rb[14], FALSE, FALSE, 0);
  gtk_widget_show (rb[14]);
  my_signal_connect (rb[14], "toggled", radio_fcdb, (gpointer)hg);

  rb[15] = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(rb[0]), "HST");
  gtk_box_pack_start(GTK_BOX(hbox), rb[15], FALSE, FALSE, 0);
  gtk_widget_show (rb[15]);
  my_signal_connect (rb[15], "toggled", radio_fcdb, (gpointer)hg);

  rb[16] = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(rb[0]), "ESO");
  gtk_box_pack_start(GTK_BOX(hbox), rb[16], FALSE, FALSE, 0);
  gtk_widget_show (rb[16]);
  my_signal_connect (rb[16], "toggled", radio_fcdb, (gpointer)hg);

  rb[17] = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(rb[0]), "Gemini");
  gtk_box_pack_start(GTK_BOX(hbox), rb[17], FALSE, FALSE, 0);
  gtk_widget_show (rb[17]);
  my_signal_connect (rb[17], "toggled", radio_fcdb, (gpointer)hg);

  cdata->fcdb_group=gtk_radio_button_get_group(GTK_RADIO_BUTTON(rb[0]));
  cdata->fcdb_type=hg->fcdb_type;

  frame = gtkut_frame_new ("<b>Query parameters</b>");
  gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
		     frame,FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 3);

  hg->query_note = gtk_notebook_new ();
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (hg->query_note), GTK_POS_TOP);
  gtk_notebook_set_scrollable (GTK_NOTEBOOK (hg->query_note), TRUE);
  gtk_container_add (GTK_CONTAINER (frame), hg->query_note);

  vbox = gtkut_vbox_new (FALSE, 0);
  label = gtk_label_new ("SIMBAD");
  gtk_notebook_append_page (GTK_NOTEBOOK (hg->query_note), vbox, label);

  table = gtkut_table_new(2, 3, FALSE, 5, 10, 5);
  gtk_container_add (GTK_CONTAINER (vbox), table);

  label = gtk_label_new ("Search Area");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtkut_table_attach(table, label, 0, 1, 0, 1,
		     GTK_FILL,GTK_SHRINK,0,0);

  label = gtk_label_new ("= Finding Chart Area");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtkut_table_attach(table, label, 1, 2, 0, 1,
		     GTK_FILL,GTK_SHRINK,0,0);

  label = gtk_label_new ("Magnitude");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_END);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
#endif
  gtkut_table_attach(table, label, 0, 1, 1, 2,
		     GTK_FILL,GTK_SHRINK,0,0);

  hbox = gtkut_hbox_new(FALSE,0);
#ifdef USE_GTK3
  gtk_widget_set_halign(hbox,GTK_ALIGN_CENTER);
  gtk_widget_set_valign(hbox,GTK_ALIGN_CENTER);
#endif
  gtkut_table_attach(table, hbox, 1, 2, 1, 2,
		     GTK_SHRINK,GTK_SHRINK,0,0);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 0);

  {
    GtkListStore *store;
    GtkTreeIter iter, iter_set;	  
    GtkCellRenderer *renderer;
      
    store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
    
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "(Nop.)",
		       1, FCDB_BAND_NOP, -1);
    if(hg->fcdb_band==FCDB_BAND_NOP) iter_set=iter;
    
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "  U  ",
		       1, FCDB_BAND_U, -1);
    if(hg->fcdb_band==FCDB_BAND_U) iter_set=iter;
    
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "  B  ",
		       1, FCDB_BAND_B, -1);
    if(hg->fcdb_band==FCDB_BAND_B) iter_set=iter;
    
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "  V  ",
		       1, FCDB_BAND_V, -1);
    if(hg->fcdb_band==FCDB_BAND_V) iter_set=iter;
    
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "  R  ",
		       1, FCDB_BAND_R, -1);
    if(hg->fcdb_band==FCDB_BAND_R) iter_set=iter;
    
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "  I  ",
		       1, FCDB_BAND_I, -1);
    if(hg->fcdb_band==FCDB_BAND_I) iter_set=iter;
    
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "  J  ",
		       1, FCDB_BAND_J, -1);
    if(hg->fcdb_band==FCDB_BAND_J) iter_set=iter;
    
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "  H  ",
		       1, FCDB_BAND_H, -1);
    if(hg->fcdb_band==FCDB_BAND_H) iter_set=iter;
    
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "  K  ",
		       1, FCDB_BAND_K, -1);
    if(hg->fcdb_band==FCDB_BAND_K) iter_set=iter;
    
    combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));
    gtk_box_pack_start(GTK_BOX(hbox),combo,FALSE,FALSE,0);
    g_object_unref(store);
    
    renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo),renderer, TRUE);
    gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT(combo), renderer, "text",0,NULL);
    	
    gtk_combo_box_set_active_iter(GTK_COMBO_BOX(combo),&iter_set);
    gtk_widget_show(combo);
    my_signal_connect (combo,"changed",cc_get_combo_box,
		       &tmp_band);
  }

  label = gtk_label_new (" < ");
  gtk_box_pack_start(GTK_BOX(hbox), label,FALSE, FALSE, 0);

  adj = (GtkAdjustment *)gtk_adjustment_new(tmp_mag,
					    8, 25, 1, 1, 0);
  spinner =  gtk_spin_button_new (adj, 0, 0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), FALSE);
  gtk_box_pack_start(GTK_BOX(hbox), spinner,FALSE, FALSE, 0);
  my_entry_set_width_chars(GTK_ENTRY(&GTK_SPIN_BUTTON(spinner)->entry),2);
  my_signal_connect (adj, "value_changed", cc_get_adj, &tmp_mag);


  label = gtk_label_new ("Object Type");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_END);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
#endif
  gtkut_table_attach(table, label, 0, 1, 2, 3,
		     GTK_FILL,GTK_SHRINK,0,0);

  {
    GtkListStore *store;
    GtkTreeIter iter, iter_set;	  
    GtkCellRenderer *renderer;
      
    store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
    
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "All Types",
		       1, FCDB_OTYPE_ALL, -1);
    if(hg->fcdb_otype==FCDB_OTYPE_ALL) iter_set=iter;

    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "Star",
		       1, FCDB_OTYPE_STAR, -1);
    if(hg->fcdb_otype==FCDB_OTYPE_STAR) iter_set=iter;

    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "ISM",
		       1, FCDB_OTYPE_ISM, -1);
    if(hg->fcdb_otype==FCDB_OTYPE_ISM) iter_set=iter;

    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "Planetary Nebula",
		       1, FCDB_OTYPE_PN, -1);
    if(hg->fcdb_otype==FCDB_OTYPE_PN) iter_set=iter;

    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "H II region",
		       1, FCDB_OTYPE_HII, -1);
    if(hg->fcdb_otype==FCDB_OTYPE_HII) iter_set=iter;

    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "Galaxy",
		       1, FCDB_OTYPE_GALAXY, -1);
    if(hg->fcdb_otype==FCDB_OTYPE_GALAXY) iter_set=iter;

    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "QSO",
		       1, FCDB_OTYPE_QSO, -1);
    if(hg->fcdb_otype==FCDB_OTYPE_QSO) iter_set=iter;

    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "gamma-ray source",
		       1, FCDB_OTYPE_GAMMA, -1);
    if(hg->fcdb_otype==FCDB_OTYPE_GAMMA) iter_set=iter;

    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "X-ray source",
		       1, FCDB_OTYPE_X, -1);
    if(hg->fcdb_otype==FCDB_OTYPE_X) iter_set=iter;

    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "IR source",
		       1, FCDB_OTYPE_IR, -1);
    if(hg->fcdb_otype==FCDB_OTYPE_IR) iter_set=iter;

    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "Radio source",
		       1, FCDB_OTYPE_RADIO, -1);
    if(hg->fcdb_otype==FCDB_OTYPE_RADIO) iter_set=iter;

    combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));
#ifdef USE_GTK3
    gtk_widget_set_halign(combo,GTK_ALIGN_CENTER);
    gtk_widget_set_valign(combo,GTK_ALIGN_CENTER);
#endif
    gtkut_table_attach(table, combo, 1, 2, 2, 3,
		       GTK_SHRINK,GTK_SHRINK,0,0);
    g_object_unref(store);
    
    renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo),renderer, TRUE);
    gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT(combo), renderer, "text",0,NULL);
    	
    gtk_combo_box_set_active_iter(GTK_COMBO_BOX(combo),&iter_set);
    gtk_widget_show(combo);
    my_signal_connect (combo,"changed",cc_get_combo_box,
		       &tmp_otype);
  }


  vbox = gtkut_vbox_new (FALSE, 0);
  label = gtk_label_new ("NED");
  gtk_notebook_append_page (GTK_NOTEBOOK (hg->query_note), vbox, label);

  table = gtkut_table_new(2, 3, FALSE, 5, 10, 5);
  gtk_container_add (GTK_CONTAINER (vbox), table);

  label = gtk_label_new ("Search Diameter");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_END);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
#endif
  gtkut_table_attach(table, label, 0, 1, 0, 1,
		     GTK_FILL,GTK_SHRINK,0,0);

  hbox = gtkut_hbox_new(FALSE,0);
#ifdef USE_GTK3
  gtk_widget_set_halign(hbox,GTK_ALIGN_CENTER);
  gtk_widget_set_valign(hbox,GTK_ALIGN_CENTER);
#endif
  gtkut_table_attach(table, hbox, 1, 2, 0, 1,
		     GTK_SHRINK,GTK_SHRINK,0,0);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 0);

  label = gtk_label_new ("< ");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtk_box_pack_start(GTK_BOX(hbox), label,FALSE, FALSE, 0);

  adj = (GtkAdjustment *)gtk_adjustment_new(tmp_ned_diam,
					    1, FCDB_ARCMIN_MAX, 1, 1, 0);
  spinner =  gtk_spin_button_new (adj, 0, 0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), FALSE);
  gtk_box_pack_start(GTK_BOX(hbox), spinner,FALSE, FALSE, 0);
  my_entry_set_width_chars(GTK_ENTRY(&GTK_SPIN_BUTTON(spinner)->entry),4);
  my_signal_connect (adj, "value_changed", cc_get_adj, &tmp_ned_diam);

  label = gtk_label_new ("[arcmin]");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtk_box_pack_start(GTK_BOX(hbox), label,FALSE, FALSE, 0);

  label = gtk_label_new ("Object Type");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_END);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
#endif
  gtkut_table_attach(table, label, 0, 1, 1, 2,
		     GTK_FILL,GTK_SHRINK,0,0);

  {
    GtkListStore *store;
    GtkTreeIter iter, iter_set;	  
    GtkCellRenderer *renderer;
      
    store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
    
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "All Types",
		       1, FCDB_NED_OTYPE_ALL, -1);
    if(hg->fcdb_ned_otype==FCDB_NED_OTYPE_ALL) iter_set=iter;

    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "Extragalactic Object",
		       1, FCDB_NED_OTYPE_EXTRAG, -1);
    if(hg->fcdb_ned_otype==FCDB_NED_OTYPE_EXTRAG) iter_set=iter;

    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "QSO",
		       1, FCDB_NED_OTYPE_QSO, -1);
    if(hg->fcdb_ned_otype==FCDB_NED_OTYPE_QSO) iter_set=iter;

    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "Star",
		       1, FCDB_NED_OTYPE_STAR, -1);
    if(hg->fcdb_ned_otype==FCDB_NED_OTYPE_STAR) iter_set=iter;

    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "Nova / Super Nova",
		       1, FCDB_NED_OTYPE_SN, -1);
    if(hg->fcdb_ned_otype==FCDB_NED_OTYPE_SN) iter_set=iter;

    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "Planetary nebula",
		       1, FCDB_NED_OTYPE_PN, -1);
    if(hg->fcdb_ned_otype==FCDB_NED_OTYPE_PN) iter_set=iter;

    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "H II region",
		       1, FCDB_NED_OTYPE_HII, -1);
    if(hg->fcdb_ned_otype==FCDB_NED_OTYPE_HII) iter_set=iter;

    combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));
#ifdef USE_GTK3
    gtk_widget_set_halign(combo,GTK_ALIGN_CENTER);
    gtk_widget_set_valign(combo,GTK_ALIGN_CENTER);
#endif
    gtkut_table_attach(table, combo, 1, 2, 1, 2,
		       GTK_SHRINK,GTK_SHRINK,0,0);
    g_object_unref(store);
    
    renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo),renderer, TRUE);
    gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT(combo), renderer, "text",0,NULL);
    	
    gtk_combo_box_set_active_iter(GTK_COMBO_BOX(combo),&iter_set);
    gtk_widget_show(combo);
    my_signal_connect (combo,"changed",cc_get_combo_box,
		       &tmp_ned_otype);
  }

  check = gtk_check_button_new_with_label("Only objects w/references");
  gtkut_table_attach(table, check, 0, 2, 2, 3,
		     GTK_FILL,GTK_SHRINK,0,0);
  my_signal_connect (check, "toggled",
		     cc_get_toggle,
		     &tmp_ned_ref);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check),
			       hg->fcdb_ned_ref);


  vbox = gtkut_vbox_new (FALSE, 0);
  label = gtk_label_new ("GSC 2.4.1");
  gtk_notebook_append_page (GTK_NOTEBOOK (hg->query_note), vbox, label);

  // SDSS
  table = gtkut_table_new(2, 2, FALSE, 5, 10, 5);
  gtk_container_add (GTK_CONTAINER (vbox), table);

  label = gtk_label_new ("Search Diameter ");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_END);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
#endif
  gtkut_table_attach(table, label, 0, 1, 0, 1,
		     GTK_FILL,GTK_SHRINK,0,0);

  hbox = gtkut_hbox_new(FALSE,0);
#ifdef USE_GTK3
  gtk_widget_set_halign(hbox,GTK_ALIGN_CENTER);
  gtk_widget_set_valign(hbox,GTK_ALIGN_CENTER);
#endif
  gtkut_table_attach(table, hbox, 1, 2, 0, 1,
		     GTK_SHRINK,GTK_SHRINK,0,0);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 0);

  label = gtk_label_new ("< "); 
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtk_box_pack_start(GTK_BOX(hbox), label,FALSE, FALSE, 0);

  adj = (GtkAdjustment *)gtk_adjustment_new(tmp_gsc_diam,
					    1, FCDB_ARCMIN_MAX, 1, 1, 0);
  spinner =  gtk_spin_button_new (adj, 0, 0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), FALSE);
  gtk_box_pack_start(GTK_BOX(hbox), spinner,FALSE, FALSE, 0);
  my_entry_set_width_chars(GTK_ENTRY(&GTK_SPIN_BUTTON(spinner)->entry),4);
  my_signal_connect (adj, "value_changed", cc_get_adj, &tmp_gsc_diam);

  label = gtk_label_new ("[arcmin]"); 
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtk_box_pack_start(GTK_BOX(hbox), label,FALSE, FALSE, 0);


  check = gtk_check_button_new_with_label("Mag. filter");
  gtkut_table_attach(table, check, 0, 1, 1, 2,
		     GTK_FILL,GTK_SHRINK,0,0);
  my_signal_connect (check, "toggled",
		     cc_get_toggle,
		     &tmp_gsc_fil);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check),
			       hg->fcdb_gsc_fil);

  hbox = gtkut_hbox_new(FALSE,0);
#ifdef USE_GTK3
  gtk_widget_set_halign(hbox,GTK_ALIGN_CENTER);
  gtk_widget_set_valign(hbox,GTK_ALIGN_CENTER);
#endif
  gtkut_table_attach(table, hbox, 1, 2, 1, 2,
		     GTK_SHRINK,GTK_SHRINK,0,0);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 0);

  label = gtk_label_new ("R < ");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_END);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
#endif
  gtk_box_pack_start(GTK_BOX(hbox), label,FALSE, FALSE, 0);

  adj = (GtkAdjustment *)gtk_adjustment_new(tmp_gsc_mag,
					    12, 22, 1, 1, 0);
  spinner =  gtk_spin_button_new (adj, 0, 0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), FALSE);
  gtk_box_pack_start(GTK_BOX(hbox), spinner,FALSE, FALSE, 0);
  my_entry_set_width_chars(GTK_ENTRY(&GTK_SPIN_BUTTON(spinner)->entry),2);
  my_signal_connect (adj, "value_changed", cc_get_adj, &tmp_gsc_mag);


  vbox = gtkut_vbox_new (FALSE, 0);
  label = gtk_label_new ("PanSTARRS-1");
  gtk_notebook_append_page (GTK_NOTEBOOK (hg->query_note), vbox, label);

  table = gtkut_table_new(3, 5, FALSE, 5, 10, 5);
  gtk_container_add (GTK_CONTAINER (vbox), table);

  label = gtk_label_new ("Search Diameter ");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
#endif
  gtkut_table_attach(table, label, 0, 1, 0, 1,
		     GTK_FILL,GTK_SHRINK,0,0);

  hbox = gtkut_hbox_new(FALSE,0);
#ifdef USE_GTK3
  gtk_widget_set_halign(hbox,GTK_ALIGN_CENTER);
  gtk_widget_set_valign(hbox,GTK_ALIGN_CENTER);
#endif
  gtkut_table_attach(table, hbox, 1, 2, 0, 1,
		     GTK_SHRINK,GTK_SHRINK,0,0);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 0);

  label = gtk_label_new ("< "); 
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtk_box_pack_start(GTK_BOX(hbox), label,FALSE, FALSE, 0);

  adj = (GtkAdjustment *)gtk_adjustment_new(tmp_ps1_diam,
					    1, FCDB_PS1_MAX_DIAM, 1, 1, 0);
  spinner =  gtk_spin_button_new (adj, 0, 0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), FALSE);
  gtk_box_pack_start(GTK_BOX(hbox), spinner,FALSE, FALSE, 0);
  my_entry_set_width_chars(GTK_ENTRY(&GTK_SPIN_BUTTON(spinner)->entry),4);
  my_signal_connect (adj, "value_changed", cc_get_adj, &tmp_ps1_diam);

  label = gtk_label_new ("[arcmin]"); 
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtk_box_pack_start(GTK_BOX(hbox), label,FALSE, FALSE, 0);


  check = gtk_check_button_new_with_label("Mag. filter");
  gtkut_table_attach(table, check, 0, 1, 1, 2,
		     GTK_FILL,GTK_SHRINK,0,0);
  my_signal_connect (check, "toggled",
		     cc_get_toggle,
		     &tmp_ps1_fil);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check),
			       hg->fcdb_ps1_fil);

  hbox = gtkut_hbox_new(FALSE,0);
#ifdef USE_GTK3
  gtk_widget_set_halign(hbox,GTK_ALIGN_CENTER);
  gtk_widget_set_valign(hbox,GTK_ALIGN_CENTER);
#endif
  gtkut_table_attach(table, hbox, 1, 2, 1, 2,
		     GTK_SHRINK,GTK_SHRINK,0,0);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 0);

  label = gtk_label_new ("r < ");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_END);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
#endif
  gtk_box_pack_start(GTK_BOX(hbox), label,FALSE, FALSE, 0);

  adj = (GtkAdjustment *)gtk_adjustment_new(tmp_ps1_mag,
					    12, 22, 1, 1, 0);
  spinner =  gtk_spin_button_new (adj, 0, 0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), FALSE);
  gtk_box_pack_start(GTK_BOX(hbox), spinner,FALSE, FALSE, 0);
  my_entry_set_width_chars(GTK_ENTRY(&GTK_SPIN_BUTTON(spinner)->entry),2);
  my_signal_connect (adj, "value_changed", cc_get_adj, &tmp_ps1_mag);

  label = gtk_label_new ("Release");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtkut_table_attach(table, label, 0, 1, 2, 3,
		     GTK_FILL,GTK_SHRINK,0,0);
  
  {
    GtkListStore *store;
    GtkTreeIter iter, iter_set;	  
    GtkCellRenderer *renderer;
      
    store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
    
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "DR1 (Old, Mean only)",
		       1, FCDB_PS1_OLD, -1);
    if(hg->fcdb_ps1_dr==FCDB_PS1_OLD) iter_set=iter;

    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "DR1 (MAST)",
		       1, FCDB_PS1_DR_1, -1);
    if(hg->fcdb_ps1_dr==FCDB_PS1_DR_1) iter_set=iter;

    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "DR2 (MAST)",
		       1, FCDB_PS1_DR_2, -1);
    if(hg->fcdb_ps1_dr==FCDB_PS1_DR_2) iter_set=iter;

    combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));
#ifdef USE_GTK3
    gtk_widget_set_halign(combo,GTK_ALIGN_CENTER);
    gtk_widget_set_valign(combo,GTK_ALIGN_CENTER);
#endif
    gtkut_table_attach(table, combo, 1, 3, 2, 3,
		       GTK_SHRINK,GTK_SHRINK,0,0);
    g_object_unref(store);
    
    renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo),renderer, TRUE);
    gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT(combo), renderer, "text",0,NULL);
    	
    gtk_combo_box_set_active_iter(GTK_COMBO_BOX(combo),&iter_set);
    gtk_widget_show(combo);
    my_signal_connect (combo,"changed",cc_get_combo_box,
		       &tmp_ps1_dr);
  }
  
  label = gtk_label_new ("Catalog");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtkut_table_attach(table, label, 0, 1, 3, 4,
		     GTK_FILL,GTK_SHRINK,0,0);
  
  {
    GtkListStore *store;
    GtkTreeIter iter, iter_set;	  
    GtkCellRenderer *renderer;
      
    store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
    
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "Mean object",
		       1, FCDB_PS1_MODE_MEAN, -1);
    if(hg->fcdb_ps1_mode==FCDB_PS1_MODE_MEAN) iter_set=iter;

    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "Stacked object",
		       1, FCDB_PS1_MODE_STACK, -1);
    if(hg->fcdb_ps1_mode==FCDB_PS1_MODE_STACK) iter_set=iter;

    combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));
#ifdef USE_GTK3
    gtk_widget_set_halign(combo,GTK_ALIGN_CENTER);
    gtk_widget_set_valign(combo,GTK_ALIGN_CENTER);
#endif
    gtkut_table_attach(table, combo, 1, 2, 3, 4,
		       GTK_SHRINK,GTK_SHRINK,0,0);
    g_object_unref(store);
    
    renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo),renderer, TRUE);
    gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT(combo), renderer, "text",0,NULL);
    	
    gtk_combo_box_set_active_iter(GTK_COMBO_BOX(combo),&iter_set);
    gtk_widget_show(combo);
    my_signal_connect (combo,"changed",cc_get_combo_box,
		       &tmp_ps1_mode);
  }
  

  label = gtk_label_new ("Minimum nDetections");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_END);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
#endif
  gtkut_table_attach(table, label, 0, 1, 4, 5,
		     GTK_FILL,GTK_SHRINK,0,0);

  hbox = gtkut_hbox_new(FALSE,0);
#ifdef USE_GTK3
  gtk_widget_set_halign(hbox,GTK_ALIGN_CENTER);
  gtk_widget_set_valign(hbox,GTK_ALIGN_CENTER);
#endif
  gtkut_table_attach(table, hbox, 1, 2, 1, 2,
		     GTK_SHRINK,GTK_SHRINK,0,0);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 0);

  adj = (GtkAdjustment *)gtk_adjustment_new(tmp_ps1_mindet,
					    1, 25, 1, 1, 0);
  spinner =  gtk_spin_button_new (adj, 0, 0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), FALSE);
#ifdef USE_GTK3
  gtk_widget_set_halign(spinner,GTK_ALIGN_CENTER);
  gtk_widget_set_valign(spinner,GTK_ALIGN_CENTER);
#endif
  gtkut_table_attach(table, spinner, 1, 2, 4, 5,
		     GTK_SHRINK,GTK_SHRINK,0,0);
  my_entry_set_width_chars(GTK_ENTRY(&GTK_SPIN_BUTTON(spinner)->entry),2);
  my_signal_connect (adj, "value_changed", cc_get_adj, &tmp_ps1_mindet);

  vbox = gtkut_vbox_new (FALSE, 0);
  label = gtk_label_new ("SDSS DR16");
  gtk_notebook_append_page (GTK_NOTEBOOK (hg->query_note), vbox, label);

  
  table = gtkut_table_new(3, 3, FALSE, 5, 10, 5);
  gtk_container_add (GTK_CONTAINER (vbox), table);

  label = gtk_label_new ("Search Diameter ");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_END);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
#endif
  gtkut_table_attach(table, label, 0, 1, 0, 1,
		     GTK_FILL,GTK_SHRINK,0,0);

  hbox = gtkut_hbox_new(FALSE,0);
#ifdef USE_GTK3
  gtk_widget_set_halign(hbox,GTK_ALIGN_CENTER);
  gtk_widget_set_valign(hbox,GTK_ALIGN_CENTER);
#endif
  gtkut_table_attach(table, hbox, 1, 3, 0, 1,
		     GTK_SHRINK,GTK_SHRINK,0,0);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 0);

  label = gtk_label_new ("< "); 
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtk_box_pack_start(GTK_BOX(hbox), label,FALSE, FALSE, 0);

  adj = (GtkAdjustment *)gtk_adjustment_new(tmp_sdss_diam,
					    1, FCDB_ARCMIN_MAX, 1, 1, 0);
  spinner =  gtk_spin_button_new (adj, 0, 0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), FALSE);
  gtk_box_pack_start(GTK_BOX(hbox), spinner,FALSE, FALSE, 0);
  my_entry_set_width_chars(GTK_ENTRY(&GTK_SPIN_BUTTON(spinner)->entry),4);
  my_signal_connect (adj, "value_changed", cc_get_adj, &tmp_sdss_diam);

  label = gtk_label_new ("[arcmin]"); 
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtk_box_pack_start(GTK_BOX(hbox), label,FALSE, FALSE, 0);

  {
    GtkListStore *store;
    GtkTreeIter iter, iter_set;	  
    GtkCellRenderer *renderer;
      
    store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
    
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "Imaging Query",
		       1, FCDB_NED_OTYPE_ALL, -1);
    if(hg->fcdb_sdss_search==FCDB_SDSS_SEARCH_IMAG) iter_set=iter;

    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "Spectro Query",
		       1, FCDB_NED_OTYPE_EXTRAG, -1);
    if(hg->fcdb_sdss_search==FCDB_SDSS_SEARCH_SPEC) iter_set=iter;

    combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));
#ifdef USE_GTK3
    gtk_widget_set_halign(combo,GTK_ALIGN_CENTER);
    gtk_widget_set_valign(combo,GTK_ALIGN_CENTER);
#endif
    gtkut_table_attach(table, combo, 0, 1, 1, 2,
		       GTK_SHRINK,GTK_SHRINK,0,0);
    g_object_unref(store);
    
    renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo),renderer, TRUE);
    gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT(combo), renderer, "text",0,NULL);
    	
    gtk_combo_box_set_active_iter(GTK_COMBO_BOX(combo),&iter_set);
    gtk_widget_show(combo);
    my_signal_connect (combo,"changed",cc_get_combo_box,
		       &tmp_sdss_search);
  }

  
  frame = gtkut_frame_new ("<b>Mag. filter</b>");
  gtk_container_set_border_width (GTK_CONTAINER (frame), 3);
  gtkut_table_attach(table, frame, 0, 2, 2, 3,
		     GTK_FILL,GTK_SHRINK,0,0);

  table1 = gtkut_table_new(6,NUM_SDSS_BAND, FALSE, 5, 5, 5);
  gtk_container_add (GTK_CONTAINER (frame), table1);

  for(i=0;i<NUM_SDSS_BAND;i++){
    check = gtk_check_button_new_with_label(NULL);
    gtkut_table_attach(table1, check, 0, 1, i, i+1,
		       GTK_FILL,GTK_SHRINK,0,0);
    my_signal_connect (check, "toggled",
		       cc_get_toggle,
		       &tmp_sdss_fil[i]);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check),
				 hg->fcdb_sdss_fil[i]);

    adj = (GtkAdjustment *)gtk_adjustment_new(tmp_sdss_magmin[i],
					      0, 30, 1, 1, 0);
    spinner =  gtk_spin_button_new (adj, 0, 0);
    gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), FALSE);
    gtkut_table_attach(table1, spinner, 1, 2, i, i+1,
		       GTK_FILL,GTK_SHRINK,0,0);
    my_entry_set_width_chars(GTK_ENTRY(&GTK_SPIN_BUTTON(spinner)->entry),2);
    my_signal_connect (adj, "value_changed", cc_get_adj, &tmp_sdss_magmin[i]);


    label = gtk_label_new ("< "); 
#ifdef USE_GTK3
    gtk_widget_set_halign(label,GTK_ALIGN_CENTER);
    gtk_widget_set_valign(label,GTK_ALIGN_CENTER);
#endif
    gtkut_table_attach(table1, label, 2, 3, i, i+1,
		       GTK_SHRINK,GTK_SHRINK,0,0);

    label = gtk_label_new (sdss_band[i]); 
#ifdef USE_GTK3
    gtk_widget_set_halign(label,GTK_ALIGN_CENTER);
    gtk_widget_set_valign(label,GTK_ALIGN_CENTER);
#endif
    gtkut_table_attach(table1, label, 3, 4, i, i+1,
		       GTK_SHRINK,GTK_SHRINK,0,0);

    label = gtk_label_new (" <"); 
#ifdef USE_GTK3
    gtk_widget_set_halign(label,GTK_ALIGN_CENTER);
    gtk_widget_set_valign(label,GTK_ALIGN_CENTER);
#endif
    gtkut_table_attach(table1, label, 4, 5, i, i+1,
		       GTK_SHRINK,GTK_SHRINK,0,0);

    adj = (GtkAdjustment *)gtk_adjustment_new(tmp_sdss_magmax[i],
					      0, 30, 1, 1, 0);
    spinner =  gtk_spin_button_new (adj, 0, 0);
    gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), FALSE);
    gtkut_table_attach(table1, spinner, 5, 6, i, i+1,
		       GTK_FILL,GTK_SHRINK,0,0);
    my_entry_set_width_chars(GTK_ENTRY(&GTK_SPIN_BUTTON(spinner)->entry),2);
    my_signal_connect (adj, "value_changed", cc_get_adj, &tmp_sdss_magmax[i]);
  }


  vbox = gtkut_vbox_new (FALSE, 0);
  label = gtk_label_new ("LAMOST");
  gtk_notebook_append_page (GTK_NOTEBOOK (hg->query_note), vbox, label);

  table = gtkut_table_new(3, 6, FALSE, 5, 10, 5);
  gtk_container_add (GTK_CONTAINER (vbox), table);

  label = gtk_label_new ("Search Diameter = Finding Chart Diameter");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtkut_table_attach(table, label, 0, 3, 0, 1,
		     GTK_FILL,GTK_SHRINK,0,0);

  {
    GtkListStore *store;
    GtkTreeIter iter, iter_set;	  
    GtkCellRenderer *renderer;
      
    store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
    
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "DR5",
		       1, FCDB_LAMOST_DR5, -1);
    if(hg->fcdb_lamost_dr==FCDB_LAMOST_DR5) iter_set=iter;

    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "DR6 (Low Resolution)",
		       1, FCDB_LAMOST_DR6, -1);
    if(hg->fcdb_lamost_dr==FCDB_LAMOST_DR6) iter_set=iter;

    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "DR6 (Medium Resolution)",
		       1, FCDB_LAMOST_DR6M, -1);
    if(hg->fcdb_lamost_dr==FCDB_LAMOST_DR6M) iter_set=iter;
    
    combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));
#ifdef USE_GTK3
    gtk_widget_set_halign(combo,GTK_ALIGN_CENTER);
    gtk_widget_set_valign(combo,GTK_ALIGN_CENTER);
#endif
    gtkut_table_attach(table, combo, 0, 1, 1, 2,
		       GTK_SHRINK,GTK_SHRINK,0,0);
    g_object_unref(store);
    
    renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo),renderer, TRUE);
    gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT(combo), renderer, "text",0,NULL);
    	
    gtk_combo_box_set_active_iter(GTK_COMBO_BOX(combo),&iter_set);
    gtk_widget_show(combo);
    my_signal_connect (combo,"changed",cc_get_combo_box,
		       &tmp_lamost_dr);
  }
  


  // USNO-B
  vbox = gtkut_vbox_new (FALSE, 0);
  label = gtk_label_new ("USNO-B");
  gtk_notebook_append_page (GTK_NOTEBOOK (hg->query_note), vbox, label);

  table = gtkut_table_new(2, 2, FALSE, 5, 10, 5);
  gtk_container_add (GTK_CONTAINER (vbox), table);

  label = gtk_label_new ("Search Area");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtkut_table_attach(table, label, 0, 1, 0, 1,
		     GTK_FILL,GTK_SHRINK,0,0);

  label = gtk_label_new ("= Finding Chart Area");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtkut_table_attach(table, label, 1, 3, 0, 1,
		     GTK_FILL,GTK_SHRINK,0,0);
  
  check = gtk_check_button_new_with_label("Mag. filter");
  gtkut_table_attach(table, check, 0, 1, 1, 2,
		     GTK_FILL,GTK_SHRINK,0,0);
  my_signal_connect (check, "toggled",
		     cc_get_toggle,
		     &tmp_usno_fil);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check),
			       hg->fcdb_usno_fil);

  hbox = gtkut_hbox_new(FALSE,0);
#ifdef USE_GTK3
  gtk_widget_set_halign(hbox,GTK_ALIGN_CENTER);
  gtk_widget_set_valign(hbox,GTK_ALIGN_CENTER);
#endif
  gtkut_table_attach(table, hbox, 1, 2, 1, 2,
		     GTK_SHRINK,GTK_SHRINK,0,0);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 0);

  label = gtk_label_new ("R2 < ");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_END);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
#endif
  gtk_box_pack_start(GTK_BOX(hbox), label,FALSE, FALSE, 0);

  adj = (GtkAdjustment *)gtk_adjustment_new(tmp_usno_mag,
					    12, 22, 1, 1, 0);
  spinner =  gtk_spin_button_new (adj, 0, 0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), FALSE);
  gtk_box_pack_start(GTK_BOX(hbox), spinner,FALSE, FALSE, 0);
  my_entry_set_width_chars(GTK_ENTRY(&GTK_SPIN_BUTTON(spinner)->entry),2);
  my_signal_connect (adj, "value_changed", cc_get_adj, &tmp_usno_mag);


  // UCAC4
  vbox = gtkut_vbox_new (FALSE, 0);
  label = gtk_label_new ("UCAC4");
  gtk_notebook_append_page (GTK_NOTEBOOK (hg->query_note), vbox, label);

  table = gtkut_table_new(2, 2, FALSE, 5, 10, 5);
  gtk_container_add (GTK_CONTAINER (vbox), table);

  label = gtk_label_new ("Search Area");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtkut_table_attach(table, label, 0, 1, 0, 1,
		     GTK_FILL,GTK_SHRINK,0,0);

  label = gtk_label_new ("= Finding Chart Area");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtkut_table_attach(table, label, 1, 3, 0, 1,
		     GTK_FILL,GTK_SHRINK,0,0);

  check = gtk_check_button_new_with_label("Mag. filter");
  gtkut_table_attach(table, check, 0, 1, 1, 2,
		     GTK_FILL,GTK_SHRINK,0,0);
  my_signal_connect (check, "toggled",
		     cc_get_toggle,
		     &tmp_ucac_fil);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check),
			       hg->fcdb_ucac_fil);

  hbox = gtkut_hbox_new(FALSE,0);
#ifdef USE_GTK3
  gtk_widget_set_halign(hbox,GTK_ALIGN_CENTER);
  gtk_widget_set_valign(hbox,GTK_ALIGN_CENTER);
#endif
  gtkut_table_attach(table, hbox, 1, 2, 1, 2,
		     GTK_SHRINK,GTK_SHRINK,0,0);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 0);

  label = gtk_label_new ("r < ");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_END);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
#endif
  gtk_box_pack_start(GTK_BOX(hbox), label,FALSE, FALSE, 0);

  adj = (GtkAdjustment *)gtk_adjustment_new(tmp_ucac_mag,
					    12, 22, 1, 1, 0);
  spinner =  gtk_spin_button_new (adj, 0, 0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), FALSE);
  gtk_box_pack_start(GTK_BOX(hbox), spinner,FALSE, FALSE, 0);
  my_entry_set_width_chars(GTK_ENTRY(&GTK_SPIN_BUTTON(spinner)->entry),2);
  my_signal_connect (adj, "value_changed", cc_get_adj, &tmp_ucac_mag);
  

  // GAIA
  vbox = gtkut_vbox_new (FALSE, 0);
  label = gtk_label_new ("GAIA");
  gtk_notebook_append_page (GTK_NOTEBOOK (hg->query_note), vbox, label);

  table = gtkut_table_new(2, 2, FALSE, 5, 10, 5);
  gtk_container_add (GTK_CONTAINER (vbox), table);

  label = gtk_label_new ("Search Area");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtkut_table_attach(table, label, 0, 1, 0, 1,
		     GTK_FILL,GTK_SHRINK,0,0);

  label = gtk_label_new ("= Finding Chart Area");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtkut_table_attach(table, label, 1, 3, 0, 1,
		     GTK_FILL,GTK_SHRINK,0,0);

  label = gtk_label_new ("Data Release");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtkut_table_attach(table, label, 0, 1, 2, 3,
		     GTK_FILL,GTK_SHRINK,0,0);

  switch(hg->gaia_dr){
  case GAIA_DR2:
    label = gtkut_label_new ("= <b>DR2</b>  (<i>switch in General TAB</i>)");
    break;
  case GAIA_EDR3:
    label = gtkut_label_new ("= <b>EDR3</b>  (<i>switch in General TAB</i>)");
    break;
  }
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtkut_table_attach(table, label, 1, 3, 2, 3,
		     GTK_FILL,GTK_SHRINK,0,0);

  check = gtk_check_button_new_with_label("Mag. filter");
  gtkut_table_attach(table, check, 0, 1, 1, 2,
		     GTK_FILL,GTK_SHRINK,0,0);
  my_signal_connect (check, "toggled",
		     cc_get_toggle,
		     &tmp_gaia_fil);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check),
			       hg->fcdb_gaia_fil);

  hbox = gtkut_hbox_new(FALSE,0);
#ifdef USE_GTK3
  gtk_widget_set_halign(hbox,GTK_ALIGN_CENTER);
  gtk_widget_set_valign(hbox,GTK_ALIGN_CENTER);
#endif
  gtkut_table_attach(table, hbox, 1, 2, 1, 2,
		     GTK_SHRINK,GTK_SHRINK,0,0);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 0);

  label = gtkut_label_new ("G (0.33 - 1.0 &#xB5;m) &lt; ");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_END);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
#endif
  gtk_box_pack_start(GTK_BOX(hbox), label,FALSE, FALSE, 0);

  adj = (GtkAdjustment *)gtk_adjustment_new(tmp_gaia_mag,
					    8, 22, 1, 1, 0);
  spinner =  gtk_spin_button_new (adj, 0, 0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), FALSE);
  gtk_box_pack_start(GTK_BOX(hbox), spinner,FALSE, FALSE, 0);
  my_entry_set_width_chars(GTK_ENTRY(&GTK_SPIN_BUTTON(spinner)->entry),2);
  my_signal_connect (adj, "value_changed", cc_get_adj, &tmp_gaia_mag);

  vbox = gtkut_vbox_new (FALSE, 0);
  label = gtk_label_new ("Kepler IC10");
  gtk_notebook_append_page (GTK_NOTEBOOK (hg->query_note), vbox, label);

  table = gtkut_table_new(3, 6, FALSE, 5, 10, 5);
  gtk_container_add (GTK_CONTAINER (vbox), table);

  label = gtk_label_new ("Search Diameter");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtkut_table_attach(table, label, 0, 1, 0, 1,
		     GTK_FILL,GTK_SHRINK,0,0);

  label = gtk_label_new ("= Finding Chart Area");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtkut_table_attach(table, label, 1, 3, 0, 1,
		     GTK_FILL,GTK_SHRINK,0,0);

  check = gtk_check_button_new_with_label("Mag. filter");
  gtkut_table_attach(table, check, 0, 1, 1, 2,
		     GTK_FILL,GTK_SHRINK,0,0);
  my_signal_connect (check, "toggled",
		     cc_get_toggle,
		     &tmp_kepler_fil);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check),
			       hg->fcdb_kepler_fil);

  hbox = gtkut_hbox_new(FALSE,0);
#ifdef USE_GTK3
  gtk_widget_set_halign(hbox,GTK_ALIGN_CENTER);
  gtk_widget_set_valign(hbox,GTK_ALIGN_CENTER);
#endif
  gtkut_table_attach(table, hbox, 1, 2, 1, 2,
		     GTK_SHRINK,GTK_SHRINK,0,0);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 0);

  label = gtkut_label_new ("Kep (0.42 - 0.90 &#xB5;m) &lt; ");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_END);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
#endif
  gtk_box_pack_start(GTK_BOX(hbox), label,FALSE, FALSE, 0);

  adj = (GtkAdjustment *)gtk_adjustment_new(tmp_kepler_mag,
					    8, 22, 1, 1, 0);
  spinner =  gtk_spin_button_new (adj, 0, 0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), FALSE);
  gtk_box_pack_start(GTK_BOX(hbox), spinner,FALSE, FALSE, 0);
  my_entry_set_width_chars(GTK_ENTRY(&GTK_SPIN_BUTTON(spinner)->entry),2);
  my_signal_connect (adj, "value_changed", cc_get_adj, &tmp_kepler_mag);



  vbox = gtkut_vbox_new (FALSE, 0);
  label = gtk_label_new ("2MASS");
  gtk_notebook_append_page (GTK_NOTEBOOK (hg->query_note), vbox, label);

  table = gtkut_table_new(2, 2, FALSE, 5, 10, 5);
  gtk_container_add (GTK_CONTAINER (vbox), table);

  label = gtk_label_new ("Search Diameter ");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_END);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
#endif
  gtkut_table_attach(table, label, 0, 1, 0, 1,
		     GTK_FILL,GTK_SHRINK,0,0);

  hbox = gtkut_hbox_new(FALSE,0);
#ifdef USE_GTK3
  gtk_widget_set_halign(hbox,GTK_ALIGN_CENTER);
  gtk_widget_set_valign(hbox,GTK_ALIGN_CENTER);
#endif
  gtkut_table_attach(table, hbox, 1, 2, 0, 1,
		     GTK_SHRINK,GTK_SHRINK,0,0);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 0);

  label = gtk_label_new ("< "); 
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtk_box_pack_start(GTK_BOX(hbox), label,FALSE, FALSE, 0);

  adj = (GtkAdjustment *)gtk_adjustment_new(tmp_2mass_diam,
					    1, FCDB_ARCMIN_MAX, 1, 1, 0);
  spinner =  gtk_spin_button_new (adj, 0, 0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), FALSE);
  gtk_box_pack_start(GTK_BOX(hbox), spinner,FALSE, FALSE, 0);
  my_entry_set_width_chars(GTK_ENTRY(&GTK_SPIN_BUTTON(spinner)->entry),4);
  my_signal_connect (adj, "value_changed", cc_get_adj, &tmp_2mass_diam);

  label = gtk_label_new ("[arcmin]"); 
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtk_box_pack_start(GTK_BOX(hbox), label,FALSE, FALSE, 0);


  check = gtk_check_button_new_with_label("Mag. filter");
  gtkut_table_attach(table, check, 0, 1, 1, 2,
		     GTK_FILL,GTK_SHRINK,0,0);
  my_signal_connect (check, "toggled",
		     cc_get_toggle,
		     &tmp_2mass_fil);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check),
			       hg->fcdb_2mass_fil);

  hbox = gtkut_hbox_new(FALSE,0);
#ifdef USE_GTK3
  gtk_widget_set_halign(hbox,GTK_ALIGN_CENTER);
  gtk_widget_set_valign(hbox,GTK_ALIGN_CENTER);
#endif
  gtkut_table_attach(table, hbox, 1, 2, 1, 2,
		     GTK_SHRINK,GTK_SHRINK,0,0);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 0);

  label = gtk_label_new ("H < ");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_END);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
#endif
  gtk_box_pack_start(GTK_BOX(hbox), label,FALSE, FALSE, 0);

  adj = (GtkAdjustment *)gtk_adjustment_new(tmp_2mass_mag,
					    8, 16, 1, 1, 0);
  spinner =  gtk_spin_button_new (adj, 0, 0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), FALSE);
  gtk_box_pack_start(GTK_BOX(hbox), spinner,FALSE, FALSE, 0);
  my_entry_set_width_chars(GTK_ENTRY(&GTK_SPIN_BUTTON(spinner)->entry),2);
  my_signal_connect (adj, "value_changed", cc_get_adj, &tmp_2mass_mag);


  vbox = gtkut_vbox_new (FALSE, 0);
  label = gtk_label_new ("WISE");
  gtk_notebook_append_page (GTK_NOTEBOOK (hg->query_note), vbox, label);

  table = gtkut_table_new(2, 6, FALSE, 5, 10, 5);
  gtk_container_add (GTK_CONTAINER (vbox), table);

  label = gtk_label_new ("Search Area");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtkut_table_attach(table, label, 0, 1, 0, 1,
		     GTK_FILL,GTK_SHRINK,0,0);

  label = gtk_label_new ("= Finding Chart Area");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtkut_table_attach(table, label, 1, 3, 0, 1,
		     GTK_FILL,GTK_SHRINK,0,0);


  check = gtk_check_button_new_with_label("Mag. filter");
  gtkut_table_attach(table, check, 0, 1, 1, 2,
		     GTK_FILL,GTK_SHRINK,0,0);
  my_signal_connect (check, "toggled",
		     cc_get_toggle,
		     &tmp_wise_fil);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check),
			       hg->fcdb_wise_fil);

  hbox = gtkut_hbox_new(FALSE,0);
#ifdef USE_GTK3
  gtk_widget_set_halign(hbox,GTK_ALIGN_CENTER);
  gtk_widget_set_valign(hbox,GTK_ALIGN_CENTER);
#endif
  gtkut_table_attach(table, hbox, 1, 3, 1, 2,
		     GTK_SHRINK,GTK_SHRINK,0,0);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 0);

  label = gtk_label_new ("W1 < ");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_END);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
#endif
  gtk_box_pack_start(GTK_BOX(hbox), label,FALSE, FALSE, 0);

  adj = (GtkAdjustment *)gtk_adjustment_new(tmp_wise_mag,
					    8, 18, 1, 1, 0);
  spinner =  gtk_spin_button_new (adj, 0, 0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), FALSE);
  gtk_box_pack_start(GTK_BOX(hbox), spinner,FALSE, FALSE, 0);
  my_entry_set_width_chars(GTK_ENTRY(&GTK_SPIN_BUTTON(spinner)->entry),2);
  my_signal_connect (adj, "value_changed", cc_get_adj, &tmp_wise_mag);

  label = gtk_label_new ("W1 [mag] : ");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtkut_table_attach(table, label, 0, 1, 2, 3,
		     GTK_FILL,GTK_SHRINK,0,0);
  label = gtkut_label_new ("3.35 &#xB5;m");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
#endif
  gtkut_table_attach(table, label, 1, 2, 2, 3,
		     GTK_FILL,GTK_SHRINK,0,0);
  label = gtkut_label_new ("2.75 - 3.87 &#xB5;m");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
#endif
  gtkut_table_attach(table, label, 2, 3, 2, 3,
		     GTK_FILL,GTK_SHRINK,0,0);

  label = gtk_label_new ("W2 [mag] : ");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
#endif
  gtkut_table_attach(table, label, 0, 1, 3, 4,
		     GTK_FILL,GTK_SHRINK,0,0);
  label = gtkut_label_new ("4.6 &#xB5;m");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
#endif
  gtkut_table_attach(table, label, 1, 2, 3, 4,
		     GTK_FILL,GTK_SHRINK,0,0);
  label = gtkut_label_new ("3.96 - 5.34 &#xB5;m");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
#endif
  gtkut_table_attach(table, label, 2, 3, 3, 4,
		     GTK_FILL,GTK_SHRINK,0,0);

  label = gtk_label_new ("W3 [mag] : ");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtkut_table_attach(table, label, 0, 1, 4, 5,
		     GTK_FILL,GTK_SHRINK,0,0);
  label = gtkut_label_new ("11.6 &#xB5;m");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
#endif
  gtkut_table_attach(table, label, 1, 2, 4, 5,
		     GTK_FILL,GTK_SHRINK,0,0);
  label = gtkut_label_new ("7.44 - 17.3 &#xB5;m");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
#endif
  gtkut_table_attach(table, label, 2, 3, 4, 5,
		     GTK_FILL,GTK_SHRINK,0,0);

  label = gtk_label_new ("W4 [mag] : ");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtkut_table_attach(table, label, 0, 1, 5, 6,
		     GTK_FILL,GTK_SHRINK,0,0);
  label = gtkut_label_new ("22.1 &#xB5;m");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
#endif
  gtkut_table_attach(table, label, 1, 2, 5, 6,
		     GTK_FILL,GTK_SHRINK,0,0);
  label = gtkut_label_new ("19.5 - 27.9 &#xB5;m");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
#endif
  gtkut_table_attach(table, label, 2, 3, 5, 6,
		     GTK_FILL,GTK_SHRINK,0,0);


  vbox = gtkut_vbox_new (FALSE, 0);
  label = gtk_label_new ("AKARI/IRC");
  gtk_notebook_append_page (GTK_NOTEBOOK (hg->query_note), vbox, label);

  table = gtkut_table_new(2, 4, FALSE, 5, 10, 5);
  gtk_container_add (GTK_CONTAINER (vbox), table);

  label = gtk_label_new ("Search Area");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtkut_table_attach(table, label, 0, 1, 0, 1,
		     GTK_FILL,GTK_SHRINK,0,0);

  label = gtk_label_new ("= Finding Chart Area");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtkut_table_attach(table, label, 1, 3, 0, 1,
		     GTK_FILL,GTK_SHRINK,0,0);

  label = gtk_label_new ("S9W [Jy] : ");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtkut_table_attach(table, label, 0, 1, 2, 3,
		     GTK_FILL,GTK_SHRINK,0,0);
  label = gtkut_label_new ("6.7 - 11.6 &#xB5;m");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
#endif
  gtkut_table_attach(table, label, 1, 2, 2, 3,
		     GTK_FILL,GTK_SHRINK,0,0);
  label = gtk_label_new ("9.4\"x9.4\"/pix");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
#endif
  gtkut_table_attach(table, label, 2, 3, 2, 3,
		     GTK_FILL,GTK_SHRINK,0,0);

  label = gtk_label_new ("L18W [Jy] : ");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtkut_table_attach(table, label, 0, 1, 3, 4,
		     GTK_FILL,GTK_SHRINK,0,0);
  label = gtkut_label_new ("13.9 - 25.6 &#xB5;m");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
#endif
  gtkut_table_attach(table, label, 1, 2, 3, 4,
		     GTK_FILL,GTK_SHRINK,0,0);
  label = gtk_label_new ("10.4\"x9.4\"/pix");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
#endif
  gtkut_table_attach(table, label, 2, 3, 3, 4,
		     GTK_FILL,GTK_SHRINK,0,0);

  vbox = gtkut_vbox_new (FALSE, 0);
  label = gtk_label_new ("AKARI/FIS");
  gtk_notebook_append_page (GTK_NOTEBOOK (hg->query_note), vbox, label);

  table = gtkut_table_new(3, 6, FALSE, 5, 10, 5);
  gtk_container_add (GTK_CONTAINER (vbox), table);

  label = gtk_label_new ("Search Area");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtkut_table_attach(table, label, 0, 1, 0, 1,
		     GTK_FILL,GTK_SHRINK,0,0);

  label = gtk_label_new ("= Finding Chart Area");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtkut_table_attach(table, label, 1, 3, 0, 1,
		     GTK_FILL,GTK_SHRINK,0,0);

  label = gtk_label_new ("N60 [Jy] : ");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtkut_table_attach(table, label, 0, 1, 2, 3,
		     GTK_FILL,GTK_SHRINK,0,0);
  label = gtkut_label_new ("50 - 80 &#xB5;m");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
#endif
  gtkut_table_attach(table, label, 1, 2, 2, 3,
		     GTK_FILL,GTK_SHRINK,0,0);
  label = gtk_label_new ("26.8\"/pix");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
#endif
  gtkut_table_attach(table, label, 2, 3, 2, 3,
		     GTK_FILL,GTK_SHRINK,0,0);

  label = gtk_label_new ("WIDE-S [Jy] : ");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtkut_table_attach(table, label, 0, 1, 3, 4,
		     GTK_FILL,GTK_SHRINK,0,0);
  label = gtkut_label_new ("60 - 110 &#xB5;m");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
#endif
  gtkut_table_attach(table, label, 1, 2, 3, 4,
		     GTK_FILL,GTK_SHRINK,0,0);
  label = gtk_label_new ("26.8\"/pix");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
#endif
  gtkut_table_attach(table, label, 2, 3, 3, 4,
		     GTK_FILL,GTK_SHRINK,0,0);

  label = gtk_label_new ("WIDE-L [Jy] : ");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtkut_table_attach(table, label, 0, 1, 4, 5,
		     GTK_FILL,GTK_SHRINK,0,0);
  label = gtkut_label_new ("110 - 180 &#xB5;m");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
#endif
  gtkut_table_attach(table, label, 1, 2, 4, 5,
		     GTK_FILL,GTK_SHRINK,0,0);
  label = gtk_label_new ("44.2\"/pix");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
#endif
  gtkut_table_attach(table, label, 2, 3, 4, 5,
		     GTK_FILL,GTK_SHRINK,0,0);

  label = gtk_label_new ("N160 [Jy] : ");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtkut_table_attach(table, label, 0, 1, 5, 6,
		     GTK_FILL,GTK_SHRINK,0,0);
  label = gtkut_label_new ("140 - 180 &#xB5;m");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
#endif
  gtkut_table_attach(table, label, 1, 2, 5, 6,
		     GTK_FILL,GTK_SHRINK,0,0);
  label = gtk_label_new ("44.2\"/pix");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
#endif
  gtkut_table_attach(table, label, 2, 3, 5, 6,
		     GTK_FILL,GTK_SHRINK,0,0);

#ifdef USE_GTK3
  button=gtkut_button_new_from_icon_name("Load Default","view-refresh");
#else
  button=gtkut_button_new_from_stock("Load Default",GTK_STOCK_REFRESH);
#endif
  gtk_dialog_add_action_widget(GTK_DIALOG(dialog),button,GTK_RESPONSE_APPLY);
  my_signal_connect(button,"pressed",
		    default_disp_para, 
		    (gpointer)cdata);

  vbox = gtkut_vbox_new (FALSE, 0);
  label = gtk_label_new ("SMOKA");
  gtk_notebook_append_page (GTK_NOTEBOOK (hg->query_note), vbox, label);

  table = gtkut_table_new(4, 5, FALSE, 5, 10, 5);
  gtk_container_add (GTK_CONTAINER (vbox), table);

  label = gtk_label_new ("Search Area = Finding Chart Area");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtkut_table_attach(table, label, 0, 4, 0, 1,
		     GTK_FILL,GTK_SHRINK,0,0);

  check = gtk_check_button_new_with_label("Shot (Suprime-Cam, Hyper Suprime-Cam, and KWFC ONLY)");
  gtkut_table_attach(table, check, 0, 4, 1, 2,
		     GTK_FILL,GTK_SHRINK,0,0);
  my_signal_connect (check, "toggled",
		     cc_get_toggle,
		     &tmp_smoka_shot);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check),
			       hg->fcdb_smoka_shot);

  vbox1 = gtkut_vbox_new(FALSE,0);
  gtkut_table_attach(table, vbox1, 0, 1, 2, 5,
		     GTK_FILL,GTK_SHRINK,0,0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox1), 0);

  frame = gtkut_frame_new ("<b>Subaru</b>");
  gtk_container_add (GTK_CONTAINER (vbox1), frame);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 3);

  vbox = gtkut_vbox_new(FALSE,0);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 0);

  for(i=0;i<NUM_SMOKA_SUBARU;i++){
    check = gtk_check_button_new_with_label(smoka_subaru[i].name);
    gtk_box_pack_start(GTK_BOX(vbox), check,FALSE, FALSE, 0);
    my_signal_connect (check, "toggled",
		       cc_get_toggle,
		       &tmp_smoka_subaru[i]);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check),
				 hg->fcdb_smoka_subaru[i]);
  }

  vbox1 = gtkut_vbox_new(FALSE,0);
#ifdef USE_GTK3
  gtk_widget_set_hexpand(vbox1,TRUE);
#endif
  gtkut_table_attach(table, vbox1, 1, 2, 2, 4,
		     GTK_FILL|GTK_EXPAND,GTK_FILL,0,0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox1), 0);

  frame = gtkut_frame_new ("<b>Kiso</b>");
  gtk_container_add (GTK_CONTAINER (vbox1), frame);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 3);

  vbox = gtkut_vbox_new(FALSE,0);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 0);

  for(i=0;i<NUM_SMOKA_KISO;i++){
    check = gtk_check_button_new_with_label(smoka_kiso[i].name);
    gtk_box_pack_start(GTK_BOX(vbox), check,FALSE, FALSE, 0);
    my_signal_connect (check, "toggled",
		       cc_get_toggle,
		       &tmp_smoka_kiso[i]);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check),
				 hg->fcdb_smoka_kiso[i]);
  }

  frame = gtkut_frame_new ("<b>Okayama</b>");
  gtk_container_add (GTK_CONTAINER (vbox1), frame);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 3);

  vbox = gtkut_vbox_new(FALSE,0);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 0);

  for(i=0;i<NUM_SMOKA_OAO;i++){
    check = gtk_check_button_new_with_label(smoka_oao[i].name);
    gtk_box_pack_start(GTK_BOX(vbox), check,FALSE, FALSE, 0);
    my_signal_connect (check, "toggled",
		       cc_get_toggle,
		       &tmp_smoka_oao[i]);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check),
				 hg->fcdb_smoka_oao[i]);
  }

  vbox1 = gtkut_vbox_new(FALSE,0);
#ifdef USE_GTK3
  gtk_widget_set_hexpand(vbox1,TRUE);
#endif
  gtkut_table_attach(table, vbox1, 2, 3, 2, 3,
		     GTK_FILL|GTK_EXPAND,GTK_FILL,0,0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox1), 0);

  frame = gtkut_frame_new ("<b>MITSuME</b>");
  gtk_container_add (GTK_CONTAINER (vbox1), frame);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 3);

  vbox = gtkut_vbox_new(FALSE,0);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 0);

  for(i=0;i<NUM_SMOKA_MTM;i++){
    check = gtk_check_button_new_with_label(smoka_mtm[i].name);
    gtk_box_pack_start(GTK_BOX(vbox), check,FALSE, FALSE, 0);
    my_signal_connect (check, "toggled",
		       cc_get_toggle,
		       &tmp_smoka_mtm[i]);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check),
				 hg->fcdb_smoka_mtm[i]);
  }

  frame = gtkut_frame_new ("<b>Hiroshima</b>");
  gtk_container_add (GTK_CONTAINER (vbox1), frame);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 3);

  vbox = gtkut_vbox_new(FALSE,0);  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 0);

  for(i=0;i<NUM_SMOKA_KANATA;i++){
    check = gtk_check_button_new_with_label(smoka_kanata[i].name);
    gtk_box_pack_start(GTK_BOX(vbox), check,FALSE, FALSE, 0);
    my_signal_connect (check, "toggled",
		       cc_get_toggle,
		       &tmp_smoka_kanata[i]);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check),
				 hg->fcdb_smoka_kanata[i]);
  }

  vbox1 = gtkut_vbox_new(FALSE,0);
#ifdef USE_GTK3
  gtk_widget_set_hexpand(vbox1,TRUE);
#endif
  gtkut_table_attach(table, vbox1, 3, 4, 2, 3,
		     GTK_FILL|GTK_EXPAND,GTK_FILL,0,0);

  vbox = gtkut_vbox_new (FALSE, 0);
  label = gtk_label_new ("HST");
  gtk_notebook_append_page (GTK_NOTEBOOK (hg->query_note), vbox, label);

  table = gtkut_table_new(4, 3, FALSE, 5, 10, 5);
  gtk_container_add (GTK_CONTAINER (vbox), table);

  label = gtk_label_new ("Search Area = Finding Chart Area");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtkut_table_attach(table, label, 0, 4, 0, 1,
		     GTK_FILL,GTK_SHRINK,0,0);


  vbox1 = gtkut_vbox_new(FALSE,0);
#ifdef USE_GTK3
  gtk_widget_set_hexpand(vbox1,TRUE);
#endif
  gtkut_table_attach(table, vbox1, 0, 1, 1, 3,
		     GTK_FILL|GTK_EXPAND,GTK_FILL,0,0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox1), 0);

  frame = gtkut_frame_new ("<b>Imaging</b>");
  gtk_container_add (GTK_CONTAINER (vbox1), frame);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 3);

  vbox = gtkut_vbox_new(FALSE,0);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 0);

  for(i=0;i<NUM_HST_IMAGE;i++){
    check = gtk_check_button_new_with_label(hst_image[i].name);
    gtk_box_pack_start(GTK_BOX(vbox), check,FALSE, FALSE, 0);
    my_signal_connect (check, "toggled",
		       cc_get_toggle,
		       &tmp_hst_image[i]);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check),
				 hg->fcdb_hst_image[i]);
  }

  vbox1 = gtkut_vbox_new(FALSE,0);
#ifdef USE_GTK3
  gtk_widget_set_hexpand(vbox1,TRUE);
#endif
  gtkut_table_attach(table, vbox1, 1, 2, 1, 3,
		     GTK_FILL|GTK_EXPAND,GTK_FILL,0,0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox1), 0);

  frame = gtkut_frame_new ("<b>Spectroscopy</b>");
  gtk_container_add (GTK_CONTAINER (vbox1), frame);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 3);

  vbox = gtkut_vbox_new(FALSE,0);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 0);

  for(i=0;i<NUM_HST_SPEC;i++){
    check = gtk_check_button_new_with_label(hst_spec[i].name);
    gtk_box_pack_start(GTK_BOX(vbox), check,FALSE, FALSE, 0);
    my_signal_connect (check, "toggled",
		       cc_get_toggle,
		       &tmp_hst_spec[i]);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check),
				 hg->fcdb_hst_spec[i]);
  }

  vbox1 = gtkut_vbox_new(FALSE,0);
#ifdef USE_GTK3
  gtk_widget_set_hexpand(vbox1,TRUE);
#endif
  gtkut_table_attach(table, vbox1, 2, 3, 1, 2,
		     GTK_FILL|GTK_EXPAND,GTK_FILL,0,0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox1), 0);

  frame = gtkut_frame_new ("<b>Other</b>");
  gtk_container_add (GTK_CONTAINER (vbox1), frame);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 3);

  vbox = gtkut_vbox_new(FALSE,0);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 0);

  for(i=0;i<NUM_HST_OTHER;i++){
    check = gtk_check_button_new_with_label(hst_other[i].name);
    gtk_box_pack_start(GTK_BOX(vbox), check,FALSE, FALSE, 0);
    my_signal_connect (check, "toggled",
		       cc_get_toggle,
		       &tmp_hst_other[i]);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check),
				 hg->fcdb_hst_other[i]);
  }

  vbox1 = gtkut_vbox_new(FALSE,0);
#ifdef USE_GTK3
  gtk_widget_set_hexpand(vbox1,TRUE);
#endif
  gtkut_table_attach(table, vbox1, 3, 4, 1, 2,
		     GTK_FILL|GTK_EXPAND,GTK_FILL,0,0);

  vbox = gtkut_vbox_new (FALSE, 0);
  label = gtk_label_new ("ESO");
  gtk_notebook_append_page (GTK_NOTEBOOK (hg->query_note), vbox, label);

  table = gtkut_table_new(4, 6, FALSE, 5, 5, 5);
  gtk_container_add (GTK_CONTAINER (vbox), table);

  label = gtk_label_new ("Search Area = Finding Chart Area");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtkut_table_attach(table, label, 0, 4, 0, 1,
		     GTK_FILL,GTK_SHRINK,0,0);

  vbox1 = gtkut_vbox_new(FALSE,0);
#ifdef USE_GTK3
  gtk_widget_set_hexpand(vbox1,TRUE);
#endif
  gtkut_table_attach(table, vbox1, 0, 1, 1, 3,
		     GTK_FILL|GTK_EXPAND,GTK_FILL,0,0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox1), 0);

  frame = gtkut_frame_new ("<b>Imaging</b>");
  gtk_container_add (GTK_CONTAINER (vbox1), frame);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 3);

  vbox = gtkut_vbox_new(FALSE,0);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 0);

  for(i=0;i<NUM_ESO_IMAGE;i++){
    check = gtk_check_button_new_with_label(eso_image[i].name);
    gtk_box_pack_start(GTK_BOX(vbox), check,FALSE, FALSE, 0);
    my_signal_connect (check, "toggled",
		       cc_get_toggle,
		       &tmp_eso_image[i]);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check),
				 hg->fcdb_eso_image[i]);
  }

  vbox1 = gtkut_vbox_new(FALSE,0);
#ifdef USE_GTK3
  gtk_widget_set_hexpand(vbox1,TRUE);
#endif
  gtkut_table_attach(table, vbox1, 1, 2, 1, 5,
		     GTK_FILL|GTK_EXPAND,GTK_FILL,0,0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox1), 0);

  frame = gtkut_frame_new ("<b>Spectroscopy</b>");
  gtk_container_add (GTK_CONTAINER (vbox1), frame);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 3);

  vbox = gtkut_vbox_new(FALSE,0);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 0);

  for(i=0;i<NUM_ESO_SPEC;i++){
    check = gtk_check_button_new_with_label(eso_spec[i].name);
    gtk_box_pack_start(GTK_BOX(vbox), check,FALSE, FALSE, 0);
    my_signal_connect (check, "toggled",
		       cc_get_toggle,
		       &tmp_eso_spec[i]);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check),
				 hg->fcdb_eso_spec[i]);
  }

  vbox1 = gtkut_vbox_new(FALSE,0);
#ifdef USE_GTK3
  gtk_widget_set_hexpand(vbox1,TRUE);
#endif
  gtkut_table_attach(table, vbox1, 2, 3, 1, 4,
		     GTK_FILL|GTK_EXPAND,GTK_FILL,0,0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox1), 0);

  frame = gtkut_frame_new ("<b>Interferometry</b>");
  gtk_container_add (GTK_CONTAINER (vbox1), frame);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 3);

  vbox = gtkut_vbox_new(FALSE,0);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 0);

  for(i=0;i<NUM_ESO_VLTI;i++){
    check = gtk_check_button_new_with_label(eso_vlti[i].name);
    gtk_box_pack_start(GTK_BOX(vbox), check,FALSE, FALSE, 0);
    my_signal_connect (check, "toggled",
		       cc_get_toggle,
		       &tmp_eso_vlti[i]);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check),
				 hg->fcdb_eso_vlti[i]);
  }

  frame = gtkut_frame_new ("<b>Polarimetry</b>");
  gtk_container_add (GTK_CONTAINER (vbox1), frame);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 3);

  vbox = gtkut_vbox_new(FALSE,0);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 0);

  for(i=0;i<NUM_ESO_POLA;i++){
    check = gtk_check_button_new_with_label(eso_pola[i].name);
    gtk_box_pack_start(GTK_BOX(vbox), check,FALSE, FALSE, 0);
    my_signal_connect (check, "toggled",
		       cc_get_toggle,
		       &tmp_eso_pola[i]);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check),
				 hg->fcdb_eso_pola[i]);
  }

  frame = gtkut_frame_new ("<b>Coronagraphy</b>");
  gtk_container_add (GTK_CONTAINER (vbox1), frame);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 3);

  vbox = gtkut_vbox_new(FALSE,0);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 0);

  for(i=0;i<NUM_ESO_CORO;i++){
    check = gtk_check_button_new_with_label(eso_coro[i].name);
    gtk_box_pack_start(GTK_BOX(vbox), check,FALSE, FALSE, 0);
    my_signal_connect (check, "toggled",
		       cc_get_toggle,
		       &tmp_eso_coro[i]);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check),
				 hg->fcdb_eso_coro[i]);
  }

  vbox1 = gtkut_vbox_new(FALSE,0);
#ifdef USE_GTK3
  gtk_widget_set_hexpand(vbox1,TRUE);
#endif
  gtkut_table_attach(table, vbox1, 3, 4, 1, 2,
		     GTK_FILL|GTK_EXPAND,GTK_FILL,0,0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox1), 0);

  frame = gtkut_frame_new ("<b>Other</b>");
  gtk_container_add (GTK_CONTAINER (vbox1), frame);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 3);

  vbox = gtkut_vbox_new(FALSE,0);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 0);

  for(i=0;i<NUM_ESO_OTHER;i++){
    check = gtk_check_button_new_with_label(eso_other[i].name);
    gtk_box_pack_start(GTK_BOX(vbox), check,FALSE, FALSE, 0);
    my_signal_connect (check, "toggled",
		       cc_get_toggle,
		       &tmp_eso_other[i]);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check),
				 hg->fcdb_eso_other[i]);
  }

  frame = gtkut_frame_new ("<b>SAM</b>");
  gtk_container_add (GTK_CONTAINER (vbox1), frame);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 3);

  vbox = gtkut_vbox_new(FALSE,0);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 0);

  for(i=0;i<NUM_ESO_SAM;i++){
    check = gtk_check_button_new_with_label(eso_sam[i].name);
    gtk_box_pack_start(GTK_BOX(vbox), check,FALSE, FALSE, 0);
    my_signal_connect (check, "toggled",
		       cc_get_toggle,
		       &tmp_eso_sam[i]);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check),
				 hg->fcdb_eso_sam[i]);
  }

  
  // Gemini
  vbox = gtkut_vbox_new (FALSE, 0);
  label = gtk_label_new ("Gemini");
  gtk_notebook_append_page (GTK_NOTEBOOK (hg->query_note), vbox, label);

  table = gtkut_table_new(4, 2, FALSE, 5, 10, 5);
  gtk_container_add (GTK_CONTAINER (vbox), table);

  label = gtk_label_new ("Search Diameter");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtkut_table_attach(table, label, 0, 1, 0, 1,
		     GTK_FILL,GTK_SHRINK,0,0);

  label = gtk_label_new ("= Finding Chart Diameter");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtkut_table_attach(table, label, 1, 4, 0, 1,
		     GTK_FILL,GTK_SHRINK,0,0);

  label = gtk_label_new ("Instrument");
#ifdef USE_GTK3
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
#else
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
  gtkut_table_attach(table, label, 0, 1, 1, 2,
		     GTK_FILL,GTK_SHRINK,0,0);

  {
    GtkListStore *store;
    GtkTreeIter iter, iter_set;	  
    GtkCellRenderer *renderer;
    gint i_inst;
      
    store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);

    for(i_inst=0;i_inst<NUM_GEMINI_INST;i_inst++){
      gtk_list_store_append(store, &iter);
      gtk_list_store_set(store, &iter, 0, gemini_inst[i_inst].name,
			 1, i_inst, -1);
      if(hg->fcdb_gemini_inst==i_inst) iter_set=iter;
    }

    combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));
#ifdef USE_GTK3
    gtk_widget_set_halign(combo,GTK_ALIGN_CENTER);
    gtk_widget_set_valign(combo,GTK_ALIGN_CENTER);
#endif
    gtkut_table_attach(table, combo, 1, 3, 1, 2,
		       GTK_SHRINK,GTK_SHRINK,0,0);
    g_object_unref(store);
    
    renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo),renderer, TRUE);
    gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT(combo), renderer, "text",0,NULL);
    	
    gtk_combo_box_set_active_iter(GTK_COMBO_BOX(combo),&iter_set);
    gtk_widget_show(combo);
    my_signal_connect (combo,"changed",cc_get_combo_box,
		       &tmp_gemini_inst);
  }

  


#ifdef USE_GTK3
  button=gtkut_button_new_from_icon_name("Cancel","window-close");
#else
  button=gtkut_button_new_from_stock("Cancel",GTK_STOCK_CANCEL);
#endif
  gtk_dialog_add_action_widget(GTK_DIALOG(dialog),button,GTK_RESPONSE_CANCEL);
  my_signal_connect(button,"pressed",
		    close_disp_para, 
		    GTK_WIDGET(dialog));

#ifdef USE_GTK3
  button=gtkut_button_new_from_icon_name("Set Params","document-save");
#else
  button=gtkut_button_new_from_stock("Set Params",GTK_STOCK_OK);
#endif
  gtk_dialog_add_action_widget(GTK_DIALOG(dialog),button,GTK_RESPONSE_OK);
  my_signal_connect(button,"pressed",
		    change_fcdb_para, 
		    (gpointer)cdata);

  gtk_widget_show_all(dialog);

  if(hg->fcdb_type<=FCDB_TYPE_GEMINI)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rb[hg->fcdb_type]),TRUE);

  gtk_main();

  if(cdata->mode!=0){
    if(cdata->mode==1){
      hg->fcdb_band  = tmp_band;
      hg->fcdb_mag   = tmp_mag;
      hg->fcdb_otype = tmp_otype;
      hg->fcdb_ned_diam = tmp_ned_diam;
      hg->fcdb_ned_otype = tmp_ned_otype;
      if(hg->fcdb_type!=cdata->fcdb_type) rebuild_flag=TRUE;
      hg->fcdb_type  = cdata->fcdb_type;
      hg->fcdb_ned_ref  = tmp_ned_ref;
      hg->fcdb_gsc_fil  = tmp_gsc_fil;
      hg->fcdb_gsc_mag  = tmp_gsc_mag;
      hg->fcdb_gsc_diam  = tmp_gsc_diam;
      hg->fcdb_ps1_fil  = tmp_ps1_fil;
      hg->fcdb_ps1_mag  = tmp_ps1_mag;
      hg->fcdb_ps1_diam  = tmp_ps1_diam;
      hg->fcdb_ps1_mindet  = tmp_ps1_mindet;
      hg->fcdb_ps1_mode  = tmp_ps1_mode;
      hg->fcdb_ps1_dr  = tmp_ps1_dr;
      hg->fcdb_sdss_search = tmp_sdss_search;
      for(i=0;i<NUM_SDSS_BAND;i++){
	hg->fcdb_sdss_fil[i]  = tmp_sdss_fil[i];
	hg->fcdb_sdss_magmax[i]  = tmp_sdss_magmax[i];
	hg->fcdb_sdss_magmax[i]  = tmp_sdss_magmax[i];
      }
      hg->fcdb_sdss_diam  = tmp_sdss_diam;
      hg->fcdb_lamost_dr  = tmp_lamost_dr;
      hg->fcdb_usno_fil  = tmp_usno_fil;
      hg->fcdb_usno_mag  = tmp_usno_mag;
      hg->fcdb_ucac_fil  = tmp_ucac_fil;
      hg->fcdb_ucac_mag  = tmp_ucac_mag;
      hg->fcdb_gaia_fil  = tmp_gaia_fil;
      hg->fcdb_gaia_mag  = tmp_gaia_mag;
      hg->fcdb_kepler_fil  = tmp_kepler_fil;
      hg->fcdb_kepler_mag  = tmp_kepler_mag;
      hg->fcdb_2mass_fil  = tmp_2mass_fil;
      hg->fcdb_2mass_mag  = tmp_2mass_mag;
      hg->fcdb_2mass_diam  = tmp_2mass_diam;
      hg->fcdb_wise_fil  = tmp_wise_fil;
      hg->fcdb_wise_mag  = tmp_wise_mag;
      hg->fcdb_smoka_shot  = tmp_smoka_shot;
      for(i=0;i<NUM_SMOKA_SUBARU;i++){
	hg->fcdb_smoka_subaru[i]  = tmp_smoka_subaru[i];
      }
      for(i=0;i<NUM_SMOKA_KISO;i++){
	hg->fcdb_smoka_kiso[i]  = tmp_smoka_kiso[i];
      }
      for(i=0;i<NUM_SMOKA_OAO;i++){
	hg->fcdb_smoka_oao[i]  = tmp_smoka_oao[i];
      }
      for(i=0;i<NUM_SMOKA_MTM;i++){
	hg->fcdb_smoka_mtm[i]  = tmp_smoka_mtm[i];
      }
      for(i=0;i<NUM_SMOKA_KANATA;i++){
	hg->fcdb_smoka_kanata[i]  = tmp_smoka_kanata[i];
      }
      for(i=0;i<NUM_HST_IMAGE;i++){
	hg->fcdb_hst_image[i]  = tmp_hst_image[i];
      }
      for(i=0;i<NUM_HST_SPEC;i++){
	hg->fcdb_hst_spec[i]  = tmp_hst_spec[i];
      }
      for(i=0;i<NUM_HST_OTHER;i++){
	hg->fcdb_hst_other[i]  = tmp_hst_other[i];
      }
      for(i=0;i<NUM_ESO_IMAGE;i++){
	hg->fcdb_eso_image[i]  = tmp_eso_image[i];
      }
      for(i=0;i<NUM_ESO_SPEC;i++){
	hg->fcdb_eso_spec[i]  = tmp_eso_spec[i];
      }
      for(i=0;i<NUM_ESO_VLTI;i++){
	hg->fcdb_eso_vlti[i]  = tmp_eso_vlti[i];
      }
      for(i=0;i<NUM_ESO_POLA;i++){
	hg->fcdb_eso_pola[i]  = tmp_eso_pola[i];
      }
      for(i=0;i<NUM_ESO_CORO;i++){
	hg->fcdb_eso_coro[i]  = tmp_eso_coro[i];
      }
      for(i=0;i<NUM_ESO_OTHER;i++){
	hg->fcdb_eso_other[i]  = tmp_eso_other[i];
      }
      for(i=0;i<NUM_ESO_SAM;i++){
	hg->fcdb_eso_sam[i]  = tmp_eso_sam[i];
      }
      hg->fcdb_gemini_inst = tmp_gemini_inst;
    }
    else{
      hg->fcdb_band  = FCDB_BAND_NOP;
      hg->fcdb_mag   = 15;
      hg->fcdb_otype = FCDB_OTYPE_ALL;
      hg->fcdb_ned_diam = FCDB_ARCMIN_MAX;
      hg->fcdb_ned_otype = FCDB_NED_OTYPE_ALL;
      if(hg->fcdb_type!=FCDB_TYPE_SIMBAD) rebuild_flag=TRUE;
      hg->fcdb_type  = FCDB_TYPE_SIMBAD;
      hg->fcdb_ned_ref = FALSE;
      hg->fcdb_gsc_fil = TRUE;
      hg->fcdb_gsc_mag = 19;
      hg->fcdb_gsc_diam = FCDB_ARCMIN_MAX;
      hg->fcdb_ps1_fil = TRUE;
      hg->fcdb_ps1_mag = 19;
      hg->fcdb_ps1_diam = FCDB_PS1_MAX_DIAM;
      hg->fcdb_ps1_mindet = FCDB_PS1_MIN_NDET;
      hg->fcdb_ps1_mode = FCDB_PS1_MODE_MEAN;
      hg->fcdb_ps1_dr = FCDB_PS1_DR_2;
      hg->fcdb_sdss_search = FCDB_SDSS_SEARCH_IMAG;
      for(i=0;i<NUM_SDSS_BAND;i++){
	hg->fcdb_sdss_fil[i] = TRUE;
	hg->fcdb_sdss_magmin[i] = 0;
	hg->fcdb_sdss_magmax[i] = 20;
      }
      hg->fcdb_sdss_diam = FCDB_ARCMIN_MAX;
      hg->fcdb_lamost_dr = FCDB_LAMOST_DR5;
      hg->fcdb_usno_fil = TRUE;
      hg->fcdb_usno_mag = 19;
      hg->fcdb_gaia_fil = TRUE;
      hg->fcdb_gaia_mag = 19;
      hg->fcdb_2mass_fil = TRUE;
      hg->fcdb_2mass_mag = 12;
      hg->fcdb_2mass_diam = FCDB_ARCMIN_MAX;
      hg->fcdb_wise_fil = TRUE;
      hg->fcdb_wise_mag = 15;
      hg->fcdb_smoka_shot  = FALSE;
      for(i=0;i<NUM_SMOKA_SUBARU;i++){
	hg->fcdb_smoka_subaru[i]  = TRUE;
      }
      for(i=0;i<NUM_SMOKA_KISO;i++){
	hg->fcdb_smoka_kiso[i]  = FALSE;
      }
      for(i=0;i<NUM_SMOKA_OAO;i++){
	hg->fcdb_smoka_oao[i]  = FALSE;
      }
      for(i=0;i<NUM_SMOKA_MTM;i++){
	hg->fcdb_smoka_mtm[i]  = FALSE;
      }
      for(i=0;i<NUM_SMOKA_KANATA;i++){
	hg->fcdb_smoka_kanata[i]  = FALSE;
      }
      for(i=0;i<NUM_HST_IMAGE;i++){
	hg->fcdb_hst_image[i]  = TRUE;
      }
      for(i=0;i<NUM_HST_SPEC;i++){
	hg->fcdb_hst_spec[i]  = TRUE;
      }
      for(i=0;i<NUM_HST_OTHER;i++){
	hg->fcdb_hst_other[i]  = TRUE;
      }
      for(i=0;i<NUM_ESO_IMAGE;i++){
	hg->fcdb_eso_image[i]  = TRUE;
      }
      for(i=0;i<NUM_ESO_SPEC;i++){
	hg->fcdb_eso_spec[i]  = TRUE;
      }
      for(i=0;i<NUM_ESO_VLTI;i++){
	hg->fcdb_eso_vlti[i]  = TRUE;
      }
      for(i=0;i<NUM_ESO_POLA;i++){
	hg->fcdb_eso_pola[i]  = TRUE;
      }
      for(i=0;i<NUM_ESO_CORO;i++){
	hg->fcdb_eso_coro[i]  = TRUE;
      }
      for(i=0;i<NUM_ESO_OTHER;i++){
	hg->fcdb_eso_other[i]  = TRUE;
      }
      for(i=0;i<NUM_ESO_SAM;i++){
	hg->fcdb_eso_sam[i]  = TRUE;
      }
      hg->fcdb_gemini_inst=GEMINI_INST_ANY;
    }

    if(flagFC){
      switch(hg->fcdb_type){
      case FCDB_TYPE_LAMOST:
	switch(hg->fcdb_lamost_dr){
	case FCDB_LAMOST_DR5:
	  tmp_str=g_strdup_printf("<b>%s DR5</b>", db_name[hg->fcdb_type]);
	  break;
	  
	case FCDB_LAMOST_DR6:
	  tmp_str=g_strdup_printf("<b>%s DR6 low</b>", db_name[hg->fcdb_type]);
	  break;

	case FCDB_LAMOST_DR6M:
	  tmp_str=g_strdup_printf("<b>%s DR6 med</b>", db_name[hg->fcdb_type]);
	  break;
	}
	break;
	
      default:
	tmp_str=g_strdup_printf("<b>%s</b>", db_name[hg->fcdb_type]);
	break;
      }
      gtkut_frame_set_label(GTK_FRAME(hg->fcdb_frame),tmp_str);
      g_free(tmp_str);
    }

    if(rebuild_flag) rebuild_fcdb_tree(hg);
  }

  g_free(cdata);
}


void change_fcdb_para(GtkWidget *w, gpointer gdata)
{ 
  confPropFCDB *cdata;

  cdata=(confPropFCDB *)gdata;

  cdata->mode=1;

  {
    GtkWidget *w;
    gint i;
    
    for(i = 0; i < g_slist_length(cdata->fcdb_group); i++){
      w = g_slist_nth_data(cdata->fcdb_group, i);
      if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w))){
	cdata->fcdb_type  = g_slist_length(cdata->fcdb_group) -1 - i;
	break;
      }
    }
  }
 
  gtk_main_quit();
  gtk_widget_destroy(GTK_WIDGET(cdata->dialog));
  flagChildDialog=FALSE;
}


void radio_fcdb(GtkWidget *button, gpointer gdata)
{ 
  typHOE *hg;
  confPropFCDB *cdata;
  GSList *group=NULL;

  hg=(typHOE *)gdata;

  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));

  {
    GtkWidget *w;
    gint i;
    
    for(i = 0; i < g_slist_length(group); i++){
      w = g_slist_nth_data(group, i);
      if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w))){
	gtk_notebook_set_current_page (GTK_NOTEBOOK(hg->query_note),g_slist_length(group)-1-i);
	break;
      }
    }
  }
}


// See eq. (2) in Coupon et al. 2017, PASJ 70, 1
gdouble hsc_sat_radius(gdouble g_mag){
  gdouble r;

  if(g_mag < 9.0){
    r = 708.9 * exp(-g_mag/8.41);
  }
  else{
    r = 694.7 * exp(-g_mag/4.04);
  }

  return(r);
}


void draw_hsc_sat(cairo_t *cr, gdouble rad, 
		  gdouble x, gdouble y, gint dss_pa, gboolean dss_flip){
  
  cairo_arc(cr, x, y, rad, 0, M_PI*2);

  cairo_translate(cr, x, y);
  if(dss_flip){
    cairo_rotate (cr, M_PI*(gdouble)(-dss_pa)/180.);
  }
  else{
    cairo_rotate (cr, -M_PI*(gdouble)(-dss_pa)/180.);
  }
  cairo_rectangle(cr, 
		  -rad*1.5,
		  -rad*0.15,
		  rad*3.0,rad*0.3);

  cairo_fill(cr);

  if(dss_flip){
    cairo_rotate (cr,-M_PI*(gdouble)(-dss_pa)/180.);
  }
  else{
    cairo_rotate (cr,M_PI*(gdouble)(-dss_pa)/180.);
  }
  cairo_translate(cr, -x, -y);
}


void draw_gs(typHOE *hg,
	     cairo_t *cr,
	     gint width, gint height,
	     gint width_file, gint height_file,
	     gdouble scale,
	     gdouble r)
{
  gdouble gs_x, gs_y, gs_d_ra, gs_d_dec;
  cairo_text_extents_t extents;
  gchar *tmp;
  gdouble yrs, new_ra_d, new_dec_d;
  
  translate_to_center(hg,cr,width,height,width_file,height_file,r);
  
  if(hg->obj[hg->dss_i].gs.flag){
    yrs=current_yrs(hg);
    new_ra_d=ra_to_deg(hg->obj[hg->dss_i].ra)+
      hg->obj[hg->dss_i].pm_ra/1000/60/60*yrs;
    new_dec_d=dec_to_deg(hg->obj[hg->dss_i].dec)+
      hg->obj[hg->dss_i].pm_dec/1000/60/60*yrs; 
    
    gs_d_ra=ra_to_deg(hg->obj[hg->dss_i].gs.ra);
    gs_d_dec=dec_to_deg(hg->obj[hg->dss_i].gs.dec);
    gs_x=-(gs_d_ra-new_ra_d)*60.
      *cos(gs_d_dec/180.*M_PI)
      *((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip;
    gs_y=-(gs_d_dec-new_dec_d)*60.
      *((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip;
    //gs_x=-(gs_d_ra-ra_to_deg(hg->obj[hg->dss_i].ra))*60.
    //  *cos(gs_d_dec/180.*M_PI)
    //  *((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip;
    //gs_y=-(gs_d_dec-dec_to_deg(hg->obj[hg->dss_i].dec))*60.
    //  *((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip;
    if(hg->dss_flip) gs_x=-gs_x;
    
    cairo_set_line_width (cr, 2*scale);
    if(hg->dss_invert) cairo_set_source_rgba(cr, 0.0, 0.5, 0.0, 1.0);
    else cairo_set_source_rgba(cr, 0.2, 1.0, 0.2, 1.0);
    cairo_arc(cr,gs_x*scale,gs_y*scale,15*scale,0,2*M_PI);
    cairo_stroke(cr);
    
    cairo_select_font_face (cr, hg->fontfamily_all, 
			    CAIRO_FONT_SLANT_NORMAL,
			    CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size (cr, (gdouble)hg->skymon_allsz*0.9*scale);
    tmp=g_strdup("Guide");
    cairo_text_extents (cr,tmp, &extents);
    cairo_move_to(cr,
		  gs_x*scale-extents.width/2,
		  gs_y*scale-17*scale);
    cairo_show_text(cr, tmp);
    if(tmp) g_free(tmp);
  }
}


void draw_nst(typHOE *hg,
	      cairo_t *cr,
	      gint width, gint height,
	      gint width_file, gint height_file,
	      gdouble scale,
	      gdouble r)
{  //Non-Sidereal Orbit
  gint i, i_step=0, i_step_max=1, i_tag=3, i_tag_max=3; 
  gdouble x, y, x0, y0;
  gdouble d_ra, d_dec;
  gdouble d_step, t_step;
  struct ln_equ_posn object, object_prec;
  struct ln_zonedate zonedate;
  cairo_text_extents_t extents;
  gchar *tmp;
  gdouble rr, dx, dy, dx1, dy1;
  
  if((hg->orbit_flag)&&(hg->obj[hg->dss_i].i_nst>=0)){
    
    translate_to_center(hg,cr,width,height,width_file,height_file,r);
    
    if(hg->dss_invert) cairo_set_source_rgba(cr, 0.0, 0.5, 0.0, 1.0);
    else cairo_set_source_rgba(cr, 0.0, 1.0, 0.0, 1.0);
    
    object.ra=ra_to_deg(hg->obj[hg->dss_i].ra);
    object.dec=dec_to_deg(hg->obj[hg->dss_i].dec);
    
    ln_get_equ_prec2 (&object, 
		      get_julian_day_of_epoch(hg->obj[hg->dss_i].equinox),
		      JD2000, &object_prec);
    
    d_ra=ra_to_deg(hg->nst[hg->obj[hg->dss_i].i_nst].eph[0].ra);
    d_dec=dec_to_deg(hg->nst[hg->obj[hg->dss_i].i_nst].eph[0].dec);
    
    x=-(d_ra-object_prec.ra)*60.
      *cos(d_dec/180.*M_PI)
      *((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip;
    y=-(d_dec-object_prec.dec)*60.
      *((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip;
    if(hg->dss_flip) x=-x;
    
    cairo_move_to(cr,x,y);
    
    for(i=1;i<hg->nst[hg->obj[hg->dss_i].i_nst].i_max;i++){
      x0=x;
      y0=y;
      
      d_ra=ra_to_deg(hg->nst[hg->obj[hg->dss_i].i_nst].eph[i].ra);
      d_dec=dec_to_deg(hg->nst[hg->obj[hg->dss_i].i_nst].eph[i].dec);
      
      x=-(d_ra-object_prec.ra)*60.
	*cos(d_dec/180.*M_PI)
	*((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip;
      y=-(d_dec-object_prec.dec)*60.
	*((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip;
      if(hg->dss_flip) x=-x;
      
      if(i==1){
	d_step=sqrt((x-x0)*(x-x0)+(y-y0)*(y-y0));
	if(d_step<(gdouble)width_file*r/20){
	  i_step_max=(gint)((gdouble)width_file*r/20./d_step);
	}
	i_step=1;
      }
      
      cairo_set_line_width (cr, 2.5*scale);
      cairo_line_to(cr,x,y);
      cairo_stroke(cr);
      
      cairo_set_font_size (cr, (gdouble)hg->skymon_allsz*1.0*scale);

      rr=sqrt((x-x0)*(x-x0)+(y-y0)*(y-y0));
      dx1=5.0*(y-y0)/rr;
      dy1=-5.0*(x-x0)/rr;
      
      if(i_step>0){
	if(i_step>=i_step_max){
	  cairo_set_line_width (cr, 1.5*scale);
	  if(fabs(x-x0)>fabs(y-y0)){
	    // likely Horizontal
	    if(i_tag>=i_tag_max){
	      ln_get_local_date(hg->nst[hg->obj[hg->dss_i].i_nst].eph[i].jd,
				&zonedate, 
				hg->obs_timezone);
	      tmp=g_strdup_printf("%d:%02d, %s %d",
				  zonedate.hours,
				  zonedate.minutes,	
				  cal_month[zonedate.months-1],
				  zonedate.days);
	      cairo_text_extents (cr, tmp, &extents);
	      cairo_move_to(cr,x,y-12);
	      cairo_rel_move_to(cr,extents.height/2, 0);
	      cairo_rotate (cr,-M_PI/2);
	      cairo_show_text(cr,tmp);
	      cairo_rotate (cr,M_PI/2);
	      if(tmp) g_free(tmp);
	      i_tag=0;
	      dx=dx1*1.6;
	      dy=dy1*1.6;
	    }
	    else{
	      i_tag++;
	      dx=dx1;
	      dy=dy1;
	    }
	  }
	  else{
	    // likely Vertical
	    if(i_tag>=i_tag_max){
	      ln_get_local_date(hg->nst[hg->obj[hg->dss_i].i_nst].eph[i].jd,
				&zonedate, 
				hg->obs_timezone);
	      tmp=g_strdup_printf("%d:%02d, %s %d",
				  zonedate.hours,
				  zonedate.minutes,	
				  cal_month[zonedate.months-1],
				  zonedate.days);
	      cairo_text_extents (cr, tmp, &extents);
	      cairo_move_to(cr,x+12,y);
	      cairo_rel_move_to(cr,0,extents.height/2);
	      cairo_show_text(cr,tmp);
	      if(tmp) g_free(tmp);
	      i_tag=0;
	      dx=dx1*1.6;
	      dy=dy1*1.6;
	    }
	    else{
	      i_tag++;
	      dx=dx1;
	      dy=dy1;
	    }
	  }
	  
	  cairo_move_to(cr,x-dx,y-dy);
	  cairo_line_to(cr,x+dx,y+dy);
	  cairo_stroke(cr);
	  
	  i_step=1;
	}
	else{
	}
	i_step++;
      }
      else{
	cairo_set_line_width (cr, 1.5*scale);
	if(fabs(x-x0)>fabs(y-y0)){
	  if(i_tag>=i_tag_max){
	    ln_get_local_date(hg->nst[hg->obj[hg->dss_i].i_nst].eph[i].jd,
			      &zonedate, 
			      hg->obs_timezone);
	    tmp=g_strdup_printf("%d:%02d, %s %d",
				zonedate.hours,
				zonedate.minutes,	
				cal_month[zonedate.months-1],
				zonedate.days);
	    cairo_text_extents (cr, tmp, &extents);
	    cairo_move_to(cr,x,y-12);
	    cairo_rel_move_to(cr,extents.height/2, 0);
	    cairo_rotate (cr,-M_PI/2);
	    cairo_show_text(cr,tmp);
	    cairo_rotate (cr,M_PI/2);
	    if(tmp) g_free(tmp);
	    i_tag=0;
	    dx=dx1*1.6;
	    dy=dy1*1.6;
	  }
	  else{
	    i_tag++;
	    dx=dx1;
	    dy=dy1;
	  }
	}
	else{	
	  if(i_tag>=i_tag_max){
	    ln_get_local_date(hg->nst[hg->obj[hg->dss_i].i_nst].eph[i].jd,
			      &zonedate, 
			      hg->obs_timezone);
	    tmp=g_strdup_printf("%d:%02d, %s %d",
				zonedate.hours,
				zonedate.minutes,	
				cal_month[zonedate.months-1],
				zonedate.days);
	    cairo_text_extents (cr, tmp, &extents);
	    cairo_move_to(cr,x+12,y);
	    cairo_rel_move_to(cr,0,extents.height/2);
	    cairo_show_text(cr,tmp);
	    if(tmp) g_free(tmp);
	    i_tag=0;
	    dx=dx1*1.6;
	    dy=dy1*1.6;
	  }
	  else{
	    i_tag++;
	    dx=dx1;
	    dy=dy1;
	  }
	}
	
	cairo_move_to(cr,x-dx,y-dy);
	cairo_line_to(cr,x+dx,y+dy);
	cairo_stroke(cr);
      }
      
      cairo_move_to(cr,x,y);
    }
    
    if(hg->fc_mag==1){
      cairo_restore(cr);
      
      cairo_save(cr);
      
      cairo_translate (cr, (width-(gint)((gdouble)width_file*r))/2,
		       (height-(gint)((gdouble)height_file*r))/2);
      
      if(hg->dss_invert) cairo_set_source_rgba(cr, 0.0, 0.5, 0.0, 1.0);
      else cairo_set_source_rgba(cr, 0.0, 1.0, 0.0, 1.0);
      
      t_step=(hg->nst[hg->obj[hg->dss_i].i_nst].eph[1].jd
	      -hg->nst[hg->obj[hg->dss_i].i_nst].eph[0].jd)
	*24.*60.*(gdouble)i_step_max; //min
      
      cairo_set_font_size (cr, (gdouble)hg->skymon_allsz*1.4*scale);
      if(t_step<5){
	tmp=g_strdup_printf("Step=%dsec",(gint)(t_step*60+0.5));
      }
      else if(t_step<60){
	tmp=g_strdup_printf("Step=%dmin",(gint)t_step);
      }
      else{
	tmp=g_strdup_printf("Step=%.1lfhrs",t_step/60);
      }
      cairo_text_extents (cr, tmp, &extents);
      cairo_move_to(cr,
		    (gdouble)width_file*r-extents.width-5*scale,
		    (gdouble)height_file*r-5*scale);
      cairo_show_text(cr,tmp);
      if(tmp) g_free(tmp);

      cairo_move_to(cr,
		    (gdouble)width_file*r-5*scale,
		    (gdouble)height_file*r-extents.height-10*scale);
      
      tmp=g_strdup_printf("%d/%d/%d  %d:%02d (%s)",
			  hg->skymon_year,
			  hg->skymon_month,
			  hg->skymon_day,
			  hg->skymon_hour,
			  hg->skymon_min,
			  hg->obs_tzname);
      cairo_text_extents (cr, tmp, &extents);
      cairo_rel_move_to(cr, -extents.width, 0);
      cairo_show_text(cr,tmp);
      if(tmp) g_free(tmp);
    }

  }
}


void draw_pts(typHOE *hg,
	      cairo_t *cr,
	      gint width, gint height,
	      gint width_file, gint height_file,
	      gint shift_x, gint shift_y,
	      gdouble scale,
	      gdouble r)
{  // Points and Distance
  gdouble distance;
  gdouble arad;
  cairo_text_extents_t extents;
  gchar *tmp;
  
  if(hg->fc_ptn>=1){
    
    if(hg->dss_invert) cairo_set_source_rgba(cr, 0.5, 0.5, 0.0, 0.8);
    else cairo_set_source_rgba(cr, 1.0, 1.0, 0.2, 0.8);
    
    cairo_set_line_width (cr, 2*scale);
    
    if(hg->fc_mag!=1){
      cairo_translate(cr,
		      -shift_x,
		      -shift_y);
    }
    
    cairo_move_to(cr,hg->fc_ptx1-5,hg->fc_pty1-5);
    cairo_rel_line_to(cr,10,10);
    
    cairo_move_to(cr,hg->fc_ptx1-5,hg->fc_pty1+5);
    cairo_rel_line_to(cr,10,-10);
    
    cairo_stroke(cr);
  }

  if(hg->fc_ptn==2){
    cairo_move_to(cr,hg->fc_ptx2-5,hg->fc_pty2-5);
    cairo_rel_line_to(cr,10,10);
    
    cairo_move_to(cr,hg->fc_ptx2-5,hg->fc_pty2+5);
    cairo_rel_line_to(cr,10,-10);
    
    cairo_stroke(cr);
    
    cairo_set_line_width (cr, 0.8*scale);
    
    cairo_move_to(cr,hg->fc_ptx1,hg->fc_pty1);
    cairo_line_to(cr,hg->fc_ptx2,hg->fc_pty2);
    
    cairo_stroke(cr);
    
    if(hg->dss_invert) cairo_set_source_rgba(cr, 0.5, 0.5, 0.0, 1.0);
    else cairo_set_source_rgba(cr, 1.0, 1.0, 0.2, 1.0);
    cairo_select_font_face (cr, hg->fontfamily_all, CAIRO_FONT_SLANT_NORMAL,
			    CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size (cr, (gdouble)hg->skymon_allsz*1.2*scale);
    
    distance=sqrt((gdouble)((hg->fc_ptx1-hg->fc_ptx2)
			    *(hg->fc_ptx1-hg->fc_ptx2))
		  +(gdouble)((hg->fc_pty1-hg->fc_pty2)
			     *(hg->fc_pty1-hg->fc_pty2)))
      /((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip)*60.0;
    
    if(distance > 300){
      tmp=g_strdup_printf("%.2lf'",distance/60.0);
    }
    else{
      tmp=g_strdup_printf("%.2lf\"",distance);
    }
    cairo_text_extents (cr, tmp, &extents);
    
    arad=atan2((hg->fc_ptx1-hg->fc_ptx2),(hg->fc_pty1-hg->fc_pty2));
    cairo_translate(cr,
		    (hg->fc_ptx1+hg->fc_ptx2)/2,
		    (hg->fc_pty1+hg->fc_pty2)/2);
    cairo_rotate (cr,-(arad+M_PI/2));
    
    cairo_move_to(cr,-extents.width/2.,-extents.height*0.8);
    cairo_show_text(cr,tmp);
    if(tmp) g_free(tmp);
  }
}


void draw_fcdb1(typHOE *hg,
		cairo_t *cr,
		gint width, gint height,
		gint width_file, gint height_file,
		gint shift_x, gint shift_y,
		gdouble scale,
		gdouble r)
{
  gdouble cx, cy;
  gdouble ptx, pty, ptx0, pty0;
  gdouble rad, rad_min=1000.0, ptr;
  gint i, i_list, i_sel=-1;
  gdouble theta;
  cairo_text_extents_t extents;
  gchar *tmp;
  
  if(hg->fc_ptn==-1){
    cx=((gdouble)width-(gdouble)width_file*r)/2+(gdouble)width_file*r/2;
    cy=((gdouble)height-(gdouble)height_file*r)/2+(gdouble)height_file*r/2;
    if(hg->fc_mag!=1){
      cx-=-shift_x;
      cy-=-shift_y;
    }

    ptx0=((gdouble)hg->fc_ptx1-cx);
    pty0=((gdouble)hg->fc_pty1-cy);
    
    switch(hg->fc_inst){
    case FC_INST_NONE:
    case FC_INST_HDS:
    case FC_INST_HDSAUTO:
    case FC_INST_HDSZENITH:
    case FC_INST_IRD:
    case FC_INST_COMICS:
    case FC_INST_FOCAS:
    case FC_INST_MOIRCS:
    case FC_INST_SWIMS:
    case FC_INST_FMOS:
      theta=-M_PI*(gdouble)hg->dss_pa/180.;
      break;
      
    case FC_INST_IRCS:
      theta=-M_PI*(gdouble)(-90+hg->dss_pa)/180.;
      break;
      
    case FC_INST_SPCAM:
      theta=-M_PI*(gdouble)(90-hg->dss_pa)/180.;
      break;
      
    case FC_INST_HSCDET:
    case FC_INST_HSCA:
      theta=-M_PI*(gdouble)(270-hg->dss_pa)/180.;
      break;
    }
    
    if(hg->dss_flip){
      theta=-theta;;
    }
    
    ptx=ptx0*cos(theta)-pty0*sin(theta);
    pty=ptx0*sin(theta)+pty0*cos(theta);

    for(i_list=0;i_list<hg->fcdb_i_max;i_list++){
      if((fabs(hg->fcdb[i_list].x-ptx)<10)&&(fabs(hg->fcdb[i_list].y-pty)<10)){
	rad=(hg->fcdb[i_list].x-ptx)*(hg->fcdb[i_list].x-ptx)
	  +(hg->fcdb[i_list].y-pty)*(hg->fcdb[i_list].y-pty);
	if(rad<rad_min){
	  i_sel=i_list;
	  rad_min=rad;
	}
      }
    }
      
    if(i_sel>=0){
      hg->fcdb_tree_focus=i_sel;
      {
	GtkTreeModel *model 
	  = gtk_tree_view_get_model(GTK_TREE_VIEW(hg->fcdb_tree));
	GtkTreePath *path;
	GtkTreeIter  iter;
	
	path=gtk_tree_path_new_first();
	
	for(i=0;i<hg->fcdb_i_max;i++){
	  gtk_tree_model_get_iter (model, &iter, path);
	  gtk_tree_model_get (model, &iter, COLUMN_FCDB_NUMBER, &i_list, -1);
	  i_list--;
	  
	  if(i_list==i_sel){
	    gtk_notebook_set_current_page (GTK_NOTEBOOK(hg->all_note), 
					   hg->page[NOTE_FCDB]);
	    gtk_widget_grab_focus (hg->fcdb_tree);
	    gtk_tree_view_set_cursor(GTK_TREE_VIEW(hg->fcdb_tree), 
				     path, NULL, FALSE);
	    break;
	  }
	  else{
	    gtk_tree_path_next(path);
	  }
	}
	gtk_tree_path_free(path);
      }
    }

    hg->fc_ptn=0;
  }
}

void draw_fcdb2(typHOE *hg,
		cairo_t *cr,
		gint width, gint height,
		gint width_file, gint height_file,
		gdouble scale,
		gdouble r)
{
  gdouble pmx, pmy;
  gdouble yrs;
  gint i_list;
  cairo_text_extents_t extents;
  gchar *tmp;
  gboolean draw_sat=FALSE;
  
  if((hg->fcdb_flag)&&(hg->fcdb_i==hg->dss_i)){

    translate_to_center(hg,cr,width,height,width_file,height_file,r);
    switch(hg->fcdb_type){
    case FCDB_TYPE_GAIA:
    case MAGDB_TYPE_GAIA:
    case MAGDB_TYPE_IRCS_GAIA:
    case MAGDB_TYPE_HDS_GAIA:
      yrs=current_yrs(hg)-15.5;
      break;

    default:
      yrs=current_yrs(hg);
      break;
    }
    
    for(i_list=0;i_list<hg->fcdb_i_max;i_list++){
      hg->fcdb[i_list].x=-(hg->fcdb[i_list].d_ra-hg->fcdb_d_ra0)*60.
	*cos(hg->fcdb[i_list].d_dec/180.*M_PI)
	*((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip;
      hg->fcdb[i_list].y=-(hg->fcdb[i_list].d_dec-hg->fcdb_d_dec0)*60.
	*((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip;
      if(hg->dss_flip) hg->fcdb[i_list].x=-hg->fcdb[i_list].x;
    }

    if(((hg->fcdb_type==FCDB_TYPE_GAIA)||(hg->fcdb_type==FCDB_TYPE_SIMBAD))
       &&(hg->hsc_sat)){
      draw_sat=TRUE;
    }

    
    if(hg->dss_invert){
      cairo_set_source_rgba(cr, 0.5, 0.5, 0.0, 1.0);
    }
    else{
      if(draw_sat){
	cairo_set_source_rgba(cr, 1.0, 1.0, 0.2, 0.7);
      }
      else{
	cairo_set_source_rgba(cr, 1.0, 1.0, 0.2, 1.0);
      }
    }
    cairo_set_line_width (cr, 2*scale);
    for(i_list=0;i_list<hg->fcdb_i_max;i_list++){
      if(hg->fcdb_tree_focus!=i_list){
	if(draw_sat){
	  if(hg->fcdb[i_list].v < 20){
	    gdouble rad=((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip/2.*hsc_sat_radius(hg->fcdb[i_list].v)/30.;
	    
	    draw_hsc_sat(cr, rad, 
			 hg->fcdb[i_list].x, hg->fcdb[i_list].y,
			 hg->dss_pa, hg->dss_flip);
	  }
	}
	else{
	  cairo_rectangle(cr,hg->fcdb[i_list].x-6,hg->fcdb[i_list].y-6,12,12);
	  cairo_stroke(cr);
	}
      }
    }
    
    if(hg->dss_invert){
      cairo_set_source_rgba(cr, 0.7, 0.0, 0.0, 1.0);
    }
    else{
      if(draw_sat){
	cairo_set_source_rgba(cr, 1.0, 0.0, 0.0, 0.7);
      }
      else{
	cairo_set_source_rgba(cr, 1.0, 0.0, 0.0, 1.0);
      }
    }
    cairo_set_line_width (cr, 4*scale);
    for(i_list=0;i_list<hg->fcdb_i_max;i_list++){
      if(hg->fcdb_tree_focus==i_list){
	if(draw_sat){
	  if(hg->fcdb[i_list].v < 20){
	    gdouble rad=((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip/2.*hsc_sat_radius(hg->fcdb[i_list].v)/30.;
	    
	    draw_hsc_sat(cr, rad, 
			 hg->fcdb[i_list].x, hg->fcdb[i_list].y,
			 hg->dss_pa, hg->dss_flip);
	  }
	}
	else{
	  cairo_rectangle(cr,hg->fcdb[i_list].x-8,hg->fcdb[i_list].y-8,16,16);
	  cairo_stroke(cr);
	}
      }
    }
    
    // Proper Motion
    if(!draw_sat){
      if(hg->dss_invert) cairo_set_source_rgba(cr, 0.0, 0.5, 0.0, 1.0);
      else cairo_set_source_rgba(cr, 0.2, 1.0, 0.2, 1.0);
      cairo_set_line_width (cr, 1.5*scale);
      for(i_list=0;i_list<hg->fcdb_i_max;i_list++){
	
	if(hg->fcdb[i_list].pm){
	  pmx=-(hg->fcdb[i_list].d_ra-hg->fcdb_d_ra0
		+hg->fcdb[i_list].pmra/1000/60/60*yrs)*60.
	    *cos(hg->fcdb[i_list].d_dec/180.*M_PI)
	    *((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip;
	  pmy=-(hg->fcdb[i_list].d_dec-hg->fcdb_d_dec0
	      +hg->fcdb[i_list].pmdec/1000/60/60*yrs)*60.
	      *((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip;
	  if(hg->dss_flip) {
	    pmx=-pmx;
	  }
	  cairo_move_to(cr,hg->fcdb[i_list].x,hg->fcdb[i_list].y);
	  cairo_line_to(cr,pmx,pmy);
	  cairo_stroke(cr);
	  cairo_arc(cr,pmx,pmy,5,0,2*M_PI);
	  cairo_fill(cr);
	}
      }
    }
    
  }
}


	
void draw_hds(typHOE *hg,
	      cairo_t *cr,
	      gint width, gint height,
	      gint width_file, gint height_file,
	      gdouble scale,
	      gdouble r)
{ // Drawing Inst (HDS)
  cairo_text_extents_t extents;
  gchar *tmp;
  
  if(hg->dss_draw_slit){
    cairo_arc(cr,((gdouble)width_file*r)/2,((gdouble)height_file*r)/2,
	      ((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip/2.,
	      0,M_PI*2);
    cairo_clip(cr);
    cairo_new_path(cr);
    
    if(hg->dss_invert) cairo_set_source_rgba(cr, 0.6, 0.0, 0.0, 0.3);
    else cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 0.3);
    cairo_set_line_width (cr, (gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip/60.*HDS_SLIT_MASK_ARCSEC);
    
    cairo_move_to(cr,((gdouble)width_file*r)/2,
		  ((gdouble)height_file*r)/2-((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip/2.);
    cairo_line_to(cr,((gdouble)width_file*r)/2,
		  ((gdouble)height_file*r)/2-(gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*HDS_SLIT_LENGTH/2./500./60.);
    
    cairo_move_to(cr,((gdouble)width_file*r)/2,
		  ((gdouble)height_file*r)/2+(gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*HDS_SLIT_LENGTH/2./500./60.);
    cairo_line_to(cr,((gdouble)width_file*r)/2,
		  ((gdouble)height_file*r)/2+((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip/2.);
    cairo_stroke(cr);
    
    cairo_set_line_width (cr, (gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip/60.*HDS_SLIT_WIDTH/500.);
    cairo_move_to(cr,((gdouble)width_file*r)/2,
		  ((gdouble)height_file*r)/2-(gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*HDS_SLIT_LENGTH/2./500./60.);
    cairo_line_to(cr,((gdouble)width_file*r)/2,
		  ((gdouble)height_file*r)/2+(gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*HDS_SLIT_LENGTH/2./500./60.);
    cairo_stroke(cr);
    
    cairo_reset_clip(cr);
  }
  
  if(hg->dss_invert) cairo_set_source_rgba(cr, 0.6, 0.0, 0.0, 0.6);
  else cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 0.6);
  
  cairo_set_line_width (cr, 3.0*scale);
  
  cairo_arc(cr,((gdouble)width_file*r)/2,((gdouble)height_file*r)/2,
	    ((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip/2.,
	    0,M_PI*2);
  cairo_stroke(cr);
  
  cairo_move_to(cr,
		((gdouble)width_file*r)/2+((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip/2.*cos(M_PI/4),
		((gdouble)height_file*r)/2-((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip/2.*sin(M_PI/4));
  cairo_set_line_width (cr, 1.5*scale);
  cairo_line_to(cr,
		((gdouble)width_file*r)/2+1.5*((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip/2.*cos(M_PI/4),
		((gdouble)height_file*r)/2-1.5*((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip/2.*sin(M_PI/4));
  cairo_stroke(cr);
  
  if(hg->dss_invert) cairo_set_source_rgba(cr, 0.6, 0.0, 0.0, 1.0);
  else cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 1.0);
  cairo_select_font_face (cr, hg->fontfamily_all, CAIRO_FONT_SLANT_NORMAL,
			  CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size (cr, (gdouble)hg->skymon_allsz*0.9*scale);
  cairo_move_to(cr,
		((gdouble)width_file*r)/2+1.5*((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip/2.*cos(M_PI/4),
		((gdouble)height_file*r)/2-1.5*((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip/2.*sin(M_PI/4));
  cairo_show_text(cr, "HDS SV FOV (1arcmin)");
}


void draw_ircs(typHOE *hg,
	       cairo_t *cr,
	       gint width, gint height,
	       gint width_file, gint height_file,
	       gdouble scale,
	       gdouble r)
{ // Drawing Inst (IRCS)
  cairo_text_extents_t extents;
  gchar *tmp;
  
  cairo_translate(cr,((gdouble)width_file*r)/2,((gdouble)height_file*r)/2);
  
  if(hg->dss_invert) cairo_set_source_rgba(cr, 0.6, 0.0, 0.0, 0.6);
  else cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 0.6);
  
  if(hg->dss_draw_slit){
    cairo_set_line_width (cr, 1.5*scale);
    cairo_arc(cr,0,0,
	      ((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip/2.*IRCS_TTGS_ARCMIN,
	      0,M_PI*2);
    cairo_stroke(cr);
    
    cairo_set_font_size (cr, (gdouble)hg->skymon_allsz*0.9*scale);
    
    tmp=g_strdup_printf("Tip-Tilt Guide Star w/LGS (%darcmin)",IRCS_TTGS_ARCMIN/2);
    cairo_text_extents (cr,tmp, &extents);
    cairo_move_to(cr,
		  -extents.width/2,
		  -IRCS_TTGS_ARCMIN/2.*((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip)-5*scale);
    cairo_show_text(cr, tmp);
    if(tmp) g_free(tmp);
  }
  
  cairo_set_line_width (cr, 3.0*scale);
  
  cairo_rectangle(cr,
		  -((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*IRCS_X_ARCSEC/60.)/2.,
		  -((gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*IRCS_Y_ARCSEC/60.)/2.,
		  (gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*IRCS_X_ARCSEC/60.,
		  (gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*IRCS_Y_ARCSEC/60.);
  cairo_stroke(cr);
  
  if(hg->dss_invert) cairo_set_source_rgba(cr, 0.6, 0.0, 0.0, 1.0);
  else cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 1.0);
  cairo_select_font_face (cr, hg->fontfamily_all, CAIRO_FONT_SLANT_NORMAL,
			  CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size (cr, (gdouble)hg->skymon_allsz*0.9*scale);
  
  tmp=g_strdup_printf("IRCS FOV (%dx%darcsec)",(gint)IRCS_X_ARCSEC, (gint)IRCS_Y_ARCSEC);
  cairo_text_extents (cr,tmp, &extents);
  cairo_move_to(cr,-extents.width/2,
		-((gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*IRCS_Y_ARCSEC/60.)/2.-5*scale);
  cairo_show_text(cr, tmp);
  if(tmp) g_free(tmp);
}


void draw_ird(typHOE *hg,
	      cairo_t *cr,
	      gint width, gint height,
	      gint width_file, gint height_file,
	      gdouble scale,
	      gdouble r)
{ // Drawing Inst (IRD)
  cairo_text_extents_t extents;
  gchar *tmp;
  
  cairo_translate(cr,((gdouble)width_file*r)/2,((gdouble)height_file*r)/2);
  
  if(hg->dss_invert) cairo_set_source_rgba(cr, 0.6, 0.0, 0.0, 0.6);
  else cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 0.6);
  
  if(hg->dss_draw_slit){
    cairo_set_line_width (cr, 1.5*scale);
    cairo_arc(cr,0,0,
	      ((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip/2.*IRD_TTGS_ARCMIN,
	      0,M_PI*2);
    cairo_stroke(cr);
    
    cairo_set_font_size (cr, (gdouble)hg->skymon_allsz*0.9*scale);
    
    tmp=g_strdup_printf("Tip-Tilt Guide Star w/LGS (%darcmin)",IRD_TTGS_ARCMIN/2);
    cairo_text_extents (cr,tmp, &extents);
    cairo_move_to(cr,
		  -extents.width/2,
		  -IRD_TTGS_ARCMIN/2.*((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip)-5*scale);
    cairo_show_text(cr, tmp);
    if(tmp) g_free(tmp);
  }
  
  cairo_set_line_width (cr, 3.0*scale);
  
  cairo_rectangle(cr,
		  -((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*IRD_X_ARCSEC/60.)/2.,
		  -((gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*IRD_Y_ARCSEC/60.)/2.,
		  (gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*IRD_X_ARCSEC/60.,
		  (gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*IRD_Y_ARCSEC/60.);
  cairo_stroke(cr);
  
  if(hg->dss_invert) cairo_set_source_rgba(cr, 0.6, 0.0, 0.0, 1.0);
  else cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 1.0);
  cairo_select_font_face (cr, hg->fontfamily_all, CAIRO_FONT_SLANT_NORMAL,
			  CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size (cr, (gdouble)hg->skymon_allsz*0.9*scale);
  
  tmp=g_strdup_printf("IRD FOV for FIM (%dx%darcsec)",(gint)IRD_X_ARCSEC, (gint)IRD_Y_ARCSEC);
  cairo_text_extents (cr,tmp, &extents);
  cairo_move_to(cr,-extents.width/2,
		-((gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*IRD_Y_ARCSEC/60.)/2.-5*scale);
  cairo_show_text(cr, tmp);
  if(tmp) g_free(tmp);
}


void draw_comics(typHOE *hg,
		 cairo_t *cr,
		 gint width, gint height,
		 gint width_file, gint height_file,
		 gdouble scale,
		 gdouble r)
{ // Drawing Inst (COMICS)
  cairo_text_extents_t extents;
  gchar *tmp;

  cairo_translate(cr,((gdouble)width_file*r)/2,((gdouble)height_file*r)/2);
  
  if(hg->dss_invert) cairo_set_source_rgba(cr, 0.6, 0.0, 0.0, 0.6);
  else cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 0.6);
  cairo_set_line_width (cr, 3.0*scale);
  
  cairo_rectangle(cr,
		  -((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*COMICS_X_ARCSEC/60.)/2.,
		  -((gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*COMICS_Y_ARCSEC/60.)/2.,
		  (gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*COMICS_X_ARCSEC/60.,
		  (gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*COMICS_Y_ARCSEC/60.);
  cairo_stroke(cr);
  
  if(hg->dss_invert) cairo_set_source_rgba(cr, 0.6, 0.0, 0.0, 1.0);
  else cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 1.0);
  cairo_select_font_face (cr, hg->fontfamily_all, CAIRO_FONT_SLANT_NORMAL,
			  CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size (cr, (gdouble)hg->skymon_allsz*0.9*scale);
  
  tmp=g_strdup_printf("COMICS FOV (%dx%darcsec)",(gint)COMICS_X_ARCSEC, (gint)COMICS_Y_ARCSEC);
  cairo_text_extents (cr,tmp, &extents);
  cairo_move_to(cr,-extents.width/2,
		-((gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*COMICS_Y_ARCSEC/60.)/2.-5*scale);
  cairo_show_text(cr, tmp);
  if(tmp) g_free(tmp);
}



void draw_focas(typHOE *hg,
		cairo_t *cr,
		gint width, gint height,
		gint width_file, gint height_file,
		gdouble scale,
		gdouble r)
{ // Drawing Inst (FOCAS)
  cairo_text_extents_t extents;
  gchar *tmp;

  cairo_translate(cr,((gdouble)width_file*r)/2,((gdouble)height_file*r)/2);
  
  if(hg->dss_invert) cairo_set_source_rgba(cr, 0.6, 0.0, 0.0, 0.6);
  else cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 0.6);
  cairo_set_line_width (cr, 3.0*scale);
  
  cairo_arc(cr,0,0,
	    ((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip/2.*FOCAS_R_ARCMIN,
	    0,M_PI*2);
  cairo_stroke(cr);
  
  if(hg->dss_invert) cairo_set_source_rgba(cr, 0.6, 0.0, 0.0, 1.0);
  else cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 1.0);
  cairo_select_font_face (cr, hg->fontfamily_all, CAIRO_FONT_SLANT_NORMAL,
			  CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size (cr, (gdouble)hg->skymon_allsz*0.9*scale);
  
  tmp=g_strdup_printf("FOCAS FOV (%darcmin)",FOCAS_R_ARCMIN);
  cairo_text_extents (cr,tmp, &extents);
  cairo_move_to(cr,
		-extents.width/2,
		-FOCAS_R_ARCMIN/2.*((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip)-5*scale);
  cairo_show_text(cr, tmp);
  if(tmp) g_free(tmp);
  
  if(hg->dss_draw_slit){
    cairo_new_path(cr);
    cairo_arc(cr,0,0,
	      ((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip/2.*FOCAS_R_ARCMIN,
	      0,M_PI*2);
    cairo_clip(cr);
    cairo_new_path(cr);
    
    if(hg->dss_invert) cairo_set_source_rgba(cr, 0.6, 0.0, 0.0, 0.6);
    else cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 0.6);
    cairo_set_line_width (cr, FOCAS_GAP_ARCSEC/60.*((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip));
    cairo_move_to(cr,-(gdouble)width/2,0);
    cairo_line_to(cr,(gdouble)width/2,0);
    cairo_stroke(cr);
    
    cairo_reset_clip(cr);
    
    if(hg->dss_invert) cairo_set_source_rgba(cr, 0.6, 0.0, 0.0, 1.0);
    else cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 1.0);
    cairo_select_font_face (cr, hg->fontfamily_all, CAIRO_FONT_SLANT_NORMAL,
			    CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size (cr, (gdouble)hg->skymon_allsz*0.9*scale);
    cairo_text_extents (cr,"Chip 2", &extents);
    
    cairo_move_to(cr,
		  cos(M_PI/4)*((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip/2.*FOCAS_R_ARCMIN+5*scale,
		  -sin(M_PI/4)*((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip/2.*FOCAS_R_ARCMIN-5*scale);
    cairo_show_text(cr,"Chip 2");
	
    cairo_move_to(cr,
		  cos(M_PI/4)*((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip/2.*FOCAS_R_ARCMIN+5*scale,
		  sin(M_PI/4)*((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip/2.*FOCAS_R_ARCMIN+extents.height+5*scale);
    cairo_show_text(cr,"Chip 1");
  }
}
  

void draw_moircs(typHOE *hg,
		 cairo_t *cr,
		 gint width, gint height,
		 gint width_file, gint height_file,
		 gdouble scale,
		 gdouble r)
{ // Drawing Inst (MOIRCS)
  cairo_text_extents_t extents;
  gchar *tmp;

  cairo_translate(cr,((gdouble)width_file*r)/2,((gdouble)height_file*r)/2);
  
  if(hg->dss_invert) cairo_set_source_rgba(cr, 0.6, 0.0, 0.0, 0.6);
  else cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 0.6);
  cairo_set_line_width (cr, 3.0*scale);
  
  cairo_rectangle(cr,
		  -((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_X_ARCMIN)/2.,
		  -((gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_Y_ARCMIN)/2.,
		  (gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_X_ARCMIN,
		  (gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_Y_ARCMIN);
  cairo_stroke(cr);
  
  if(hg->dss_invert) cairo_set_source_rgba(cr, 0.6, 0.0, 0.0, 1.0);
  else cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 1.0);
  cairo_select_font_face (cr, hg->fontfamily_all, CAIRO_FONT_SLANT_NORMAL,
			  CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size (cr, (gdouble)hg->skymon_allsz*0.9*scale);
  
  tmp=g_strdup_printf("MOIRCS FOV (%dx%darcmin)",(gint)MOIRCS_X_ARCMIN, (gint)MOIRCS_Y_ARCMIN);
  cairo_text_extents (cr,tmp, &extents);
  cairo_move_to(cr,-extents.width/2,
		-((gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_Y_ARCMIN)/2.-5*scale);
  cairo_show_text(cr, tmp);
  if(tmp) g_free(tmp);
  
  if(hg->dss_draw_slit){
    cairo_new_path(cr);
    cairo_rectangle(cr,
		    -((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_X_ARCMIN)/2.,
		    -((gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_Y_ARCMIN)/2.,
		    (gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_X_ARCMIN,
		    (gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_Y_ARCMIN);
    cairo_clip(cr);
    cairo_new_path(cr);
	
    if(hg->dss_invert) cairo_set_source_rgba(cr, 0.6, 0.0, 0.0, 0.6);
    else cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 0.6);
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
    
    if(hg->dss_invert) cairo_set_source_rgba(cr, 0.6, 0.0, 0.0, 0.6);
    else cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 0.6);
    
    cairo_set_line_width(cr,1.5*scale);
    cairo_arc(cr,0,0,
	      (gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_VIGR_ARCMIN/2.,
	      0,M_PI*2);
    cairo_stroke(cr);
    
    if(hg->dss_invert) cairo_set_source_rgba(cr, 0.6, 0.0, 0.0, 1.0);
    else cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 1.0);
    cairo_select_font_face (cr, hg->fontfamily_all, CAIRO_FONT_SLANT_NORMAL,
			    CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size (cr, (gdouble)hg->skymon_allsz*0.9*scale);
    cairo_text_extents (cr,"Detector 2", &extents);
    
    cairo_move_to(cr,
		  ((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_X_ARCMIN)/2.+5*scale,
		  -((gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_Y_ARCMIN)/2.+extents.height);
    cairo_show_text(cr,"Detector 2");
    
    cairo_move_to(cr,
		  ((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_X_ARCMIN)/2.+5*scale,
		  ((gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_Y_ARCMIN)/2.);
    cairo_show_text(cr,"Detector 1");
    
    cairo_rotate (cr,-M_PI/2);
    cairo_text_extents (cr,"6 arcmin from the center", &extents);
    cairo_move_to(cr,-extents.width/2.,
		  -(gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*MOIRCS_VIGR_ARCMIN/2.-5*scale);
    cairo_show_text(cr,"6 arcmin from the center");
  }
}


void draw_swims(typHOE *hg,
		 cairo_t *cr,
		 gint width, gint height,
		 gint width_file, gint height_file,
		 gdouble scale,
		 gdouble r)
{ // Drawing Inst (MOIRCS)
  cairo_text_extents_t extents;
  gchar *tmp;

  cairo_translate(cr,((gdouble)width_file*r)/2,((gdouble)height_file*r)/2);
  
  if(hg->dss_invert) cairo_set_source_rgba(cr, 0.6, 0.0, 0.0, 0.6);
  else cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 0.6);
  cairo_set_line_width (cr, 3.0*scale);
  
  cairo_rectangle(cr,
		  -((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*SWIMS_X_ARCMIN)/2.,
		  -((gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*SWIMS_Y_ARCMIN)/2.,
		  (gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*SWIMS_X_ARCMIN,
		  (gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*SWIMS_Y_ARCMIN);
  cairo_stroke(cr);
  
  if(hg->dss_invert) cairo_set_source_rgba(cr, 0.6, 0.0, 0.0, 1.0);
  else cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 1.0);
  cairo_select_font_face (cr, hg->fontfamily_all, CAIRO_FONT_SLANT_NORMAL,
			  CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size (cr, (gdouble)hg->skymon_allsz*0.9*scale);
  
  tmp=g_strdup_printf("SWIMS FOV (%.1lfx%.1lfarcmin)",SWIMS_X_ARCMIN, SWIMS_Y_ARCMIN);
  cairo_text_extents (cr,tmp, &extents);
  cairo_move_to(cr,-extents.width/2,
		-((gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*SWIMS_Y_ARCMIN)/2.-5*scale);
  cairo_show_text(cr, tmp);
  if(tmp) g_free(tmp);
  
  if(hg->dss_draw_slit){
    cairo_new_path(cr);
    cairo_rectangle(cr,
		    -((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*SWIMS_X_ARCMIN)/2.,
		    -((gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*SWIMS_Y_ARCMIN)/2.,
		    (gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*SWIMS_X_ARCMIN,
		    (gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*SWIMS_Y_ARCMIN);
    cairo_clip(cr);
    cairo_new_path(cr);
	
    if(hg->dss_invert) cairo_set_source_rgba(cr, 0.6, 0.0, 0.0, 0.6);
    else cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 0.6);
    cairo_set_line_width (cr, SWIMS_GAP_ARCSEC/60.*((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip));
    cairo_move_to(cr,0, -(gdouble)height/2);
    cairo_line_to(cr,0, (gdouble)height/2);
    cairo_stroke(cr);
	
    cairo_reset_clip(cr);
    
    if(hg->dss_invert) cairo_set_source_rgba(cr, 0.6, 0.0, 0.0, 0.6);
    else cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 0.6);
    
    cairo_set_line_width(cr,1.5*scale);
    cairo_arc(cr,0,0,
	      (gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*SWIMS_R_ARCMIN/2.,
	      0,M_PI*2);
    cairo_stroke(cr);
       
    cairo_rotate (cr,-M_PI/2);
    cairo_text_extents (cr,"7.2 arcmin from the center", &extents);
    cairo_move_to(cr,-extents.width/2.,
		  -(gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*SWIMS_R_ARCMIN/2.-5*scale);
    cairo_show_text(cr,"7.2 arcmin from the center");
  }
}


void draw_spcam(typHOE *hg,
		cairo_t *cr,
		gint width, gint height,
		gint width_file, gint height_file,
		gdouble scale,
		gdouble r)
{ // Drawing Inst (SupCam)
  gdouble x_ccd, y_ccd, gap_ccd;
  cairo_text_extents_t extents;
  gchar *tmp;
  
  cairo_translate(cr,((gdouble)width_file*r)/2,((gdouble)height_file*r)/2);
  
  if(hg->dss_invert) cairo_set_source_rgba(cr, 0.6, 0.0, 0.0, 0.6);
  else cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 0.6);
  cairo_set_line_width (cr, 3.0*scale);
  
  cairo_rectangle(cr,
		  -((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*SPCAM_X_ARCMIN)/2.,
		  -((gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*SPCAM_Y_ARCMIN)/2.,
		  (gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*SPCAM_X_ARCMIN,
		  (gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*SPCAM_Y_ARCMIN);
  cairo_stroke(cr);
  
  if(hg->dss_invert) cairo_set_source_rgba(cr, 0.6, 0.0, 0.0, 1.0);
  else cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 1.0);
  cairo_select_font_face (cr, hg->fontfamily_all, CAIRO_FONT_SLANT_NORMAL,
			  CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size (cr, (gdouble)hg->skymon_allsz*0.9*scale);
  
  tmp=g_strdup_printf("Suprime-Cam FOV (%dx%darcmin)",SPCAM_X_ARCMIN, SPCAM_Y_ARCMIN);
  cairo_text_extents (cr,tmp, &extents);
  cairo_move_to(cr,-extents.width/2,
		-((gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*SPCAM_Y_ARCMIN)/2.-5*scale);
  cairo_show_text(cr, tmp);
  if(tmp) g_free(tmp);
  
  if(hg->dss_draw_slit){
    if(hg->dss_invert) cairo_set_source_rgba(cr, 0.6, 0.0, 0.0, 0.3);
    else cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 0.3);
    cairo_set_line_width (cr, 1.5*scale);
    
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
    
    if(hg->dss_invert) cairo_set_source_rgba(cr, 0.6, 0.0, 0.0, 1.0);
    else cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 1.0);
    cairo_select_font_face (cr, hg->fontfamily_all, CAIRO_FONT_SLANT_NORMAL,
			    CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size (cr, (gdouble)hg->skymon_allsz*0.9*scale);
    cairo_text_extents (cr,"2. fio", &extents);
    
    //2 fio
    cairo_move_to(cr,-x_ccd/2.+15*scale,-y_ccd-gap_ccd/2.+15*scale+extents.height);
    cairo_show_text(cr,"2. fio");
    
    //5 satsuki
    cairo_move_to(cr,-x_ccd/2.+15*scale,+gap_ccd/2.+y_ccd-15*scale);
    cairo_show_text(cr,"5. satsuki");
    
    //7 clarisse
    cairo_move_to(cr,-x_ccd/2*3.-gap_ccd+15*scale,-y_ccd-gap_ccd/2.+15*scale+extents.height);
    cairo_show_text(cr,"7. clarisse");
    
    //9 san
    cairo_move_to(cr,-x_ccd/2.*3.-gap_ccd+15*scale,+gap_ccd/2.+y_ccd-15*scale);
    cairo_show_text(cr,"9. san");
    
    //6 chihiro
    cairo_move_to(cr,-x_ccd/2*5.-gap_ccd*2.+15*scale,-y_ccd-gap_ccd/2.+15*scale+extents.height);
    cairo_show_text(cr,"6. chihiro");
    
    //8 ponyo
    cairo_move_to(cr,-x_ccd/2.*5.-gap_ccd*2.+15*scale,+gap_ccd/2.+y_ccd-15*scale);
    cairo_show_text(cr,"8. ponyo");
    
    //1 kiki
    cairo_move_to(cr,x_ccd/2.+gap_ccd+15*scale,-y_ccd-gap_ccd/2.+15*scale+extents.height);
    cairo_show_text(cr,"1. kiki");
    
    //4 sheeta
    cairo_move_to(cr,x_ccd/2.+gap_ccd+15*scale,+gap_ccd/2.+y_ccd-15*scale);
    cairo_show_text(cr,"4. sheeta");
    
    //0 nausicaa
    cairo_move_to(cr,x_ccd/2.*3.+gap_ccd*2.+15*scale,-y_ccd-gap_ccd/2.+15*scale+extents.height);
    cairo_show_text(cr,"0. nausicaa");
    
    //3 sophie
    cairo_move_to(cr,x_ccd/2.*3.+gap_ccd*2+15*scale,+gap_ccd/2.+y_ccd-15*scale);
    cairo_show_text(cr,"3. sophie");
  }
}


void draw_hsc(typHOE *hg,
	      cairo_t *cr,
	      gint width, gint height,
	      gint width_file, gint height_file,
	      gdouble scale,
	      gdouble r)
{ // Drawing Inst (HSC)
  cairo_text_extents_t extents;
  gchar *tmp;

  cairo_translate(cr,((gdouble)width_file*r)/2,((gdouble)height_file*r)/2);

  if(flagHSCDialog){
    translate_hsc_offset(cr, hg, width_file, r);
    translate_hsc_dith(cr, hg, width_file, r, hg->hsc_show_dith_i, FALSE);
  }
  
  if(hg->dss_invert) cairo_set_source_rgba(cr, 0.6, 0.0, 0.0, 0.6);
  else cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 0.6);
  cairo_set_line_width (cr, 3.0*scale);
  
  cairo_arc(cr,0,0,
	    ((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip/2.*HSC_R_ARCMIN,
	    0,M_PI*2);
  cairo_stroke(cr);
  
  if(hg->dss_invert) cairo_set_source_rgba(cr, 0.6, 0.0, 0.0, 1.0);
  else cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 1.0);
  cairo_select_font_face (cr, hg->fontfamily_all, CAIRO_FONT_SLANT_NORMAL,
			  CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size (cr, (gdouble)hg->skymon_allsz*0.9*scale);
  
  if(!hg->dss_draw_slit){
    tmp=g_strdup_printf("HSC FOV (%darcmin)",HSC_R_ARCMIN);
    cairo_text_extents (cr,tmp, &extents);
    cairo_move_to(cr,
		  -extents.width/2,
		  -HSC_R_ARCMIN/2.*((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip)-5*scale);
    cairo_show_text(cr, tmp);
    if(tmp) g_free(tmp);
  }
  else{
    gint i_chip;
    gdouble pscale;
    gdouble x_0,y_0;
    
    pscale=(1.5*60.*60./(497./0.015))/60.*((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip;
    // HSC pix scale 1.5deg = 497mm phi
    
    if(hg->dss_invert) cairo_set_source_rgba(cr, 0.6, 0.0, 0.0, 0.6);
    else cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 0.6);
    cairo_set_line_width (cr, 0.8*scale);
    
    cairo_select_font_face (cr, hg->fontfamily_all, CAIRO_FONT_SLANT_NORMAL,
			    CAIRO_FONT_WEIGHT_BOLD);
    
    // Dead chips
    {
      gint i_dead;
      if(hg->dss_invert) cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.3);
      else cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.3);
      
      for(i_dead=0;i_dead<HSC_DEAD_ALL;i_dead++){
	
	y_0=(-(gdouble)hsc_dead[i_dead].crpix1*(gdouble)hsc_dead[i_dead].cd1_1/0.015-(gdouble)hsc_dead[i_dead].crpix2*(gdouble)hsc_dead[i_dead].cd1_2/0.015)*pscale;
	x_0=(-(gdouble)hsc_dead[i_dead].crpix1*(gdouble)hsc_dead[i_dead].cd2_1/0.015-(gdouble)hsc_dead[i_dead].crpix2*(gdouble)hsc_dead[i_dead].cd2_2/0.015)*pscale;
	if((hsc_dead[i_dead].cd1_2<0)&&(hsc_dead[i_dead].cd2_1<0)){
	  cairo_rectangle(cr, x_0-2048*pscale/4*(hsc_dead[i_dead].ch),
			  y_0-4224*pscale, 2048*pscale/4, 4224*pscale );
	}
	else if((hsc_dead[i_dead].cd1_2>0)&&(hsc_dead[i_dead].cd2_1>0)){
	  cairo_rectangle(cr,x_0+2048*pscale/4*(hsc_dead[i_dead].ch-1), y_0, 2048*pscale/4, 4224*pscale);
	}
	else if((hsc_dead[i_dead].cd1_1>0)&&(hsc_dead[i_dead].cd2_2<0)){
	  cairo_rectangle(cr,x_0-4224*pscale, y_0+2048*pscale/4*(hsc_dead[i_dead].ch-1),  4224*pscale, 2048*pscale/4);
	}
	else{
	  cairo_rectangle(cr,x_0, y_0-2048*pscale/4*(hsc_dead[i_dead].ch), 4224*pscale, 2048*pscale/4);
	}
	cairo_fill(cr);
      }
    }
    
    for(i_chip=0;i_chip<HSC_CHIP_ALL;i_chip++){
      
      if(hsc_chips[i_chip].bees==2){
	if(hg->dss_invert) cairo_set_source_rgba(cr, 0.0, 0.6, 0.0, 0.6);
	else cairo_set_source_rgba(cr, 0.4, 1.0, 0.4, 0.6);
      }
      else if(hsc_chips[i_chip].bees==0){
	if(hg->dss_invert) cairo_set_source_rgba(cr, 0.5, 0.0, 0.5, 0.6);
	else cairo_set_source_rgba(cr, 0.8, 0.4, 0.8, 0.6);
      }
      else{
	if(hg->dss_invert) cairo_set_source_rgba(cr, 0.6, 0.0, 0.0, 0.6);
	else cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 0.6);
      }
      
      cairo_set_font_size (cr, 600*pscale);
      cairo_text_extents (cr,"000", &extents);
      
      y_0=(-(gdouble)hsc_chips[i_chip].crpix1*(gdouble)hsc_chips[i_chip].cd1_1/0.015-(gdouble)hsc_chips[i_chip].crpix2*(gdouble)hsc_chips[i_chip].cd1_2/0.015)*pscale;
      x_0=(-(gdouble)hsc_chips[i_chip].crpix1*(gdouble)hsc_chips[i_chip].cd2_1/0.015-(gdouble)hsc_chips[i_chip].crpix2*(gdouble)hsc_chips[i_chip].cd2_2/0.015)*pscale;
      
      if((hsc_chips[i_chip].cd1_2<0)&&(hsc_chips[i_chip].cd2_1<0)){
	cairo_rectangle(cr, x_0-2048*pscale, y_0-4224*pscale, 2048*pscale, 4224*pscale );
	cairo_move_to(cr, x_0-2048*pscale+2044*pscale*0.05, y_0-4224*pscale+2044*pscale*0.05-extents.y_bearing);
      }
      else if((hsc_chips[i_chip].cd1_2>0)&&(hsc_chips[i_chip].cd2_1>0)){
	cairo_rectangle(cr,x_0, y_0, 2048*pscale, 4224*pscale);
	cairo_move_to(cr, x_0+2048*pscale*0.05, y_0+2048*pscale*0.05-extents.y_bearing);
      }
      else if((hsc_chips[i_chip].cd1_1>0)&&(hsc_chips[i_chip].cd2_2<0)){
	cairo_rectangle(cr,x_0-4224*pscale, y_0,  4224*pscale, 2048*pscale);
	cairo_move_to(cr, x_0-4224*pscale+2048*pscale*0.05, y_0+2048*pscale*0.05-extents.y_bearing);
      }
      else{
	cairo_rectangle(cr,x_0, y_0-2048*pscale, 4224*pscale, 2048*pscale );
	cairo_move_to(cr, x_0+2048*pscale*0.05, y_0-2048*pscale+2048*pscale*0.05-extents.y_bearing);
      }
      
      cairo_set_font_size (cr, 600*pscale);
      if(hg->fc_inst==FC_INST_HSCDET){
	tmp=g_strdup_printf("%d",hsc_chips[i_chip].det_id);
      }
      else{
	
	tmp=g_strdup_printf("%02d",hsc_chips[i_chip].hsca);
      }
      cairo_show_text(cr,tmp);
      if(tmp) g_free(tmp);
      
      if(hsc_chips[i_chip].hsca==35){
	cairo_set_font_size (cr, 1600*pscale);
	tmp=g_strdup_printf("BEES%d",hsc_chips[i_chip].bees);
	cairo_text_extents (cr,tmp, &extents);
	
	if(hsc_chips[i_chip].bees==0){
	  cairo_move_to(cr, x_0+4224*pscale-2048*pscale*0.5-extents.width, y_0+2048*pscale*0.2-extents.y_bearing);
	}
	else{
	  cairo_move_to(cr, x_0-4224*pscale+2048*pscale*0.5, y_0-2048*pscale*0.2);
	}
	cairo_show_text(cr,tmp);
	if(tmp) g_free(tmp);
      }
      
      cairo_stroke(cr);
      
    }
  }
}


void draw_hsc_ol(typHOE *hg,
		 cairo_t *cr,
		 gint width, gint height,
		 gint width_file, gint height_file,
		 gdouble scale,
		 gdouble r)
{ // Drawing Inst (HSC) dithering area overlay
  cairo_text_extents_t extents;
  gchar *tmp;
  gint i_chip, i_cnl, i_dead, i_dith;;
  gdouble pscale;
  gdouble x_0,y_0;
  gboolean ok_ch[4], has_dead;
  gboolean flag_draw;


  cairo_translate(cr,((gdouble)width_file*r)/2,((gdouble)height_file*r)/2);
  pscale=(1.5*60.*60./(497./0.015))/60.*((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip;
  // HSC pix scale 1.5deg = 497mm phi
    
  switch(hg->hsc_show_dith_p){
  case HSC_DITH_NO:
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
    break;

  case HSC_DITH_5:
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0,
			  1.0/5.0);
    break;

  case HSC_DITH_N:
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0,
			  1.0/(double)hg->hsc_show_dith_n);
    break;
  }
  
  translate_hsc_offset(cr, hg, width_file, r);

  for(i_dith=0;i_dith<hg->hsc_show_dith_i;i_dith++){
    translate_hsc_dith(cr, hg, width_file, r, i_dith+1, FALSE);
    
    for(i_chip=0;i_chip<HSC_CHIP_ALL;i_chip++){
      // Dead channel check
      for(i_cnl=0;i_cnl<4;i_cnl++){
	ok_ch[i_cnl]=TRUE;
	has_dead=FALSE;
      }
      
      for(i_dead=0;i_dead<HSC_DEAD_ALL;i_dead++){
	if(hsc_chips[i_chip].hsca==hsc_dead[i_dead].hsca){
	  if(hsc_chips[i_chip].bees==hsc_dead[i_dead].bees){
	    ok_ch[hsc_dead[i_dead].ch-1]=FALSE;
	    has_dead=TRUE;
	  }
	}
      }
      
      if(hsc_chips[i_chip].bees==2){ // Guide Chip
	flag_draw=FALSE;
      }
      else if(hsc_chips[i_chip].bees==0){
	switch(hsc_chips[i_chip].hsca){ // Focusing chips
	case 48:
	case 51:
	case 52:
	case 57:
	  flag_draw=FALSE;
	  break;
	  
	default:
	  flag_draw=TRUE;
	  break;
	}
      }
      else{
	switch(hsc_chips[i_chip].hsca){ // Focusing chips
	case 48:
	case 51:
	case 52:
	case 57:
	  flag_draw=FALSE;
	  break;
	  
	default:
	  flag_draw=TRUE;
	  break;
	}
      }
      
      y_0=(-(gdouble)hsc_chips[i_chip].crpix1*(gdouble)hsc_chips[i_chip].cd1_1/0.015-(gdouble)hsc_chips[i_chip].crpix2*(gdouble)hsc_chips[i_chip].cd1_2/0.015)*pscale;
      x_0=(-(gdouble)hsc_chips[i_chip].crpix1*(gdouble)hsc_chips[i_chip].cd2_1/0.015-(gdouble)hsc_chips[i_chip].crpix2*(gdouble)hsc_chips[i_chip].cd2_2/0.015)*pscale;
      
      if(!has_dead){
	if((hsc_chips[i_chip].cd1_2<0)&&(hsc_chips[i_chip].cd2_1<0)){
	  cairo_rectangle(cr, x_0-2048*pscale, y_0-4224*pscale, 2048*pscale, 4224*pscale );
	  cairo_move_to(cr, x_0-2048*pscale+2044*pscale*0.05, y_0-4224*pscale+2044*pscale*0.05-extents.y_bearing);
	}
	else if((hsc_chips[i_chip].cd1_2>0)&&(hsc_chips[i_chip].cd2_1>0)){
	  cairo_rectangle(cr,x_0, y_0, 2048*pscale, 4224*pscale);
	  cairo_move_to(cr, x_0+2048*pscale*0.05, y_0+2048*pscale*0.05-extents.y_bearing);
	}
	else if((hsc_chips[i_chip].cd1_1>0)&&(hsc_chips[i_chip].cd2_2<0)){
	  cairo_rectangle(cr,x_0-4224*pscale, y_0,  4224*pscale, 2048*pscale);
	  cairo_move_to(cr, x_0-4224*pscale+2048*pscale*0.05, y_0+2048*pscale*0.05-extents.y_bearing);
	}
	else{
	  cairo_rectangle(cr,x_0, y_0-2048*pscale, 4224*pscale, 2048*pscale );
	  cairo_move_to(cr, x_0+2048*pscale*0.05, y_0-2048*pscale+2048*pscale*0.05-extents.y_bearing);
	}
	cairo_fill(cr);
      } // has_dead
      else{
	for(i_cnl=0;i_cnl<4;i_cnl++){
	  if(ok_ch[i_cnl]){
	    if((hsc_chips[i_chip].cd1_2<0)&&(hsc_chips[i_chip].cd2_1<0)){
	      cairo_rectangle(cr, x_0-2048*pscale/4*(i_cnl+1),
			      y_0-4224*pscale, 2048*pscale/4, 4224*pscale );
	    }
	    else if((hsc_chips[i_chip].cd1_2>0)&&(hsc_chips[i_chip].cd2_1>0)){
	      cairo_rectangle(cr,x_0+2048*pscale/4*(i_cnl+1-1), y_0, 2048*pscale/4, 4224*pscale);
	    }
	    else if((hsc_chips[i_chip].cd1_1>0)&&(hsc_chips[i_chip].cd2_2<0)){
	      cairo_rectangle(cr,x_0-4224*pscale, y_0+2048*pscale/4*(i_cnl+1-1),  4224*pscale, 2048*pscale/4);
	    }
	    else{
	      cairo_rectangle(cr,x_0, y_0-2048*pscale/4*(i_cnl+1), 4224*pscale, 2048*pscale/4);
	    }
	    cairo_fill(cr);
	  }
	}
      }
    }
    // Back to dith center
    translate_hsc_dith(cr, hg, width_file, r, i_dith+1, TRUE);
  }
}


void draw_fmos(typHOE *hg,
	       cairo_t *cr,
	       gint width, gint height,
	       gint width_file, gint height_file,
	       gdouble scale,
	       gdouble r)
{ // Drawing Inst (FMOS)
  cairo_text_extents_t extents;
  gchar *tmp;
  
  cairo_translate(cr,((gdouble)width_file*r)/2,((gdouble)height_file*r)/2);
  
  if(hg->dss_invert) cairo_set_source_rgba(cr, 0.6, 0.0, 0.0, 0.6);
  else cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 0.6);
  cairo_set_line_width (cr, 3.0*scale);
  
  cairo_arc(cr,0,0,
	    ((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip/2.*FMOS_R_ARCMIN,
	    0,M_PI*2);
  cairo_stroke(cr);
  
  if(hg->dss_invert) cairo_set_source_rgba(cr, 0.6, 0.0, 0.0, 1.0);
  else cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 1.0);
  cairo_select_font_face (cr, hg->fontfamily_all, CAIRO_FONT_SLANT_NORMAL,
			  CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size (cr, (gdouble)hg->skymon_allsz*0.9*scale);
  
  tmp=g_strdup_printf("FMOS FOV (%darcmin)",FMOS_R_ARCMIN);
  cairo_text_extents (cr,tmp, &extents);
  cairo_move_to(cr,
		-extents.width/2,
		-FMOS_R_ARCMIN/2.*((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip)-5*scale);
  cairo_show_text(cr, tmp);
  if(tmp) g_free(tmp);
}


void draw_pfs(typHOE *hg,
	      cairo_t *cr,
	      gint width, gint height,
	      gint width_file, gint height_file,
	      gdouble scale,
	      gdouble r)
{ // Drawing Inst (PFS)
  cairo_text_extents_t extents;
  gchar *tmp;

  cairo_translate(cr,((gdouble)width_file*r)/2,((gdouble)height_file*r)/2);
  
  if(hg->dss_invert) cairo_set_source_rgba(cr, 0.6, 0.0, 0.0, 0.6);
  else cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 0.6);
  cairo_set_line_width (cr, 3.0*scale);
  
  cairo_arc(cr,0,0,
	    ((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip/2.*PFS_R_ARCMIN,
	    0,M_PI*2);
  cairo_stroke(cr);
  
  if(hg->dss_invert) cairo_set_source_rgba(cr, 0.6, 0.0, 0.0, 1.0);
  else cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 1.0);
  cairo_select_font_face (cr, hg->fontfamily_all, CAIRO_FONT_SLANT_NORMAL,
			  CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size (cr, (gdouble)hg->skymon_allsz*0.9*scale);
  
  tmp=g_strdup_printf("PFS FOV (%.1farcmin)",PFS_R_ARCMIN);
  cairo_text_extents (cr,tmp, &extents);
  cairo_move_to(cr,
		-extents.width/2,
		-PFS_R_ARCMIN/2.*((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip)-5*scale);
  cairo_show_text(cr, tmp);
  if(tmp) g_free(tmp);
}


void draw_kools(typHOE *hg,
		cairo_t *cr,
		gint width, gint height,
		gint width_file, gint height_file,
		gdouble scale,
		gdouble r)
{ // Drawing Inst (ZWO-Cam for KOOLS-IFU)
  cairo_text_extents_t extents;
  gchar *tmp;

  cairo_translate(cr,((gdouble)width_file*r)/2,((gdouble)height_file*r)/2);
  
  if(hg->dss_invert) cairo_set_source_rgba(cr, 0.6, 0.0, 0.0, 0.6);
  else cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 0.6);

  cairo_set_line_width (cr, 3.0*scale);
  cairo_rectangle(cr,
		  -((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*ZWOCAM_X_ARCSEC/60.)/2.,
		  -((gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*ZWOCAM_Y_ARCSEC/60.)/2.,
		  (gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*ZWOCAM_X_ARCSEC/60.,
		  (gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*ZWOCAM_Y_ARCSEC/60.);
  cairo_stroke(cr);

  /*
  cairo_set_line_width (cr, 3.0*scale);
  cairo_rectangle(cr,
		  -((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*KOOLS_X_ARCSEC/60.)/2.,
		  -((gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*KOOLS_Y_ARCSEC/60.)/2.,
		  (gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*KOOLS_X_ARCSEC/60.,
		  (gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*KOOLS_Y_ARCSEC/60.);
  cairo_stroke(cr);
  */

  if(hg->dss_draw_slit){
    // Reticle
    cairo_set_line_width (cr, 1.5*scale);

    cairo_move_to(cr,0,
		  -((gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*ZWOCAM_Y_ARCSEC/60.)/2.);
    cairo_line_to(cr,0,
		  -((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip/2.*ZWOCAM_RETICLE1_ARCSEC/60.);
    cairo_stroke(cr);

    cairo_line_to(cr,0,
		  +((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip/2.*ZWOCAM_RETICLE1_ARCSEC/60.);
    cairo_line_to(cr,0,
		  +((gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*ZWOCAM_Y_ARCSEC/60.)/2.);
    cairo_stroke(cr);

    

    cairo_move_to(cr,-((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*ZWOCAM_X_ARCSEC/60.)/2.,
		  0);
    cairo_line_to(cr,-((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip/2.*ZWOCAM_RETICLE1_ARCSEC/60.,
		  0);
    cairo_stroke(cr);
    
    cairo_move_to(cr,+((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip/2.*ZWOCAM_RETICLE1_ARCSEC/60.,
		  0);
    cairo_line_to(cr,+((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*ZWOCAM_X_ARCSEC/60.)/2.,
		  0);
    cairo_stroke(cr);

    
    cairo_arc(cr,0,0,
	      ((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip/2.*ZWOCAM_RETICLE1_ARCSEC/60.,
	      0,M_PI*2);
    cairo_stroke(cr);
    cairo_arc(cr,0,0,
	      ((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip/2.*ZWOCAM_RETICLE2_ARCSEC/60.,
	      0,M_PI*2);
    cairo_stroke(cr);
    cairo_arc(cr,0,0,
	      ((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip/2.*ZWOCAM_RETICLE3_ARCSEC/60.,
	      0,M_PI*2);
    cairo_stroke(cr);
  }
  
  
  if(hg->dss_invert) cairo_set_source_rgba(cr, 0.6, 0.0, 0.0, 1.0);
  else cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 1.0);
  cairo_select_font_face (cr, hg->fontfamily_all, CAIRO_FONT_SLANT_NORMAL,
			  CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size (cr, (gdouble)hg->skymon_allsz*0.9*scale);

  /*
  tmp=g_strdup_printf("KOOLS-IFU FOV (%.1lfx%.1lfarcsec)",KOOLS_X_ARCSEC, KOOLS_Y_ARCSEC);
  cairo_text_extents (cr,tmp, &extents);
  cairo_move_to(cr,-extents.width/2,
		-((gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*KOOLS_Y_ARCSEC/60.)/2.-5*scale);
  cairo_show_text(cr, tmp);
  if(tmp) g_free(tmp);
  */
 
  tmp=g_strdup_printf("ZWO-Cam FOV (%dx%darcsec)",(gint)ZWOCAM_X_ARCSEC, (gint)ZWOCAM_Y_ARCSEC);
  cairo_text_extents (cr,tmp, &extents);
  cairo_move_to(cr,-extents.width/2,
		-((gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*ZWOCAM_Y_ARCSEC/60.)/2.-5*scale);
  cairo_show_text(cr, tmp);
  if(tmp) g_free(tmp);

}


void draw_triccs(typHOE *hg,
		 cairo_t *cr,
		 gint width, gint height,
		 gint width_file, gint height_file,
		 gdouble scale,
		 gdouble r)
{ // Drawing Inst (TriCCS)
  cairo_text_extents_t extents;
  gchar *tmp;

  cairo_translate(cr,((gdouble)width_file*r)/2,((gdouble)height_file*r)/2);
  
  if(hg->dss_invert) cairo_set_source_rgba(cr, 0.6, 0.0, 0.0, 0.6);
  else cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 0.6);
  cairo_set_line_width (cr, 3.0*scale);
  
  cairo_rectangle(cr,
		  -((gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*TRICCS_X_ARCMIN)/2.,
		  -((gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*TRICCS_Y_ARCMIN)/2.,
		  (gdouble)width_file*r/(gdouble)hg->dss_arcmin_ip*TRICCS_X_ARCMIN,
		  (gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*TRICCS_Y_ARCMIN);
  cairo_stroke(cr);
  
  if(hg->dss_invert) cairo_set_source_rgba(cr, 0.6, 0.0, 0.0, 1.0);
  else cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 1.0);
  cairo_select_font_face (cr, hg->fontfamily_all, CAIRO_FONT_SLANT_NORMAL,
			  CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size (cr, (gdouble)hg->skymon_allsz*0.9*scale);
  
  tmp=g_strdup_printf("TriCCS FOV (%.1lfx%.1lfarcmin)",TRICCS_X_ARCMIN, TRICCS_Y_ARCMIN);
  cairo_text_extents (cr,tmp, &extents);
  cairo_move_to(cr,-extents.width/2,
		-((gdouble)height_file*r/(gdouble)hg->dss_arcmin_ip*TRICCS_Y_ARCMIN)/2.-5*scale);
  cairo_show_text(cr, tmp);
  if(tmp) g_free(tmp);
  
}


void draw_fc_label(typHOE *hg,
		   cairo_t *cr,
		   gint width, gint height,
		   gint width_file, gint height_file,
		   gdouble scale,
		   gdouble r)
{ // Drawing Labels  (Object Name,  RA/Dec,  Image Source)
  struct ln_equ_posn object;
  struct lnh_equ_posn hobject;
  cairo_text_extents_t extents;
  gchar *tmp;
  gdouble yrs;

  cairo_translate (cr, (width-(gint)((gdouble)width_file*r))/2,
		   (height-(gint)((gdouble)height_file*r))/2);
  
  yrs=current_yrs(hg);
  object.ra=ra_to_deg(hg->obj[hg->dss_i].ra)+
    hg->obj[hg->dss_i].pm_ra/1000/60/60*yrs;
  object.dec=dec_to_deg(hg->obj[hg->dss_i].dec)+
    hg->obj[hg->dss_i].pm_dec/1000/60/60*yrs; 
  //object.ra=ra_to_deg(hg->obj[hg->dss_i].ra);
  //object.dec=dec_to_deg(hg->obj[hg->dss_i].dec);
  
  ln_equ_to_hequ(&object, &hobject);
  
  if(hg->dss_invert) cairo_set_source_rgba(cr, 0.3, 0.45, 0.0, 1.0);
  else cairo_set_source_rgba(cr, 1.0, 1.0, 0.4, 1.0);
  cairo_select_font_face (cr, hg->fontfamily_all, CAIRO_FONT_SLANT_NORMAL,
			  CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size (cr, (gdouble)hg->skymon_allsz*1.1*scale);
  cairo_move_to(cr,5*scale,(gdouble)height_file*r-5*scale);
  tmp=g_strdup_printf("RA=%02d:%02d:%05.2lf  Dec=%s%02d:%02d:%05.2lf (%.1lf)",
		      hobject.ra.hours,hobject.ra.minutes,
		      hobject.ra.seconds,
		      (hobject.dec.neg) ? "-" : "+", 
		      hobject.dec.degrees, hobject.dec.minutes,
		      hobject.dec.seconds,
		      hg->obj[hg->dss_i].equinox);
  cairo_text_extents (cr, tmp, &extents);
  cairo_show_text(cr,tmp);
  if(tmp) g_free(tmp);
  
  cairo_select_font_face (cr, hg->fontfamily_all, CAIRO_FONT_SLANT_NORMAL,
			  CAIRO_FONT_WEIGHT_BOLD);
  cairo_move_to(cr,5*scale,(gdouble)height_file*r-5*scale-extents.height-5*scale);
  cairo_show_text(cr,hg->obj[hg->dss_i].name);
  
  
  if(hg->dss_invert) cairo_set_source_rgba(cr, 0.6, 0.0, 0.0, 1.0);
  else cairo_set_source_rgba(cr, 1.0, 0.4, 0.4, 1.0);
  cairo_select_font_face (cr, hg->fontfamily_all, CAIRO_FONT_SLANT_NORMAL,
			  CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size (cr, (gdouble)hg->skymon_allsz*scale);
  
  tmp=g_strdup_printf("%s  %dx%d arcmin",
		      FC_img[hg->fc_mode_get],
		      hg->dss_arcmin_ip,hg->dss_arcmin_ip);
  
  cairo_text_extents (cr, tmp, &extents);
  cairo_move_to(cr,
		(gdouble)width_file*r-extents.width-5*scale,
		extents.height+5*scale);
  cairo_show_text(cr,tmp);
  if(tmp) g_free(tmp);
}
    


void draw_hsc_dither(typHOE *hg,
		     cairo_t *cr,
		     gint width, gint height,
		     gint width_file, gint height_file,
		     gdouble scale,
		     gdouble r)
{ // Drawing HSC Dithering Positions
  cairo_text_extents_t extents;
  gchar *tmp;
  gint i;
  double x0, y0, dra, ddec, theta;
  
  translate_to_center(hg,cr,width,height,width_file,height_file,r);
  
  // Dithering
  
  switch(hg->hsc_show_dith_p){
  case HSC_DITH_NO:
    break;
    
  case HSC_DITH_5:
    dra=((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip
      *(gdouble)hg->hsc_show_dith_ra/60.;
    ddec=((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip
      *(gdouble)hg->hsc_show_dith_dec/60.;
    
    // 1
    if(hg->hsc_show_dith_i==1){
      if(hg->dss_invert) cairo_set_source_rgba(cr, 0.5, 0.5, 0.0, 1.0);
      else cairo_set_source_rgba(cr, 1.0, 0.7, 0.2, 1.0);
      cairo_set_line_width (cr, 2.5*scale);
    }
    else{
      if(hg->dss_invert) cairo_set_source_rgba(cr, 0.5, 0.5, 0.0, 0.5);
      else cairo_set_source_rgba(cr, 1.0, 1.0, 0.2, 0.5);
      cairo_set_line_width (cr, 1.5*scale);
    }
    cairo_move_to(cr, 0, 0);
    cairo_rel_move_to(cr, (hg->hsc_show_dith_i==1) ? -7.5 : -5, 0);
    cairo_rel_line_to(cr, (hg->hsc_show_dith_i==1) ? 15 : 10, 0);
    cairo_stroke(cr);
    cairo_move_to(cr, 0, 0);
    cairo_rel_move_to(cr, 0, (hg->hsc_show_dith_i==1) ? -7.5 : -5);
    cairo_rel_line_to(cr, 0, (hg->hsc_show_dith_i==1) ? 15: 10);
    cairo_stroke(cr);
    
    // 2
    if(hg->hsc_show_dith_i==2){
      if(hg->dss_invert) cairo_set_source_rgba(cr, 0.5, 0.5, 0.0, 1.0);
      else cairo_set_source_rgba(cr, 1.0, 0.7, 0.2, 1.0);
      cairo_set_line_width (cr, 2.5*scale);
    }
    else{
      if(hg->dss_invert) cairo_set_source_rgba(cr, 0.5, 0.5, 0.0, 0.5);
      else cairo_set_source_rgba(cr, 1.0, 1.0, 0.2, 0.5);
      cairo_set_line_width (cr, 1.5*scale);
    }
    cairo_move_to(cr, -dra*1, +ddec*2);
    cairo_rel_move_to(cr, (hg->hsc_show_dith_i==2) ? -7.5 : -5, 0);
    cairo_rel_line_to(cr, (hg->hsc_show_dith_i==2) ? 15 : 10, 0);
    cairo_stroke(cr);
    cairo_move_to(cr, -dra*1, +ddec*2);
    cairo_rel_move_to(cr, 0, (hg->hsc_show_dith_i==2) ? -7.5 : -5);
    cairo_rel_line_to(cr, 0, (hg->hsc_show_dith_i==2) ? 15: 10);
    cairo_stroke(cr);
      
    // 3
    if(hg->hsc_show_dith_i==3){
      if(hg->dss_invert) cairo_set_source_rgba(cr, 0.5, 0.5, 0.0, 1.0);
      else cairo_set_source_rgba(cr, 1.0, 0.7, 0.2, 1.0);
      cairo_set_line_width (cr, 2.5*scale);
    }
    else{
      if(hg->dss_invert) cairo_set_source_rgba(cr, 0.5, 0.5, 0.0, 0.5);
      else cairo_set_source_rgba(cr, 1.0, 1.0, 0.2, 0.5);
      cairo_set_line_width (cr, 1.5*scale);
    }
    cairo_move_to(cr, -dra*2, -ddec*1);
    cairo_rel_move_to(cr, (hg->hsc_show_dith_i==3) ? -7.5 : -5, 0);
    cairo_rel_line_to(cr, (hg->hsc_show_dith_i==3) ? 15 : 10, 0);
    cairo_stroke(cr);
    cairo_move_to(cr, -dra*2, -ddec*1);
    cairo_rel_move_to(cr, 0, (hg->hsc_show_dith_i==3) ? -7.5 : -5);
    cairo_rel_line_to(cr, 0, (hg->hsc_show_dith_i==3) ? 15 : 10);
    cairo_stroke(cr);
    
    // 4
    if(hg->hsc_show_dith_i==4){
      if(hg->dss_invert) cairo_set_source_rgba(cr, 0.5, 0.5, 0.0, 1.0);
      else cairo_set_source_rgba(cr, 1.0, 0.7, 0.2, 1.0);
      cairo_set_line_width (cr, 2.5*scale);
    }
    else{
      if(hg->dss_invert) cairo_set_source_rgba(cr, 0.5, 0.5, 0.0, 0.5);
      else cairo_set_source_rgba(cr, 1.0, 1.0, 0.2, 0.5);
      cairo_set_line_width (cr, 1.5*scale);
    }
    cairo_move_to(cr, +dra*1, -ddec*2);
    cairo_rel_move_to(cr, (hg->hsc_show_dith_i==4) ? -7.5 : -5, 0);
    cairo_rel_line_to(cr, (hg->hsc_show_dith_i==4) ? 15 : 10, 0);
    cairo_stroke(cr);
    cairo_move_to(cr, +dra*1, -ddec*2);
    cairo_rel_move_to(cr, 0, (hg->hsc_show_dith_i==4) ? -7.5 : -5);
    cairo_rel_line_to(cr, 0, (hg->hsc_show_dith_i==4) ? 15 : 10);
    cairo_stroke(cr);
    
    // 5
    if(hg->hsc_show_dith_i==5){
      if(hg->dss_invert) cairo_set_source_rgba(cr, 0.5, 0.5, 0.0, 1.0);
      else cairo_set_source_rgba(cr, 1.0, 0.7, 0.2, 1.0);
      cairo_set_line_width (cr, 2.5*scale);
    }
    else{
      if(hg->dss_invert) cairo_set_source_rgba(cr, 0.5, 0.5, 0.0, 0.5);
      else cairo_set_source_rgba(cr, 1.0, 1.0, 0.2, 0.5);
      cairo_set_line_width (cr, 1.5*scale);
    }
    cairo_move_to(cr, +dra*2, +ddec*1);
    cairo_rel_move_to(cr, (hg->hsc_show_dith_i==5) ? -7.5 : -5, 0);
    cairo_rel_line_to(cr, (hg->hsc_show_dith_i==5) ? 15 : 10, 0);
    cairo_stroke(cr);
    cairo_move_to(cr, +dra*2, +ddec*1);
    cairo_rel_move_to(cr, 0, (hg->hsc_show_dith_i==5) ? -7.5 : -5);
    cairo_rel_line_to(cr, 0, (hg->hsc_show_dith_i==5) ? 15 : 10);
    cairo_stroke(cr);
    break;
    
  case HSC_DITH_N:
    for(i=0;i<hg->hsc_show_dith_n;i++){
      if(hg->hsc_show_dith_i==i+1){
	if(hg->dss_invert) cairo_set_source_rgba(cr, 0.5, 0.5, 0.0, 1.0);
	else cairo_set_source_rgba(cr, 1.0, 0.7, 0.2, 1.0);
	cairo_set_line_width (cr, 2.5*scale);
      }
      else{
	if(hg->dss_invert) cairo_set_source_rgba(cr, 0.5, 0.5, 0.0, 0.5);
	else cairo_set_source_rgba(cr, 1.0, 1.0, 0.2, 0.5);
	cairo_set_line_width (cr, 1.5*scale);
      }
      
      y0=((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip
	*(double)hg->hsc_show_dith_r/60*
	cos(-(double)hg->hsc_show_dith_t*M_PI/180
	    -2*M_PI/(double)hg->hsc_show_dith_n*(double)i-M_PI/2);
      x0=((gdouble)width_file*r)/(gdouble)hg->dss_arcmin_ip
	*(double)hg->hsc_show_dith_r/60*
	sin(-(double)hg->hsc_show_dith_t*M_PI/180
	    -2*M_PI/(double)hg->hsc_show_dith_n*(double)i-M_PI/2);
      
      cairo_move_to(cr, x0, y0);
      cairo_rel_move_to(cr, (hg->hsc_show_dith_i==i+1) ? -7.5 : -5, 0);
      cairo_rel_line_to(cr, (hg->hsc_show_dith_i==i+1) ? 15 : 10, 0);
      cairo_stroke(cr);
      cairo_move_to(cr, x0, y0);
      cairo_rel_move_to(cr, 0, (hg->hsc_show_dith_i==i+1) ? -7.5 : -5);
      cairo_rel_line_to(cr, 0, (hg->hsc_show_dith_i==i+1) ? 15 : 10);
      cairo_stroke(cr);
    }
    break;
  }
}


void draw_pa(typHOE *hg,
	     cairo_t *cr,
	     gint width, gint height,
	     gint width_file, gint height_file,
	     gint shift_x, gint shift_y,
	     gdouble scale,
	     gdouble r)
{ // Drawing Position Angle
  gdouble wh_small;
  gdouble xsec,ysec;
  gdouble pscale;
  cairo_text_extents_t extents;
  gchar *tmp;

  if(hg->dss_invert) cairo_set_source_rgba(cr, 0.5, 0.5, 0.0, 1.0);
  else cairo_set_source_rgba(cr, 1.0, 1.0, 0.2, 1.0);
  cairo_select_font_face (cr, hg->fontfamily_all, CAIRO_FONT_SLANT_NORMAL,
			  CAIRO_FONT_WEIGHT_BOLD);
  cairo_set_font_size (cr, (gdouble)hg->skymon_allsz*1.1*scale);
  cairo_text_extents (cr, "N", &extents);
  
  // Position Angle (w/o Magnificaion)
  if(hg->fc_mag==1){
    cairo_translate (cr, (width-(gint)((gdouble)width_file*r))/2,
		     (height-(gint)((gdouble)height_file*r))/2);
    cairo_translate (cr, 
		     5+(gdouble)width_file*r*0.05+extents.width*1.5,
		     5+(gdouble)width_file*r*0.05+extents.height*1.5);

    rot_pa(hg, cr);

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
    
    cairo_set_line_width (cr, 1.5*scale*hg->fc_mag);
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
      cairo_rel_move_to(cr,-extents.width/2.,extents.height+5*scale);
      cairo_show_text(cr,"(flipped)");
    }
  } // Position Angle (w/o Magnification)
  else{    // Position Angle (w/ Magnificaion)
    wh_small=(gdouble)(width>height?height:width)/(gdouble)hg->fc_mag;
    pscale=(gdouble)hg->dss_arcmin_ip*60./wh_small;
    xsec=(gdouble)width*pscale/(gdouble)hg->fc_mag/(gdouble)hg->fc_mag;
    ysec=(gdouble)height*pscale/(gdouble)hg->fc_mag/(gdouble)hg->fc_mag;
    
    if(hg->dss_invert) cairo_set_source_rgba(cr, 0.5, 0.5, 0.0, 1.0);
    else cairo_set_source_rgba(cr, 1.0, 1.0, 0.2, 1.0);
    cairo_select_font_face (cr, hg->fontfamily_all, CAIRO_FONT_SLANT_NORMAL,
			    CAIRO_FONT_WEIGHT_BOLD);
    
    cairo_set_font_size (cr, (gdouble)hg->skymon_allsz*1.4*scale);
    if((xsec>60.) && (ysec>60.)){
      tmp=g_strdup_printf("x%d : %.2lfx%.2lf arcmin",hg->fc_mag,
			  xsec/60.,
			  ysec/60.);
    }
    else{
      tmp=g_strdup_printf("x%d : %.1lfx%.1lf arcsec",hg->fc_mag,xsec,ysec);
    }
    
    // Edge for magnification (moved to top of this function)
    
    cairo_text_extents (cr, tmp, &extents);
    cairo_translate(cr,
		    width/(gdouble)hg->fc_mag-shift_x,
		    height/(gdouble)hg->fc_mag-shift_y);
    cairo_move_to(cr,
		  -extents.width-wh_small*0.02,
		  -wh_small*0.02);
    cairo_show_text(cr,tmp);
    if(tmp) g_free(tmp);
    
    cairo_translate(cr,
		    -width/(gdouble)hg->fc_mag,
		    -height/(gdouble)hg->fc_mag);
    
    cairo_select_font_face (cr, hg->fontfamily_all, CAIRO_FONT_SLANT_NORMAL,
			    CAIRO_FONT_WEIGHT_BOLD);
    
    cairo_set_font_size (cr, (gdouble)hg->skymon_allsz*1.1*scale);
    cairo_text_extents (cr, "N", &extents);
    
    if(hg->dss_invert) cairo_set_source_rgba(cr, 0.5, 0.5, 0.0, 1.0);
    else cairo_set_source_rgba(cr, 1.0, 1.0, 0.2, 1.0);
    
    cairo_translate(cr,
		    extents.height+wh_small*0.07,
		    extents.height+wh_small*0.07);
    
    rot_pa(hg, cr);
    
    cairo_move_to(cr,
		  -extents.width/2,
		  -wh_small*0.05);
    cairo_show_text(cr,"N");
    cairo_move_to(cr,
		  -wh_small*0.05-extents.width,
		  +extents.height/2);
    if(hg->dss_flip){
      cairo_show_text(cr,"W");
    }
    else{
      cairo_show_text(cr,"E");
    }
      
    cairo_set_line_width (cr, 1.5*scale);
    cairo_move_to(cr,
		  0,
		  -wh_small*0.05);
    cairo_line_to(cr, 0, 0);
    cairo_line_to(cr,
		  -wh_small*0.05, 0);
    
    cairo_stroke(cr);
    
    if(hg->dss_flip){
      cairo_move_to(cr,0,0);
      cairo_text_extents (cr, "(flipped)", &extents);
      cairo_rel_move_to(cr,-extents.width/2.,extents.height+5*scale);
      cairo_show_text(cr,"(flipped)");
    }
   }
}
  

//////////////////////////////////////////////////////////////////////
////////////// global functions
//////////////////////////////////////////////////////////////////////

gdouble current_yrs(typHOE *hg){
  double JD;
  struct ln_zonedate zonedate;

  zonedate.years=hg->fr_year;
  zonedate.months=hg->fr_month;
  zonedate.days=hg->fr_day;
  zonedate.hours=24;
  zonedate.minutes=0;
  zonedate.seconds=0;
  zonedate.gmtoff=(long)hg->obs_timezone*60;
  JD = ln_get_julian_local_date(&zonedate);
  return((JD-JD2000)/365.25);
}


void fc_item (GtkWidget *widget, gpointer data)
{
  typHOE *hg = (typHOE *)data;
  set_pa(hg);
  fc_item2(hg, FC_MODE_OBJ);
}

void fc_item_trdb (GtkWidget *widget, gpointer data)
{
  typHOE *hg = (typHOE *)data;
  
  switch(hg->trdb_used){
  case TRDB_TYPE_SMOKA:
    hg->fcdb_type=FCDB_TYPE_SMOKA;
    break;
  case TRDB_TYPE_HST:
    hg->fcdb_type=FCDB_TYPE_HST;
    break;
  case TRDB_TYPE_ESO:
    hg->fcdb_type=FCDB_TYPE_ESO;
    break;
  case TRDB_TYPE_GEMINI:
    hg->fcdb_type=FCDB_TYPE_GEMINI;
    break;
  case MAGDB_TYPE_SIMBAD:
    hg->fcdb_type=FCDB_TYPE_SIMBAD;
    break;
  case MAGDB_TYPE_NED:
    hg->fcdb_type=FCDB_TYPE_NED;
    break;
  case MAGDB_TYPE_LAMOST:
    hg->fcdb_type=FCDB_TYPE_LAMOST;
    break;
  case MAGDB_TYPE_GSC:
    hg->fcdb_type=FCDB_TYPE_GSC;
    break;
  case MAGDB_TYPE_PS1:
    hg->fcdb_type=FCDB_TYPE_PS1;
    break;
  case MAGDB_TYPE_SDSS:
    hg->fcdb_type=FCDB_TYPE_SDSS;
    break;
  case MAGDB_TYPE_GAIA:
    hg->fcdb_type=FCDB_TYPE_GAIA;
    break;
  case MAGDB_TYPE_KEPLER:
    hg->fcdb_type=FCDB_TYPE_KEPLER;
    break;
  case MAGDB_TYPE_2MASS:
    hg->fcdb_type=FCDB_TYPE_2MASS;
    break;
  default:
    break;
  }
  rebuild_fcdb_tree(hg);
  fc_item2(hg, FC_MODE_TRDB);
}


void fc_item_plan (GtkWidget *widget, gpointer data)
{
  GtkTreeIter iter;
  typHOE *hg = (typHOE *)data;
  GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(hg->plan_tree));
  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(hg->plan_tree));
  gboolean valid=FALSE;
  
  if (gtk_tree_selection_get_selected (selection, NULL, &iter)){
    gint i, i_list;
    GtkTreePath *path;
    
    
    path = gtk_tree_model_get_path (model, &iter);
    i = gtk_tree_path_get_indices (path)[0];

    hg->plot_i_plan=i;

    gtk_tree_path_free (path);

    if(hg->plan[hg->plot_i_plan].type==PLAN_TYPE_OBJ){
      hg->dss_i=hg->plan[hg->plot_i_plan].obj_i;
      set_pa(hg);
      fc_item2(hg, FC_MODE_PLAN);
      valid=TRUE;
    }
  }

  if(!valid){
    popup_message(hg->plan_main,
#ifdef USE_GTK3
		  "dialog-warning", 
#else
		  GTK_STOCK_DIALOG_WARNING,
#endif
		  POPUP_TIMEOUT,
		  "Please select an \"Object\" line in your plan.",
		  NULL);
  }
}


void fcdb_para_item (GtkWidget *widget, gpointer data)
{
  typHOE *hg = (typHOE *)data;
  
  create_fcdb_para_dialog(hg);
}


void create_fc_all_dialog (typHOE *hg)
{
  GtkWidget *dialog, *label, *button, *frame, *spinner;
  GtkWidget *hbox, *hbox2, *check, *table;
  GtkAdjustment *adj;
  
  dialog = gtk_dialog_new_with_buttons("HOE : Creating Finding Charts",
				       GTK_WINDOW(hg->w_top),
				       GTK_DIALOG_MODAL,
#ifdef USE_GTK3
				       "_Cancel",GTK_RESPONSE_CANCEL,
				       "_OK",GTK_RESPONSE_OK,
#else
				       GTK_STOCK_CANCEL,GTK_RESPONSE_CANCEL,
				       GTK_STOCK_OK,GTK_RESPONSE_OK,
#endif
				       NULL);

  gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK); 


  hbox = gtkut_hbox_new(FALSE,2);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 0);
  gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
		     hbox,FALSE, FALSE, 0);

  frame = gtkut_frame_new ("<b>Source</b>");
  gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 5);

  table = gtkut_table_new(5, 2, FALSE, 3, 0, 0);
  gtk_container_add (GTK_CONTAINER (frame), table);

  {
    GtkWidget *combo;
    GtkListStore *store;
    GtkTreeIter iter, iter_set;	  
    GtkCellRenderer *renderer;
    GtkWidget *bar;
    gint i_fc;
    
    store = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_INT, G_TYPE_BOOLEAN);

    for(i_fc=0;i_fc<NUM_FC;i_fc++){
      if(FC_markup[i_fc]){
	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, 0, FC_markup[i_fc],
			   1, i_fc, 2, TRUE, -1);
	if(hg->fc_mode==i_fc) iter_set=iter;
      }
      else{
	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter,
			    0, NULL,
			    1, i_fc, 2, FALSE, -1);
      }
    }

    combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));
#ifdef USE_GTK3      
    gtk_widget_set_hexpand(combo,TRUE);
#endif
    gtkut_table_attach(table, combo, 1, 2, 1, 2,
			GTK_FILL|GTK_EXPAND,GTK_SHRINK,0,0);
    g_object_unref(store);
	
    renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo),renderer, TRUE);
    gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT(combo), renderer, "markup",0,NULL);
	
    gtk_combo_box_set_row_separator_func (GTK_COMBO_BOX (combo), 
					  is_separator, NULL, NULL);	

    gtk_combo_box_set_active_iter(GTK_COMBO_BOX(combo),&iter_set);
    gtk_widget_show(combo);
    my_signal_connect (combo,"changed",cc_get_fc_mode_pdf,
		       (gpointer)hg);
  }

  frame = gtk_frame_new ("Size [\']");
#ifdef USE_GTK3      
  gtk_widget_set_halign(frame,GTK_ALIGN_CENTER);
  gtk_widget_set_valign(frame,GTK_ALIGN_CENTER);
#endif
  gtkut_table_attach(table, frame, 2, 3, 0, 2,
		      GTK_SHRINK,GTK_SHRINK,0,0);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 0);

  hbox2 = gtkut_hbox_new(FALSE,0);
  gtk_container_add (GTK_CONTAINER (frame), hbox2);

  adj = (GtkAdjustment *)gtk_adjustment_new(hg->dss_arcmin,
					    DSS_ARCMIN_MIN, DSS_ARCMIN_MAX,
					    1.0, 1.0, 0);
  spinner =  gtk_spin_button_new (adj, 0, 0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), FALSE);
  gtk_box_pack_start(GTK_BOX(hbox2),spinner,FALSE,FALSE,0);
  my_entry_set_width_chars(GTK_ENTRY(&GTK_SPIN_BUTTON(spinner)->entry),3);
  my_signal_connect (adj, "value_changed",
		     cc_get_adj,
		     &hg->dss_arcmin);
  

  hg->fc_frame_col_pdf = gtk_frame_new ("Scale");
#ifdef USE_GTK3      
  gtk_widget_set_halign(hg->fc_frame_col_pdf,GTK_ALIGN_CENTER);
  gtk_widget_set_valign(hg->fc_frame_col_pdf,GTK_ALIGN_CENTER);
#endif
  gtkut_table_attach(table, hg->fc_frame_col_pdf, 3, 4, 0, 2,
		      GTK_SHRINK,GTK_SHRINK,0,0);
  gtk_container_set_border_width (GTK_CONTAINER (hg->fc_frame_col_pdf), 0);

  hbox2 = gtkut_hbox_new(FALSE,0);
  gtk_container_add (GTK_CONTAINER (hg->fc_frame_col_pdf), hbox2);

  set_fc_frame_col_pdf(hg);

  {
    GtkWidget *combo;
    GtkListStore *store;
    GtkTreeIter iter, iter_set;	  
    GtkCellRenderer *renderer;
    
    store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
    
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "Linear",
		       1, FC_SCALE_LINEAR, -1);
    if(hg->dss_scale==FC_SCALE_LINEAR) iter_set=iter;
	
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "Log",
		       1, FC_SCALE_LOG, -1);
    if(hg->dss_scale==FC_SCALE_LOG) iter_set=iter;
	
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "Sqrt",
		       1, FC_SCALE_SQRT, -1);
    if(hg->dss_scale==FC_SCALE_SQRT) iter_set=iter;
	
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "HistEq",
		       1, FC_SCALE_HISTEQ, -1);
    if(hg->dss_scale==FC_SCALE_HISTEQ) iter_set=iter;
	
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, "LogLog",
		       1, FC_SCALE_LOGLOG, -1);
    if(hg->dss_scale==FC_SCALE_LOGLOG) iter_set=iter;
	
	
    combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));
    gtk_box_pack_start(GTK_BOX(hbox2),combo,FALSE,FALSE,0);
    g_object_unref(store);
	
    renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo),renderer, TRUE);
    gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT(combo), renderer, "text",0,NULL);
	
    gtk_combo_box_set_active_iter(GTK_COMBO_BOX(combo),&iter_set);
    gtk_widget_show(combo);
    my_signal_connect (combo,"changed",cc_get_combo_box,
		       &hg->dss_scale);
  }

  button=gtk_check_button_new_with_label("Inv");
  gtk_container_set_border_width (GTK_CONTAINER (button), 0);
  gtk_box_pack_start(GTK_BOX(hbox2),button,FALSE,FALSE,0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),hg->dss_invert);
  my_signal_connect(button,"toggled",
		    G_CALLBACK (cc_get_toggle), 
		    &hg->dss_invert);

  frame = gtkut_frame_new ("<b>Instrument</b>");
  gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 5);

  table = gtkut_table_new(4, 2, FALSE, 3, 0, 0);
  gtk_container_add (GTK_CONTAINER (frame), table);


  {
    GtkWidget *combo;
    GtkListStore *store;
    GtkTreeIter iter, iter_set;	  
    GtkCellRenderer *renderer;
    gint i_inst;
    
    store = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_INT, G_TYPE_BOOLEAN);
    
    for(i_inst=0;i_inst<NUM_FC_INST;i_inst++){
      if(FC_instname[i_inst]){
	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, 0, FC_instname[i_inst],
			   1, i_inst, 2, TRUE, -1);
	if(hg->fc_inst==i_inst) iter_set=iter;
      }
      else{
	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter,
			    0, NULL,
			    1, i_inst, 2, FALSE, -1);
      }
    }

    combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));
#ifdef USE_GTK3      
    gtk_widget_set_halign(combo,GTK_ALIGN_CENTER);
    gtk_widget_set_valign(combo,GTK_ALIGN_CENTER);
#endif
    gtkut_table_attach(table, combo, 1, 2, 1, 2,
			GTK_SHRINK,GTK_SHRINK,0,0);
    g_object_unref(store);
	
    renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo),renderer, TRUE);
    gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT(combo), renderer, "text",0,NULL);
	
    gtk_combo_box_set_row_separator_func (GTK_COMBO_BOX (combo), 
					  is_separator, NULL, NULL);	

    gtk_combo_box_set_active_iter(GTK_COMBO_BOX(combo),&iter_set);
    gtk_widget_show(combo);
    my_signal_connect (combo,"changed",cc_get_combo_box,
		       &hg->fc_inst);
  }

  button=gtk_check_button_new_with_label("Detail");
  gtk_container_set_border_width (GTK_CONTAINER (button), 0);
#ifdef USE_GTK3      
  gtk_widget_set_halign(button,GTK_ALIGN_CENTER);
  gtk_widget_set_valign(button,GTK_ALIGN_CENTER);
#endif
  gtkut_table_attach(table, button, 2, 3, 1, 2,
		      GTK_SHRINK,GTK_SHRINK,0,0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),hg->dss_draw_slit);
  my_signal_connect(button,"toggled",
		    G_CALLBACK (cc_get_toggle), 
		    &hg->dss_draw_slit);


  button=gtk_check_button_new_with_label("Flip");
  gtk_container_set_border_width (GTK_CONTAINER (button), 0);
  gtkut_table_attach(table, button, 3, 4, 0, 2,
		      GTK_SHRINK,GTK_SHRINK,0,0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),hg->dss_flip);
  my_signal_connect(button,"toggled",
		    G_CALLBACK (cc_get_toggle), 
		    &hg->dss_flip);
  
  hbox = gtkut_hbox_new(FALSE,2);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);
  gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
		     hbox,FALSE, FALSE, 0);

  button=gtk_check_button_new_with_label("Skip objects w/Mag < 10 (for Service programs)");
  gtk_container_set_border_width (GTK_CONTAINER (button), 0);
  gtk_box_pack_start(GTK_BOX(hbox),button,FALSE,FALSE,0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),hg->fc_all_magskip);
  my_signal_connect(button,"toggled",
		    G_CALLBACK (cc_get_toggle), 
		    &hg->fc_all_magskip);
  

  gtk_widget_show_all(dialog);

  if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
    gtk_widget_destroy(dialog);
    fc_dl_draw_all(hg);
  }
  else{
    gtk_widget_destroy(dialog);
  }
}


gboolean draw_fc_cairo(GtkWidget *widget,typHOE *hg){
  cairo_t *cr;
  cairo_surface_t *surface;
  cairo_text_extents_t extents;
  double x,y;
  gint i_list;
  gint from_set, to_rise;
  int width, height, alloc_w, alloc_h;
  int width_file, height_file;
  gdouble r_w,r_h, r;
  gint shift_x=0, shift_y=0;

  gchar *tmp;
  GdkPixbuf *pixbuf_flip=NULL;
  gdouble scale;

  if(!flagFC) return (FALSE);

  switch(hg->fc_output){
  case FC_OUTPUT_PDF:
    width= hg->sz_plot;
    height= hg->sz_plot;
    alloc_w=width;
    alloc_h=height;
    scale=(gdouble)(hg->skymon_objsz)/(gdouble)(SKYMON_DEF_OBJSZ);

    surface = cairo_pdf_surface_create(hg->filename_pdf, width, height);
    cr = cairo_create(surface); 

    cairo_set_source_rgb(cr, 1, 1, 1);
    break;

  case FC_OUTPUT_PRINT:
    width =  (gint)gtk_print_context_get_width(hg->context);
    height =  (gint)gtk_print_context_get_height(hg->context);
    alloc_w=width;
    alloc_h=height;
#ifdef USE_WIN32
    scale=(gdouble)width/(gint)(hg->sz_plot*1.5)
      *(gdouble)(hg->skymon_objsz)/(gdouble)(SKYMON_DEF_OBJSZ);
#else
    scale=(gdouble)(hg->skymon_objsz)/(gdouble)(SKYMON_DEF_OBJSZ);
#endif

    cr = gtk_print_context_get_cairo_context (hg->context);

    cairo_set_source_rgb(cr, 1, 1, 1);
    break;

  default:
    {
      GtkAllocation *allocation=g_new(GtkAllocation, 1);
      gtk_widget_get_allocation(widget,allocation);
      
      width= allocation->width*hg->fc_mag;
      height= allocation->height*hg->fc_mag;
      alloc_w=allocation->width;
      alloc_h=allocation->height;
    
      if(width<=1){
	gtk_window_get_size(GTK_WINDOW(hg->fc_main), &width, &height);
      }
      scale=(gdouble)(hg->skymon_objsz)/(gdouble)(SKYMON_DEF_OBJSZ);
      
      // Edge for magnification
      if(hg->fc_mag!=1){
	shift_x=-(hg->fc_magx*hg->fc_mag-width/2/hg->fc_mag);
	shift_y=-(hg->fc_magy*hg->fc_mag-height/2/hg->fc_mag);
	
	if(shift_x>0){
	  shift_x=0;
	}
	else if((width+shift_x)<allocation->width){
	  shift_x=allocation->width-width;
	}
	
	if(shift_y>0){
	  shift_y=0;
	}
	else if((height+shift_y)<allocation->height){
	  shift_y=allocation->height-height;
	}
      }
      g_free(allocation);
    }

#ifdef USE_GTK3
    surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
					 width, height);
    
    cr = cairo_create(surface);
#else
    if(pixmap_fcbk) g_object_unref(G_OBJECT(pixmap_fcbk));
    pixmap_fcbk = gdk_pixmap_new(gtk_widget_get_window(widget),
				 width,
				 height,
				 -1);
  
    cr = gdk_cairo_create(pixmap_fcbk);
#endif
    
    
    if(hg->dss_invert){
      cairo_set_source_rgba(cr, 0.8, 0.8, 0.8, 1.0);
    }
    else{
      cairo_set_source_rgba(cr, 0.3, 0.3, 0.3, 1.0);
    }
    break;
  }

  /* draw the background */
  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
  cairo_paint (cr);
  
  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

  if(!pixbuf_fc){
    gdouble l_h;

    cairo_rectangle(cr, 0,0,
		    width,
		    height);
    if(hg->fc_output==FC_OUTPUT_WINDOW){
      cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0);
    }
    else{
      cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
    }
    cairo_fill(cr);

    if(hg->fc_output==FC_OUTPUT_WINDOW){
      cairo_set_source_rgba(cr, 1.0, 0.5, 0.5, 1.0);
    }
    else{
      cairo_set_source_rgba(cr, 0.8, 0.0, 0.0, 1.0);
    }
    cairo_set_font_size (cr, (gdouble)hg->skymon_allsz*1.2*scale);

    cairo_select_font_face (cr, hg->fontfamily_all, CAIRO_FONT_SLANT_NORMAL,
			  CAIRO_FONT_WEIGHT_BOLD);
    tmp=g_strdup("Error : Failed to load the image for the finding chart!");
    cairo_text_extents (cr,tmp, &extents);
    l_h=extents.height;
    cairo_move_to(cr,width/2-extents.width/2,
		  height/2-l_h*1.5);
    cairo_show_text(cr, tmp);
    if(tmp) g_free(tmp);
 
    cairo_select_font_face (cr, hg->fontfamily_all, CAIRO_FONT_SLANT_NORMAL,
			  CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size (cr, (gdouble)hg->skymon_allsz*scale);
    tmp=g_strdup("The position might be out of the surveyed area.");
    cairo_text_extents (cr,tmp, &extents);
    cairo_move_to(cr,width/2-extents.width/2,
		  height/2+l_h*1.5);
    cairo_show_text(cr, tmp);
    if(tmp) g_free(tmp);
    
    tmp=g_strdup("or");
    cairo_text_extents (cr,tmp, &extents);
    cairo_move_to(cr,width/2-extents.width/2,
		  height/2+l_h*3);
    cairo_show_text(cr, tmp);
    if(tmp) g_free(tmp);
    
    tmp=g_strdup("An HTTP error might be occured in the server side.");
    cairo_text_extents (cr,tmp, &extents);
    cairo_move_to(cr,width/2-extents.width/2,
		  height/2+l_h*4.5);
    cairo_show_text(cr, tmp);
    if(tmp) g_free(tmp);
  }
  else{
    width_file = gdk_pixbuf_get_width(pixbuf_fc);
    height_file = gdk_pixbuf_get_height(pixbuf_fc);

    
    r_w =  (gdouble)width/(gdouble)width_file;
    r_h =  (gdouble)height/(gdouble)height_file;
    
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

    translate_to_center(hg,cr,width,height,width_file,height_file,r);

    cairo_translate (cr, -(gdouble)width_file*r/2,
		     -(gdouble)height_file*r/2);
    gdk_cairo_set_source_pixbuf(cr, pixbuf2_fc, 0, 0);
    
    cairo_rectangle(cr, 0,0,
		    (gint)((gdouble)width_file*r),
		    (gint)((gdouble)height_file*r));
    cairo_fill(cr);

    if(hg->dss_invert){
      cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0);
      cairo_set_line_width (cr, 1.0*scale);
      cairo_rectangle(cr, 0,0,
		      (gint)((gdouble)width_file*r),
		      (gint)((gdouble)height_file*r));
      cairo_stroke(cr);
    }

    cairo_restore(cr);

    cairo_save(cr);
    cairo_translate (cr, (width-(gint)((gdouble)width_file*r))/2,
		     (height-(gint)((gdouble)height_file*r))/2);

    switch(hg->fc_inst){
    case FC_INST_HDS:
    case FC_INST_HDSAUTO:
    case FC_INST_HDSZENITH:
      draw_hds(hg, cr, width, height, width_file, height_file, scale, r);
      break;

    case FC_INST_IRD:
      draw_ird(hg, cr, width, height, width_file, height_file, scale, r);
      break;

    case FC_INST_IRCS:
      draw_ircs(hg, cr, width, height, width_file, height_file, scale, r);
      break;

    case FC_INST_COMICS:
      draw_comics(hg, cr, width, height, width_file, height_file, scale, r);
      break;

    case FC_INST_FOCAS:
      draw_focas(hg, cr, width, height, width_file, height_file, scale, r);
      break;

    case FC_INST_MOIRCS:
      draw_moircs(hg, cr, width, height, width_file, height_file, scale, r);
      break;

    case FC_INST_SWIMS:
      draw_swims(hg, cr, width, height, width_file, height_file, scale, r);
      break;

    case FC_INST_SPCAM:
      draw_spcam(hg, cr, width, height, width_file, height_file, scale, r);
      break;

    case FC_INST_HSCDET:
    case FC_INST_HSCA:
      if(hg->hsc_show_ol){
	draw_hsc_ol(hg, cr, width, height, width_file, height_file, scale, r);
      }
      else{
	draw_hsc(hg, cr, width, height, width_file, height_file, scale, r);
      }
      break;

    case FC_INST_FMOS:
      draw_fmos(hg, cr, width, height, width_file, height_file, scale, r);
      break;

    case FC_INST_PFS:
      draw_pfs(hg, cr, width, height, width_file, height_file, scale, r);
      break;

    case FC_INST_KOOLS:
      draw_kools(hg, cr, width, height, width_file, height_file, scale, r);
      break;

    case FC_INST_TRICCS:
      draw_triccs(hg, cr, width, height, width_file, height_file, scale, r);
      break;
    }

    cairo_restore(cr);

    cairo_save(cr);
    draw_fc_label(hg, cr, width, height, width_file, height_file, scale, r);
    cairo_restore(cr);

    if((hg->fc_inst==FC_INST_HSCA)||(hg->fc_inst==FC_INST_HSCDET)){
      if(flagHSCDialog){
	cairo_save (cr);
	draw_hsc_dither(hg, cr, width, height, width_file, height_file, scale, r);
	cairo_restore (cr);
      }
    }
    
    // Position Angle
    cairo_save (cr);
    draw_pa(hg, cr, width, height, width_file, height_file,
	    shift_x, shift_y, scale, r);
    cairo_restore(cr);

    // DB results
    cairo_save(cr);
    draw_fcdb1(hg, cr, width, height, width_file, height_file,
	       shift_x, shift_y, scale, r);
    cairo_restore(cr);
    
    // DB selected and Proper Motion
    cairo_save(cr);
    draw_fcdb2(hg, cr, width, height, width_file, height_file, scale, r);
    cairo_restore(cr);
    
    // Guide Star
    cairo_save(cr);
    draw_gs(hg, cr, width, height, width_file, height_file, scale, r);
    cairo_restore(cr);
    
    // Non-Sidereal Track
    cairo_save(cr);
    draw_nst(hg, cr, width, height, width_file, height_file, scale, r);
    cairo_restore(cr);

    // Points and Distance
    cairo_save(cr);
    draw_pts(hg, cr, width, height, width_file, height_file,
	     shift_x, shift_y, scale, r);
    cairo_restore(cr);
  }


  // Output
  switch(hg->fc_output){
  case FC_OUTPUT_PDF:
    cairo_destroy(cr);
    cairo_show_page(cr); 
    cairo_surface_destroy(surface);
    break;

  case FC_OUTPUT_PRINT:
    break;

  default:
    gtk_widget_show_all(widget);
    cairo_destroy(cr);
    hg->fc_shift_x=shift_x;
    hg->fc_shift_y=shift_y;
#ifdef USE_GTK3
    if(pixbuf_fcbk) g_object_unref(G_OBJECT(pixbuf_fcbk));
    pixbuf_fcbk=gdk_pixbuf_get_from_surface(surface,0,0,width,height);
    cairo_surface_destroy(surface);
    gtk_widget_queue_draw(widget);
#else
    {
      GtkStyle *style=gtk_widget_get_style(widget);

      gdk_draw_drawable(gtk_widget_get_window(widget),
			style->fg_gc[gtk_widget_get_state(widget)],
			pixmap_fcbk,
			0,0,shift_x,shift_y,
			width,
			height);
    }
    
    //g_object_unref(G_OBJECT(pixmap_fcbk));
#endif
    break;
  }

 return TRUE;
}


void set_fc_mode (typHOE *hg)
{
  set_dss_arcmin_upper(hg);
  
  switch(hg->fc_mode){
  case FC_STSCI_DSS1R:
  case FC_STSCI_DSS1B:
  case FC_STSCI_DSS2R:
  case FC_STSCI_DSS2B:
  case FC_STSCI_DSS2IR:
    if(hg->dss_host) g_free(hg->dss_host);
    hg->dss_host             =g_strdup(FC_HOST_STSCI);
    if(hg->dss_file) g_free(hg->dss_file);
    hg->dss_file             =g_strconcat(hg->temp_dir,
					  G_DIR_SEPARATOR_S,
					  FC_FILE_GIF,NULL);
    
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
    
  case FC_ESO_DSS1R:
  case FC_ESO_DSS2R:
  case FC_ESO_DSS2B:
  case FC_ESO_DSS2IR:
    if(hg->dss_host) g_free(hg->dss_host);
    hg->dss_host             =g_strdup(FC_HOST_ESO);
    if(hg->dss_path) g_free(hg->dss_path);
    hg->dss_path             =g_strdup(FC_PATH_ESO);
    if(hg->dss_file) g_free(hg->dss_file);
    hg->dss_file             =g_strconcat(hg->temp_dir,
					  G_DIR_SEPARATOR_S,
					  FC_FILE_GIF,NULL);
    if(hg->dss_tmp) g_free(hg->dss_tmp);
    hg->dss_tmp=g_strconcat(hg->temp_dir,
			    G_DIR_SEPARATOR_S,
			    FC_FILE_HTML,NULL);
    if(hg->dss_src) g_free(hg->dss_src);
    switch(hg->fc_mode){
    case FC_ESO_DSS1R:
      hg->dss_src             =g_strdup(FC_SRC_ESO_DSS1R);
      break;
    case FC_ESO_DSS2R:
      hg->dss_src             =g_strdup(FC_SRC_ESO_DSS2R);
      break;
    case FC_ESO_DSS2B:
      hg->dss_src             =g_strdup(FC_SRC_ESO_DSS2B);
      break;
    case FC_ESO_DSS2IR:
      hg->dss_src             =g_strdup(FC_SRC_ESO_DSS2IR);
      break;
    }
    break;
    
  case FC_SKYVIEW_GALEXF:
  case FC_SKYVIEW_GALEXN:
  case FC_SKYVIEW_DSS1R:
  case FC_SKYVIEW_DSS1B:
  case FC_SKYVIEW_DSS2R:
  case FC_SKYVIEW_DSS2B:
  case FC_SKYVIEW_DSS2IR:
  case FC_SKYVIEW_SDSSU:
  case FC_SKYVIEW_SDSSG:
  case FC_SKYVIEW_SDSSR:
  case FC_SKYVIEW_SDSSI:
  case FC_SKYVIEW_SDSSZ:
  case FC_SKYVIEW_2MASSJ:
  case FC_SKYVIEW_2MASSH:
  case FC_SKYVIEW_2MASSK:
  case FC_SKYVIEW_WISE34:
  case FC_SKYVIEW_WISE46:
  case FC_SKYVIEW_WISE12:
  case FC_SKYVIEW_WISE22:
  case FC_SKYVIEW_AKARIN60:
  case FC_SKYVIEW_AKARIWS:
  case FC_SKYVIEW_AKARIWL:
  case FC_SKYVIEW_AKARIN160:
  case FC_SKYVIEW_NVSS:
    if(hg->dss_host) g_free(hg->dss_host);
    hg->dss_host             =g_strdup(FC_HOST_SKYVIEW);
    if(hg->dss_path) g_free(hg->dss_path);
    hg->dss_path             =g_strdup(FC_PATH_SKYVIEW);
    if(hg->dss_file) g_free(hg->dss_file);
    hg->dss_file=g_strconcat(hg->temp_dir,
			     G_DIR_SEPARATOR_S,
			     FC_FILE_JPEG,NULL);
    if(hg->dss_tmp) g_free(hg->dss_tmp);
    hg->dss_tmp=g_strconcat(hg->temp_dir,
			    G_DIR_SEPARATOR_S,
			    FC_FILE_HTML,NULL);
    if(hg->dss_src) g_free(hg->dss_src);
    switch(hg->fc_mode){
    case FC_SKYVIEW_GALEXF:
      hg->dss_src             =g_strdup(FC_SRC_SKYVIEW_GALEXF);
      break;
    case FC_SKYVIEW_GALEXN:
      hg->dss_src             =g_strdup(FC_SRC_SKYVIEW_GALEXN);
      break;
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
    case FC_SKYVIEW_SDSSU:
      hg->dss_src             =g_strdup(FC_SRC_SKYVIEW_SDSSU);
      break;
    case FC_SKYVIEW_SDSSG:
      hg->dss_src             =g_strdup(FC_SRC_SKYVIEW_SDSSG);
      break;
    case FC_SKYVIEW_SDSSR:
      hg->dss_src             =g_strdup(FC_SRC_SKYVIEW_SDSSR);
      break;
    case FC_SKYVIEW_SDSSI:
      hg->dss_src             =g_strdup(FC_SRC_SKYVIEW_SDSSI);
      break;
    case FC_SKYVIEW_SDSSZ:
      hg->dss_src             =g_strdup(FC_SRC_SKYVIEW_SDSSZ);
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
    case FC_SKYVIEW_WISE34:
      hg->dss_src             =g_strdup(FC_SRC_SKYVIEW_WISE34);
      break;
    case FC_SKYVIEW_WISE46:
      hg->dss_src             =g_strdup(FC_SRC_SKYVIEW_WISE46);
      break;
    case FC_SKYVIEW_WISE12:
      hg->dss_src             =g_strdup(FC_SRC_SKYVIEW_WISE12);
      break;
    case FC_SKYVIEW_WISE22:
      hg->dss_src             =g_strdup(FC_SRC_SKYVIEW_WISE22);
      break;
    case FC_SKYVIEW_AKARIN60:
      hg->dss_src             =g_strdup(FC_SRC_SKYVIEW_AKARIN60);
      break;
    case FC_SKYVIEW_AKARIWS:
      hg->dss_src             =g_strdup(FC_SRC_SKYVIEW_AKARIWS);
      break;
    case FC_SKYVIEW_AKARIWL:
      hg->dss_src             =g_strdup(FC_SRC_SKYVIEW_AKARIWL);
      break;
    case FC_SKYVIEW_AKARIN160:
      hg->dss_src             =g_strdup(FC_SRC_SKYVIEW_AKARIN160);
      break;
    case FC_SKYVIEW_NVSS:
      hg->dss_src             =g_strdup(FC_SRC_SKYVIEW_NVSS);
      break;
    }
    break;

  case FC_SDSS:
    if(hg->dss_host) g_free(hg->dss_host);
    hg->dss_host             =g_strdup(FC_HOST_SDSS);
    if(hg->dss_path) g_free(hg->dss_path);
    hg->dss_path             =g_strdup(FC_PATH_SDSS);
    if(hg->dss_file) g_free(hg->dss_file);
    hg->dss_file=g_strconcat(hg->temp_dir,
			     G_DIR_SEPARATOR_S,
			     FC_FILE_JPEG,NULL);
    break;
    
  case FC_SDSS13:
    if(hg->dss_host) g_free(hg->dss_host);
    hg->dss_host             =g_strdup(FC_HOST_SDSS13);
    if(hg->dss_path) g_free(hg->dss_path);
    hg->dss_path             =g_strdup(FC_PATH_SDSS13);
    if(hg->dss_file) g_free(hg->dss_file);
    hg->dss_file=g_strconcat(hg->temp_dir,
			     G_DIR_SEPARATOR_S,
			     FC_FILE_JPEG,NULL);
    break;

  case FC_PANCOL:
  case FC_PANG:
  case FC_PANR:
  case FC_PANI:
  case FC_PANZ:
  case FC_PANY:
    if(hg->dss_arcmin>PANSTARRS_MAX_ARCMIN){
      if(flagFC){
	gtk_adjustment_set_value(hg->fc_adj_dss_arcmin, 
				 (gdouble)(PANSTARRS_MAX_ARCMIN));
      }
      else{
	hg->dss_arcmin=PANSTARRS_MAX_ARCMIN;
      }
    }
    if(hg->dss_tmp) g_free(hg->dss_tmp);
    hg->dss_tmp=g_strconcat(hg->temp_dir,
			    G_DIR_SEPARATOR_S,
			    FC_FILE_HTML,NULL);
    if(hg->dss_host) g_free(hg->dss_host);
    hg->dss_host             =g_strdup(FC_HOST_PANCOL);
    if(hg->dss_path) g_free(hg->dss_path);
    switch(hg->fc_mode){
    case FC_PANCOL:
      hg->dss_path             =g_strdup(FC_PATH_PANCOL);
      break;

    case FC_PANG:
      hg->dss_path             =g_strdup(FC_PATH_PANG);
      break;
 
    case FC_PANR:
      hg->dss_path             =g_strdup(FC_PATH_PANR);
      break;
 
    case FC_PANI:
      hg->dss_path             =g_strdup(FC_PATH_PANI);
      break;
 
    case FC_PANZ:
      hg->dss_path             =g_strdup(FC_PATH_PANZ);
      break;
 
    case FC_PANY:
      hg->dss_path             =g_strdup(FC_PATH_PANY);
      break;
    }
    if(hg->dss_file) g_free(hg->dss_file);
    hg->dss_file=g_strconcat(hg->temp_dir,
			     G_DIR_SEPARATOR_S,
			     FC_FILE_JPEG,NULL);
    break;

  }
}


void cc_get_fc_mode0 (GtkWidget *widget,  gpointer gdata)
{
  GtkTreeIter iter;
  typHOE *hg = (typHOE *)gdata;

  if(gtk_combo_box_get_active_iter(GTK_COMBO_BOX(widget), &iter)){
    gint n;
    GtkTreeModel *model;
    
    model=gtk_combo_box_get_model(GTK_COMBO_BOX(widget));
    gtk_tree_model_get (model, &iter, 1, &n, -1);

    hg->fc_mode0=n;

    hg->fc_mode=hg->fc_mode0;

    if(flagFC){
      set_fc_frame_col(hg);
    }
    set_fc_mode(hg);
  }
}


void pdf_fc (typHOE *hg)
{
  hg->fc_output=FC_OUTPUT_PDF;

  if(flagFC){
    draw_fc_cairo(hg->fc_dw,hg);
  }

  hg->fc_output=FC_OUTPUT_WINDOW;
}

