//    hskymon  from HDS OPE file Editor
//          New SkyMonitor for Subaru Gen2
//      post_lamost.h  --- POST body for FCDB access to LAMOST web search
//   
//                                           2017.11.15  A.Tajitsu

static const PARAMpost lamost_post[] = { 
  {POST_CONST, "sForm", "0"},
  {POST_NULL,  "pos.ramin",    NULL},
  {POST_NULL,  "pos.decmin",   NULL},
  {POST_NULL,  "pos.ramax",    NULL},
  {POST_NULL,  "pos.decmax",   NULL},
  {POST_CONST, "pos.type",     "cone"},
  {POST_INPUT, "pos.racenter", NULL},
  {POST_INPUT, "pos.deccenter",NULL},
  {POST_INPUT, "pos.radius",   NULL},
  {POST_NULL,  "pos.radecTextarea",  NULL},
  {POST_NULL,  "obsidTextarea",NULL},
  {POST_NULL,  "fitsnamearea", NULL},
  {POST_NULL,  "mjd",          NULL},
  {POST_NULL,  "planid",       NULL},
  {POST_NULL,  "spid",         NULL},
  {POST_NULL,  "fiberid",      NULL},
  {POST_NULL,  "magtype",      NULL},
  {POST_NULL,  "mag1",         NULL},
  {POST_NULL,  "mag1_max",     NULL},
  {POST_NULL,  "mag2_min",     NULL},
  {POST_NULL,  "mag2_max",     NULL},
  {POST_NULL,  "mag3_min",     NULL},
  {POST_NULL,  "mag3_max",     NULL},
  {POST_NULL,  "mag4_min",     NULL},
  {POST_NULL,  "mag4_max",     NULL},
  {POST_NULL,  "mag5_min",     NULL},
  {POST_NULL,  "mag5_max",     NULL},
  {POST_NULL,  "mag6_min",     NULL},
  {POST_NULL,  "mag6_max",     NULL},
  {POST_NULL,  "mag7_min",     NULL},
  {POST_NULL,  "mag7_max",     NULL},
  {POST_NULL,  "z_min",        NULL},
  {POST_NULL,  "z_max",        NULL},
  {POST_NULL,  "z_max",        NULL},
  {POST_NULL,  "class",        NULL},
  {POST_NULL,  "subclass",     NULL},
  {POST_NULL,  "snru_min",     NULL},
  {POST_NULL,  "snru_max",     NULL},
  {POST_NULL,  "snrg_min",     NULL},
  {POST_NULL,  "snrg_max",     NULL},
  {POST_NULL,  "snrr_min",     NULL},
  {POST_NULL,  "snrr_max",     NULL},
  {POST_NULL,  "snri_min",     NULL},
  {POST_NULL,  "snri_max",     NULL},
  {POST_NULL,  "snrz_min",     NULL},
  {POST_NULL,  "snrz_max",     NULL},
  {POST_NULL,  "objtype",      NULL},
  {POST_NULL,  "tsource",      NULL},
  {POST_NULL,  "tfrom",        NULL},
  {POST_NULL,  "stellar.teff_min",NULL},
  {POST_NULL,  "stellar.teff_max",NULL},
  {POST_NULL,  "stellar.teff_err_min",NULL},
  {POST_NULL,  "stellar.teff_err_max",NULL},
  {POST_NULL,  "stellar.logg_min",NULL},
  {POST_NULL,  "stellar.logg_max",NULL},
  {POST_NULL,  "stellar.logg_err_min",NULL},
  {POST_NULL,  "stellar.logg_err_max",NULL},
  {POST_NULL,  "stellar.feh_min",NULL},
  {POST_NULL,  "stellar.feh_max",NULL},
  {POST_NULL,  "stellar.feh_err_min",NULL},
  {POST_NULL,  "stellar.feh_err_max",NULL},
  {POST_NULL,  "stellar.rv_min",NULL},
  {POST_NULL,  "stellar.rv_max",NULL},
  {POST_NULL,  "stellar.rv_err_min",NULL},
  {POST_NULL,  "stellar.rv_err_max",NULL},
  {POST_NULL,  "astellar.kp12_min",NULL},
  {POST_NULL,  "astellar.kp12_max",NULL},
  {POST_NULL,  "astellar.kp18_min",NULL},
  {POST_NULL,  "astellar.kp18_max",NULL},
  {POST_NULL,  "astellar.kp6_min",NULL},
  {POST_NULL,  "astellar.kp6_max",NULL},
  {POST_NULL,  "astellar.hdelta12_min",NULL},
  {POST_NULL,  "astellar.hdelta12_max",NULL},
  {POST_NULL,  "astellar.hdelta24_min",NULL},
  {POST_NULL,  "astellar.hdelta24_max",NULL},
  {POST_NULL,  "astellar.hdelta48_min",NULL},
  {POST_NULL,  "astellar.hdelta48_max",NULL},
  {POST_NULL,  "astellar.hdelta64_min",NULL},
  {POST_NULL,  "astellar.hdelta64_max",NULL},
  {POST_NULL,  "astellar.hgamma12_min",NULL},
  {POST_NULL,  "astellar.hgamma12_max",NULL},
  {POST_NULL,  "astellar.hgamma24_min",NULL},
  {POST_NULL,  "astellar.hgamma24_max",NULL},
  {POST_NULL,  "astellar.hgamma48_min",NULL},
  {POST_NULL,  "astellar.hgamma48_max",NULL},
  {POST_NULL,  "astellar.hgamma54_min",NULL},
  {POST_NULL,  "astellar.hgamma54_max",NULL},
  {POST_NULL,  "astellar.hbeta12_min",NULL},
  {POST_NULL,  "astellar.hbeta12_max",NULL},
  {POST_NULL,  "astellar.hbeta24_min",NULL},
  {POST_NULL,  "astellar.hbeta24_max",NULL},
  {POST_NULL,  "astellar.hbeta48_min",NULL},
  {POST_NULL,  "astellar.hbeta48_max",NULL},
  {POST_NULL,  "astellar.hbeta60_min",NULL},
  {POST_NULL,  "astellar.hbeta60_max",NULL},
  {POST_NULL,  "astellar.halpha12_min",NULL},
  {POST_NULL,  "astellar.halpha12_max",NULL},
  {POST_NULL,  "astellar.halpha24_min",NULL},
  {POST_NULL,  "astellar.halpha24_max",NULL},
  {POST_NULL,  "astellar.halpha48_min",NULL},
  {POST_NULL,  "astellar.halpha48_max",NULL},
  {POST_NULL,  "astellar.halpha70_min",NULL},
  {POST_NULL,  "astellar.halpha70_max",NULL},
  {POST_NULL,  "astellar.paschen13_min",NULL},
  {POST_NULL,  "astellar.paschen13_max",NULL},
  {POST_NULL,  "astellar.paschen142_min",NULL},
  {POST_NULL,  "astellar.paschen142_max",NULL},
  {POST_NULL,  "astellar.paschen242_min",NULL},
  {POST_NULL,  "astellar.paschen242_max",NULL},
  {POST_NULL,  "astellar.halpha_d02_min",NULL},
  {POST_NULL,  "astellar.halpha_d02_max",NULL},
  {POST_NULL,  "astellar.hbeta_d02_min",NULL},
  {POST_NULL,  "astellar.hbeta_d02_max",NULL},
  {POST_NULL,  "astellar.hgamma_d02_min",NULL},
  {POST_NULL,  "astellar.hgamma_d02_max",NULL},
  {POST_NULL,  "astellar.hdelta_d02_min",NULL},
  {POST_NULL,  "astellar.hdelta_d02_max",NULL},
  {POST_NULL,  "mstellar.ewha_min",NULL},
  {POST_NULL,  "mstellar.ewha_max",NULL},
  {POST_NULL,  "mstellar.ewerr_min",NULL},
  {POST_NULL,  "mstellar.ewerr_max",NULL},
  {POST_NULL,  "mstellar.tio5_min",NULL},
  {POST_NULL,  "mstellar.tio5_max",NULL},
  {POST_NULL,  "mstellar.cah2_min",NULL},
  {POST_NULL,  "mstellar.cah2_max",NULL},
  {POST_NULL,  "mstellar.cah3_min",NULL},
  {POST_NULL,  "mstellar.cah3_max",NULL},
  {POST_NULL,  "mstellar.tio1_min",NULL},
  {POST_NULL,  "mstellar.tio1_max",NULL},
  {POST_NULL,  "mstellar.tio2_min",NULL},
  {POST_NULL,  "mstellar.tio2_max",NULL},
  {POST_NULL,  "mstellar.tio3_min",NULL},
  {POST_NULL,  "mstellar.tio3_max",NULL},
  {POST_NULL,  "mstellar.tio4_min",NULL},
  {POST_NULL,  "mstellar.tio4_max",NULL},
  {POST_NULL,  "mstellar.cah1_min",NULL},
  {POST_NULL,  "mstellar.cah1_max",NULL},
  {POST_NULL,  "mstellar.caoh_min",NULL},
  {POST_NULL,  "mstellar.caoh_max",NULL},
  {POST_NULL,  "mstellar.tio5err_min",NULL},
  {POST_NULL,  "mstellar.tio5err_max",NULL},
  {POST_NULL,  "mstellar.cah2err_min",NULL},
  {POST_NULL,  "mstellar.cah2err_max",NULL},
  {POST_NULL,  "mstellar.cah3err_min",NULL},
  {POST_NULL,  "mstellar.cah3err_max",NULL},
  {POST_NULL,  "mstellar.tio1err_min",NULL},
  {POST_NULL,  "mstellar.tio1err_max",NULL},
  {POST_NULL,  "mstellar.tio2err_min",NULL},
  {POST_NULL,  "mstellar.tio2err_max",NULL},
  {POST_NULL,  "mstellar.tio3err_min",NULL},
  {POST_NULL,  "mstellar.tio3err_max",NULL},
  {POST_NULL,  "mstellar.tio4err_min",NULL},
  {POST_NULL,  "mstellar.tio4err_max",NULL},
  {POST_NULL,  "mstellar.cah1err_min",NULL},
  {POST_NULL,  "mstellar.cah1err_max",NULL},
  {POST_NULL,  "mstellar.caoherr_min",NULL},
  {POST_NULL,  "mstellar.caoherr_max",NULL},
  {POST_NULL,  "mstellar.zeta_min",NULL},
  {POST_NULL,  "mstellar.zeta_max",NULL},
  {POST_NULL,  "mstellar.zetaerr_min",NULL},
  {POST_NULL,  "mstellar.zetaerr_max",NULL},
  {POST_NULL,  "mstellar.type_min",NULL},
  {POST_NULL,  "mstellar.type_max",NULL},
  {POST_NULL,  "mstellar.na_min",NULL},
  {POST_NULL,  "mstellar.na_max",NULL},
  {POST_CONST, "sBtn",     "Search"},
  {POST_CONST, "output.collection","typical"},
  {POST_CONST, "output.fmt","vot"},  // vot or html
  {POST_CONST, "output.catalogue.obsid",   "on"},
  {POST_CONST, "output.catalogue.designation","on"},
  {POST_CONST, "output.catalogue.obsdate", "on"},
  {POST_CONST, "output.catalogue.lmjd",    "on"},
  {POST_CONST, "output.catalogue.mjd",     "on"},
  {POST_CONST, "output.catalogue.planid",  "on"},
  {POST_CONST, "output.catalogue.spid",    "on"},
  {POST_CONST, "output.catalogue.fiberid", "on"},
  {POST_CONST, "output.catalogue.ra",      "on"},
  {POST_CONST, "output.catalogue.dec",     "on"},
  {POST_CONST, "output.catalogue.class",   "on"},
  {POST_CONST, "output.catalogue.subclass","on"},
  {POST_CONST, "output.catalogue.z",       "on"},
  {POST_CONST, "output.stellar.teff",      "on"},
  {POST_CONST, "output.stellar.logg",      "on"},
  {POST_CONST, "output.stellar.feh",       "on"},
  {POST_CONST, "output.stellar.rv",        "on"},
  {POST_NULL,  NULL, NULL}};

