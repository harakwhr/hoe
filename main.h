#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif  

#undef USE_OSX

#include<gtk/gtk.h>
#include <cairo.h>
#include <cairo-pdf.h>

#include<stdio.h>
#include<stdlib.h>
#include<sys/stat.h>
#include<time.h>
#include<signal.h>
#include<unistd.h>
#ifdef HAVE_PWD_H
#include<pwd.h>
#endif
#include<sys/types.h>
#include<errno.h>
#include<math.h>
#include<string.h>

#ifdef USE_WIN32
#include <windows.h>
#include <winnt.h>
#endif

#include "libnova/libnova.h"
#include"hoe_icon.h"
#include"hskymon_icon.h"

#include "std.h"
#include "post.h"
#include "post_sdss.h"
#include "post_lamost.h"
#include "post_smoka.h"
#include "post_hst.h"
#include "post_eso.h"
#include "get_gemini.h"

#ifdef SIGRTMIN
#define SIGHSKYMON1 SIGRTMIN
#define SIGHSKYMON2 SIGRTMIN+1
#else
#define SIGHSKYMON1 SIGUSR1
#define SIGHSKYMON2 SIGUSR2
#endif


#define WWW_BROWSER "google-chrome"

#ifdef USE_WIN32
#define DSS_URL "http://skyview.gsfc.nasa.gov/current/cgi/runquery.pl?Interface=quick&Position=%d+%d+%.2lf%%2C+%s%d+%d+%.2lf&SURVEY=Digitized+Sky+Survey"
#define SIMBAD_URL "http://simbad.harvard.edu/simbad/sim-coo?CooDefinedFrames=none&CooEquinox=2000&Coord=%d%%20%d%%20%.2lf%%20%s%d%%20%d%%20%.2lf&submit=submit%%20query&Radius.unit=arcmin&CooEqui=2000&CooFrame=FK5&Radius=2&output.format=HTML"
#define DR8_URL "http://skyserver.sdss3.org/dr8/en/tools/quicklook/quickobj.asp?ra=%d:%d:%.2lf&dec=%s%d:%d:%.2lf"
#define DR14_URL "http://skyserver.sdss.org/dr14/en/tools/quicklook/summary.aspx?ra=%lf&dec=%s%lf"
#define NED_URL "http://ned.ipac.caltech.edu/cgi-bin/nph-objsearch?search_type=Near+Position+Search&in_csys=Equatorial&in_equinox=J2000.0&lon=%d%%3A%d%%3A%.2lf&lat=%s%d%%3A%d%%3A%.2lf&radius=2.0&hconst=73&omegam=0.27&omegav=0.73&corr_z=1&z_constraint=Unconstrained&z_value1=&z_value2=&z_unit=z&ot_include=ANY&nmp_op=ANY&out_csys=Equatorial&out_equinox=J2000.0&obj_sort=Distance+to+search+center&of=pre_text&zv_breaker=30000.0&list_limit=5&img_stamp=YES"
#define MAST_URL "http://archive.stsci.edu/xcorr.php?target=%.5lf%s%.10lf&max_records=10&action=Search&resolver=SIMBAD&missions[]=EUVE&missions[]=WFC3-IMAGE&missions[]=WFPC1&missions[]=WFPC2&missions[]=FOC&missions[]=ACS-IMAGE&missions[]=UIT&missions[]=STIS-IMAGE&missions[]=COS-IMAGE&missions[]=GALEX&missions[]=XMM-OM&missions[]=NICMOS-IMAGE&missions[]=FUSE&missions[]=IMAPS&missions[]=BEFS&missions[]=TUES&missions[]=IUE&missions[]=COPERNICUS&missions[]=HUT&missions[]=WUPPE&missions[]=GHRS&missions[]=STIS-SPECTRUM&missions[]=COS-SPECTRUM&missions[]=WFC3-SPECTRUM&missions[]=ACS-SPECTRUM&missions[]=FOS&missions[]=HPOL&missions[]=NICMOS-SPECTRUM&missions[]=FGS&missions[]=HSP&missions[]=KEPLER"
#define MASTP_URL "https://mast.stsci.edu/portal/Mashup/Clients/Mast/Portal.html?searchQuery=%lf%%20%s%lf"
#define KECK_URL "https://koa.ipac.caltech.edu/cgi-bin/bgServices/nph-bgExec?bgApp=/KOA/nph-KOA&instrument_de=deimos&instrument_es=esi&instrument_hi=hires&instrument_lr=lris&instrument_lw=lws&instrument_mf=mosfire&instrument_n1=nirc&instrument_n2=nirc2&instrument_ns=nirspec&instrument_os=osiris&filetype=science&calibassoc=assoc&locstr=%.6lf+%+.6lf&regSize=120&resolver=ned&radunits=arcsec&spt_obj=spatial&single_multiple=single"
#define GEMINI_URL "https://archive.gemini.edu/searchform/sr=120/cols=CTOWEQ/notengineering/ra=%.6lf/dec=%+.6lf/NotFail/OBJECT"
#define IRSA_URL "http://irsa.ipac.caltech.edu/cgi-bin/Radar/nph-estimation?mission=All&objstr=%d%%3A%d%%3A%.2lf+%s%d%%3A%d%%3A%.2lf&mode=cone&radius=2&radunits=arcmin&range=6.25+Deg.&data=Data+Set+Type&radnum=2222&irsa=IRSA+Only&submit=Get+Inventory&output=%%2Firsa%%2Fcm%%2Fops_2.0%%2Firsa%%2Fshare%%2Fwsvc%%2FRadar%%2Fcatlist.tbl_type&url=%%2Fworkspace%%2FTMP_3hX3SO_29666&dir=%%2Fwork%%2FTMP_3hX3SO_29666&snull=matches+only&datav=Data+Set+Type"
#define SPITZER_URL "http://sha.ipac.caltech.edu/applications/Spitzer/SHA/#id=SearchByPosition&DoSearch=true&SearchByPosition.field.radius=0.033333333&UserTargetWorldPt=%.5lf;%.10lf;EQ_J2000&SimpleTargetPanel.field.resolvedBy=nedthensimbad&MoreOptions.field.prodtype=aor,pbcd&startIdx=0&pageSize=0&shortDesc=Position&isBookmarkAble=true&isDrillDownRoot=true&isSearchResult=true"
#define CASSIS_URL "http://cassis.sirtf.com/atlas/cgi/radec.py?ra=%.5lf&dec=%.6lf&radius=120"
#define RAPID_URL "http://simbad.harvard.edu/simbad/sim-sam?Criteria=%%28ra>%.2lf%sra<%.2lf%%29%%26dec>%.2lf%%26dec<%.2lf%%26rot.vsini>%d%%26Vmag<%d%%26sptypes<%s&submit=submit%%20query&output.max=%d&OutputMode=LIST"
#define MIRSTD_URL "http://simbad.harvard.edu/simbad/sim-sam?Criteria=%%28ra>%.2lf%sra<%.2lf%%29%%26dec>%.2lf%%26dec<%.2lf%%26iras.f12>%d%%26iras.f25>%d&submit=submit%%20query&output.max=%d&OutputMode=LIST"
#define SSLOC_URL "http://simbad.harvard.edu/simbad/sim-sam?Criteria=cat=%s%%26%%28ra>%.2lf%sra<%.2lf%%29%%26dec>%.2lf%%26dec<%.2lf%%26%%28%s>%d%%26%s<%d%%29%s&submit=submit%%20query&output.max=%d&OutputMode=LIST&output.format=HTML"
#define STD_SIMBAD_URL "http://simbad.harvard.edu/simbad/sim-id?Ident=%s&NbIdent=1&Radius=2&Radius.unit=arcmin&submit=submit+id&output.format=HTML"
#define FCDB_NED_URL "http://ned.ipac.caltech.edu/cgi-bin/objsearch?objname=%s&extend=no&hconst=73&omegam=0.27&omegav=0.73&corr_z=1&out_csys=Equatorial&out_equinox=J2000.0&obj_sort=RA+or+Longitude&of=pre_text&zv_breaker=30000.0&list_limit=5&img_stamp=YES"
#define FCDB_SDSS_URL "http://skyserver.sdss.org/dr14/en/tools/quicklook/summary.aspx?id=%s"
#define FCDB_LAMOST_URL "http://dr3.lamost.org/spectrum/view?obsid=%d"
#define FCDB_SMOKA_URL "http://smoka.nao.ac.jp/info.jsp?frameid=%s&date_obs=%s&i=%d"
#define FCDB_SMOKA_SHOT_URL "http://smoka.nao.ac.jp/fssearch?frameid=%s*&instruments=%s&obs_mod=all&data_typ=all&dispcol=default&diff=1000&action=Search&asciitable=table&obs_cat=all"
#define FCDB_HST_URL "http://archive.stsci.edu/cgi-bin/mastpreview?mission=hst&dataid=%s"
#define FCDB_ESO_URL "http://archive.eso.org/wdb/wdb/eso/eso_archive_main/query?dp_id=%s&format=DecimDeg&tab_stat_plot=on&aladin_colour=aladin_instrument"
#define FCDB_GEMINI_URL "https://archive.gemini.edu/searchform/cols=CTOWEQ/notengineering/NotFail/OBJECT/%s"
#define TRDB_GEMINI_URL "https://archive.gemini.edu/searchform/sr=%d/cols=CTOWEQ/notengineering%sra=%.6lf/%s/science%sdec=%s%.6lf/NotFail/OBJECT"
#define HASH_URL "http://202.189.117.101:8999/gpne/objectInfoPage.php?id=%d"

