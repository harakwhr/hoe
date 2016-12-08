//    HDS OPE file Editor
//      edit.c  --- Edit OPE file
//   
//                                           2004.6.18  A.Tajitsu


#include"main.h"    // 設定ヘッダ
#include"version.h"

void create_opedit_dialog();
void close_opedit();
void menu_close_opedit();
void save_opedit();

GtkWidget *make_edit_menu();

void add_FocusAGSequence();
void add_FocusAG();
void add_BIAS();
void add_Setup();
void add_Comp();
void add_Obj();
void add_Def();

static void cc_e_list();
static void get_adj ();
static void get_entry_int ();

#ifdef USE_GTK2
void GTK2InsertText();
#endif

void insert();

GtkWidget *opedit_main;
GtkWidget *opedit_text;
#ifdef USE_GTK2
GtkTextBuffer *text_buffer;
#endif

extern gboolean flagChildDialog;
extern void my_signal_connect();
extern gboolean my_main_iteration();
extern void my_entry_set_width_chars();
extern gchar *make_tgt();

GdkColor color_com1 = {0, 0x0000, 0x8888, 0x0000};
GdkColor color_com2 = {0, 0xBBBB, 0x8888, 0x0000};
GdkColor color_com3 = {0, 0xDDDD, 0x0000, 0x0000};



// Ya is temporary (using Yb setting)
extern const SetupEntry setups[];