static const PARAMpost lamost_med_post[] = { 
  {POST_CONST, "sForm", "0"},
  {POST_NULL,  "pos.ramin",    NULL},
  {POST_NULL,  "pos.decmin",   NULL},
  {POST_NULL,  "pos.ramax",    NULL},
  {POST_NULL,  "pos.decmax",   NULL},
  {POST_CONST, "pos.type",     "cone"},
  {POST_INPUT, "pos.racenter", NULL},
  {POST_INPUT, "pos.deccenter",NULL},
  {POST_INPUT, "pos.radius",   NULL},
  {POST_NULL,  "pos.radecTextarea",  NULL},
  {POST_NULL,  "obsidTextarea",NULL},
  {POST_NULL,  "fitsnamearea", NULL},
  {POST_NULL,  "mjd",          NULL},
  {POST_NULL,  "planid",       NULL},
  {POST_NULL,  "spid",         NULL},
  {POST_NULL,  "fiberid",      NULL},
  {POST_NULL,  "magtype",      NULL},
  {POST_NULL,  "mag1",         NULL},
  {POST_NULL,  "mag1_max",     NULL},
  {POST_NULL,  "mag2_min",     NULL},
  {POST_NULL,  "mag2_max",     NULL},
  {POST_NULL,  "mag3_min",     NULL},
  {POST_NULL,  "mag3_max",     NULL},
  {POST_NULL,  "mag4_min",     NULL},
  {POST_NULL,  "mag4_max",     NULL},
  {POST_NULL,  "mag5_min",     NULL},
  {POST_NULL,  "mag5_max",     NULL},
  {POST_NULL,  "mag6_min",     NULL},
  {POST_NULL,  "mag6_max",     NULL},
  {POST_NULL,  "mag7_min",     NULL},
  {POST_NULL,  "mag7_max",     NULL},
  {POST_NULL,  "snr_min",      NULL},
  {POST_NULL,  "snr_max",      NULL},
  {POST_NULL,  "objtype",      NULL},
  {POST_NULL,  "tsource",      NULL},
  {POST_NULL,  "tfrom",        NULL},
  {POST_NULL,  "rv_b0_min",    NULL},
  {POST_NULL,  "rv_b0_max",    NULL},
  {POST_NULL,  "rv_b0_err_min",    NULL},
  {POST_NULL,  "rv_b0_err_max",    NULL},
  {POST_NULL,  "rv_b1_min",    NULL},
  {POST_NULL,  "rv_b1_max",    NULL},
  {POST_NULL,  "rv_b1_err_min",    NULL},
  {POST_NULL,  "rv_b1_err_max",    NULL},
  {POST_NULL,  "rv_r0_min",    NULL},
  {POST_NULL,  "rv_r0_max",    NULL},
  {POST_NULL,  "rv_r0_err_min",    NULL},
  {POST_NULL,  "rv_r0_err_max",    NULL},
  {POST_NULL,  "rv_r1_min",    NULL},
  {POST_NULL,  "rv_r1_max",    NULL},
  {POST_NULL,  "rv_r1_err_min",    NULL},
  {POST_NULL,  "rv_r1_err_max",    NULL},
  {POST_NULL,  "rv_r_flag_min",    NULL},
  {POST_NULL,  "rv_r_flag_max",    NULL},
  {POST_NULL,  "rv_br0_min",    NULL},
  {POST_NULL,  "rv_br0_max",    NULL},
  {POST_NULL,  "rv_br0_err_min",    NULL},
  {POST_NULL,  "rv_br0_err_max",    NULL},
  {POST_NULL,  "rv_br1_min",    NULL},
  {POST_NULL,  "rv_br1_max",    NULL},
  {POST_NULL,  "rv_br1_err_min",    NULL},
  {POST_NULL,  "rv_br1_err_max",    NULL},
  {POST_NULL,  "rv_br_flag_min",    NULL},
  {POST_NULL,  "rv_br_flag_max",    NULL},
  {POST_NULL,  "rv_lasp0_min",    NULL},
  {POST_NULL,  "rv_lasp0_max",    NULL},
  {POST_NULL,  "rv_lasp0_err_min",    NULL},
  {POST_NULL,  "rv_lasp0_err_max",    NULL},
  {POST_NULL,  "rv_lasp1_min",    NULL},
  {POST_NULL,  "rv_lasp1_max",    NULL},
  {POST_NULL,  "rv_lasp1_err_min",    NULL},
  {POST_NULL,  "rv_lasp1_err_max",    NULL},
  {POST_NULL,  "rv_lasp1_err_max",    NULL},
  {POST_NULL,  "coadd",    NULL},
  {POST_NULL,  "teff_lasp_min",    NULL},
  {POST_NULL,  "teff_lasp_max",    NULL},
  {POST_NULL,  "teff_lasp_err_min",    NULL},
  {POST_NULL,  "teff_lasp_err_max",    NULL},
  {POST_NULL,  "logg_lasp_min",    NULL},
  {POST_NULL,  "logg_lasp_max",    NULL},
  {POST_NULL,  "logg_lasp_err_min",    NULL},
  {POST_NULL,  "logg_lasp_err_max",    NULL},
  {POST_NULL,  "feh_lasp_min",    NULL},
  {POST_NULL,  "feh_lasp_max",    NULL},
  {POST_NULL,  "feh_lasp_err_min",    NULL},
  {POST_NULL,  "feh_lasp_err_max",    NULL},
  {POST_NULL,  "vsini_lasp_min",    NULL},
  {POST_NULL,  "vsini_lasp_max",    NULL},
  {POST_NULL,  "vsini_lasp_err_min",    NULL},
  {POST_NULL,  "vsini_lasp_err_max",    NULL},
  {POST_NULL,  "alpha_m_cnn_min",    NULL},
  {POST_NULL,  "alpha_m_cnn_max",    NULL},
  {POST_NULL,  "teff_cnn_min",    NULL},
  {POST_NULL,  "teff_cnn_max",    NULL},
  {POST_NULL,  "logg_cnn_min",    NULL},
  {POST_NULL,  "logg_cnn_max",    NULL},
  {POST_NULL,  "feh_cnn_min",    NULL},
  {POST_NULL,  "feh_cnn_max",    NULL},
  {POST_NULL,  "c_fe_min",    NULL},
  {POST_NULL,  "c_fe_max",    NULL},
  {POST_NULL,  "n_fe_min",    NULL},
  {POST_NULL,  "n_fe_max",    NULL},
  {POST_NULL,  "o_fe_min",    NULL},
  {POST_NULL,  "o_fe_max",    NULL},
  {POST_NULL,  "mg_fe_min",    NULL},
  {POST_NULL,  "mg_fe_max",    NULL},
  {POST_NULL,  "al_fe_min",    NULL},
  {POST_NULL,  "al_fe_max",    NULL},
  {POST_NULL,  "si_fe_min",    NULL},
  {POST_NULL,  "si_fe_max",    NULL},
  {POST_NULL,  "s_fe_min",    NULL},
  {POST_NULL,  "s_fe_max",    NULL},
  {POST_NULL,  "ca_fe_min",    NULL},
  {POST_NULL,  "ca_fe_max",    NULL},
  {POST_NULL,  "ti_fe_min",    NULL},
  {POST_NULL,  "ti_fe_max",    NULL},
  {POST_NULL,  "cr_fe_min",    NULL},
  {POST_NULL,  "cr_fe_max",    NULL},
  {POST_NULL,  "ni_fe_min",    NULL},
  {POST_NULL,  "ni_fe_max",    NULL},
  {POST_NULL,  "cu_fe_min",    NULL},
  {POST_NULL,  "cu_fe_max",    NULL},
  {POST_CONST, "output.collection","typical"},
  {POST_CONST, "output.fmt","vot"},  // vot or html
  {POST_CONST, "output.med_catalogue.obsid",   "on"},
  {POST_CONST, "output.med_catalogue.designation","on"},
  {POST_CONST, "output.med_catalogue.obsdate", "on"},
  {POST_CONST, "output.med_catalogue.lmjd",    "on"},
  {POST_CONST, "output.med_catalogue.mjd",     "on"},
  {POST_CONST, "output.med_catalogue.planid",  "on"},
  {POST_CONST, "output.med_catalogue.spid",    "on"},
  {POST_CONST, "output.med_catalogue.fiberid", "on"},
  {POST_CONST, "output.med_catalogue.ra",      "on"},
  {POST_CONST, "output.med_catalogue.dec",     "on"},
  {POST_CONST, "output.med_stellar.teff_lasp",      "on"},
  {POST_CONST, "output.med_stellar.logg_lasp",      "on"},
  {POST_CONST, "output.med_stellar.feh_lasp",       "on"},
  {POST_CONST, "output.med_stellar.rv_b0",        "on"},
  {POST_CONST, "output.med_stellar.rv_b1",        "on"},
  {POST_CONST, "output.med_stellar.rv_r0",        "on"},
  {POST_CONST, "output.med_stellar.rv_r1",        "on"},
  {POST_CONST, "sBtn",     "Search"},
  {POST_NULL,  NULL, NULL}};