#elif defined(USE_OSX)
// in OSX    
//    - add  "open" at the beginning
//    - "&" --> "\\&"
#define DSS_URL "open http://skyview.gsfc.nasa.gov/current/cgi/runquery.pl?Interface=quick\\&Position=%d+%d+%.2lf%%2C+%s%d+%d+%.2lf\\&SURVEY=Digitized+Sky+Survey"
#define SIMBAD_URL "open http://simbad.harvard.edu/simbad/sim-coo?CooDefinedFrames=none\\&CooEquinox=2000\\&Coord=%d%%20%d%%20%.2lf%%20%s%d%%20%d%%20%.2lf\\&submit=submit%%20query\\&Radius.unit=arcmin\\&CooEqui=2000\\&CooFrame=FK5\\&Radius=2\\&output.format=HTML"
#define DR8_URL "open http://skyserver.sdss3.org/dr8/en/tools/quicklook/quickobj.asp?ra=%d:%d:%.2lf\\&dec=%s%d:%d:%.2lf"
#define DR14_URL "open http://skyserver.sdss.org/dr14/en/tools/quicklook/summary.aspx?ra=%lf\\&dec=%s%lf"
#define NED_URL "open http://ned.ipac.caltech.edu/cgi-bin/nph-objsearch?search_type=Near+Position+Search\\&in_csys=Equatorial\\&in_equinox=J2000.0\\&lon=%d%%3A%d%%3A%.2lf\\&lat=%s%d%%3A%d%%3A%.2lf\\&radius=2.0\\&hconst=73\\&omegam=0.27\\&omegav=0.73\\&corr_z=1\\&z_constraint=Unconstrained\\&z_value1=\\&z_value2=\\&z_unit=z\\&ot_include=ANY\\&nmp_op=ANY\\&out_csys=Equatorial\\&out_equinox=J2000.0\\&obj_sort=Distance+to+search+center\\&of=pre_text\\&zv_breaker=30000.0\\&list_limit=5\\&img_stamp=YES"
#define MAST_URL "open http://archive.stsci.edu/xcorr.php?target=%.5lf%s%.10lf\\&max_records=10\\&action=Search\\&resolver=SIMBAD\\&missions[]=EUVE\\&missions[]=WFC3-IMAGE\\&missions[]=WFPC1\\&missions[]=WFPC2\\&missions[]=FOC\\&missions[]=ACS-IMAGE\\&missions[]=UIT\\&missions[]=STIS-IMAGE\\&missions[]=COS-IMAGE\\&missions[]=GALEX\\&missions[]=XMM-OM\\&missions[]=NICMOS-IMAGE\\&missions[]=FUSE\\&missions[]=IMAPS\\&missions[]=BEFS\\&missions[]=TUES\\&missions[]=IUE\\&missions[]=COPERNICUS\\&missions[]=HUT\\&missions[]=WUPPE\\&missions[]=GHRS\\&missions[]=STIS-SPECTRUM\\&missions[]=COS-SPECTRUM\\&missions[]=WFC3-SPECTRUM\\&missions[]=ACS-SPECTRUM\\&missions[]=FOS\\&missions[]=HPOL\\&missions[]=NICMOS-SPECTRUM\\&missions[]=FGS\\&missions[]=HSP\\&missions[]=KEPLER"
#define MASTP_URL "open https://mast.stsci.edu/portal/Mashup/Clients/Mast/Portal.html?searchQuery=%lf%%20%s%lf"
#define KECK_URL "open https://koa.ipac.caltech.edu/cgi-bin/bgServices/nph-bgExec?bgApp=/KOA/nph-KOA&instrument_de=deimos&instrument_es=esi&instrument_hi=hires&instrument_lr=lris&instrument_lw=lws&instrument_mf=mosfire&instrument_n1=nirc&instrument_n2=nirc2&instrument_ns=nirspec&instrument_os=osiris&filetype=science&calibassoc=assoc&locstr=%.6lf+%+.6lf&regSize=120&resolver=ned&radunits=arcsec&spt_obj=spatial&single_multiple=single"
#define GEMINI_URL "open https://archive.gemini.edu/searchform/sr=120/cols=CTOWEQ/notengineering/ra=%.6lf/dec=%+.6lf/NotFail/OBJECT"
#define IRSA_URL "open http://irsa.ipac.caltech.edu/cgi-bin/Radar/nph-estimation?mission=All\\&objstr=%d%%3A%d%%3A%.2lf+%s%d%%3A%d%%3A%.2lf\\&mode=cone\\&radius=2\\&radunits=arcmin\\&range=6.25+Deg.\\&data=Data+Set+Type\\&radnum=2222\\&irsa=IRSA+Only\\&submit=Get+Inventory\\&output=%%2Firsa%%2Fcm%%2Fops_2.0%%2Firsa%%2Fshare%%2Fwsvc%%2FRadar%%2Fcatlist.tbl_type\\&url=%%2Fworkspace%%2FTMP_3hX3SO_29666\\&dir=%%2Fwork%%2FTMP_3hX3SO_29666\\&snull=matches+only\\&datav=Data+Set+Type"
#define SPITZER_URL "open http://sha.ipac.caltech.edu/applications/Spitzer/SHA/#id=SearchByPosition\\&DoSearch=true\\&SearchByPosition.field.radius=0.033333333\\&UserTargetWorldPt=%.5lf;%.10lf;EQ_J2000\\&SimpleTargetPanel.field.resolvedBy=nedthensimbad\\&MoreOptions.field.prodtype=aor,pbcd\\&startIdx=0\\&pageSize=0\\&shortDesc=Position\\&isBookmarkAble=true\\&isDrillDownRoot=true\\&isSearchResult=true"
#define CASSIS_URL "open http://cassis.sirtf.com/atlas/cgi/radec.py?ra=%.5lf\\&dec=%.6lf&radius=120"
#define RAPID_URL "open http://simbad.harvard.edu/simbad/sim-sam?Criteria=%%28ra>%.2lf%sra<%.2lf%%29%%26dec>%.2lf%%26dec<%.2lf%%26rot.vsini>%d%%26Vmag<%d%%26sptype<%s\\&submit=submit%%20query\\&output.max=%d\\&OutputMode=LIST"
#define MIRSTD_URL "open http://simbad.harvard.edu/simbad/sim-sam?Criteria=%%28ra>%.2lf%sra<%.2lf%%29%%26dec>%.2lf%%26dec<%.2lf%%26iras.f12>%d%%26iras.f25>%d\\&submit=submit%%20query\\&output.max=%d\\&OutputMode=LIST"
#define SSLOC_URL "open http://simbad.harvard.edu/simbad/sim-sam?Criteria=cat=%s%%26%%28ra>%.2lf%sra<%.2lf%%29%%26dec>%.2lf%%26dec<%.2lf%%26%%28%s>%d%%26%s<%d%%29%s\\&submit=submit%%20query\\&output.max=%d\\&OutputMode=LIST&output.format=HTML"
#define STD_SIMBAD_URL "open http://simbad.harvard.edu/simbad/sim-id?Ident=%s\\&NbIdent=1\\&Radius=2\\&Radius.unit=arcmin\\&submit=submit+id\\&output.format=HTML"
#define FCDB_NED_URL "open http://ned.ipac.caltech.edu/cgi-bin/objsearch?objname=%s\\&extend=no\\&hconst=73\\&omegam=0.27\\&omegav=0.73\\&corr_z=1\\&out_csys=Equatorial\\&out_equinox=J2000.0\\&obj_sort=RA+or+Longitude\\&of=pre_text\\&zv_breaker=30000.0\\&list_limit=5\\&img_stamp=YES"
#define FCDB_SDSS_URL "open http://skyserver.sdss.org/dr14/en/tools/quicklook/summary.aspx?id=%s"
#define FCDB_LAMOST_URL "open http://dr3.lamost.org/spectrum/view?obsid=%d"
#define FCDB_SMOKA_URL "open http://smoka.nao.ac.jp/info.jsp?frameid=%s\\&date_obs=%s\\&i=%d"
#define FCDB_SMOKA_SHOT_URL "open http://smoka.nao.ac.jp/fssearch?frameid=%s*\\&instruments=%s\\&obs_mod=all\\&data_typ=all\\&dispcol=default\\&diff=1000\\&action=Search\\&asciitable=table\\&obs_cat=all"
#define FCDB_HST_URL "open http://archive.stsci.edu/cgi-bin/mastpreview?mission=hst\\&dataid=%s"
#define FCDB_ESO_URL "open http://archive.eso.org/wdb/wdb/eso/eso_archive_main/query?dp_id=%s\\&format=DecimDeg\\&tab_stat_plot=on\\&aladin_colour=aladin_instrument"
#define FCDB_GEMINI_URL "open https://archive.gemini.edu/searchform/cols=CTOWEQ/notengineering/NotFail/OBJECT/%s"
#define TRDB_GEMINI_URL "open https://archive.gemini.edu/searchform/sr=%d/cols=CTOWEQ/notengineering%sra=%.6lf/%s/science%sdec=%s%.6lf/NotFail/OBJECT"
#define HASH_URL "open http://202.189.117.101:8999/gpne/objectInfoPage.php?id=%d"

#else
// in UNIX    
//    - just add a pair of \" at the beginning and the end of each phrase.
#define DSS_URL "\"http://skyview.gsfc.nasa.gov/current/cgi/runquery.pl?Interface=quick&Position=%d+%d+%.2lf%%2C+%s%d+%d+%.2lf&SURVEY=Digitized+Sky+Survey\""
#define SIMBAD_URL "\"http://simbad.harvard.edu/simbad/sim-coo?CooDefinedFrames=none&CooEquinox=2000&Coord=%d%%20%d%%20%.2lf%%20%s%d%%20%d%%20%.2lf&submit=submit%%20query&Radius.unit=arcmin&CooEqui=2000&CooFrame=FK5&Radius=2&output.format=HTML\""
#define DR8_URL "\"http://skyserver.sdss3.org/dr8/en/tools/quicklook/quickobj.asp?ra=%d:%d:%.2lf&dec=%s%d:%d:%.2lf\""
#define DR14_URL "\"http://skyserver.sdss.org/dr14/en/tools/quicklook/summary.aspx?ra=%lf&dec=%s%lf\""
#define NED_URL "\"http://ned.ipac.caltech.edu/cgi-bin/nph-objsearch?search_type=Near+Position+Search&in_csys=Equatorial&in_equinox=J2000.0&lon=%d%%3A%d%%3A%.2lf&lat=%s%d%%3A%d%%3A%.2lf&radius=2.0&hconst=73&omegam=0.27&omegav=0.73&corr_z=1&z_constraint=Unconstrained&z_value1=&z_value2=&z_unit=z&ot_include=ANY&nmp_op=ANY&out_csys=Equatorial&out_equinox=J2000.0&obj_sort=Distance+to+search+center&of=pre_text&zv_breaker=30000.0&list_limit=5&img_stamp=YES\""
#define MAST_URL "\"http://archive.stsci.edu/xcorr.php?target=%.5lf%s%.10lf&max_records=10&action=Search&resolver=SIMBAD&missions[]=EUVE&missions[]=WFC3-IMAGE&missions[]=WFPC1&missions[]=WFPC2&missions[]=FOC&missions[]=ACS-IMAGE&missions[]=UIT&missions[]=STIS-IMAGE&missions[]=COS-IMAGE&missions[]=GALEX&missions[]=XMM-OM&missions[]=NICMOS-IMAGE&missions[]=FUSE&missions[]=IMAPS&missions[]=BEFS&missions[]=TUES&missions[]=IUE&missions[]=COPERNICUS&missions[]=HUT&missions[]=WUPPE&missions[]=GHRS&missions[]=STIS-SPECTRUM&missions[]=COS-SPECTRUM&missions[]=WFC3-SPECTRUM&missions[]=ACS-SPECTRUM&missions[]=FOS&missions[]=HPOL&missions[]=NICMOS-SPECTRUM&missions[]=FGS&missions[]=HSP&missions[]=KEPLER\""
#define MASTP_URL "\"https://mast.stsci.edu/portal/Mashup/Clients/Mast/Portal.html?searchQuery=%lf%%20%s%lf\""
#define KECK_URL "\"https://koa.ipac.caltech.edu/cgi-bin/bgServices/nph-bgExec?bgApp=/KOA/nph-KOA&instrument_de=deimos&instrument_es=esi&instrument_hi=hires&instrument_lr=lris&instrument_lw=lws&instrument_mf=mosfire&instrument_n1=nirc&instrument_n2=nirc2&instrument_ns=nirspec&instrument_os=osiris&filetype=science&calibassoc=assoc&locstr=%.6lf+%+.6lf&regSize=120&resolver=ned&radunits=arcsec&spt_obj=spatial&single_multiple=single\""
#define GEMINI_URL "\"https://archive.gemini.edu/searchform/sr=120/cols=CTOWEQ/notengineering/ra=%.6lf/dec=%+.6lf/NotFail/OBJECT\""
#define IRSA_URL "\"http://irsa.ipac.caltech.edu/cgi-bin/Radar/nph-estimation?mission=All&objstr=%d%%3A%d%%3A%.2lf+%s%d%%3A%d%%3A%.2lf&mode=cone&radius=2&radunits=arcmin&range=6.25+Deg.&data=Data+Set+Type&radnum=2222&irsa=IRSA+Only&submit=Get+Inventory&output=%%2Firsa%%2Fcm%%2Fops_2.0%%2Firsa%%2Fshare%%2Fwsvc%%2FRadar%%2Fcatlist.tbl_type&url=%%2Fworkspace%%2FTMP_3hX3SO_29666&dir=%%2Fwork%%2FTMP_3hX3SO_29666&snull=matches+only&datav=Data+Set+Type\""
#define SPITZER_URL "\"http://sha.ipac.caltech.edu/applications/Spitzer/SHA/#id=SearchByPosition&DoSearch=true&SearchByPosition.field.radius=0.033333333&UserTargetWorldPt=%.5lf;%.10lf;EQ_J2000&SimpleTargetPanel.field.resolvedBy=nedthensimbad&MoreOptions.field.prodtype=aor,pbcd&startIdx=0&pageSize=0&shortDesc=Position&isBookmarkAble=true&isDrillDownRoot=true&isSearchResult=true\""
#define CASSIS_URL "\"http://cassis.sirtf.com/atlas/cgi/radec.py?ra=%.5lf&dec=%.6lf&radius=120\""
#define RAPID_URL "\"http://simbad.harvard.edu/simbad/sim-sam?Criteria=%%28ra>%.2lf%sra<%.2lf%%29%%26dec>%.2lf%%26dec<%.2lf%%26rot.vsini>%d%%26Vmag<%d%%26sptype<%s&submit=submit%%20query&output.max=%d&OutputMode=LIST\""
#define MIRSTD_URL "\"http://simbad.harvard.edu/simbad/sim-sam?Criteria=%%28ra>%.2lf%sra<%.2lf%%29%%26dec>%.2lf%%26dec<%.2lf%%26iras.f12>%d%%26iras.f25>%d&submit=submit%%20query&output.max=%d&OutputMode=LIST\""
#define SSLOC_URL "\"http://simbad.harvard.edu/simbad/sim-sam?Criteria=cat=%s%%26%%28ra>%.2lf%sra<%.2lf%%29%%26dec>%.2lf%%26dec<%.2lf%%26%%28%s>%d%%26%s<%d%%29%s&submit=submit%%20query&output.max=%d&OutputMode=LIST&output.format=HTML\""
#define STD_SIMBAD_URL "\"http://simbad.harvard.edu/simbad/sim-id?Ident=%s&NbIdent=1&Radius=2&Radius.unit=arcmin&submit=submit+id&output.format=HTML\""
#define FCDB_NED_URL "\"http://ned.ipac.caltech.edu/cgi-bin/objsearch?objname=%s&extend=no&hconst=73&omegam=0.27&omegav=0.73&corr_z=1&out_csys=Equatorial&out_equinox=J2000.0&obj_sort=RA+or+Longitude&of=pre_text&zv_breaker=30000.0&list_limit=5&img_stamp=YES\""
#define FCDB_SDSS_URL "\"http://skyserver.sdss.org/dr14/en/tools/quicklook/summary.aspx?id=%s\""
#define FCDB_LAMOST_URL "\"http://dr3.lamost.org/spectrum/view?obsid=%d\""
#define FCDB_SMOKA_URL "\"http://smoka.nao.ac.jp/info.jsp?frameid=%s&date_obs=%s&i=%d\""
#define FCDB_SMOKA_SHOT_URL "\"http://smoka.nao.ac.jp/fssearch?frameid=%s*&instruments=%s&obs_mod=all&data_typ=all&dispcol=default&diff=1000&action=Search&asciitable=table&obs_cat=all\""
#define FCDB_HST_URL "\"http://archive.stsci.edu/cgi-bin/mastpreview?mission=hst&dataid=%s\""
#define FCDB_ESO_URL "\"http://archive.eso.org/wdb/wdb/eso/eso_archive_main/query?dp_id=%s&format=DecimDeg&tab_stat_plot=on&aladin_colour=aladin_instrument\""
#define FCDB_GEMINI_URL "\"https://archive.gemini.edu/searchform/cols=CTOWEQ/notengineering/NotFail/OBJECT/%s\""
#define TRDB_GEMINI_URL "\"https://archive.gemini.edu/searchform/sr=%d/cols=CTOWEQ/notengineering%sra=%.6lf/%s/science%sdec=%s%.6lf/NotFail/OBJECT\""
#define HASH_URL "\"http://202.189.117.101:8999/gpne/objectInfoPage.php?id=%d\""
#endif