// Create OPE Edit Window
void create_opedit_dialog(typHOE *hg)
{
  GtkWidget *opedit_tbl;
  GtkWidget *button;
  GtkWidget *opedit_scroll;
  GtkWidget *opedit_wbox;
  GtkWidget *table1,*frame,*frame2,*hbox;
  GtkWidget *combo,*label,*spinner;
  GtkAdjustment *adj;
  gchar ope_buffer[BUFFSIZE];
  gchar *fp_1, *fp_2;
  guint nchars;
#ifdef USE_GTK2
  GtkTextIter start_iter, end_iter;
  GtkTextMark *end_mark;
#endif
  gchar *title_tmp;
  FILE *infile;
  GtkWidget *editbar;
  gchar tmp[10];
  confEdit *cdata[MAX_USESETUP];
  gint i_use,i_list;


  

  // Win構築は重いので先にExposeイベント等をすべて処理してから
  while (my_main_iteration(FALSE));

#ifdef USE_GTK2
  opedit_main = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  text_buffer = gtk_text_buffer_new(NULL);
#else
  opedit_main = gtk_window_new(GTK_WINDOW_DIALOG);
#endif


  opedit_wbox = gtk_vbox_new(FALSE,0);
  gtk_container_add (GTK_CONTAINER (opedit_main), opedit_wbox);

  editbar=make_edit_menu(hg);
  gtk_box_pack_start(GTK_BOX(opedit_wbox), editbar,FALSE, FALSE, 0);
  
  //gtk_widget_set_usize (opedit_main, 680,400);
  title_tmp=g_strconcat("HOE : ",g_path_get_basename(hg->filename_write),NULL);
  gtk_window_set_title(GTK_WINDOW(opedit_main), title_tmp);
  gtk_widget_realize(opedit_main);
  my_signal_connect(opedit_main,"destroy",
		    close_opedit, 
		    GTK_WIDGET(opedit_main));
  gtk_container_set_border_width (GTK_CONTAINER (opedit_main), 0);
  

  // Command Add
  frame = gtk_frame_new ("Add Commands");
  gtk_box_pack_start(GTK_BOX(opedit_wbox), frame,FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 5);
	
  table1 = gtk_table_new(5,1,FALSE);
  gtk_container_add (GTK_CONTAINER (frame), table1);
  gtk_container_set_border_width (GTK_CONTAINER (table1), 5);
  //gtk_table_set_row_spacings (GTK_TABLE (table1), 5);
  gtk_table_set_col_spacings (GTK_TABLE (table1), 5);

  //Focus
  frame2 = gtk_frame_new ("Focus");
  gtk_table_attach(GTK_TABLE(table1), frame2, 0, 1, 0, 1,
		   GTK_FILL,GTK_FILL,0,0);
  gtk_container_set_border_width (GTK_CONTAINER (frame2), 0);

  hbox = gtk_hbox_new(FALSE,2);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 2);
  gtk_container_add (GTK_CONTAINER (frame2), hbox);
  
  button=gtk_button_new_with_label("FocusAG");
  gtk_box_pack_start(GTK_BOX(hbox),button,TRUE,TRUE,0);
  my_signal_connect(button,"pressed",
		    add_FocusAGSequence, 
		    NULL);
  

  //BIAS
  frame2 = gtk_frame_new ("BIAS");
  gtk_table_attach(GTK_TABLE(table1), frame2, 1, 2, 0, 1,
		   GTK_FILL,GTK_FILL,0,0);
  gtk_container_set_border_width (GTK_CONTAINER (frame2), 0);

  hbox = gtk_hbox_new(FALSE,2);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 2);
  gtk_container_add (GTK_CONTAINER (frame2), hbox);
  
  button=gtk_button_new_with_label("BIAS");
  gtk_box_pack_start(GTK_BOX(hbox),button,TRUE,TRUE,0);
  my_signal_connect(button,"pressed",
		    add_BIAS, 
		    NULL);
  
  //Setup
  frame2 = gtk_frame_new ("Change Setup");
  gtk_table_attach(GTK_TABLE(table1), frame2, 2, 3, 0, 1,
		   GTK_FILL,GTK_FILL,0,0);
  gtk_container_set_border_width (GTK_CONTAINER (frame2), 0);

  hbox = gtk_hbox_new(FALSE,2);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 2);
  gtk_container_add (GTK_CONTAINER (frame2), hbox);
  
  for(i_use=0;i_use<MAX_USESETUP;i_use++){
    sprintf(tmp," %d ",i_use+1);
    button=gtk_button_new_with_label(tmp);
    gtk_box_pack_start(GTK_BOX(hbox),button,TRUE,TRUE,0);
    cdata[i_use]=g_malloc0(sizeof(confEdit));
    cdata[i_use]->i_use=i_use;
    cdata[i_use]->hg=hg;
    my_signal_connect(button,"pressed",
		      add_Setup, 
		      (gpointer *)cdata[i_use]);
    if(hg->setup[i_use].use){
      gtk_widget_set_sensitive(button, TRUE);
    }
    else{
      gtk_widget_set_sensitive(button, FALSE);
    }
  }


  // Comparison
  frame2 = gtk_frame_new ("Comparison");
  gtk_table_attach(GTK_TABLE(table1), frame2, 3, 4, 0, 1,
		   GTK_FILL,GTK_FILL,0,0);
  gtk_container_set_border_width (GTK_CONTAINER (frame2), 0);

  hbox = gtk_hbox_new(FALSE,2);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 2);
  gtk_container_add (GTK_CONTAINER (frame2), hbox);
  
  for(i_use=0;i_use<MAX_USESETUP;i_use++){
    sprintf(tmp," %d ",i_use+1);
    button=gtk_button_new_with_label(tmp);
    gtk_box_pack_start(GTK_BOX(hbox),button,TRUE,TRUE,0);
    if(hg->setup[i_use].use){
      gtk_widget_set_sensitive(button, TRUE);
    }
    else{
      gtk_widget_set_sensitive(button, FALSE);
    }
    my_signal_connect(button,"pressed",
		      add_Comp, 
		      (gpointer *)cdata[i_use]);
  }

  // Object
  frame2 = gtk_frame_new ("Object");
  gtk_table_attach(GTK_TABLE(table1), frame2, 4, 5, 0, 1,
		   GTK_FILL,GTK_FILL,0,0);
  gtk_container_set_border_width (GTK_CONTAINER (frame2), 0);

  hbox = gtk_hbox_new(FALSE,2);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 2);
  gtk_container_add (GTK_CONTAINER (frame2), hbox);
  
  {
    GtkListStore *store;
    GtkTreeIter iter, iter_set;	  
    GtkCellRenderer *renderer;
    
    store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);

    for(i_list=0;i_list<hg->i_max;i_list++){
      if(hg->obj[0].name){
	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, 0, hg->obj[i_list].name,
			   1, i_list, -1);
      }
    }
    
    combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));
    gtk_box_pack_start(GTK_BOX(hbox),combo,FALSE,FALSE,0);
    g_object_unref(store);
    
    renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo),renderer, TRUE);
    gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT(combo), renderer, "text",0,NULL);
    
    
    hg->e_list=0;
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo),0);
    gtk_widget_show(combo);
    my_signal_connect (combo,
		       "changed",
		       cc_e_list,
		       (gpointer)hg);
  }

  hg->e_entry = gtk_entry_new ();
  gtk_box_pack_start(GTK_BOX(hbox),hg->e_entry,FALSE,FALSE,0);
  if(hg->obj[0].name){
    sprintf(tmp,"%d",hg->obj[0].exp);
    hg->e_exp=hg->obj[0].exp;
    gtk_entry_set_text(GTK_ENTRY(hg->e_entry),tmp);
  }
  gtk_entry_set_editable(GTK_ENTRY(hg->e_entry),TRUE);
  my_entry_set_width_chars(GTK_ENTRY(hg->e_entry),4);
  my_signal_connect (hg->e_entry,
		     "changed",
		     get_entry_int,
		     &hg->e_exp);
  
  label = gtk_label_new ("[s]x");
  gtk_box_pack_start(GTK_BOX(hbox),label,TRUE,TRUE,0);
  

  hg->e_times=hg->obj[0].repeat;
  hg->e_adj = (GtkAdjustment *)gtk_adjustment_new(hg->e_times,
						  1, 20, 1.0, 1.0, 0);
  my_signal_connect (hg->e_adj, "value_changed",
		     get_adj,
		     &hg->e_times);
  spinner =  gtk_spin_button_new (hg->e_adj, 0, 0);
  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner),
			    FALSE);
  gtk_entry_set_editable(GTK_ENTRY(&GTK_SPIN_BUTTON(spinner)->entry),
			 FALSE);
  my_entry_set_width_chars(GTK_ENTRY(&GTK_SPIN_BUTTON(spinner)->entry),2);
  gtk_box_pack_start(GTK_BOX(hbox),spinner,FALSE,FALSE,0);
  
  for(i_use=0;i_use<MAX_USESETUP;i_use++){
    sprintf(tmp," %d ",i_use+1);
    hg->e_button[i_use]=gtk_button_new_with_label(tmp);
    gtk_box_pack_start(GTK_BOX(hbox),hg->e_button[i_use],TRUE,TRUE,0);
    if((hg->obj[0].setup[i_use])&&(hg->setup[i_use].use)){
      gtk_widget_set_sensitive(hg->e_button[i_use], TRUE);
    }
    else{
      gtk_widget_set_sensitive(hg->e_button[i_use], FALSE);
    }
    my_signal_connect(hg->e_button[i_use],"pressed",
		      add_Obj, 
		      (gpointer *)cdata[i_use]);
  }

  button=gtk_button_new_with_label("Def");
  gtk_box_pack_start(GTK_BOX(hbox),button,TRUE,TRUE,0);
  if(hg->setup[i_use].use){
    gtk_widget_set_sensitive(button, TRUE);
  }
  else{
    gtk_widget_set_sensitive(button, FALSE);
  }
  my_signal_connect(button,"pressed",
		    add_Def, 
		    (gpointer)hg);


  // Text Editor
  opedit_tbl = gtk_table_new (6, 1, FALSE);
  gtk_container_add (GTK_CONTAINER (opedit_wbox), opedit_tbl);
  