#define DSS_ARCMIN_MIN 1
#define DSS_ARCMIN 3
#define DSS_ARCMIN_MAX 120
#define DSS_PIX 1500

#define FC_HOST_STSCI "archive.stsci.edu"
#define FC_PATH_STSCI "/cgi-bin/dss_search?v=%s&r=%d+%d+%lf&d=%s%d+%d+%lf&e=J2000&h=%d.0&w=%d.0&f=gif&c=none&fov=NONE&v3="
#define FC_FILE_GIF "dss.gif"
#define FC_FILE_JPEG "dss.jpg"
#define FC_FILE_HTML "dss.html"

#define FC_HOST_ESO "archive.eso.org"
#define FC_PATH_ESO "/dss/dss?ra=%d%%20%d%%20%lf&dec=%s%d%%20%d%%20%lf&mime-type=image/gif&x=%d.0&y=%d.0&Sky-Survey=%s"


#define FC_HOST_SKYVIEW "skyview.gsfc.nasa.gov"
#define FC_PATH_SKYVIEW "/current/cgi/runquery.pl?survey=%s&coordinates=J%.1lf&projection=Tan&scaling=%s&sampler=LI&lut=colortables/blue-white.bin%ssize=%lf,%lf&pixels=%d&position=%lf,%lf"

#define FC_SRC_STSCI_DSS1R "poss1_red"
#define FC_SRC_STSCI_DSS1B "poss1_blue"
#define FC_SRC_STSCI_DSS2R "poss2ukstu_red"
#define FC_SRC_STSCI_DSS2B "poss2ukstu_blue"
#define FC_SRC_STSCI_DSS2IR "poss2ukstu_ir"

#define FC_SRC_ESO_DSS1R "DSS1"
#define FC_SRC_ESO_DSS2R "DSS2-red"
#define FC_SRC_ESO_DSS2B "DSS2-blue"
#define FC_SRC_ESO_DSS2IR "DSS2-infrared"

#define FC_SRC_SKYVIEW_GALEXF "GALEX%20Far%20UV"
#define FC_SRC_SKYVIEW_GALEXN "GALEX%20Near%20UV"
#define FC_SRC_SKYVIEW_DSS1R "DSS1%20Red"
#define FC_SRC_SKYVIEW_DSS1B "DSS1%20Blue"
#define FC_SRC_SKYVIEW_DSS2R "DSS2%20Red"
#define FC_SRC_SKYVIEW_DSS2B "DSS2%20Blue"
#define FC_SRC_SKYVIEW_DSS2IR "DSS2%20IR"
#define FC_SRC_SKYVIEW_SDSSU "SDSSu"
#define FC_SRC_SKYVIEW_SDSSG "SDSSg"
#define FC_SRC_SKYVIEW_SDSSR "SDSSr"
#define FC_SRC_SKYVIEW_SDSSI "SDSSi"
#define FC_SRC_SKYVIEW_SDSSZ "SDSSz"
#define FC_SRC_SKYVIEW_2MASSJ "2MASS-J"
#define FC_SRC_SKYVIEW_2MASSH "2MASS-H"
#define FC_SRC_SKYVIEW_2MASSK "2MASS-K"
#define FC_SRC_SKYVIEW_WISE34 "WISE%203.4"
#define FC_SRC_SKYVIEW_WISE46 "WISE%204.6"
#define FC_SRC_SKYVIEW_WISE12 "WISE%2012"
#define FC_SRC_SKYVIEW_WISE22 "WISE%2022"
#define FC_SRC_SKYVIEW_NVSS "NVSS"

#define FC_HOST_SDSS "casjobs.sdss.org"
#define FC_PATH_SDSS "/ImgCutoutDR7/getjpeg.aspx?ra=%lf&dec=%+lf&scale=%f&width=%d&height=%d&opt=%s%s&query=%s%s"
#define FC_HOST_SDSS8 "skyservice.pha.jhu.edu"
#define FC_PATH_SDSS8 "/DR8/ImgCutout/getjpeg.aspx?ra=%lf&dec=%lf&scale=%f&opt=&width=%d&height=%d&opt=%s%s&query=%s%s"
#define SDSS_SCALE 0.39612
#define FC_HOST_SDSS13 "skyserver.sdss.org"
#define FC_PATH_SDSS13 "/dr13/SkyServerWS/ImgCutout/getjpeg?TaskName=Skyserver.Chart.image&ra=%lf&dec=%lf&scale=%f&width=%d&height=%d&opt=%s%s&query=%s%s"
#define FC_HOST_PANCOL "ps1images.stsci.edu"
#define FC_PATH_PANCOL "/cgi-bin/ps1cutouts?pos=%lf+%+lf&filter=color&filetypes=stack&auxiliary=data&size=%d&output_size=1024&verbose=0&autoscale=99.500000&catlist="
#define FC_PATH_PANG "/cgi-bin/ps1cutouts?pos=%lf+%+lf&filter=g&filetypes=stack&auxiliary=data&size=%d&output_size=1024&verbose=0&autoscale=99.500000&catlist="
#define FC_PATH_PANR "/cgi-bin/ps1cutouts?pos=%lf+%+lf&filter=r&filetypes=stack&auxiliary=data&size=%d&output_size=1024&verbose=0&autoscale=99.500000&catlist="
#define FC_PATH_PANI "/cgi-bin/ps1cutouts?pos=%lf+%+lf&filter=i&filetypes=stack&auxiliary=data&size=%d&output_size=1024&verbose=0&autoscale=99.500000&catlist="
#define FC_PATH_PANZ "/cgi-bin/ps1cutouts?pos=%lf+%+lf&filter=z&filetypes=stack&auxiliary=data&size=%d&output_size=1024&verbose=0&autoscale=99.500000&catlist="
#define FC_PATH_PANY "/cgi-bin/ps1cutouts?pos=%lf+%+lf&filter=y&filetypes=stack&auxiliary=data&size=%d&output_size=1024&verbose=0&autoscale=99.500000&catlist="

#define STDDB_PATH_SSLOC "/simbad/sim-sam?Criteria=cat=%s%%26%%28ra>%.2lf%sra<%.2lf%%29%%26dec>%.2lf%%26dec<%.2lf%%26%%28%s>%d%%26%s<%d%%29%s&submit=submit%%20query&output.max=%d&OutputMode=LIST&output.format=VOTABLE"
#define STDDB_PATH_RAPID "/simbad/sim-sam?Criteria=%%28ra>%.2lf%sra<%.2lf%%29%%26dec>%.2lf%%26dec<%.2lf%%26rot.vsini>%d%%26Vmag<%d%%26sptype<%s&submit=submit%%20query&output.max=%d&OutputMode=LIST&output.format=VOTABLE"
#define STDDB_PATH_MIRSTD "/simbad/sim-sam?Criteria=%%28ra>%.2lf%sra<%.2lf%%29%%26dec>%.2lf%%26dec<%.2lf%%26iras.f12>%d%%26iras.f25>%d&submit=submit%%20query&output.max=%d&OutputMode=LIST&output.format=VOTABLE"
#define STDDB_FILE_XML "simbad.xml"

#define FCDB_HOST_SIMBAD_STRASBG "simbad.u-strasbg.fr"
#define FCDB_HOST_SIMBAD_HARVARD "simbad.harvard.edu"
#define FCDB_PATH "/simbad/sim-sam?Criteria=region%%28box%%2C%lf%s%lf%%2C%+lfm%+lfm%%29%s%s&submit=submit+query&OutputMode=LIST&maxObject=%d&CriteriaFile=&output.format=VOTABLE"
#define FCDB_FILE_XML "database_fc.xml"
#define FCDB_FILE_TXT "database_fc.txt"
#define FCDB_FILE_HTML "database_fc.html"
#define FCDB_FILE_JSON "database_fc.json"

enum{ FCDB_SIMBAD_STRASBG, FCDB_SIMBAD_HARVARD } FCDBSimbad;

// Instrument
#define HDS_SLIT_MASK_ARCSEC 9.2
//micron
#define HDS_SLIT_LENGTH 10000
#define HDS_SLIT_WIDTH 500
#define HDS_PA_OFFSET (-58.4)
#define HDS_SIZE 3