#ifdef USE_GTK2
  opedit_scroll = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(opedit_scroll),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  
  opedit_text = gtk_text_view_new_with_buffer (text_buffer);
  gtk_text_view_set_editable (GTK_TEXT_VIEW (opedit_text), TRUE);
  gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (opedit_text), TRUE);
  
  gtk_container_add(GTK_CONTAINER(opedit_scroll), opedit_text);
  
  gtk_table_attach_defaults (GTK_TABLE (opedit_tbl), opedit_scroll, 0, 5, 0, 1);
#else
  opedit_text = gtk_text_new (NULL, NULL);
  gtk_text_set_editable (GTK_TEXT (opedit_text), TRUE);
  gtk_table_attach_defaults (GTK_TABLE(opedit_tbl), opedit_text, 0, 5, 0, 1);
  
  opedit_scroll = gtk_vscrollbar_new (GTK_TEXT (opedit_text)->vadj);
  gtk_table_attach (GTK_TABLE (opedit_tbl), opedit_scroll, 5, 6, 0, 1,
		    GTK_FILL, 
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_text_freeze(GTK_TEXT(opedit_text));
#endif
  
  infile=fopen(hg->filename_write,"r");
  
  if(infile){
#ifdef USE_GTK2
    gtk_text_buffer_create_tag (text_buffer, "underline",
                              "underline", PANGO_UNDERLINE_SINGLE, NULL);
    gtk_text_buffer_create_tag (text_buffer, "heading",
				"weight", PANGO_WEIGHT_BOLD,
				//"size", 15 * PANGO_SCALE,
				NULL);
    gtk_text_buffer_create_tag(text_buffer, "color_com1",
			       "foreground-gdk", &color_com1,
			       NULL);
    gtk_text_buffer_create_tag(text_buffer, "color_com2",
			       "foreground-gdk", &color_com2,
			       NULL);
    gtk_text_buffer_create_tag(text_buffer, "color_com3",
			       "foreground-gdk", &color_com3,
			       NULL);

    gtk_text_buffer_get_start_iter(text_buffer, &start_iter);
#endif
    while(!feof(infile)){
      if(fgets(ope_buffer,BUFFSIZE-1,infile)!=NULL){
#ifdef USE_GTK2
	GTK2InsertText(text_buffer, &start_iter, ope_buffer);
      //gtk_text_buffer_insert (text_buffer, &start_iter, ope_buffer, -1);
#else
	gtk_text_insert (GTK_TEXT (opedit_text), NULL, NULL, NULL,
		       ope_buffer, -1);
#endif
      }
    }
    /*
    while ((nchars = fread(ope_buffer, 1, BUFFSIZE-1, infile)) >0) {
      
#ifdef USE_GTK2
      gtk_text_buffer_insert (text_buffer, &start_iter, ope_buffer, nchars);
#else
      gtk_text_insert (GTK_TEXT (opedit_text), NULL, NULL, NULL,
		       ope_buffer, nchars);
#endif
      if(nchars < BUFFSIZE-1) break;
      }
    */

    fclose(infile);
    
#ifdef USE_GTK2
    //gtk_text_buffer_get_end_iter(text_buffer, &end_iter);
    //gtk_text_buffer_place_cursor(text_buffer, &end_iter);
    //end_mark= gtk_text_buffer_create_mark(text_buffer, "end", &end_iter, FALSE);
    //gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(opedit_text),
    //				 end_mark, 0.0, FALSE, 0.0, 0.0);
    //gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(opedit_text),
    //			 &end_iter,0.0, FALSE,0.0, 0.0); 
#else
    //gtk_adjustment_set_value(GTK_TEXT (opedit_text)->vadj,
    //		     (gfloat)(GTK_TEXT(opedit_text)->text_len));
#endif
  }
    

  
  gtk_widget_show_all(opedit_main);

#ifndef USE_GTK2
  gtk_text_thaw(GTK_TEXT(opedit_text));
#endif

  gtk_main();

  for(i_use=0;i_use<MAX_USESETUP;i_use++){
    g_free(cdata[i_use]);
  }
  g_free(title_tmp);

  flagChildDialog=FALSE;
  gdk_flush();
}


void close_opedit(GtkWidget *w, GtkWidget *dialog)
{
  gtk_main_quit();
  gtk_widget_destroy(GTK_WIDGET(dialog));

}


void menu_close_opedit(GtkWidget *widget,gpointer gdata)
{
  gtk_main_quit();
  gtk_widget_destroy(GTK_WIDGET(opedit_main));
}


void save_opedit(GtkWidget *widget, gpointer gdata)
{
  FILE *outfile;
  gchar *ope_buffer;
  gint nchars;
  typHOE *hg;
#ifdef USE_GTK2
  GtkTextIter start_iter, end_iter;
#endif

  hg=(typHOE *)gdata;

#ifndef USE_GTK2
  gtk_text_freeze(GTK_TEXT(opedit_text));
#endif

  outfile = fopen(hg->filename_write, "w");

  if(outfile){
#ifdef USE_GTK2
    gtk_text_buffer_get_start_iter(text_buffer, &start_iter);
    gtk_text_buffer_get_end_iter(text_buffer, &end_iter);
    ope_buffer=gtk_text_buffer_get_text(text_buffer,
					&start_iter,
					&end_iter,
					TRUE);
#else
    ope_buffer=gtk_editable_get_chars(GTK_EDITABLE(opedit_text),
				      (gint)0,
				      (gint)gtk_text_get_length(GTK_TEXT(opedit_text)));
				     
#endif    
    nchars = fwrite(ope_buffer,sizeof(gchar),strlen(ope_buffer),outfile);
    
    fclose(outfile);

    if(nchars!=strlen(ope_buffer)){
      fprintf(stderr," File Write Error  \"%s\" \n", hg->filename_write);
    }
    
    g_free(ope_buffer);
  }
  else{
    fprintf(stderr," File Write Error  \"%s\" \n", hg->filename_write);
  }
 
#ifndef USE_GTK2
  gtk_text_thaw(GTK_TEXT(opedit_text));
#endif

}


GtkWidget *make_edit_menu(typHOE *hg){
  GtkWidget *menu_bar;
  GtkWidget *menu_item;
  GtkWidget *menu;
  GtkWidget *popup_button;
  GtkWidget *bar;
#ifdef __GTK_STOCK_H__
  GtkWidget *image;
#endif

  menu_bar=gtk_menu_bar_new();
  gtk_widget_show (menu_bar);

  //// File
#ifdef __GTK_STOCK_H__
#ifdef GTK_STOCK_FILE
  image=gtk_image_new_from_stock (GTK_STOCK_FILE, GTK_ICON_SIZE_MENU);
#else
  image=gtk_image_new_from_stock (GTK_STOCK_NEW, GTK_ICON_SIZE_MENU);
#endif
  menu_item =gtk_image_menu_item_new_with_label ("File");
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item),image);
#else
  menu_item =gtk_menu_item_new_with_label ("File");
#endif
  gtk_widget_show (menu_item);
#ifdef USE_GTK2
  gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), menu_item);
#else
  gtk_menu_bar_append(GTK_MENU_BAR(menu_bar), menu_item);
#endif
  
  menu=gtk_menu_new();
  gtk_widget_show (menu);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), menu);
  
  //File/Save
#ifdef __GTK_STOCK_H__
  image=gtk_image_new_from_stock (GTK_STOCK_SAVE, GTK_ICON_SIZE_MENU);
  popup_button =gtk_image_menu_item_new_with_label ("Save");
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(popup_button),image);
#else
  popup_button =gtk_menu_item_new_with_label ("Save");
#endif
  gtk_widget_show (popup_button);
  gtk_container_add (GTK_CONTAINER (menu), popup_button);
  my_signal_connect (popup_button, "activate",save_opedit,(gpointer)hg);

  //File/Save
#ifdef __GTK_STOCK_H__
  image=gtk_image_new_from_stock (GTK_STOCK_QUIT, GTK_ICON_SIZE_MENU);
  popup_button =gtk_image_menu_item_new_with_label ("Quit");
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(popup_button),image);
#else
  popup_button =gtk_menu_item_new_with_label ("Quit");
#endif
  gtk_widget_show (popup_button);
  gtk_container_add (GTK_CONTAINER (menu), popup_button);
  my_signal_connect (popup_button, "activate",menu_close_opedit,NULL);


  gtk_widget_show_all(menu_bar);
  return(menu_bar);
}

/*
GtkWidget *make_edit_menu(typHOE *hg){
  GtkAccelGroup *accel_group;
  GtkItemFactory *item_factory;
  GtkItemFactoryEntry menu_items[] = {
    {"/_File", NULL, NULL, 0, "<Branch>"},
    {"/File/Save", NULL, save_opedit, 0, NULL},
    {"/File/Quit", NULL, menu_close_opedit, 0, NULL},
  };



  accel_group=gtk_accel_group_new();
  item_factory= gtk_item_factory_new(GTK_TYPE_MENU_BAR,
				     "<edit>",
				     accel_group);
  gtk_item_factory_create_items(item_factory, (guint)3, menu_items, (gpointer)hg);

#ifdef USE_GTK2
//  _gtk_accel_group_attach(accel_group, GTK_OBJECT(opedit_main));
#else
  gtk_accel_group_attach(accel_group, GTK_OBJECT(opedit_main));
#endif
  return(gtk_item_factory_get_widget(item_factory, "<edit>"));
}
*/

void add_FocusAGSequence(GtkWidget *widget, gpointer gdata){
  insert("### FocusAG\n");
  insert("FocusAGSequence $DEF_PROTO\n");
  insert("\n");
}

void add_FocusAG(GtkWidget *widget, gpointer gdata){
  insert("### FocusAG\n");
  insert("FocusAG $DEF_PROTO\n");
  insert("\n");
}