#define FMOS_SIZE 40
#define FMOS_R_ARCMIN 30

#define SPCAM_X_ARCMIN 34
#define SPCAM_Y_ARCMIN 27
#define SPCAM_GAP_ARCSEC 14.
#define SPCAM_SIZE 40

#define HSC_R_ARCMIN 90
enum{ HSC_DITH_NO, HSC_DITH_5, HSC_DITH_N} HSC_Dith;
#define HSC_DRA 120
#define HSC_DDEC 120
#define HSC_TDITH 15
#define HSC_RDITH 120

#define FOCAS_R_ARCMIN 6
#define FOCAS_GAP_ARCSEC 5.
#define FOCAS_SIZE 10

#define IRCS_X_ARCSEC 54.
#define IRCS_Y_ARCSEC 54.
#define IRCS_SIZE 3
#define IRCS_TTGS_ARCMIN 2

#define COMICS_X_ARCSEC 30.
#define COMICS_Y_ARCSEC 40.
#define COMICS_SIZE 3

#define MOIRCS_X_ARCMIN 4.0
#define MOIRCS_Y_ARCMIN 7.0
#define MOIRCS_GAP_ARCSEC 2.
#define MOIRCS_VIG1X_ARCSEC 29.
#define MOIRCS_VIG1Y_ARCSEC 29.
#define MOIRCS_VIG2X_ARCSEC 47.
#define MOIRCS_VIG2Y_ARCSEC 45.
#define MOIRCS_VIGR_ARCMIN 6.
#define MOIRCS_SIZE 10

#define HSC_SIZE 110

#define HOE_HTTP_ERROR_GETHOST  -1
#define HOE_HTTP_ERROR_SOCKET   -2
#define HOE_HTTP_ERROR_CONNECT  -3
#define HOE_HTTP_ERROR_TEMPFILE -4

// Sky Monitor
#if GTK_CHECK_VERSION(2,8,0)
#define USE_SKYMON
#else
#undef USE_SKYMON
#endif

#define SKYMON_WINSIZE 500

#ifdef USE_WIN32
#define SKYMON_FONT "Arial 10"
#else
#define SKYMON_FONT "Arial 10"
#endif

// $B%]%C%W%"%C%W%a%C%;!<%8(B
#define GTK_MSG
// $B%(%i!<%]%C%W%"%C%W$N%?%$%`%"%&%H(B[sec]
#define POPUP_TIMEOUT 2


//#define VERSION "0.8.0"
#define AZEL_INTERVAL 60*1000

#ifdef USE_SKYMON
#define SKYMON_INTERVAL 200
#define SKYMON_STEP 10
#endif

#define MAX_OBJECT 5000
#define MAX_ROPE 32
#define MAX_PLAN 200
#define MAX_PP 200
#define MAX_STD 100
#define MAX_FCDB 5000
#define MAX_TRDB_BAND 100

#define DEF_EXP 600

#define DEF_SV_EXP 1000
#define DEF_SV_CALC 60
#define DEF_SV_SLITX 314.0
#define DEF_SV_SLITY 315.0
#define DEF_SV_ISX 411.0
#define DEF_SV_ISY 332.0
#define DEF_SV_IS3X 446.0
#define DEF_SV_IS3Y 346.0

#define CAMZ_B -356
#define CAMZ_R -326

#define D_CROSS 130

#define MAG_SVFILTER1 8.0
#define MAG_SVFILTER2 4.0

#define BUFFSIZE 65535

#define LONGITUDE_SUBARU -155.4706 //[deg]
#define LATITUDE_SUBARU 19.8255    //[deg]
#define ALTITUDE_SUBARU 4163    //[m]
//#define LONGITUDE_SUBARU -(155.+28./60.+50./3600.) //[deg]
//#define LATITUDE_SUBARU (19.+49./60.+43./3600.)    //[deg]

#define TIMEZONE_SUBARU -10
#define WAVE1_SUBARU 3500   //A
#define WAVE0_SUBARU 5500   //A
#define TEMP_SUBARU 0       //C
#define PRES_SUBARU 625     //hPa

#define DEF_ECHELLE 900


#define TIME_SETUP_FIELD 300
#define TIME_ACQ 60
#define TIME_FOCUS_AG 300
#define TIME_SETUP_FULL 600
#define TIME_SETUP_EASY 120
#define TIME_SETUP_SLIT 60
#define TIME_I2 60
#define TIME_COMP 180
#define TIME_FLAT 180




// Setup
enum{ StdUb, StdUa, StdBa, StdBc, StdYa, StdI2b, StdYd, StdYb, StdYc, StdI2a, StdRa, StdRb, StdNIRc, StdNIRb, StdNIRa, StdHa} StdSetup;

// Binning Mode
enum{ BIN11, BIN21, BIN22, BIN24, BIN41, BIN44} BinMode;
#define MAX_BINNING BIN44+1
#define HALF_BINNING BIN22+1

enum{ AZEL_NORMAL, AZEL_POSI, AZEL_NEGA} AZElMode;

// Image Rotator
enum{ IMR_NO, IMR_LINK, IMR_ZENITH} ImRMode;

// Image Slicer
enum{ IS_NO, IS_030X5, IS_045X3, IS_020X3} ISMode;

#define IS_FLAT_FACTOR 1.35

// Color
enum{COL_BLUE, COL_RED} CrossColor;

enum{
  PLAN_TYPE_COMMENT,
  PLAN_TYPE_OBJ,
  PLAN_TYPE_FOCUS,
  PLAN_TYPE_BIAS,
  PLAN_TYPE_FLAT,
  PLAN_TYPE_COMP,
  PLAN_TYPE_SETUP,
  PLAN_TYPE_I2,
  PLAN_TYPE_SetAzEl
} PlanType;

enum{
  PLAN_CMODE_FULL,
  PLAN_CMODE_EASY,
  PLAN_CMODE_SLIT
} PlanCmode;

enum{
  PLAN_FOCUS_SV,
  PLAN_FOCUS_AG
} PlanFocus;

enum{
  PLAN_OMODE_FULL,
  PLAN_OMODE_SET,
  PLAN_OMODE_GET
} PlanOmode;

enum{
  PLAN_I2_IN,
  PLAN_I2_OUT
} PlanI2;

enum{
  PLAN_START_EVENING,
  PLAN_START_SPECIFIC
} PlanStart;

enum{
  PLAN_COMMENT_TEXT,
  PLAN_COMMENT_SUNRISE,
  PLAN_COMMENT_SUNSET
} PlanComment;

enum{
  SV_FILTER_NONE,
  SV_FILTER_R,
  SV_FILTER_BP530,
  SV_FILTER_ND2,
} SVFilter;

enum{
  NOTE_GENERAL,
  NOTE_AG,
  NOTE_HDS,
  NOTE_OBJ,
  NOTE_STDDB,
  NOTE_FCDB,
  NOTE_TRDB,
  NOTE_LINE
} ALL_NOTE;


enum
{
  COLUMN_OBJTREE_CHECK,
  COLUMN_OBJTREE_NUMBER,
  COLUMN_OBJTREE_NAME,
  COLUMN_OBJTREE_EXP,
  COLUMN_OBJTREE_REPEAT,
  COLUMN_OBJTREE_MAG,
  COLUMN_OBJTREE_RA,
  COLUMN_OBJTREE_DEC,
  COLUMN_OBJTREE_EQUINOX,
  COLUMN_OBJTREE_HORIZON,
  COLUMN_OBJTREE_RISE,
  COLUMN_OBJTREE_RISE_COL,
  COLUMN_OBJTREE_TRANSIT,
  COLUMN_OBJTREE_TRANSIT_COL,
  COLUMN_OBJTREE_SET,
  COLUMN_OBJTREE_SET_COL,
  COLUMN_OBJTREE_PA,
  COLUMN_OBJTREE_GUIDE,
  COLUMN_OBJTREE_SETUP1,
  COLUMN_OBJTREE_SETUP2,
  COLUMN_OBJTREE_SETUP3,
  COLUMN_OBJTREE_SETUP4,
  COLUMN_OBJTREE_SETUP5,
  COLUMN_OBJTREE_NOTE,
  NUM_OBJTREE_COLUMNS
};


// StdTreeview
enum
{
  COLUMN_STD_NUMBER,
  COLUMN_STD_NAME,
  COLUMN_STD_RA,
  COLUMN_STD_DEC,
  COLUMN_STD_SP,
  COLUMN_STD_SEP,
  COLUMN_STD_ROT,
  COLUMN_STD_U,
  COLUMN_STD_B,
  COLUMN_STD_V,
  COLUMN_STD_R,
  COLUMN_STD_I,
  COLUMN_STD_J,
  COLUMN_STD_H,
  COLUMN_STD_K,
  NUM_COLUMN_STD
};

// FCDBTreeview
enum
{
  COLUMN_FCDB_NUMBER,
  COLUMN_FCDB_NAME,
  COLUMN_FCDB_RA,
  COLUMN_FCDB_DEC,
  COLUMN_FCDB_SEP,
  COLUMN_FCDB_OTYPE,
  COLUMN_FCDB_SP,
  COLUMN_FCDB_U,
  COLUMN_FCDB_B,
  COLUMN_FCDB_V,
  COLUMN_FCDB_R,
  COLUMN_FCDB_I,
  COLUMN_FCDB_J,
  COLUMN_FCDB_H,
  COLUMN_FCDB_K,
  COLUMN_FCDB_NEDMAG,
  COLUMN_FCDB_NEDZ,
  COLUMN_FCDB_REF,
  COLUMN_FCDB_PLX,
  COLUMN_FCDB_FID,
  COLUMN_FCDB_DATE,
  COLUMN_FCDB_MODE,
  COLUMN_FCDB_TYPE,
  COLUMN_FCDB_FIL,
  COLUMN_FCDB_WV,
  COLUMN_FCDB_OBS,
  NUM_COLUMN_FCDB
};


// Tree DataBase
enum
{
  COLUMN_TRDB_NUMBER,
  COLUMN_TRDB_NAME,
  COLUMN_TRDB_DATA,
  NUM_COLUMN_TRDB
};



enum
{
  COLUMN_NUMBER_TEXT,
  NUM_NUMBER_COLUMNS
};



#define MAX_SETUP StdHa+1

#define MAX_USESETUP 5
// You should edit create_items_model in objtree.c, 
// when you change this value. (Number of G_TYPE_BOOLEAN)
// Add COLUMN_OBJTREE_SETUP%d above, too

#define MAX_NONSTD 4

#define PS_FILE "plot.ps"
#define PA_INPUT "pos.ip"

#define OPE_EXTENSION "ope"
#define HOE_EXTENSION "hoe"
#define LIST1_EXTENSION "list"
#define LIST2_EXTENSION "lst"
#define LIST3_EXTENSION "txt"
#define PLAN_EXTENSION "_plan.txt"
#define PDF_EXTENSION "pdf"
#define YAML_EXTENSION "yml"
#define CSV_EXTENSION "csv"

#define MAX_LINE 20
enum{PLOT_PSFILE, PLOT_XWIN} plot_device;
enum{MODE_EFS, MODE_FSR} ModeEFS;

// OCS version
enum{ OCS_SOSS, OCS_GEN2} OCSVer;