void add_BIAS(GtkWidget *widget, gpointer gdata){
  insert("### BIAS\n");
  insert("GetBias $DEF_SPEC OBJECT=BIAS\n");
  insert("\n");
}


void add_Setup(GtkWidget *widget, gpointer gdata){
  guint i_use,i_set;
  gchar tmp[BUFFSIZE];
  confEdit *cdata;

  cdata = (confEdit *)gdata;
  
  i_use=cdata->i_use;

  if(cdata->hg->setup[i_use].use){
    if(cdata->hg->setup[i_use].setup<0){ // NonStd
      i_set=-cdata->hg->setup[i_use].setup-1;
      sprintf(tmp, "### Change Setup  NonStd-%d\n", i_set+1);
      insert(tmp);
      sprintf(tmp, "SetupOBE $DEF_SPEC SLIT_LENGTH=%d\n",
	      cdata->hg->setup[i_use].slit_length);
      insert(tmp);
      if(cdata->hg->nonstd[i_set].col==COL_BLUE){
	sprintf(tmp, "SetupOBE $DEF_SPEC FILTER_1=%s FILTER_2=%s CROSS=Blue CROSS_SCAN=%d COLLIMATOR=Blue $CAMZ_B",
		cdata->hg->setup[i_use].fil1,cdata->hg->setup[i_use].fil2,
		cdata->hg->nonstd[i_set].cross);
	insert(tmp);
	if(cdata->hg->nonstd[i_set].echelle!=1440){
	  sprintf(tmp, " ECHELLE=%d", cdata->hg->nonstd[i_set].echelle);
	  insert(tmp);
	}
	if(cdata->hg->nonstd[i_set].camr!=-3600){
	  sprintf(tmp, " CAMERA_ROTATE=%d", cdata->hg->nonstd[i_set].camr);
	  insert(tmp);
	}
      }
      else{
	sprintf(tmp, "SetupOBE $DEF_SPEC FILTER_1=%s FILTER_2=%s CROSS=Red CROSS_SCAN=%d COLLIMATOR=Red $CAMZ_R",
		cdata->hg->setup[i_use].fil1,cdata->hg->setup[i_use].fil2,
		cdata->hg->nonstd[i_set].cross);
	insert(tmp);
	if(cdata->hg->nonstd[i_set].echelle!=1440){
	  sprintf(tmp, " ECHELLE=%d", cdata->hg->nonstd[i_set].echelle);
	  insert(tmp);
	}
	if(cdata->hg->nonstd[i_set].camr!=-3600){
	  sprintf(tmp, " CAMERA_ROTATE=%d", cdata->hg->nonstd[i_set].camr);
	  insert(tmp);
	}
      }
    }
    else{ //Std
      i_set=cdata->hg->setup[i_use].setup;
      
      sprintf(tmp, "### Change Setup  Std%s\n", setups[i_set].initial);
      insert(tmp);
      
      sprintf(tmp, "SetupOBE $DEF_SPEC SLIT_LENGTH=%d\n",
	      cdata->hg->setup[i_use].slit_length);
      insert(tmp);
      if(i_set<StdI2b){
	sprintf(tmp, "SetupOBE $DEF_SPEC FILTER_1=%s FILTER_2=%s CROSS=%s CROSS_SCAN=Std%s COLLIMATOR=%s $CAMZ_B\n",
		cdata->hg->setup[i_use].fil1,
		cdata->hg->setup[i_use].fil2,
		setups[i_set].cross,
		setups[i_set].initial,
		setups[i_set].col);
	insert(tmp);
      }
      else{
	sprintf(tmp, "SetupOBE $DEF_SPEC FILTER_1=%s FILTER_2=%s CROSS=%s CROSS_SCAN=Std%s COLLIMATOR=%s $CAMZ_R\n",
		cdata->hg->setup[i_use].fil1,
		cdata->hg->setup[i_use].fil2,
		setups[i_set].cross,
		setups[i_set].initial,
		setups[i_set].col);
	insert(tmp);
      }
    }
    insert("\n");
  }
}


void add_Comp(GtkWidget *widget, gpointer gdata){
  guint i_use;
  gchar tmp[BUFFSIZE];
  confEdit *cdata;

  cdata = (confEdit *)gdata;
  
  i_use=cdata->i_use;

  if(cdata->hg->setup[i_use].use){
    {
      gint i_bin;
      i_bin=cdata->hg->setup[i_use].binning;
      if(cdata->hg->setup[i_use].setup < 0){ // NonStd
	sprintf(tmp, "### COMPARISON  for NonStd-%d  %dx%dBINNING\n", 
		-cdata->hg->setup[i_use].setup,cdata->hg->binning[i_bin].x, cdata->hg->binning[i_bin].y);
	insert(tmp);
      }
      else{ // Std
	sprintf(tmp, "### COMPARISON  for Std%s  %dx%dBINNING\n", 
		setups[cdata->hg->setup[i_use].setup].initial,cdata->hg->binning[i_bin].x, cdata->hg->binning[i_bin].y);
	insert(tmp);
      }
      sprintf(tmp, "SetupOBE $DEF_SPEC SLIT_LENGTH=%d SLIT_WIDTH=%d\n"
	      ,cdata->hg->setup[i_use].slit_length,cdata->hg->setup[i_use].slit_width);
      insert(tmp);
      sprintf(tmp, "SETUPCOMPARISON_HCT $DEF_PROTO OBJECT=HCT LAMP=HCT1 AMP=15.0 FILTER01=1 FILTER02=1 FILTER03=1 FILTER04=1 F_SELECT=NS_OPT\n");
      insert(tmp);
      sprintf(tmp, "GetComparison $DEF_SPEC OBJECT=Comparison Exptime=%d\n",
	      20/cdata->hg->binning[i_bin].x/cdata->hg->binning[i_bin].y);
      insert(tmp);
      insert("### [LAUNCHER/HDS] ShutdownComparison HCT LampOff during ReadOut #####\n");
    }
    insert("\n");
  }
}


void add_Obj(GtkWidget *widget, gpointer gdata){
  guint i_use,i_list,i_times;
  gchar tmp[BUFFSIZE];
  confEdit *cdata;
  gchar *tgt;

  cdata = (confEdit *)gdata;
  
  i_use=cdata->i_use;
  i_list=cdata->hg->e_list;
  
  switch(cdata->hg->obj[i_list].guide){
  case NO_GUIDE:
    if(cdata->hg->obj[i_list].note){
      sprintf(tmp, "# %s   No Guide,   %s\n",cdata->hg->obj[i_list].name,
	      cdata->hg->obj[i_list].note);
      insert(tmp);
    }
    else{
      sprintf(tmp, "# %s   No Guide\n",cdata->hg->obj[i_list].name);
      insert(tmp);
    }
    if(cdata->hg->setup[i_use].setup < 0){ //Non-Std
      sprintf(tmp, "#    NonStd-%d %dx%dbinning\n",
	      -cdata->hg->setup[i_use].setup,
	      cdata->hg->binning[cdata->hg->setup[i_use].binning].x,
	      cdata->hg->binning[cdata->hg->setup[i_use].binning].y);
      insert(tmp);
    }
    else{  //Std
      sprintf(tmp, "#    Std%s %dx%dbinning\n",
	      setups[cdata->hg->setup[i_use].setup].initial,
	      cdata->hg->binning[cdata->hg->setup[i_use].binning].x,
	      cdata->hg->binning[cdata->hg->setup[i_use].binning].y);
      insert(tmp);
    }
    if(cdata->hg->setup[i_use].imr){
      tgt=make_tgt(cdata->hg->obj[i_list].name);
      sprintf(tmp, "SetupField Guide=NO $DEF_PROTO $%s SVRegion=%d Exptime_SV=%d IMGROT_FLAG=1 Slit_PA=%.1f Slit_Length=%d Slit_Width=%d SVIntegrate=%d $SV_X $SV_Y\n",
	      tgt, 
	      cdata->hg->sv_region,cdata->hg->exptime_sv,
	      cdata->hg->obj[i_list].pa, 
	      cdata->hg->setup[i_use].slit_length,
	      cdata->hg->setup[i_use].slit_width,
	      cdata->hg->sv_integrate);
      g_free(tgt);
      insert(tmp);
    }
    else{
      tgt=make_tgt(cdata->hg->obj[i_list].name);
      sprintf(tmp, "SetupField Guide=NO $DEF_PROTO $%s SVRegion=%d Exptime_SV=%d IMGROT_FLAG=0 Slit_Length=%d Slit_Width=%d SVIntegrate=%d $SV_X $SV_Y\n",
	      tgt, 
	      cdata->hg->sv_region,cdata->hg->exptime_sv,
	      cdata->hg->setup[i_use].slit_length,
	      cdata->hg->setup[i_use].slit_width,
	      cdata->hg->sv_integrate);
      g_free(tgt);
      insert(tmp);
    }
    break;
  case AG_GUIDE:
    if(cdata->hg->obj[i_list].note){
      sprintf(tmp, "# %s   AG Guide,   %s\n",cdata->hg->obj[i_list].name,
	      cdata->hg->obj[i_list].note);
      insert(tmp);
    }
    else{
      sprintf(tmp, "# %s   AG Guide\n",cdata->hg->obj[i_list].name);
      insert(tmp);
    }
    if(cdata->hg->setup[i_use].setup < 0){ //Non-Std
      sprintf(tmp, "#    NonStd-%d %dx%dbinning\n",
	      -cdata->hg->setup[i_use].setup,
	      cdata->hg->binning[cdata->hg->setup[i_use].binning].x,
	      cdata->hg->binning[cdata->hg->setup[i_use].binning].y);
      insert(tmp);
    }
    else{  //Std
      sprintf(tmp, "#    Std%s %dx%dbinning\n",
	      setups[cdata->hg->setup[i_use].setup].initial,
	      cdata->hg->binning[cdata->hg->setup[i_use].binning].x,
	      cdata->hg->binning[cdata->hg->setup[i_use].binning].y);
      insert(tmp);
    }
    if(cdata->hg->setup[i_use].imr){
      tgt=make_tgt(cdata->hg->obj[i_list].name);
      sprintf(tmp, "SetupField Guide=AG $DEF_PROTO $%s SVRegion=%d Exptime_SV=%d IMGROT_FLAG=1 Slit_PA=%.1f Slit_Length=%d Slit_Width=%d Exptime_Factor=%d Brightness=%d SVIntegrate=%d $SV_X $SV_Y\n",
	      tgt, 
	      cdata->hg->sv_region,cdata->hg->exptime_sv,
	      cdata->hg->obj[i_list].pa, 
	      cdata->hg->setup[i_use].slit_length,
	      cdata->hg->setup[i_use].slit_width,
	      cdata->hg->exptime_factor,cdata->hg->brightness,cdata->hg->sv_integrate);
      g_free(tgt);
      insert(tmp);
    }
    else{
      tgt=make_tgt(cdata->hg->obj[i_list].name);
      sprintf(tmp, "SetupField Guide=AG $DEF_PROTO $%s SVRegion=%d Exptime_SV=%d IMGROT_FLAG=0 Slit_Length=%d Slit_Width=%d Exptime_Factor=%d Brightness=%d SVIntegrate=%d $SV_X $SV_Y\n",
	      tgt, 
	      cdata->hg->sv_region,cdata->hg->exptime_sv,
	      cdata->hg->setup[i_use].slit_length,
	      cdata->hg->setup[i_use].slit_width,
	      cdata->hg->exptime_factor,cdata->hg->brightness,cdata->hg->sv_integrate);
      g_free(tgt);
      insert(tmp);
    }
    break;
  case SV_GUIDE:
    if(cdata->hg->obj[i_list].note){
      sprintf(tmp, "# %s   SV Guide,   %s\n",cdata->hg->obj[i_list].name,
	      cdata->hg->obj[i_list].note);
      insert(tmp);
    }
    else{
      sprintf(tmp, "# %s   SV Guide\n",cdata->hg->obj[i_list].name);
      insert(tmp);
    }
    if(cdata->hg->setup[i_use].setup < 0){ //Non-Std
      sprintf(tmp, "#    NonStd-%d %dx%dbinning\n",
	      -cdata->hg->setup[i_use].setup,
	      cdata->hg->binning[cdata->hg->setup[i_use].binning].x,
	      cdata->hg->binning[cdata->hg->setup[i_use].binning].y);
      insert(tmp);
    }
    else{  //Std
      sprintf(tmp, "#    Std%s %dx%dbinning\n",
	      setups[cdata->hg->setup[i_use].setup].initial,
	      cdata->hg->binning[cdata->hg->setup[i_use].binning].x,
	      cdata->hg->binning[cdata->hg->setup[i_use].binning].y);
      insert(tmp);
    }
    if(cdata->hg->setup[i_use].imr){
      tgt=make_tgt(cdata->hg->obj[i_list].name);
      sprintf(tmp, "SetupField Guide=SV $DEF_PROTO $%s ReadRegion=%d CalcRegion=%d Exptime_SV=%d IMGROT_FLAG=1 Slit_PA=%.1f Slit_Length=%d Slit_Width=%d Exptime_Factor=%d Brightness=%d SVIntegrate=%d ReadArea=Part Mode=SemiAuto $SV_X $SV_Y\n",
	      tgt, 
	      cdata->hg->sv_region,cdata->hg->sv_calc,cdata->hg->exptime_sv,
	      cdata->hg->obj[i_list].pa, 
	      cdata->hg->setup[i_use].slit_length,
	      cdata->hg->setup[i_use].slit_width,
	      cdata->hg->exptime_factor,cdata->hg->brightness,cdata->hg->sv_integrate);
      g_free(tgt);
      insert(tmp);
    }
    else{
      tgt=make_tgt(cdata->hg->obj[i_list].name);
      sprintf(tmp, "SetupField Guide=SV $DEF_PROTO $%s ReadRegion=%d CalcRegion=%d Exptime_SV=%d IMGROT_FLAG=0 Slit_Length=%d Slit_Width=%d Exptime_Factor=%d Brightness=%d SVIntegrate=%d ReadArea=Part Mode=SemiAuto $SV_X $SV_Y\n",
	      tgt, 
	      cdata->hg->sv_region,cdata->hg->sv_calc,cdata->hg->exptime_sv,
	      cdata->hg->setup[i_use].slit_length,
	      cdata->hg->setup[i_use].slit_width,
	      cdata->hg->exptime_factor,cdata->hg->brightness,cdata->hg->sv_integrate);
      g_free(tgt);
      insert(tmp);
    }
    break;
  case SVSAFE_GUIDE:
    if(cdata->hg->obj[i_list].note){
      sprintf(tmp, "# %s   SV[Safe] Guide,   %s\n",cdata->hg->obj[i_list].name,
	      cdata->hg->obj[i_list].note);
      insert(tmp);
    }
    else{
      sprintf(tmp, "# %s   SV[Safe] Guide\n",cdata->hg->obj[i_list].name);
      insert(tmp);
    }
    if(cdata->hg->setup[i_use].setup < 0){ //Non-Std
      sprintf(tmp, "#    NonStd-%d %dx%dbinning\n",
	      -cdata->hg->setup[i_use].setup,
	      cdata->hg->binning[cdata->hg->setup[i_use].binning].x,
	      cdata->hg->binning[cdata->hg->setup[i_use].binning].y);
      insert(tmp);
    }
    else{  //Std
      sprintf(tmp, "#    Std%s %dx%dbinning\n",
	      setups[cdata->hg->setup[i_use].setup].initial,
	      cdata->hg->binning[cdata->hg->setup[i_use].binning].x,
	      cdata->hg->binning[cdata->hg->setup[i_use].binning].y);
      insert(tmp);
    }
    if(cdata->hg->setup[i_use].imr){
      tgt=make_tgt(cdata->hg->obj[i_list].name);
      sprintf(tmp, "SetupField Guide=SV $DEF_PROTO $%s ReadRegion=%d CalcRegion=%d Exptime_SV=%d IMGROT_FLAG=1 Slit_PA=%.1f Slit_Length=%d Slit_Width=%d Exptime_Factor=%d Brightness=%d SVIntegrate=%d ReadArea=Part Mode=Safe $SV_X $SV_Y\n",
	      tgt, 
	      cdata->hg->sv_region,cdata->hg->sv_calc,cdata->hg->exptime_sv,
	      cdata->hg->obj[i_list].pa, 
	      cdata->hg->setup[i_use].slit_length,
	      cdata->hg->setup[i_use].slit_width,
	      cdata->hg->exptime_factor,cdata->hg->brightness,cdata->hg->sv_integrate);
      g_free(tgt);
      insert(tmp);
    }
    else{
      tgt=make_tgt(cdata->hg->obj[i_list].name);
      sprintf(tmp, "SetupField Guide=SV $DEF_PROTO $%s ReadRegion=%d CalcRegion=%d Exptime_SV=%d IMGROT_FLAG=0 Slit_Length=%d Slit_Width=%d Exptime_Factor=%d Brightness=%d SVIntegrate=%d ReadArea=Part Mode=Safe $SV_X $SV_Y\n",
	      tgt, 
	      cdata->hg->sv_region,cdata->hg->sv_calc,cdata->hg->exptime_sv,
	      cdata->hg->setup[i_use].slit_length,
	      cdata->hg->setup[i_use].slit_width,
	      cdata->hg->exptime_factor,cdata->hg->brightness,cdata->hg->sv_integrate);
      g_free(tgt);
      insert(tmp);
    }
    break;
  }

  tgt=make_tgt(cdata->hg->obj[i_list].name);
  sprintf(tmp, "GetObject $DEF_SPEC Exptime=%d SVIntegrate=%d $%s\n",
	  cdata->hg->e_exp, cdata->hg->sv_integrate, tgt);
  g_free(tgt);
  for(i_times=0;i_times<cdata->hg->e_times;i_times++){
    insert(tmp);
  }
  insert("\n");
  
}