// Finding Chart
// Finding Chart
enum{FC_STSCI_DSS1R, 
     FC_STSCI_DSS1B, 
     FC_STSCI_DSS2R,
     FC_STSCI_DSS2B,
     FC_STSCI_DSS2IR,
     FC_SEP1,
     FC_ESO_DSS1R,
     FC_ESO_DSS2R,
     FC_ESO_DSS2B,
     FC_ESO_DSS2IR,
     FC_SEP2,
     FC_SKYVIEW_GALEXF,
     FC_SKYVIEW_GALEXN,
     FC_SKYVIEW_DSS1R,
     FC_SKYVIEW_DSS1B,
     FC_SKYVIEW_DSS2R,
     FC_SKYVIEW_DSS2B,
     FC_SKYVIEW_DSS2IR,
     FC_SKYVIEW_SDSSU,
     FC_SKYVIEW_SDSSG,
     FC_SKYVIEW_SDSSR,
     FC_SKYVIEW_SDSSI,
     FC_SKYVIEW_SDSSZ,
     FC_SKYVIEW_2MASSJ,
     FC_SKYVIEW_2MASSH,
     FC_SKYVIEW_2MASSK,
     FC_SKYVIEW_WISE34,
     FC_SKYVIEW_WISE46,
     FC_SKYVIEW_WISE12,
     FC_SKYVIEW_WISE22,
     FC_SKYVIEW_NVSS,
     FC_SEP3,
     FC_SDSS,
     FC_SDSS13,
     FC_SEP4,
     FC_PANCOL,
     FC_PANG,
     FC_PANR,
     FC_PANI,
     FC_PANZ,
     FC_PANY} ModeFC;

#define PANSTARRS_MAX_ARCMIN 25

// Guiding mode
enum{ NO_GUIDE, AG_GUIDE, SV_GUIDE, SVSAFE_GUIDE, NUM_GUIDE_MODE} GuideMode;

// SV Read Area
enum{ SV_PART, SV_FULL} SVArea;

#ifdef USE_SKYMON
// SKYMON Mode
enum{ SKYMON_CUR, SKYMON_SET, SKYMON_PLAN_OBJ, SKYMON_PLAN_TIME} SkymonMode;

#define SUNSET_OFFSET 25
#define SUNRISE_OFFSET 25

#define SKYMON_DEF_OBJSZ 10
#endif

// SIZE$B!!(BOF GUI ENTRY
#define SMALL_ENTRY_SIZE 24
#define LARGE_ENTRY_SIZE 28

#define HSKYMON_HTTP_ERROR_GETHOST  -1
#define HSKYMON_HTTP_ERROR_SOCKET   -2
#define HSKYMON_HTTP_ERROR_CONNECT  -3
#define HSKYMON_HTTP_ERROR_TEMPFILE -4
#ifdef USE_SSL
#define HSKYMON_HTTP_ERROR_SSL -5
#endif

// SOSs
#define SOSS_HOSTNAME "sumda.sum.subaru.nao.ac.jp"
#define SOSS_PATH "Procedure"	 //#define SOSS_PATH "tmp"
#define PY_COM "python"
#define SFTP_PY "hoe_sftp.py"
#define SFTP_LOG "hoe_sftp.log"

// Plot Mode
enum{ PLOT_EL, PLOT_AZ, PLOT_AD} PlotMode;
enum{ PLOT_OBJTREE, PLOT_PLAN} PlotTarget;
enum{ PLOT_OUTPUT_WINDOW, PLOT_OUTPUT_PDF} PlotOutput;
enum{ SKYMON_OUTPUT_WINDOW, SKYMON_OUTPUT_PDF} SkymonOutput;
enum{ PLOT_ALL_SINGLE, PLOT_ALL_SELECTED,PLOT_ALL_ALL,PLOT_ALL_PLAN} PlotAll;

#define PLOT_INTERVAL 60*1000

#define PLOT_WINSIZE 400

#define PLOT_WIDTH 600
#define PLOT_HEIGHT 400

#define FC_WINSIZE 400
enum{ FC_OUTPUT_WINDOW, FC_OUTPUT_PDF, FC_OUTPUT_PRINT, FC_OUTPUT_PDF_ALL} FCOutput;
enum{ FC_INST_HDS, FC_INST_HDSAUTO, FC_INST_HDSZENITH, FC_INST_NONE, FC_INST_IRCS, FC_INST_COMICS, FC_INST_FOCAS, FC_INST_MOIRCS, FC_INST_FMOS, FC_INST_SPCAM, FC_INST_HSCDET,FC_INST_HSCA, FC_INST_NO_SELECT} FCInst;
enum{ FC_SCALE_LINEAR, FC_SCALE_LOG, FC_SCALE_SQRT, FC_SCALE_HISTEQ, FC_SCALE_LOGLOG} FCScale;

#define EFS_WIDTH 800
#define EFS_HEIGHT 600
enum{ EFS_PLOT_EFS, EFS_PLOT_FSR} EFSMode;
enum{ EFS_OUTPUT_WINDOW, EFS_OUTPUT_PDF} EFSOutput;

#define SKYMON_WIDTH 600
#define SKYMON_HEIGHT 600

//=====================  FCDB  =====================//

#define FCDB_ARCMIN_MAX 100
#define FCDB_PS1_ARCMIN_MAX 60
#define FCDB_USNO_ARCMIN_MAX 24

#define STDDB_PATH_SSLOC "/simbad/sim-sam?Criteria=cat=%s%%26%%28ra>%.2lf%sra<%.2lf%%29%%26dec>%.2lf%%26dec<%.2lf%%26%%28%s>%d%%26%s<%d%%29%s&submit=submit%%20query&output.max=%d&OutputMode=LIST&output.format=VOTABLE"
#define STDDB_PATH_RAPID "/simbad/sim-sam?Criteria=%%28ra>%.2lf%sra<%.2lf%%29%%26dec>%.2lf%%26dec<%.2lf%%26rot.vsini>%d%%26Vmag<%d%%26sptype<%s&submit=submit%%20query&output.max=%d&OutputMode=LIST&output.format=VOTABLE"
#define STDDB_PATH_MIRSTD "/simbad/sim-sam?Criteria=%%28ra>%.2lf%sra<%.2lf%%29%%26dec>%.2lf%%26dec<%.2lf%%26iras.f12>%d%%26iras.f25>%d&submit=submit%%20query&output.max=%d&OutputMode=LIST&output.format=VOTABLE"
#define STDDB_FILE_XML "simbad.xml"

#define FCDB_HOST_SIMBAD_STRASBG "simbad.u-strasbg.fr"
#define FCDB_HOST_SIMBAD_HARVARD "simbad.harvard.edu"
#define FCDB_PATH "/simbad/sim-sam?Criteria=region%%28box%%2C%lf%s%lf%%2C%+lfm%+lfm%%29%s%s&submit=submit+query&OutputMode=LIST&maxObject=%d&CriteriaFile=&output.format=VOTABLE"
#define FCDB_FILE_XML "database_fc.xml"
#define FCDB_FILE_TXT "database_fc.txt"
#define FCDB_FILE_HTML "database_fc.html"
#define FCDB_FILE_JSON "database_fc.json"

#define FCDB_HOST_NED "ned.ipac.caltech.edu"
#define FCDB_NED_PATH "/cgi-bin/nph-objsearch?search_type=Near+Position+Search&in_csys=Equatorial&in_equinox=J2000.0&lon=%d%%3A%d%%3A%.2lf&lat=%s%d%%3A%d%%3A%.2lf&radius=%.2lf&hconst=73&omegam=0.27&omegav=0.73&corr_z=1&z_constraint=Unconstrained&z_value1=&z_value2=&z_unit=z&ot_include=ANY&nmp_op=ANY%sout_csys=Equatorial&out_equinox=J2000.0&obj_sort=Distance+to+search+center&of=pre_text&zv_breaker=30000.0&list_limit=0&img_stamp=YES&of=xml_main"

#define FCDB_HOST_GSC "gsss.stsci.edu"
#define FCDB_GSC_PATH "/webservices/vo/ConeSearch.aspx?RA=%lf&DEC=%+lf&SR=%lf%sMAX_OBJ=5000&FORMAT=VOTABLE"

#define FCDB_HOST_PS1 "gsss.stsci.edu"
#define FCDB_PS1_PATH  "/webservices/vo/CatalogSearch.aspx?CAT=PS1V3OBJECTS&RA=%lf&DEC=%+lf&SR=%lf&MINDET=%d%sMAXOBJ=5000"

#define FCDB_HOST_SDSS "skyserver.sdss.org"
#define FCDB_SDSS_PATH "/dr14/en/tools/search/x_results.aspx"

#define FCDB_HOST_USNO "www.nofs.navy.mil"
#define FCDB_USNO_PATH "/cgi-bin/vo_cone.cgi?CAT=USNO-B1&RA=%lf&DEC=%+lf&SR=%lf%sVERB=1"

#define FCDB_HOST_GAIA "vizier.u-strasbg.fr"
#define FCDB_GAIA_PATH "/viz-bin/votable?-source=I/337/gaia&-c=%lf%%20%+lf&-c.u=deg&-c.bs=%dx%d&-c.geom=r&-out.max=5000%s-out.form=VOTable"

#define FCDB_HOST_2MASS "gsss.stsci.edu"
#define FCDB_2MASS_PATH "/webservices/vo/CatalogSearch.aspx?CAT=2MASS&RA=%lf&DEC=%+lf&SR=%lf%sMAXOBJ=5000"

#define FCDB_HOST_WISE "vizier.u-strasbg.fr"
#define FCDB_WISE_PATH "/viz-bin/votable?-source=II/311/wise&-c=%lf%%20%+lf&-c.u=deg&-c.bs=%dx%d&-c.geom=r&-out.max=5000%s-out.form=VOTable"

#define FCDB_HOST_IRC "vizier.u-strasbg.fr"
#define FCDB_IRC_PATH "/viz-bin/votable?-source=II/297/irc&-c=%lf%%20%+lf&-c.u=deg&-c.bs=%dx%d&-c.geom=r&-out.max=5000&-out.form=VOTable"

#define FCDB_HOST_FIS "vizier.u-strasbg.fr"
#define FCDB_FIS_PATH "/viz-bin/votable?-source=II/298/fis&-c=%lf%%20%+lf&-c.u=deg&-c.bs=%dx%d&-c.geom=r&-out.max=5000&-out.form=VOTable"

#define FCDB_HOST_LAMOST "dr3.lamost.org"
#define FCDB_LAMOST_PATH "/q"

#define FCDB_HOST_SMOKA "smoka.nao.ac.jp"
#define FCDB_SMOKA_PATH "/fssearch"

#define FCDB_HOST_HST "archive.stsci.edu"
#define FCDB_HST_PATH "/hst/search.php"

#define FCDB_HOST_ESO "archive.eso.org"
#define FCDB_ESO_PATH "/wdb/wdb/eso/eso_archive_main/query"

#define FCDB_HOST_GEMINI "archive.gemini.edu"
#define FCDB_GEMINI_PATH "/jsonsummary/sr=%d/notengineering%sra=%.6lf/science/dec=%s%.6lf/NotFail/OBJECT/present/canonical"
#define TRDB_GEMINI_PATH "/jsonsummary/sr=%d/notengineering%sra=%.6lf/%s/science%sdec=%s%.6lf/NotFail/OBJECT/present/canonical"



#define ADDOBJ_SIMBAD_PATH "/simbad/sim-id?Ident=%s&NbIdent=1&Radius=2&Radius.unit=arcmin&submit=submit+id&output.format=VOTABLE"
#define ADDOBJ_NED_PATH "/cgi-bin/objsearch?objname=%s&extend=no&hconst=73&omegam=0.27&omegav=0.73&corr_z=1&out_csys=Equatorial&out_equinox=J2000.0&obj_sort=RA+or+Longitude&of=pre_text&zv_breaker=30000.0&list_limit=5&img_stamp=YES&of=xml_main"

#define FC_MAX_MAG 5

// FCDB_TYPE
enum
{
  FCDB_TYPE_SIMBAD,
  FCDB_TYPE_NED,
  FCDB_TYPE_GSC,
  FCDB_TYPE_PS1,
  FCDB_TYPE_SDSS,
  FCDB_TYPE_LAMOST,
  FCDB_TYPE_USNO,
  FCDB_TYPE_GAIA,
  FCDB_TYPE_2MASS,
  FCDB_TYPE_WISE,
  FCDB_TYPE_IRC,
  FCDB_TYPE_FIS,
  FCDB_TYPE_SMOKA,
  FCDB_TYPE_HST,
  FCDB_TYPE_ESO,
  FCDB_TYPE_GEMINI,
  FCDB_TYPE_WWWDB_SMOKA,
  FCDB_TYPE_WWWDB_HST,
  FCDB_TYPE_WWWDB_ESO,
  TRDB_TYPE_SMOKA,
  TRDB_TYPE_HST,
  TRDB_TYPE_ESO,
  TRDB_TYPE_GEMINI,
  TRDB_TYPE_WWWDB_SMOKA,
  TRDB_TYPE_WWWDB_HST,
  TRDB_TYPE_WWWDB_ESO,
  TRDB_TYPE_FCDB_SMOKA,
  TRDB_TYPE_FCDB_HST,
  TRDB_TYPE_FCDB_ESO,
  TRDB_TYPE_FCDB_GEMINI
};

enum{ WWWDB_SIMBAD, 
      WWWDB_NED, 
      WWWDB_DR8, 
      WWWDB_DR14, 
      WWWDB_MAST, 
      WWWDB_MASTP,
      WWWDB_KECK, 
      WWWDB_GEMINI, 
      WWWDB_IRSA, 
      WWWDB_SPITZER, 
      WWWDB_CASSIS, 
      WWWDB_SEP1, 
      WWWDB_SSLOC, 
      WWWDB_RAPID, 
      WWWDB_MIRSTD, 
      WWWDB_SEP2, 
      WWWDB_SMOKA, 
      WWWDB_HST, 
      WWWDB_ESO} WWWDBMode;

enum{ STDDB_SSLOC, 
      STDDB_RAPID, 
      STDDB_MIRSTD, 
      STDDB_ESOSTD, 
      STDDB_IRAFSTD, 
      STDDB_CALSPEC, 
      STDDB_HDSSTD} STDDBMode;

#define STD_DRA 20
#define STD_DDEC 10
#define STD_VSINI 100
#define STD_VMAG 8
#define STD_SPTYPE "A0"
#define STD_IRAS12 5
#define STD_IRAS25 10
#define STD_CAT "FS"
#define STD_MAG1 5
#define STD_MAG2 15
#define STD_BAND "Jmag"
#define STD_SPTYPE_ALL "%20"
#define STD_SPTYPE_O   "%26(sptype>=O0%26sptype<=O9.9)"
#define STD_SPTYPE_B   "%26(sptype>=B0%26sptype<=B9.9)"
#define STD_SPTYPE_A   "%26(sptype>=A0%26sptype<=A9.9)"
#define STD_SPTYPE_F   "%26(sptype>=F0%26sptype<=F9.9)"
#define STD_SPTYPE_G   "%26(sptype>=G0%26sptype<=G9.9)"
#define STD_SPTYPE_K   "%26(sptype>=K0%26sptype<=K9.9)"
#define STD_SPTYPE_M   "%26(sptype>=M0%26sptype<=M11.9)"

enum
{
  FCDB_BAND_NOP,
  FCDB_BAND_U,
  FCDB_BAND_B,
  FCDB_BAND_V,
  FCDB_BAND_R,
  FCDB_BAND_I,
  FCDB_BAND_J,
  FCDB_BAND_H,
  FCDB_BAND_K,
  NUM_FCDB_BAND
};

enum
{
  FCDB_OTYPE_ALL,
  FCDB_OTYPE_STAR,
  FCDB_OTYPE_ISM,
  FCDB_OTYPE_PN,
  FCDB_OTYPE_HII,
  FCDB_OTYPE_GALAXY,
  FCDB_OTYPE_QSO,
  FCDB_OTYPE_GAMMA,
  FCDB_OTYPE_X,
  FCDB_OTYPE_IR,
  FCDB_OTYPE_RADIO,
  NUM_FCDB_OTYPE
};

enum
{
  FCDB_NED_OTYPE_ALL,
  FCDB_NED_OTYPE_EXTRAG,
  FCDB_NED_OTYPE_QSO,
  FCDB_NED_OTYPE_STAR,
  FCDB_NED_OTYPE_SN,
  FCDB_NED_OTYPE_PN,
  FCDB_NED_OTYPE_HII,
  NUM_FCDB_NED_OTYPE
};

//=====================  end of FCDB  =====================//


typedef struct _SetupEntry SetupEntry;
struct _SetupEntry{
  gchar *initial;
  gchar *col;
  gchar *cross;
  gchar *fil1;
  gchar *fil2;

  gchar *f1_amp;
  guint f1_fil1;
  guint f1_fil2;
  guint f1_fil3;
  guint f1_fil4;
  guint f1_exp;

  gchar *f2_amp;
  guint f2_fil1;
  guint f2_fil2;
  guint f2_fil3;
  guint f2_fil4;
  guint f2_exp;

  gdouble slit_length;
  gdouble cross_scan;
};


typedef struct _Binpara Binpara;
struct _Binpara{
  gchar *name;
  guint x;
  guint y;
  guint readout;
};


typedef struct _EPHpara EPHpara;
struct _EPHpara{
  gdouble jd;
  gdouble ra;
  gdouble dec;
  gdouble equinox;
  gdouble geo_d;
};

typedef struct _NSTpara NSTpara;
struct _NSTpara{
  gchar*  filename;
  gint    i_max;
  EPHpara* eph;
  gint    c_fl;
  gint    s_fl;
};


typedef struct _OBJpara OBJpara;
struct _OBJpara{
  gchar *name;
  gdouble ra;
  gdouble dec;
  gdouble equinox;
  gdouble mag;

  gint i_nst;

  gint horizon;
  gdouble rise;
  gdouble transit;
  gdouble set;

  gint guide;
  
  gdouble pa;
  gboolean setup[MAX_USESETUP];

  gint exp;
  gint repeat;
  gint svfilter;

  /*
  GtkWidget *w_az;
  GtkWidget *w_ha;
  GtkWidget *w_pa;
  */
  gdouble c_az;
  gdouble c_el;
  gdouble c_ha;
  gdouble c_pa;
  gdouble c_ad;
  gdouble c_hpa;

#ifdef USE_SKYMON
  gdouble s_az;
  gdouble s_el;
  gdouble s_ha;
  gdouble s_pa;
  gdouble s_ad;
  gdouble s_hpa;
#endif

  gboolean check_sm;

  gchar *note;

  gchar *trdb_str;
  gchar *trdb_mode[MAX_TRDB_BAND];
  gchar *trdb_band[MAX_TRDB_BAND];
  gdouble trdb_exp[MAX_TRDB_BAND];
  gint trdb_shot[MAX_TRDB_BAND];
  gint trdb_band_max;
};

typedef struct _PLANpara PLANpara;
struct _PLANpara{
  guint type;
  gchar *txt;
  
  guint setup;  // Obj Flat
  guint repeat;  // Obj Flat Bias
  gboolean slit_or;
  guint slit_width;  
  guint slit_length;
  
  // Get Object
  gint  obj_i;
  //gchar *objname;
  //gchar *objnote;
  //gchar *tgt;
  guint exp;

  guint omode;
  gint guide;


  // BIAS
  // Flat
  // Comp
  // Focus
  guint focus_mode;

  // Setup
  guint cmode;

  // I2
  guint i2_pos;

  gboolean daytime;
  
  gchar *comment;
  guint comtype;

  gint time;
  glong sod;

  gboolean pa_or;
  gdouble pa;
  gboolean sv_or;
  guint sv_exp;
  guint sv_fil;
  gboolean backup;

  gdouble setaz;
  gdouble setel;

  gdouble az0;
  gdouble az1;
  gdouble el0;
  gdouble el1;

  gchar *txt_az;
  gchar *txt_el;
};


typedef struct _PPpara PPpara;
struct _PPpara{
  gdouble ut;

  gdouble az;
  gdouble el;

  guint i_plan;
  
  gboolean start;
};


typedef struct _PApara PApara;
struct _PApara{
  gchar *name;
  gdouble ra;
  gdouble dec;

  gint year;
  gint month;
  gint day;
};


typedef struct _Setuppara Setuppara;
struct _Setuppara{
  gint  setup;
  gboolean use;
  guint    binning;
  guint    slit_width;
  guint    slit_length;
  gchar    *fil1;
  gchar    *fil2;
  guint    imr;
  guint    is;
  gboolean i2;
};


typedef struct _Nonstdpara Nonstdpara;
struct _Nonstdpara{
  guint col;
  guint cross;
  gint  echelle;
  gint  camr;
};

typedef struct _Linepara Linepara;
struct _Linepara{
  gchar *name;
  gdouble  wave;
};

typedef struct _STDpara STDpara;
struct _STDpara{
  gchar *name;
  gdouble ra;
  gdouble dec;
  gdouble d_ra;
  gdouble d_dec;
  gdouble pmra;
  gdouble pmdec;
  gboolean pm;
  gdouble equinox;
  gchar *sp;
  gdouble sep;
  gdouble rot;
  gdouble u;
  gdouble b;
  gdouble v;
  gdouble r;
  gdouble i;
  gdouble j;
  gdouble h;
  gdouble k;
  gdouble c_az;
  gdouble c_el;
  gdouble c_elmax;
  gdouble s_az;
  gdouble s_el;
  gdouble s_elmax;

  gdouble x;
  gdouble y;
};

typedef struct _FCDBpara FCDBpara;
struct _FCDBpara{
  gchar *name;
  gdouble ra;
  gdouble dec;
  gdouble d_ra;
  gdouble d_dec;
  gdouble pmra;
  gdouble pmdec;
  gboolean pm;
  gdouble equinox;
  gchar *otype;
  gchar *sp;
  gdouble sep;
  gchar *nedmag;
  gdouble nedvel;
  gdouble nedz;
  gdouble plx;
  gdouble u;
  gdouble b;
  gdouble v;
  gdouble r;
  gdouble i;
  gdouble j;
  gdouble h;
  gdouble k;
  gchar *fid;
  gchar *date;
  gchar *mode;
  gchar *type;
  gchar *fil;
  gchar *wv;
  gchar *obs;
  gdouble x;
  gdouble y;
  gint ref;
};

typedef struct _HMSpara my_hms;
struct _HMSpara{
  gint hours;
  gint minutes;
  gdouble seconds;
};

typedef struct _Moonpara typMoon;
struct _Moonpara{
  struct ln_hms c_ra;
  struct ln_dms c_dec;
  gdouble c_az;
  gdouble c_el;
  gdouble c_ha;
  gdouble c_disk;
  gdouble c_phase;
  gdouble c_limb;
  my_hms c_rise;
  my_hms c_set;
  gboolean c_circum;

#ifdef USE_SKYMON
  struct ln_hms s_ra;
  struct ln_dms s_dec;
  gdouble s_az;
  gdouble s_el;
  gdouble s_ha;
  gdouble s_disk;
  gdouble s_phase;
  gdouble s_limb;
  my_hms s_rise;
  my_hms s_set;
  gboolean s_circum;

  struct ln_hms p_ra[MAX_PP];
  struct ln_dms p_dec[MAX_PP];
  gdouble p_az[MAX_PP];
  gdouble p_el[MAX_PP];
#endif
};

typedef struct _Sunpara typSun;
struct _Sunpara{
  my_hms c_rise;
  my_hms c_set;
  gboolean c_circum;

#ifdef USE_SKYMON
  my_hms s_rise;
  my_hms s_set;
  gboolean s_circum;
#endif
};


typedef struct _typHOE typHOE;
struct _typHOE{
  gchar *temp_dir;
  gchar *home_dir;

  gint sz_skymon;
  gint sz_plot;
  gint sz_fc;