static void cc_e_list (GtkWidget *widget, gpointer gdata)
{
  gint i_use;
  typHOE *hg;
  gchar tmp[10];

  hg = (typHOE *) gdata;

  {
    GtkTreeIter iter;
    if(gtk_combo_box_get_active_iter(GTK_COMBO_BOX(widget), &iter)){
      gint n;
      GtkTreeModel *model;
      
      model=gtk_combo_box_get_model(GTK_COMBO_BOX(widget));
      gtk_tree_model_get (model, &iter, 1, &hg->e_list, -1);
      
    }
  }
  sprintf(tmp,"%d",hg->obj[hg->e_list].exp);
  hg->e_exp=hg->obj[hg->e_list].exp;
  gtk_entry_set_text(GTK_ENTRY(hg->e_entry),tmp);

  gtk_adjustment_set_value(hg->e_adj,(gdouble)hg->obj[hg->e_list].repeat);

  for(i_use=0;i_use<MAX_USESETUP;i_use++){
    if((hg->obj[hg->e_list].setup[i_use])&&(hg->setup[i_use].use)){
      gtk_widget_set_sensitive(hg->e_button[i_use], TRUE);
    }
    else{
      gtk_widget_set_sensitive(hg->e_button[i_use], FALSE);
    }
  }
}