  GtkWidget *w_top;
  GtkWidget *w_box;
  GtkWidget *all_note;
  GtkWidget *scrwin;

  GtkWidget *plan_note;
  GtkWidget *query_note;

  GtkWidget *pbar;
  GtkWidget *plabel;
  GtkWidget *pbar2;

  GtkPrintContext *context;

  gchar *fontname;  
  gchar *fontfamily;  
  gchar *fontname_all;  
  gchar *fontfamily_all;  
  gint  skymon_allsz;

#ifdef USE_SKYMON
  guint skymon_timer;
#endif
  gchar *filename_read;
  gchar *filename_write;
  gchar *filename_pdf;
  gchar *filename_txt;
  gchar *filename_hoe;
  gchar *filename_log;
  gchar *filename_fcdb;
  gchar *filename_trdb;
  gchar *filename_nst;
  gchar *filename_jpl;
  gchar *filename_tscconv;
  gchar *filehead;

  gint i_max;
  
  OBJpara obj[MAX_OBJECT];
  NSTpara nst[MAX_ROPE];
  PLANpara plan[MAX_PLAN];
  STDpara std[MAX_STD];
  FCDBpara fcdb[MAX_FCDB];

  PPpara pp[MAX_PP];

  gint fr_year,fr_month,fr_day;
  gint nights;
  gchar *prop_id;
  gchar *prop_pass;
  gchar *observer;
  gint timezone;
  gint ocs;
  gint obs_timezone;
  guint wave1;
  guint wave0;
  guint pres;
  gint  temp;

  Setuppara setup[MAX_USESETUP];

  Binpara binning[MAX_BINNING];

  gint camz_b;
  gint camz_r;
  gint d_cross;

  guint exptime_factor;
  guint brightness;
  guint sv_area;
  guint sv_integrate;
  guint sv_acq;
  guint sv_region;
  guint sv_calc;
  guint exptime_sv;
  gdouble sv_slitx;
  gdouble sv_slity;
  gdouble sv_isx;
  gdouble sv_isy;
  gdouble sv_is3x;
  gdouble sv_is3y;

  gint def_guide;
  gdouble def_pa;
  guint def_exp;
  
  Nonstdpara nonstd[MAX_NONSTD];

  Linepara line[MAX_LINE];

  guint efs_setup;
  gchar* efs_ps;

  guint e_list;
  guint e_exp;
  guint e_times;
  GtkWidget *e_entry;
  GtkAdjustment *e_adj;
  GtkWidget *e_button[MAX_USESETUP];
  //GtkWidget *c_label;

  gint lst_hour;
  gint lst_min;
  gint lst_sec;

#ifdef USE_SKYMON
  gint skymon_lst_hour;
  gint skymon_lst_min;
  gint skymon_lst_sec;
#endif

  gchar *add_obj;
  GtkWidget *e_add_obj;
  gint add_num;

  GtkWidget *e_exp8mag;
  gint exp8mag;

  gint azel_mode;

  gint wwwdb_mode;
  gint stddb_mode;

  gboolean flag_bunnei;
  gboolean flag_secz;
  gdouble secz_factor;


#ifdef USE_SKYMON
  GtkWidget *skymon_main;
  GtkWidget *skymon_dw;
  GtkWidget *skymon_frame_mode;
  GtkWidget *skymon_frame_date;
  GtkWidget *skymon_frame_time;
  GtkWidget *skymon_frame_sz;
  GtkWidget *skymon_button_set;
  GtkWidget *skymon_button_fwd;
  GtkWidget *skymon_button_rev;
  GtkWidget *skymon_button_morn;
  GtkWidget *skymon_button_even;
  GtkAdjustment *skymon_adj_year;
  GtkAdjustment *skymon_adj_month;
  GtkAdjustment *skymon_adj_day;
  GtkAdjustment *skymon_adj_hour;
  GtkAdjustment *skymon_adj_min;
  gint skymon_mode;
  gint skymon_year,skymon_month,skymon_day,skymon_min,skymon_hour;
  gint skymon_objsz;
#endif

  typMoon moon;
  typSun sun;
  typSun atw;

  gint plot_mode;
  gint plot_target;
  gboolean plot_moon;
  GtkWidget *plot_main;
  GtkWidget *plot_dw;
  gint plot_i;
  gint plot_i_plan;
  gint plot_output;
  gint skymon_output;

  gint efs_mode;
  GtkWidget *efs_main;
  GtkWidget *efs_dw;
  gint efs_output;


  GtkWidget *objtree;
  GtkWidget *sw_objtree;

  GtkWidget *linetree;
  GtkWidget *sw_linetree;

  GtkWidget *tree_search_label;
  gchar *tree_search_text;
  guint tree_search_i;
  guint tree_search_iobj[MAX_OBJECT];
  guint tree_search_imax;

  gchar *www_com;

  gint fc_mode;
  gint fc_mode_get;
  gint fc_mode_def;
  gint fc_inst;
  gint fc_output;
  GtkWidget *fc_frame_col;
  GtkWidget *fc_frame_col_pdf;
  GtkWidget *fc_button_flip;
  gint dss_arcmin;
  gint dss_arcmin_ip;
  gint dss_pix;
  gint dss_scale;
  gboolean dss_invert;

  gint dss_i;
  gchar *dss_host;
  gchar *dss_path;
  gchar *dss_src;
  gchar *dss_tmp;
  gchar *dss_file;
  gint dss_pa;
  GtkAdjustment *fc_adj_dss_pa;
  GtkAdjustment *fc_adj_dss_arcmin;
  gboolean dss_flip;
  gboolean dss_draw_slit;
  gboolean sdss_photo;
  gboolean sdss_spec;
  GtkWidget *fc_main;
  GtkWidget *fc_dw;
  gint fc_mag;
  gint fc_magx;
  gint fc_magy;
  gint fc_magmode;
  gint fc_ptn;
  gint fc_ptx1;
  gint fc_pty1;
  gint fc_ptx2;
  gint fc_pty2;

  gint hsc_dithi;
  gint hsc_dithp;
  gint hsc_dra;
  gint hsc_ddec;
  gint hsc_ndith;
  gint hsc_tdith;
  gint hsc_rdith;
  gint hsc_offra;
  gint hsc_offdec;
  GtkWidget *hsc_label_dith;

  guint alldss_check_timer;

  GtkWidget *plan_tree;
  guint i_plan_max;

  guint i_pp_max;
  guint i_pp_moon_max;

  GtkWidget *f_objtree_arud;

  gboolean  plan_tmp_or;
  guint  plan_tmp_sw;
  guint  plan_tmp_sl;
  guint  plan_tmp_setup;

  GtkAdjustment *plan_obj_adj;
  GtkWidget *plan_obj_guide_combo;
  gint  plan_obj_i;
  guint  plan_obj_exp;
  guint  plan_obj_repeat;

  guint  plan_obj_omode;
  guint  plan_obj_guide;

  gboolean plan_setazel_daytime;
  gdouble plan_setaz;
  gdouble plan_setel;

  guint  plan_bias_repeat;
  gboolean plan_bias_daytime;

  gboolean plan_comp_daytime;

  guint  plan_flat_repeat;
  gboolean plan_flat_daytime;

  guint  plan_focus_mode;

  guint  plan_setup_cmode;
  gboolean plan_setup_daytime;

  guint  plan_i2_pos;
  gboolean plan_i2_daytime;

  gchar  *plan_comment;
  guint  plan_comment_time;
  guint  plan_comment_type;

  guint  plan_start;
  guint  plan_start_hour;
  guint  plan_start_min;

  guint  plan_delay;

  gint plot_all;

  gchar *std_file;
  gchar *std_host;
  gchar *std_path;
  gint  std_i;
  gint  std_i_max;
  GtkWidget *stddb_tree;
  GtkWidget *std_tgt;
  gint stddb_tree_focus;
  GtkWidget *stddb_label;
  GtkWidget *stddb_button;
  gchar *stddb_label_text;
  gboolean stddb_flag;

  gint std_dra;
  gint std_ddec;
  gint std_vsini;
  gint std_vmag;
  gchar *std_sptype;
  gint std_iras12;
  gint std_iras25;
  gchar *std_cat;
  gint std_mag1;
  gint std_mag2;
  gchar *std_band;
  gchar *std_sptype2;

  gint fcdb_type;
  gint fcdb_type_tmp;
  gboolean fcdb_post;
  gchar *fcdb_file;
  gint fcdb_simbad;
  gchar *fcdb_host;
  gchar *fcdb_path;
  gint fcdb_i;
  gint fcdb_tree_focus;
  gdouble fcdb_d_ra0;
  gdouble fcdb_d_dec0;
  gint  fcdb_i_max;
  gint  fcdb_i_all;
  GtkWidget *fcdb_tree;
  GtkWidget *fcdb_sw;
  GtkWidget *fcdb_label;
  GtkWidget *fcdb_frame;
  GtkWidget *fcdb_button;
  GtkWidget *fcdb_tgt;
  gchar *fcdb_label_text;
  gboolean fcdb_flag;
  gint fcdb_band;
  gint fcdb_mag;
  gint fcdb_otype;
  gint fcdb_ned_diam;
  gint fcdb_ned_otype;
  gboolean fcdb_auto;
  gboolean fcdb_ned_ref;
  gboolean fcdb_gsc_fil;
  gint fcdb_gsc_mag;
  gint fcdb_gsc_diam;
  gboolean fcdb_ps1_fil;
  gint fcdb_ps1_mag;
  gint fcdb_ps1_diam;
  gint fcdb_ps1_mindet;
  gint fcdb_sdss_search;
  gint fcdb_sdss_magmin[NUM_SDSS_BAND];
  gint fcdb_sdss_magmax[NUM_SDSS_BAND];
  gboolean fcdb_sdss_fil[NUM_SDSS_BAND];
  gint fcdb_sdss_diam;
  gint fcdb_usno_mag;
  gint fcdb_usno_diam;
  gboolean fcdb_usno_fil;
  gint fcdb_gaia_mag;
  gint fcdb_gaia_diam;
  gboolean fcdb_gaia_fil;
  gint fcdb_2mass_mag;
  gint fcdb_2mass_diam;
  gboolean fcdb_2mass_fil;
  gint fcdb_wise_mag;
  gint fcdb_wise_diam;
  gboolean fcdb_wise_fil;
  gboolean fcdb_smoka_shot;
  gboolean fcdb_smoka_subaru[NUM_SMOKA_SUBARU];
  gboolean fcdb_smoka_kiso[NUM_SMOKA_KISO];
  gboolean fcdb_smoka_oao[NUM_SMOKA_OAO];
  gboolean fcdb_smoka_mtm[NUM_SMOKA_MTM];
  gboolean fcdb_smoka_kanata[NUM_SMOKA_KANATA];
  gboolean fcdb_hst_image[NUM_HST_IMAGE];
  gboolean fcdb_hst_spec[NUM_HST_SPEC];
  gboolean fcdb_hst_other[NUM_HST_OTHER];
  gboolean fcdb_eso_image[NUM_ESO_IMAGE];
  gboolean fcdb_eso_spec[NUM_ESO_SPEC];
  gboolean fcdb_eso_vlti[NUM_ESO_VLTI];
  gboolean fcdb_eso_pola[NUM_ESO_POLA];
  gboolean fcdb_eso_coro[NUM_ESO_CORO];
  gboolean fcdb_eso_other[NUM_ESO_OTHER];
  gboolean fcdb_eso_sam[NUM_ESO_SAM];
  gint fcdb_gemini_inst;

  GtkWidget *trdb_tree;
  GtkWidget *trdb_sw;
  GtkWidget *trdb_label;
  gint trdb_i_max;
  gint trdb_tree_focus;
  gboolean trdb_disp_flag;
  gchar *trdb_label_text;
  GtkWidget *trdb_search_label;
  gchar *trdb_search_text;
  guint trdb_search_i;
  guint trdb_search_iobj[MAX_OBJECT];
  guint trdb_search_imax;
  gint trdb_used;
  gint trdb_arcmin;
  gint trdb_arcmin_used;

  gint trdb_smoka_inst;
  gint trdb_smoka_inst_used;
  gchar *trdb_smoka_date;
  gchar *trdb_smoka_date_used;
  gboolean trdb_smoka_shot;
  gboolean trdb_smoka_shot_used;
  gboolean trdb_smoka_imag;
  gboolean trdb_smoka_imag_used;
  gboolean trdb_smoka_spec;
  gboolean trdb_smoka_spec_used;
  gboolean trdb_smoka_ipol;
  gboolean trdb_smoka_ipol_used;

  gint trdb_hst_mode;
  gint trdb_hst_mode_used;
  gchar *trdb_hst_date;
  gchar *trdb_hst_date_used;
  gint trdb_hst_image;
  gint trdb_hst_image_used;
  gint trdb_hst_spec;
  gint trdb_hst_spec_used;
  gint trdb_hst_other;
  gint trdb_hst_other_used;

  gint trdb_eso_mode;
  gint trdb_eso_mode_used;
  gchar *trdb_eso_stdate;
  gchar *trdb_eso_stdate_used;
  gchar *trdb_eso_eddate;
  gchar *trdb_eso_eddate_used;
  gint trdb_eso_image;
  gint trdb_eso_image_used;
  gint trdb_eso_spec;
  gint trdb_eso_spec_used;
  gint trdb_eso_vlti;
  gint trdb_eso_vlti_used;
  gint trdb_eso_pola;
  gint trdb_eso_pola_used;
  gint trdb_eso_coro;
  gint trdb_eso_coro_used;
  gint trdb_eso_other;
  gint trdb_eso_other_used;
  gint trdb_eso_sam;
  gint trdb_eso_sam_used;

  gint trdb_gemini_inst;
  gint trdb_gemini_inst_used;
  gint trdb_gemini_mode;
  gint trdb_gemini_mode_used;
  gchar *trdb_gemini_date;
  gchar *trdb_gemini_date_used;

  gint addobj_type;
  gchar *addobj_name;
  gchar *addobj_voname;
  gchar *addobj_votype;
  gdouble addobj_ra;
  gdouble addobj_dec;
  gchar *addobj_magsp;
  GtkWidget *addobj_label;
  GtkWidget *addobj_entry_ra;
  GtkWidget *addobj_entry_dec;

  GdkPixmap *pixmap_fc;

  gboolean orbit_flag;
  gint nst_max;
};


// Struct for Callback
typedef struct{
  typHOE *hg;
  gint i_obj;
}confPA;



// Struct for Callback
typedef struct{
  typHOE *hg;
  gint i_use;
  GtkWidget *length_entry;
  GtkWidget *fil1_combo;
  GtkWidget *fil2_combo;
}confSetup;

// Struct for Callback
typedef struct{
  typHOE *hg;
  gint i_use;
}confEdit;

// Struct for Callback
typedef struct{
  GtkWidget *dialog;
  gint mode;
}confProp;

// Struct for Callback
typedef struct{
  GtkWidget *dialog;
  gint mode;
  GSList *fcdb_group;
  gint fcdb_type;
}confPropFCDB;


#define is_num_char(c) ((c>='0' && c<='9')||(c==' ')||(c=='\t')||(c=='.')||(c=='+')||(c=='-')||(c=='\n'))



static const char* binname[]={"1x1 [86s]",
			      "2x1 [60s]",
			      "2x2 [44s]",
			      "2x4 [36s]",
			      "4x1 [44s]",
			      "4x4 [33s]"};

static const char* filtername1[]={"Free",
				  "OG530",
				  "SQ",
				  "U340",
				  "ND1",
				  "Halpha",
				  "O5007"};
#define MAX_FILTER1 7

static const char* filtername2[]={"Free",
				  "KV370",
				  "KV389",
				  "SC42",
				  "SC46",
				  "GG495"};
#define MAX_FILTER2 6

// Ya is temporary (using Yb setting)
static const SetupEntry setups[] = {
  {"Ub",  "Blue","Blue",  "Free",  "Free",  "4.0",2,1,3,2,24, "4.0",1,1,1,2,16, 4.0,17100}, 
  {"Ua",  "Blue","Blue",  "Free",  "Free",  "4.0",2,1,3,2,24, "4.0",1,1,1,2,16, 4.0,17820}, 
  {"Ba",  "Blue","Blue",  "Free",  "Free",  "4.0",2,1,1,2,12, "4.0",1,1,2,2, 4, 5.0,19260}, 
  {"Bc",  "Blue","Blue",  "Free",  "Free",  "4.0",2,1,1,2,12, "4.0",1,1,2,2, 4, 6.0,19890}, 
  {"Ya",  "Blue","Blue",  "Free",  "Free",  "4.0",2,1,1,2,12, "4.0",2,1,1,2, 24, 8.0,21960}, 
  {"I2b", "Red", "Red",   "Free",  "Free",  "3.0",2,1,1,2,16, "4.0",2,1,1,2, 16, 3.6,14040}, 
  {"Yd",  "Red", "Red",   "Free",  "Free",  "3.0",2,1,1,2,12, "4.0",2,1,1,2, 8, 4.0,15480}, 
  {"Yb",  "Red", "Red",   "Free",  "KV370", "3.0",2,1,1,2,12, "4.0",2,1,1,2, 8, 4.0,15730}, 
  {"Yc",  "Red", "Red",   "Free",  "KV389",  "3.0",2,1,1,2,12, "4.0",2,1,1,2, 5, 5.0,16500}, 
  {"I2a", "Red", "Red",   "Free",  "SC46",  "3.0",2,1,1,2,12, "3.0",2,1,1,2, 12, 7.0,18000}, 
  {"Ra",  "Red", "Red",   "Free",  "SC46",  "3.0",2,1,1,2,12, "3.0",2,1,1,2, 12, 7.0,18455}, 
  {"Rb",  "Red", "Red",   "Free",  "SC46",  "3.0",2,1,1,2,12, "3.0",2,1,1,2, 12, 8.0,19080}, 
  {"NIRc","Red", "Red",   "OG530", "Free",  "3.0",2,1,1,2,10, "3.0",2,1,1,2,10, 10.0,21360}, 
  {"NIRb","Red", "Red",   "OG530", "Free",  "3.0",2,1,1,2,10, "3.0",2,1,1,2,10, 10.0,22860}, 
  {"NIRa","Red", "Red",   "OG530", "Free",  "3.0",2,1,1,2,10, "3.0",2,1,1,2,10, 15.0,25200}, 
  {"Ha",  "Red", "Mirror","Halpha","Free",  "4.0",2,1,1,2,15, "4.0",2,1,1,2,15, 60.0,0}
};


static GdkColor color_comment = {0, 0xDDDD, 0x0000, 0x0000};
static GdkColor color_focus = {0, 0x8888, 0x4444, 0x0000};
static GdkColor color_calib = {0, 0x0000, 0x8888, 0x0000};
static GdkColor color_black = {0, 0, 0, 0};
static GdkColor color_gray1 = {0, 0x6666, 0x6666, 0x6666};
static GdkColor color_gray2 = {0, 0xFFFF, 0x6666, 0x6666};
static GdkColor color_pink = {0, 0xFFFF, 0x6666, 0x6666};
static GdkColor color_com1 = {0, 0x0000, 0x8888, 0x0000};
static GdkColor color_com2 = {0, 0xBBBB, 0x8888, 0x0000};
static GdkColor color_com3 = {0, 0xDDDD, 0x0000, 0x0000};

////////////////////// Global Args //////////////////////
gboolean flagChildDialog;
gboolean flagSkymon;
gboolean flagPlot;
gboolean flagFC;
gboolean flag_getFCDB;
gboolean flag_make_obj_tree;
gboolean flag_make_line_tree;
int debug_flg;

#ifndef USE_WIN32
pid_t fc_pid;
#endif
pid_t fcdb_pid;
pid_t stddb_pid;


////////////////////// Proto types () //////////////////////
// main.c
gchar * fgets_new();
GtkWidget * gtkut_button_new_from_stock();
GtkWidget * gtkut_toggle_button_new_from_stock();
GtkWidget * gtkut_toggle_button_new_from_pixbuf();
GtkWidget * gtkut_button_new_from_pixbuf();
gboolean is_separator();
void my_signal_connect();
void do_save_pdf();
void do_save_efs_pdf();
void do_save_plan();
void do_save_plan_txt();
void do_save_plan_yaml();
void do_save_skymon_pdf();
void do_save_fc_pdf();
void do_save_FCDB_List();
void my_entry_set_width_chars();
gchar *make_tgt();
void ext_play();
void cc_get_combo_box ();
void cc_get_toggle ();
void cc_get_adj();
void cc_get_adj_double();
void cc_get_entry();
void cc_get_entry_double();
void cc_get_entry_int();
void recalc_rst();
void popup_message(gchar*, gint , ...);
void default_disp_para();
void change_disp_para();
void close_disp_para();
gchar *strip_spc();

// calcpa.c
void calcpa2_main();
void calcpa2_skymon();
void calcpa2_plan();
void calc_moon();
void calc_moon_skymon();
void calc_sun_plan();
void pdf_plot();
gdouble get_julian_day_of_epoch();
void create_plot_dialog();
gdouble ra_to_deg();
gdouble dec_to_deg();
gdouble deg_to_ra();
gdouble deg_to_dec();
gdouble deg_sep();

// edit.c
void create_opedit_dialog();

// efs.c
void go_efs();
void pdf_efs();


// fc.c
void fcdb_para_item();
gdouble current_yrs();
void create_fc_all_dialog();
void pdf_fc();
void fcdb_dl();
gboolean progress_timeout();
void fc_item ();

//fc_output.c
void Export_FCDB_List();
void Export_TRDB_CSV();

// http_client.c
int ftp_c();
int scp_c();
int get_dss();
int get_stdb();
int get_fcdb();

// line_tree.c
void make_line_tree();
void linetree_init();
void linetree_nebula();
void linetree_star();

// objtree.c
void make_obj_tree();
void add_item_objtree();
void up_item_objtree();
void down_item_objtree();
void remove_item_objtree();
void wwwdb_item();
void do_update_exp();
void export_def ();
void do_plot();
void plot2_objtree_item();
void addobj_dialog();
void str_replace();
gchar *make_simbad_id();
void cc_search_text();
void search_item();

// plan.c 
void create_plan_dialog();
gchar * get_txt_tod();
gchar * make_plan_txt();
void remake_sod();


// skymon.c
void create_skymon_dialog();
gboolean draw_skymon_cairo();
void pdf_skymon();

// stdtree.c
void stddb_item();
void stddb_toggle();
void std_make_tree();
void std_add_columns();
GtkTreeModel *std_create_items_model ();
void std_focus_item ();
void std_simbad ();
void add_item_std();
void create_std_para_dialog();
void make_std_tgt();

// fcdbtree.c
void fcdb_make_tree();
void cancel_fcdb();
#ifndef USE_WIN32
void fcdb_signal();
#endif
void fcdb_item();
GtkTreeModel * fcdb_create_items_model();
void fcdb_add_columns();
void fcdb_clear_tree();
void rebuild_fcdb_tree();
void fcdb_append_tree();
void fcdb_simbad();
void add_item_fcdb();
void make_fcdb_tgt();

// trdbtree.c
void trdb_smoka();
void trdb_hst();
void trdb_eso();
void trdb_gemini();
void trdb_simbad();
void trdb_append_tree();
void trdb_dbtab();
void trdb_cc_search_text();
void trdb_search_item();

// treeview.c
gchar *make_simbad_id();


// votable.c
void make_band_str();