void add_Def(GtkWidget *widget, gpointer gdata){
  typHOE *hg;
  guint i_list;
  gchar tmp[BUFFSIZE];
  gchar *tgt;

  hg=(typHOE *)gdata;
  i_list=hg->e_list;

  if(hg->obj[i_list].name){
    tgt=make_tgt(hg->obj[i_list].name);
    sprintf(tmp, "%s=OBJECT=\"%s\" RA=%09.2f DEC=%+010.2f EQUINOX=%7.2f\n",
	    tgt, hg->obj[i_list].name, 
	    hg->obj[i_list].ra,  hg->obj[i_list].dec, hg->obj[i_list].epoch);
    g_free(tgt);
    insert(tmp);
  }
}


void insert(gchar *insert_text){
#ifdef USE_GTK2
  GtkTextIter iter;

  gtk_text_buffer_insert_at_cursor(text_buffer,
  	   insert_text,
  	   strlen(insert_text));
#else
	gtk_text_insert (GTK_TEXT (opedit_text), NULL, NULL, NULL,
			 insert_text,
			 strlen(insert_text));
#endif
}

static void get_adj (GtkWidget *widget, gint * gdata)
{
  *gdata=GTK_ADJUSTMENT(widget)->value;
}

static void get_entry_int (GtkWidget *widget, gint *gdata)
{
  *gdata=(gint)g_strtod(gtk_entry_get_text(GTK_ENTRY(widget)),NULL);
}

#ifdef USE_GTK2
void GTK2InsertText(GtkTextBuffer *buffer,
		    GtkTextIter   *iter,
                    const gchar   *text){
  if(strncmp(text,"###",3)==0){
    gtk_text_buffer_insert_with_tags_by_name (buffer, iter,
					      text, -1,
					      "color_com3",
					      NULL);
  }
  else if(strncmp(text,"##",2)==0){
    gtk_text_buffer_insert_with_tags_by_name (buffer, iter,
					      text, -1,
					      "color_com2",
					      NULL);
  }
  else if(strncmp(text,"#",1)==0){
    gtk_text_buffer_insert_with_tags_by_name (buffer, iter,
					      text, -1,
					      "color_com1",
					      NULL);
  }
  else{
    gtk_text_buffer_insert (buffer, iter, text, -1);
  }
}
#endif
