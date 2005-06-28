/**
 * @file   catalog.cxx
 * @brief  Basic routines for Catalog class.
 * The 3 static members should be read in configuration files in the future.
 * The methods give information on: these static members,
 * the few members that describe the catalog itself, 
 * the quantity data (with or without selection).
 *
 * @author A. Sauvageon
 *
 * $Header $
 */

#include "catalog.h"

namespace catalogAccess {
  using namespace tip;
/**********************************************************************/
/*  DEFINING CLASS CONSTANTS                                          */
/**********************************************************************/

// available mirror sites for VizieR (from CDS)
const char *Catalog::s_CatalogURL[MAX_URL]={
      "cds   fr vizier.u-strasbg.fr/",    //CDS - Strasbourg, France
      "cfa   us vizier.cfa.harvard.edu/", //CFA - Harvard, USA
      "cadc  ca vizier.hia.nrc.ca/",      //CADC - Canada
      "adac  jp vizier.nao.ac.jp/",       //ADAC - Tokyo, Japan
      "ukirt hawaii www.ukirt.jach.hawaii.edu/", //JAC - Hawaii, USA
      "cambridge uk archive.ast.cam.ac.uk/",
      "iucaa in urania.iucaa.ernet.in/",  //IUCAA - Pune, India
      "moscow ru www.inasan.rssi.ru/",    //INASAN - Russia (DOES IT WORK ?)
      "bejing cn data.bao.ac.cn/"         //Bejing Obs. - China
      };

// information on catalog size laste updated on: JAN 2005
const char *Catalog::s_CatalogList[2*MAX_CAT]={
      "EGRET3 sources", "J/ApJS/123/79/3eg",    //   271 rows (20 columns)
      "EGRET3 fluxes",  "J/ApJS/123/79/fluxes", //  5245 rows (10 columns)
      "EGRET3 periods", "J/ApJS/123/79/table1", //   169 rows ( 6 columns)
      "ROSAT 1RXS",     "IX/10A/1rxs",          // 18806 rows (28 columns)
      "Veron (11th) quasar", "VII/235/table1",  // 48921 rows (21 columns)
      "Veron (11th) BL Lac", "VII/235/table2",  //   876 rows (21 columns)
      "Veron (11th) AGN",    "VII/235/table3",  // 15069 rows (21 columns)
      "WGACAT of ROSAT",     "IX/31/wgacat",    // 88621 rows (101 columns)
      "ROSAT faint sources", "IX/29/rass_fsc",  //105924 rows (30 columns)
      "Green galactic SNR",  "VII/227/snrs"     //   231 rows (14 columns)
      };


const char *Catalog::s_CatalogGeneric[MAX_CAT][MAX_GEN]={
     {"3EG", "RAJ2000", "DEJ2000", "theta95", "GLON", "GLAT"},
     {"3EG", "", "", "", "", ""},
     {"",    "+", "+", "", "GLON", "GLAT"},
     {"1RXS", "RAJ2000", "DEJ2000", "PosErr", "+", "+"},
     {"Name", "+", "+", "", "+", "+"},   // original format of RA/DEC
     {"Name", "+", "+", "", "+", "+"},   // is in sexagesimal
     {"Name", "+", "+", "", "+", "+"},   // ==> need to be added in decimal
     {"1WGA", "RAJ2000", "DEJ2000", "ErrorRad", "GLON", "GLAT"},
     {"1RXS", "RAJ2000", "DEJ2000", "PosErr", "+", "+"},
     {"SNR",  "+", "+", "", "+", "+"}   // RA/DEC is in sexagesimal
};


/**********************************************************************/
// erase m_strings, m_numericals but keep catalog definition
void Catalog::deleteContent() {

  int vecSize, i;
  m_numRows=0;
  m_numSelRows=0;
  vecSize=m_rowIsSelected.size();
  if (vecSize > 0 ) {
    for (i=0; i<vecSize; i++) m_rowIsSelected[i].clear();
    m_rowIsSelected.clear();
  }
  vecSize=m_numericals.size();
  if (vecSize > 0 ) {
    for (i=0; i<vecSize; i++) m_numericals[i].clear();
    m_numericals.clear();
  }
  vecSize=m_strings.size();
  if (vecSize > 0 ) {
    for (i=0; i<vecSize; i++) m_strings[i].clear();
    m_strings.clear();
  }
}
/**********************************************************************/
// erase catalog definition (private method)
void Catalog::deleteDescription() {

  m_code="";
  m_URL ="";
  m_catName   ="";
  m_catRef    ="";
  m_tableName ="";
  m_tableRef  ="";

  m_filename  ="";
  m_filePos   =0;
  m_posErrSys = -1.0;
  m_posErrFactor=1.0;  // "deg" by default
  m_quantities.clear();
  m_loadQuantity.clear();

  m_numRows   =IMPORT_NEED;
  m_numOriRows=0;
  m_selection="";
  m_criteriaORed=false;
  m_numSelRows=0;
  m_indexErr= -1;
  m_indexRA = -1;
  m_indexDEC= -1;
  m_selRegion=false;
  m_selEllipse.clear();
}
/**********************************************************************/
// erase elements in m_quantities according to m_loadQuantity,
// deleteContent() MUST be done before
void Catalog::deleteQuantities() {

  unsigned int maxSize=m_loadQuantity.size();
  int  nbA=0, nbD=0;

  if (maxSize != m_quantities.size()) {
    printErr("deleteQuantities",
             "problem in code, this error should not occur");
    return;
  }
  /* these vectors can be set if import was succesful with 0 row */
  m_rowIsSelected.clear();
  m_numericals.clear();
  m_strings.clear();
  std::vector<Quantity>::iterator quantIter;
  quantIter=m_quantities.begin();
  for (unsigned int i=0; i<maxSize; i++) {
    if (m_loadQuantity[i]) {/* change the index in 2D tables */
      if (quantIter->m_format[0] == 'A') quantIter->m_index=nbA++;
      else quantIter->m_index=nbD++;
      quantIter++;
    }
    else m_quantities.erase(quantIter);
  }/* loop on quantities */
  /* now that index have changed, update the index shortcuts */
  for (nbA=0; nbA<MAX_CAT; nbA++) {
    if (Catalog::s_CatalogList[2*nbA+1] == m_tableName) break;
  }
  setGeneric(nbA);
}
/**********************************************************************/
// Copy constructor needed to allocate arrays in copy
Catalog::Catalog(const Catalog & myCat) {

  #ifdef DEBUG_CAT
  std::cout << "!! DEBUG Catalog COPY constructor on: "
            << myCat.m_tableName << std::endl;
  #endif  
  std::string errText;
  // copying values
  m_code=myCat.m_code;
  m_URL =myCat.m_URL;
  m_catName   =myCat.m_catName;
  m_catRef    =myCat.m_catRef;
  m_tableName =myCat.m_tableName;
  m_tableRef  =myCat.m_tableRef;

  m_filename   =myCat.m_filename;
  m_filePos    =myCat.m_filePos;
  m_posErrSys=myCat.m_posErrSys;
  m_posErrFactor=myCat.m_posErrFactor;

  m_numRows   =myCat.m_numRows;
  m_numOriRows=myCat.m_numOriRows;
  m_selection =myCat.m_selection;
  m_criteriaORed=myCat.m_criteriaORed;
  // following three data members needed for efficient selection
  m_numSelRows=myCat.m_numSelRows;
  m_indexErr  =myCat.m_indexErr;
  m_indexRA   =myCat.m_indexRA;
  m_indexDEC  =myCat.m_indexDEC;

  // copying elliptical region definition
  m_selRegion=myCat.m_selRegion;
  m_selEllipseCentRA_deg =myCat.m_selEllipseCentRA_deg;
  m_selEllipseCentDEC_deg=myCat.m_selEllipseCentDEC_deg;
  m_selEllipseMinAxis_deg=myCat.m_selEllipseMinAxis_deg;
  m_selEllipseMajAxis_deg=myCat.m_selEllipseMajAxis_deg;
  m_selEllipseRot_deg=myCat.m_selEllipseRot_deg;

  // copying vectors
//try {
  long i;
  int vecSize, j;
  // following data member m_selEllipse[] needed for efficient selection
  try {
    vecSize=myCat.m_selEllipse.size(); // size is only 7
    for (j=0; j<vecSize; j++) m_selEllipse.push_back(myCat.m_selEllipse.at(j));
  }
  catch (const std::exception &err) {
    errText=std::string("EXCEPTION on m_selEllipse[]: ")+err.what();
    printErr("Catalog copy constructor", errText);
    throw;
  }
  // following data member m_loadQuantity[] needed for importSelected()
  try {
    vecSize=myCat.m_loadQuantity.size();
    m_loadQuantity.reserve(vecSize);  // pre-allocate the memory 
    for (j=0; j<vecSize; j++)
      m_loadQuantity.push_back(myCat.m_loadQuantity.at(j));
  }
  catch (const std::exception &err) {
    errText=std::string("EXCEPTION on m_loadQuantity[]: ")+err.what();
    printErr("Catalog copy constructor", errText);
    throw;
  }
  try {
    std::vector<Quantity>::const_iterator itQ;
    for (itQ=myCat.m_quantities.begin(); itQ!=myCat.m_quantities.end(); itQ++)
      m_quantities.push_back(*itQ);
  }
  catch (const std::exception &err) {
    errText=std::string("EXCEPTION on m_quantities[]: ")+err.what();
    printErr("Catalog copy constructor", errText);
    throw;
  }
  // suppose that each column has same number of rows
  vecSize=myCat.m_rowIsSelected.size();
  if (vecSize > 0 ) {
    try {
      m_rowIsSelected.resize(vecSize);
      for (j=0; j<vecSize; j++) {
        // pre-allocate the memory for each vector
        m_rowIsSelected[j].reserve(m_numRows);
        for (i=0; i<m_numRows; i++)
          m_rowIsSelected[j].push_back(myCat.m_rowIsSelected[j].at(i));
      }
    }
    catch (const std::exception &err) {
      errText=std::string("EXCEPTION on m_rowIsSelected[][]: ")+err.what();
      printErr("Catalog copy constructor", errText);
      throw;
    }
  }
  vecSize=myCat.m_strings.size();
  if (vecSize > 0 ) {
    try {
      m_strings.resize(vecSize);
      for (j=0; j<vecSize; j++) {
        // pre-allocate the memory for each vector
        m_strings[j].reserve(m_numRows);
        for (i=0; i<m_numRows; i++)
          m_strings[j].push_back(myCat.m_strings[j].at(i));
      }
    }
    catch (const std::exception &err) {
      errText=std::string("EXCEPTION on m_strings[][]: ")+err.what();
      printErr("Catalog copy constructor", errText);
      throw;
    }
  }
  vecSize=myCat.m_numericals.size();
  if (vecSize > 0 ) {
    try {
      m_numericals.resize(vecSize);
      for (j=0; j<vecSize; j++) {
        // pre-allocate the memory for each vector
        m_numericals[j].reserve(m_numRows);
        for (i=0; i<m_numRows; i++)
          m_numericals[j].push_back(myCat.m_numericals[j].at(i));
      }
    }
    catch (const std::exception &err) {
      errText=std::string("EXCEPTION on m_numericals[][]: ")+err.what();
      printErr("Catalog copy constructor", errText);
      throw;
    }
  }
/* line is commented on purpose to TEMINATE the program on EXCEPTION */
//} catch (...) { printErr("Catalog copy constructor", ""); }
}


/**********************************************************************/
/*  METHODS giving GENERAL INFORMATION                                */
/**********************************************************************/
// return a list of all supported catalog names
void Catalog::getCatList(std::vector<std::string> *names, const bool isCode) {

  names->clear();
  #ifdef DEBUG_CAT
  std::cout << "!! DEBUG number of known catalogs: " << MAX_CAT
            << std::endl;
  #endif
  std::string text;
  int j=1;
  if (isCode) j=0;
  try {
    for (int i=0; i<MAX_CAT; i++) {
      text=Catalog::s_CatalogList[2*i+j]; /* convert C string to C++ string */
      names->push_back(text);
    }
  }
  catch (const std::exception &prob) {
    text=std::string("EXCEPTION filling names: ")+prob.what();
    printErr("getCatList", text);
    throw;
  }
}
/**********************************************************************/
// return a list of all supported web site names
void Catalog::getWebList(std::vector<std::string> *names, const bool isCode) {

  names->clear();
  #ifdef DEBUG_CAT
  std::cout << "!! DEBUG number of web sites: " << MAX_URL
            << std::endl;
  #endif
  std::string text;
  try {
    std::string s;
    unsigned int pos;
    for (int i=0; i<MAX_URL; i++) {
      s=Catalog::s_CatalogURL[i]; /* convert C string to C++ string */
      if (isCode) {
        pos=s.find(' ');
        if (pos == std::string::npos) text="";
        else text=s.substr(0, pos);
      }
      else {
        pos=s.rfind(' ');
        if (pos == std::string::npos) text=s;
        else text=s.substr(pos+1, s.length()-pos);
      }
      names->push_back(text);
    }
  }
  catch (const std::exception &prob) {
    text=std::string("EXCEPTION filling names: ")+prob.what();
    printErr("getWebList", text);
    throw;
  }
}

/**********************************************************************/
// to know the maximal number of rows in the file
int Catalog::getMaxNumRows(long *nrows, const std::string &fileName,
                           const std::string ext) {

  int i, err;
  std::string text, origin="getMaxNumRows";
  *nrows=0;
  err=checkImport(origin, false);
  if (err < IS_VOID) return err;

  m_numRows=0;
  i=fileName.length();
  if (i == 0) {
    text=": FILENAME is EMPTY";
    printErr(origin, text);
    err=BAD_FILENAME;
    m_numRows=err;
    return err;
  }
  std::fstream myFile (fileName.c_str(), std::ios::in);
  // file can be opened ?
  if ( !myFile.is_open() ) {
    text=": FILENAME \""+fileName+"\" cannot be opened";
    printErr(origin, text);
    err=BAD_FILENAME;
    m_numRows=err;
    return err;
  }
/*  DIR *testDir=opendir(fileName.c_str());
  if (testDir != NULL) {
    closedir(testDir);
    text=": FILENAME \""+fileName+"\" is a directory";
    printErr(origin, text);
    err=BAD_FILETYPE;
    m_numRows=err;
    return err;
  }*/
  myFile.close();
  std::ostringstream sortie;
  bool  myTest;
  const Extension *myEXT = 0;
  // cannot use readTable because FITS IMAGE returns also error 1
  try {
    myEXT=IFileSvc::instance().readExtension(fileName, ext);
  }
  catch (const TipException &x) {
    err=x.code();
    if (err == 1) {
      // This non-cfitsio error number means the file does not exist
      // or is not a table, or is not in either Root nor Fits format. 
      err=BAD_FITS;
/*      printWarn(origin, "FILENAME is NOT fits");*/
    }
    else {
      // Other errors come from cfitsio, but apply to a file
      // which is in FITS format but has some sort of format error.
      sortie << ": FILENAME is FITS, but cfitsio returned error=" << err;
      printErr(origin, sortie.str());
      err=BAD_FITS;
      m_numRows=err;
      delete myEXT;
      return err;
    }
  }
  if (err == IS_OK) myTest=myEXT->isTable();
  delete myEXT;
  if (err == IS_OK) {
 
    if (!myTest) {
      text=": FILENAME is FITS, but NOT a TABLE";
      printErr(origin, text);
      err=BAD_FITS;
      m_numRows=err;
      return err;
    }
    const Table *myDOL = 0;
    try {
      myDOL=IFileSvc::instance().readTable(fileName, ext);
      *nrows=myDOL->getNumRecords();
    }
    catch (const TipException &x) {
      sortie << ": FITS is TABLE, but cfitsio returned error=" << err;
      printErr(origin, sortie.str());
      err=BAD_FITS;
      m_numRows=err;
      delete myDOL;
      return err;
    }
    delete myDOL;

  }
  else { // (err == BAD_FITS)

    err=IS_OK;
    unsigned long tot=0ul; // number of lines read
    bool testCR=false; // true if lines ends with CR (on windows)
    int what=0;        // 0 for unkwown, >0 for csv, <0 to tsv
                       // fabs()=1 for standard, =2 for meta QUERY

    myFile.open(fileName.c_str(), std::ios::in);
    err=analyze_head(&tot, &what, &testCR, &myFile);
    if (!tot) {
      text=": FILENAME \""+fileName+"\" is fits without extension[] specified";
      printErr(origin, text);
      if (err == IS_OK) err=BAD_FILENAME;
      // in case, for strange reason, file.good() fails at first time
    }
    else if (err < IS_OK) {
      sortie << ": FILENAME \"" << fileName << "\" is empty or "
             << "has unknown structure (stopped step " << -1*err << ")";
      printErr(origin, sortie.str());
      sortie.str(""); // Will empty the string.
      if (err == 0) err=BAD_FILENAME; /* nothing is read */
      else err=BAD_FILETYPE; /* can search if catalog name exist */
    }
    else if (what == 2) {
      sortie << "closing input META file (" << m_numOriRows;
      sortie << " rows in whole CDS catalog)";
      printLog(0, sortie.str());
    }
    else {
      unsigned long refRow=0ul;
      long maxRows=0l;
      // get columns description and units, data
      err=analyze_body(&tot, &what, testCR, true, &myFile, &maxRows);
      // only possible error: found < 4
      if (err == IS_OK) {
        // decription is read until separation line starting with ---
        char line[MAX_LINE];
        int  posC,
             maxLine=MAX_LINE-1; // to avoid computation each line
        refRow=tot+1;
        while ( myFile.good() ) {
          myFile.getline(line, MAX_LINE);
          tot++;
          i=strlen(line);
          if (i < 2) break; // in any case, stop on empty string contrary to
                            //  load() which continues, skipping lines
          // string max size is MAX_LINE-1;
          if (i >= maxLine) {
            sortie << "line #" << tot << " exceeds maximal size ("
                   << MAX_LINE << ")";
            printErr(origin, sortie.str());
            sortie.str(""); // Will empty the string.
            err=BAD_FILELINE;
            break;
          }
          //if (testCR) line[--i]='\0';
          posC=strncmp(line, "#Table", 6);
          if (posC == 0) {
            // should not happen, normally preceded by empty lines
            sortie << "line #" << tot << ": second table start (not read)";
            printWarn(origin, sortie.str());
            sortie.str(""); // Will empty the string.
            err=BAD_ROW;
            break;
          }
        }
        if (err == BAD_ROW) err=IS_OK;
      }// analyze_body() is OK
 
      if ((err < IS_OK) && (err >= -3)) {
        sortie << ": FILENAME \"" << fileName
            << "\" has unknown type (stopped step " << 5-1*err << ")";
        printErr(origin, sortie.str());
        sortie.str(""); // Will empty the string.
        err=BAD_FILETYPE;
      }
      else if (err == IS_OK) *nrows=tot-refRow;
      sortie << "closing input text file: " << tot << " lines read";
      printLog(0, sortie.str());
    }// analyze_head() is OK
    myFile.close();

  }// file is read

  deleteDescription();
  return err;
}
/**********************************************************************/
// to know the maximal number of rows in the CDS catalog
int Catalog::getMaxNumRowsWeb(long *nrows, const std::string catName,
                              const std::string urlCode) {

  int i, err;
  unsigned int pos;
  std::string web, text, origin="getMaxNumRowsWeb";
  *nrows=0;
  err=BAD_URL;
  if (urlCode.length() == 0) {
    text=": CODE for URL (web http address) is empty";
    printErr(origin, text);
    return err;
  }
  for (i=0; i<MAX_URL; i++) {
    text=Catalog::s_CatalogURL[i]; /* convert C string to C++ string */
    pos=text.find(' ');
    if (pos == std::string::npos) continue;
    if (text.substr(0, pos) == urlCode) break;
  }
  if (i == MAX_URL) {
    text=": CODE for URL (web http address) do not exist";
    printErr(origin, text);
    return err;
  }
  pos=text.rfind(' ');
  if (pos == std::string::npos) web=text;
  else web=text.substr(pos+1, text.length()-pos);

  int iCat=checkCatName(origin, catName);
  if (iCat < 0) return iCat;

/* IS IT REALLY NEEDED ?
  err=checkImport(origin, false);
  if (err < IS_VOID) return err;
*/
/* now, ASCII query must be created
  and sent, using CDS package to given URL
*/
  err=-9;
  text="Web query not implemented";
  printErr(origin, text);

  return err;
}


/**********************************************************************/
/*  ACCESSING Catalog DEFINITION                                      */
/**********************************************************************/
// get the 3 definition strings
void Catalog::getCatalogTitles(std::vector<std::string> *titles) {

  int nb=titles->size();
  if (nb == 0) titles->push_back(m_code);
  else (*titles)[0]=m_code;
  if (nb > 1) (*titles)[1]=m_URL;       else return;
  if (nb > 2) (*titles)[2]=m_catName;   else return;
  if (nb > 3) (*titles)[3]=m_catRef;    else return;
  if (nb > 4) (*titles)[4]=m_tableName; else return;
  if (nb > 5) (*titles)[5]=m_tableRef;  else return;
}

/**********************************************************************/
// get an iterator on the quantities


/**********************************************************************/
// get a copy of the quantity vector
int Catalog::getQuantityDescription(std::vector<Quantity> *myQuantities) {

  int quantSize=checkImport("getQuantityDescription", true);
  if (quantSize < IS_VOID) {
    printWarn("getQuantityDescription", "returning unchanged vector.");
    return quantSize;
  }
  *myQuantities=m_quantities;
  return quantSize;
}

/**********************************************************************/
// get only Quantity member name
int Catalog::getQuantityNames(std::vector<std::string> *names) {

  const std::string origin="getQuantityNames";
  int quantSize=checkImport(origin, true);
  names->clear();
  if (quantSize < IS_VOID) {
    printWarn(origin, "returning empty vector.");
    return quantSize;
  }
  try {
    for (int i=0; i<quantSize; i++)
      names->push_back(m_quantities.at(i).m_name);
  }
  catch (const std::exception &err) {
    std::string errText;
    errText=std::string("EXCEPTION on names: ")+err.what();
    printErr(origin, errText);
    throw;
  }
  return quantSize;
}
/**********************************************************************/
// get only Quantity member unit
int Catalog::getQuantityUnits(std::vector<std::string> *units) {

  const std::string origin="getQuantityUnits";
  int quantSize=checkImport(origin, true);
  units->clear();
  if (quantSize < IS_VOID) {
    printWarn(origin, "returning empty vector.");
    return quantSize;
  } 
  try {
    for (int i=0; i<quantSize; i++)
      units->push_back(m_quantities.at(i).m_unit);
  }
  catch (const std::exception &err) {
    std::string errText;
    errText=std::string("EXCEPTION on units: ")+err.what();
    printErr(origin, errText);
    throw;
  }
  return quantSize;
}
/**********************************************************************/
// get only Quantity member ucd
int Catalog::getQuantityUCDs (std::vector<std::string> *ucds) {

  const std::string origin="getQuantityUCDs";
  int quantSize=checkImport(origin, true);
  ucds->clear();
  if (quantSize < IS_VOID) {
    printWarn(origin, "returning empty vector.");
    return quantSize;
  } 
  try {
    for (int i=0; i<quantSize; i++)
      ucds->push_back(m_quantities.at(i).m_ucd);
  }
  catch (const std::exception &err) {
    std::string errText;
    errText=std::string("EXCEPTION on ucds: ")+err.what();
    printErr(origin, errText);
    throw;
  }
  return quantSize;
}
/**********************************************************************/
// get only Quantity member type
int Catalog::getQuantityTypes(std::vector<Quantity::QuantityType> *types) {

  const std::string origin="getQuantityTypes";
  int quantSize=checkImport(origin, true);
  types->clear();
  if (quantSize < IS_VOID) {
    printWarn(origin, "returning empty vector.");
    return quantSize;
  } 
  try {
    for (int i=0; i<quantSize; i++)
      types->push_back(m_quantities.at(i).m_type);
  }
  catch (const std::exception &err) {
    std::string errText;
    errText=std::string("EXCEPTION on types: ")+err.what();
    printErr(origin, errText);
    throw;
  }
  return quantSize;
}

/**********************************************************************/
// get the member "statErrName" of given quantity "name"
int Catalog::getStatErrorName(const std::string name,
                              std::string *statErrName) {

  int quantSize=checkImport("getStatErrorName", true);
  if (quantSize < IS_VOID) return quantSize;
  quantSize=checkQuant_name("getStatErrorName", name);
  if (quantSize < 0) return quantSize;
  *statErrName=m_quantities.at(quantSize).m_statError;
  return IS_OK;
}
/**********************************************************************/
// get the member "sysErrName" of given quantity "name"
int Catalog::getSysErrorName(const std::string name,
                             std::string *sysErrName) {

  int quantSize=checkImport("getSysErrorName", true);
  if (quantSize < IS_VOID) return quantSize;
  quantSize=checkQuant_name("getSysErrorName", name);
  if (quantSize < 0) return quantSize;
  *sysErrName=m_quantities.at(quantSize).m_sysError;
  return IS_OK;
}
  
/**********************************************************************/
// get the list of names of the quantities which form the given vector "name"



/**********************************************************************/
/*  ACCESSING ALL Catalog CONTENTS in MEMORY (IGNORING SELECTION)     */
/**********************************************************************/
// get the number of rows in the catalog
void Catalog::getNumRows(long *nrows) {

  if (m_numRows < 0) *nrows=0;
  else *nrows=m_numRows;
}

/**********************************************************************/
// get the value of given string quantity in given row
int Catalog::getSValue(const std::string name, const long row,
                       std::string *stringVal) {

  const std::string origin="getSValue";
  int num=checkSize_row(origin, row);
  if (num <= IS_VOID) return num;
  num=checkQuant_name(origin, name);
  if (num < 0) return num;
  if (m_quantities.at(num).m_type != Quantity::STRING) {
    std::string errText;
    errText="given Quantity name ("+name+") is not of STRING type";
    printWarn(origin, errText);
    return BAD_QUANT_TYPE;
  }
  num=m_quantities.at(num).m_index;
  #ifdef DEBUG_CAT
    std::cout << "!! DEBUG STRING index = " << num << std::endl;
  #endif
  *stringVal=m_strings[num].at(row);
  return IS_OK;
}
/**********************************************************************/
// get the value of given numerical quantity in given row
int Catalog::getNValue(const std::string name, const long row,
                       double *realVal) {

  const std::string origin="getNValue";
  int num=checkSize_row(origin, row);
  if (num <= IS_VOID) return num;
  num=checkQuant_name(origin, name);
  if (num < 0) return num;
  if (m_quantities.at(num).m_type != Quantity::NUM) {  
    std::string errText;
    errText="given Quantity name ("+name+") is not of NUM type";
    printWarn(origin, errText);
    return BAD_QUANT_TYPE;
  }
  num=m_quantities.at(num).m_index;
  #ifdef DEBUG_CAT
    std::cout << "!! DEBUG NUM index = " << num << std::endl;
  #endif
  *realVal=m_numericals[num].at(row);
  return IS_OK;
}

/**********************************************************************/
// get the values of given vector quantity in given row

/**********************************************************************/
// get the value of the statistical error of given quantity in given row
int Catalog::getStatError(const std::string name, const long row,
                          double *realValStat) {

  *realValStat = -1.0;
  const std::string origin="getStatError";
  int num=checkSize_row(origin, row);
  if (num <= IS_VOID) return num;
  num=checkQuant_name(origin, name);
  if (num < 0) return num;
  std::string statName=m_quantities.at(num).m_statError;
  if (statName.empty()) {;
    statName="given Quantity name ("+name+") has no statistical error";
    printWarn(origin, statName);
    return NO_QUANT_ERR;
  }
  num=checkQuant_name(origin, statName);
  if (num < 0) return num;
  num=m_quantities.at(num).m_index;
  #ifdef DEBUG_CAT
    std::cout << "!! DEBUG NUM STAT index = " << num << std::endl;
  #endif
  *realValStat=m_numericals[num].at(row);
  return IS_OK;
}
/**********************************************************************/
// get the value of the systematic error of given quantity in given row
  int Catalog::getSysError(const std::string name, const long row,
                           double *realValSys) {

  *realValSys = -1.0;
  const std::string origin="getSysError";
  int num=checkSize_row(origin, row);
  if (num <= IS_VOID) return num;
  num=checkQuant_name(origin, name);
  if (num < 0) return num;
  std::string sysName=m_quantities.at(num).m_sysError;
  if (sysName.empty()) {
    sysName="given Quantity name ("+name+") has no systematic error";
    printWarn(origin, sysName);
    return NO_QUANT_ERR;
  }
  num=checkQuant_name(origin, sysName);
  if (num < 0) return num;
  num=m_quantities.at(num).m_index;
  #ifdef DEBUG_CAT
    std::cout << "!! DEBUG NUM SYS. index = " << num << std::endl;
  #endif
  *realValSys=m_numericals[num].at(row);
  return IS_OK;
}

/**********************************************************************/
// get the values of the statistical error of given vector in given row

/**********************************************************************/
// get the values of the systematic error of given vector in given row


/**********************************************************************/
// for string quantity: the list of different values assumed
int Catalog::getSValues(const std::string name,
                        std::vector<std::string> *values) {

  values->clear();
  const std::string origin="getSValues";
  int num=checkSize_row(origin, 0);
  if (num <= IS_VOID) return num;
  num=checkQuant_name(origin, name);
  if (num < 0) return num;
  if (m_quantities.at(num).m_type != Quantity::STRING) {
    std::string errText;
    errText="given Quantity name ("+name+") is not of STRING type";
    printWarn(origin, errText);
    return BAD_QUANT_TYPE;
  }
  num=m_quantities.at(num).m_index;
  std::string text;
  try {
    values->assign(1, m_strings[num].at(0));
    long i, j, max;
    for (i=1; i<m_numRows; i++) {
      text=m_strings[num].at(i);
      max=values->size();
      for (j=0; j<max; j++) if (values->at(j) == text) break;
      if (j == max) values->push_back(text);
    }
  }
  catch (const std::exception &err) {
    text=std::string("EXCEPTION on values: ")+err.what();
    printErr(origin, text);
    throw;
  }
  return values->size();
}

/**********************************************************************/
// for numerical quantity: the minimum value in the catalog
int Catalog::minVal(const std::string name, double *realVal) {

  *realVal=NO_SEL_CUT;
  const std::string origin="minVal";
  int num=checkSize_row(origin, 0);
  if (num <= IS_VOID) return num;
  num=checkQuant_name(origin, name);
  if (num < 0) return num;
  if (m_quantities.at(num).m_type != Quantity::NUM) {
    std::string errText;
    errText="given Quantity name ("+name+") is not of NUM type";
    printWarn(origin, errText);
    return BAD_QUANT_TYPE;
  }
  num=m_quantities.at(num).m_index;
  long i=0;
  double r;
  do { *realVal=m_numericals[num].at(i);
/*  std::cout << "NaN == NaN ? " << (*realVal == m_numericals[num].at(0))
            << std::endl;*/
  }
  while ((isnan(*realVal)) && (++i < m_numRows));
  for (; i<m_numRows; i++) {
    r=m_numericals[num].at(i);
    if (r < *realVal) *realVal=r;
  }
  return IS_OK;
}
/**********************************************************************/
// for numerical quantity: the maximum value in the catalog
int Catalog::maxVal(const std::string name, double *realVal) {

  *realVal=NO_SEL_CUT;
  const std::string origin="maxVal";
  int num=checkSize_row(origin, 0);
  if (num <= IS_VOID) return num;
  num=checkQuant_name(origin, name);
  if (num < 0) return num;
  if (m_quantities.at(num).m_type != Quantity::NUM) {
    std::string errText;
    errText="given Quantity name ("+name+") is not of NUM type";
    printWarn(origin, errText);
    return BAD_QUANT_TYPE;
  }
  num=m_quantities.at(num).m_index;
  long i=0;
  double r;
  do { *realVal=m_numericals[num].at(i); }
  while ((isnan(*realVal)) && (++i < m_numRows));
  for (; i<m_numRows; i++) {
    r=m_numericals[num].at(i);
    if (r > *realVal) *realVal=r;
  }
  return IS_OK;
}


/**********************************************************************/
/*  ACCESSING the Catalog CONTENTS with in-memory SELECTION CRITERIA  */
/**********************************************************************/
// get the number of selected rows in the catalog
void Catalog::getNumSelRows(long *nrows) {

 *nrows=m_numSelRows;
}

/**********************************************************************/
// get the value of given string quantity in given selected row
int Catalog::getSelSValue(const std::string name, const long srow,
                          std::string *stringVal) {

  const std::string origin="getSelSValue";
  int num=checkSel_row(origin, srow);
  if (num <= IS_VOID) return num;
  // above test avoid searching for srow when no row is selected
  num=checkQuant_name(origin, name);
  if (num < 0) return num;
  if (m_quantities.at(num).m_type != Quantity::STRING) {
    std::string errText;
    errText="given Quantity name ("+name+") is not of STRING type";
    printWarn(origin, errText);
    return BAD_QUANT_TYPE;
  }
  num=m_quantities.at(num).m_index;
  long tot=0;
  // first bit indicates global selection
  for (long i=0; i<m_numRows; i++) if (m_rowIsSelected[0].at(i) & 1) {
    if (tot == srow) { *stringVal=m_strings[num].at(i); break; }
    tot++;
    //if (tot == m_numSelRows) break; should not happen
  }
  if (tot < m_numSelRows) return IS_OK;
  return IS_VOID; // should not happen
}
/**********************************************************************/
// get the value of given numerical quantity in given selected row
int Catalog::getSelNValue(const std::string name, const long srow,
                          double *realVal) {

  const std::string origin="getSelNValue";
  int num=checkSel_row(origin, srow);
  if (num <= IS_VOID) return num;
  // above test avoid searching for srow when no row is selected
  num=checkQuant_name(origin, name);
  if (num < 0) return num;
  if (m_quantities.at(num).m_type != Quantity::NUM) {  
    std::string errText;
    errText="given Quantity name ("+name+") is not of NUM type";
    printWarn(origin, errText);
    return BAD_QUANT_TYPE;
  }
  num=m_quantities.at(num).m_index;
  long tot=0;
  // first bit indicates global selection
  for (long i=0; i<m_numRows; i++) if (m_rowIsSelected[0].at(i) & 1) {
    if (tot == srow) { *realVal=m_numericals[num].at(i); break; }
    tot++;
    //if (tot == m_numSelRows) break; should not happen
  }
  if (tot < m_numSelRows) return IS_OK;
  return IS_VOID; // should not happen
}

/**********************************************************************/
// get the values of given vector quantity in given selected row

/**********************************************************************/
// get value of the statistical error of given quantity in given selected row
int Catalog::getSelStatError(const std::string name, const long srow,
                             double *realValStat) {

  *realValStat = -1.0;
  const std::string origin="getSelStatError";
  int num=checkSel_row(origin, srow);
  if (num <= IS_VOID) return num;
  // above test avoid searching for srow when no row is selected
  num=checkQuant_name(origin, name);
  if (num < 0) return num;
  std::string statName=m_quantities.at(num).m_statError;
  if (statName.empty()) {;
    statName="given Quantity name ("+name+") has no statistical error";
    printWarn(origin, statName);
    return NO_QUANT_ERR;
  }
  num=checkQuant_name(origin, statName);
  if (num < 0) return num;
  num=m_quantities.at(num).m_index;
  #ifdef DEBUG_CAT
    std::cout << "!! DEBUG NUM STAT index = " << num << std::endl;
  #endif
  long tot=0;
  // first bit indicates global selection
  for (long i=0; i<m_numRows; i++) if (m_rowIsSelected[0].at(i) & 1) {
    if (tot == srow) { *realValStat=m_numericals[num].at(i); break; }
    tot++;
  }
  if (tot < m_numSelRows) return IS_OK;
  return IS_VOID; // should not happen
}
/**********************************************************************/
// get value of the systematic error of given quantity in given selected row
int Catalog::getSelSysError(const std::string name, const long srow,
                             double *realValSys) {

  *realValSys = -1.0;
  const std::string origin="getSelSysError";
  int num=checkSel_row(origin, srow);
  if (num <= IS_VOID) return num;
  // above test avoid searching for srow when no row is selected
  num=checkQuant_name(origin, name);
  if (num < 0) return num;
  std::string sysName=m_quantities.at(num).m_sysError;
  if (sysName.empty()) {;
    sysName="given Quantity name ("+name+") has no systematic error";
    printWarn(origin, sysName);
    return NO_QUANT_ERR;
  }
  num=checkQuant_name(origin, sysName);
  if (num < 0) return num;
  num=m_quantities.at(num).m_index;
  #ifdef DEBUG_CAT
    std::cout << "!! DEBUG NUM SYS. index = " << num << std::endl;
  #endif
  long tot=0;
  // first bit indicates global selection
  for (long i=0; i<m_numRows; i++) if (m_rowIsSelected[0].at(i) & 1) {
    if (tot == srow) { *realValSys=m_numericals[num].at(i); break; }
    tot++;
  }
  if (tot < m_numSelRows) return IS_OK;
  return IS_VOID; // should not happen
}
/**********************************************************************/
// get the values of the statistical error of given vector in given selected row

/**********************************************************************/
// get the values of the systematic error of given vector in given selected row


/**********************************************************************/
// for string quantity: the list of different selected values assumed
int Catalog::getSelSValues(const std::string name,
                           std::vector<std::string> *values) {

  values->clear();
  const std::string origin="getSelSValues";
  int num=checkSel_row(origin, 0);
  if (num <= IS_VOID) return num;
  // above test avoid searching when no row is selected
  num=checkQuant_name(origin, name);
  if (num < 0) return num;
  if (m_quantities.at(num).m_type != Quantity::STRING) {
    std::string errText;
    errText="given Quantity name ("+name+") is not of STRING type";
    printWarn(origin, errText);
    return BAD_QUANT_TYPE;
  }
  num=m_quantities.at(num).m_index;
  std::string text;
  try {
    long j, max=0, tot=0;
    // first bit indicates global selection
    for (long i=0; i<m_numRows; i++) if (m_rowIsSelected[0].at(i) & 1) {
      text=m_strings[num].at(i);
      if (max == 0) {values->assign(1, text); max++;}
      else {
        for (j=0; j<max; j++) if (values->at(j) == text) break;
        if (j == max) {values->push_back(text); max++;}
      }
      if (++tot == m_numSelRows) break; // to speed up
    }
  }
  catch (const std::exception &err) {
    text=std::string("EXCEPTION on values: ")+err.what();
    printErr(origin, text);
    throw;
  }
  return values->size();
}

/**********************************************************************/
// for numerical quantity: the minimum value in the selected rows
int Catalog::minSelVal(const std::string name, double *realVal) {

  *realVal=NO_SEL_CUT;
  const std::string origin="minSelVal";
  int num=checkSel_row(origin, 0);
  if (num <= IS_VOID) return num;
  // above test avoid searching when no row is selected
  num=checkQuant_name(origin, name);
  if (num < 0) return num;
  if (m_quantities.at(num).m_type != Quantity::NUM) {
    std::string errText;
    errText="given Quantity name ("+name+") is not of NUM type";
    printWarn(origin, errText);
    return BAD_QUANT_TYPE;
  }
  num=m_quantities.at(num).m_index;

  long i, tot=0;
  double r;
  for (i=0; i<m_numRows; i++) if (m_rowIsSelected[0].at(i) & 1) {
    *realVal=m_numericals[num].at(i);
    if (!isnan(*realVal)) break;
    if (++tot == m_numSelRows) break; // to speed up
  }
  if (tot == m_numSelRows) return IS_OK;
  // stop when selection contains only NaN
  for (; i<m_numRows; i++) if (m_rowIsSelected[0].at(i) & 1) {
    r=m_numericals[num].at(i);
    if (r < *realVal) *realVal=r;
    if (++tot == m_numSelRows) break; // to speed up
  }
  return IS_OK;
}
/**********************************************************************/
// for numerical quantity: the maximum value in the selected rows
int Catalog::maxSelVal(const std::string name, double *realVal) {

  *realVal=NO_SEL_CUT;
  const std::string origin="maxSelVal";
  int num=checkSel_row(origin, 0);
  if (num <= IS_VOID) return num;
  // above test avoid searching when no row is selected
  num=checkQuant_name(origin, name);
  if (num < 0) return num;
  if (m_quantities.at(num).m_type != Quantity::NUM) {
    std::string errText;
    errText="given Quantity name ("+name+") is not of NUM type";
    printWarn(origin, errText);
    return BAD_QUANT_TYPE;
  }
  num=m_quantities.at(num).m_index;

  long i, tot=0;
  double r;
  for (i=0; i<m_numRows; i++) if (m_rowIsSelected[0].at(i) & 1) {
    *realVal=m_numericals[num].at(i);
    if (!isnan(*realVal)) break;
    if (++tot == m_numSelRows) break; // to speed up
  }
  if (tot == m_numSelRows) return IS_OK;
  // stop when selection contains only NaN
  for (; i<m_numRows; i++) if (m_rowIsSelected[0].at(i) & 1) {
    r=m_numericals[num].at(i);
    if (r > *realVal) *realVal=r;
    if (++tot == m_numSelRows) break; // to speed up
  }
  return IS_OK;
}


/**********************************************************************/
/*  ACCESSING the Catalog GENERIC CONTENTS                            */
/**********************************************************************/
// access to generic quantity for RA  (degrees)
int Catalog::ra_deg(const long row, double *realVal, const bool inSelection) {

  const std::string origin="ra_deg";
  if (m_indexRA < 0) {
    if (m_numRows < 0)
      printWarn(origin, "must first use one 'import' method");
    else
      printWarn(origin, "missing generic RA position quantity");
    return NO_RA_DEC;
  }
  int num;
  if (!inSelection) {
    num=checkSize_row(origin, row);
    if (num <= IS_VOID) return num;
    num=m_quantities[m_indexRA].m_index;
    *realVal=m_numericals[num].at(row);
  }
  else {
    num=checkSel_row(origin, row);
    if (num <= IS_VOID) return num;
    long tot=0;
    num=m_quantities[m_indexRA].m_index;
    // first bit indicates global selection
    for (long k=0; k<m_numRows; k++) if (m_rowIsSelected[0].at(k) & 1) {
      if (tot == row) { *realVal=m_numericals[num].at(k); break; }
      tot++;
    }
    if (tot >= m_numSelRows) return IS_VOID; // should not happen
  }
  return IS_OK;
}
/**********************************************************************/
// access to generic quantity for DEC (degrees)
int Catalog::dec_deg(const long row, double *realVal, const bool inSelection) {

  const std::string origin="dec_deg";
  if (m_indexDEC < 0) {
    if (m_numRows < 0)
      printWarn(origin, "must first use one 'import' method");
    else
      printWarn(origin, "missing generic DEC position quantity");
    return NO_RA_DEC;
  }
  int num;
  if (!inSelection) {
    num=checkSize_row(origin, row);
    if (num <= IS_VOID) return num;
    num=m_quantities[m_indexDEC].m_index;
    *realVal=m_numericals[num].at(row);
  }
  else {
    num=checkSel_row(origin, row);
    if (num <= IS_VOID) return num;
    long tot=0;
    num=m_quantities[m_indexDEC].m_index;
    // first bit indicates global selection
    for (long k=0; k<m_numRows; k++) if (m_rowIsSelected[0].at(k) & 1) {
      if (tot == row) { *realVal=m_numericals[num].at(k); break; }
      tot++;
    }
    if (tot >= m_numSelRows) return IS_VOID; // should not happen
  }
  return IS_OK;
}
/**********************************************************************/
// access to generic quantity for position uncertainty (degrees)
int Catalog::posError_deg(const long row, double *realVal,
                          const bool inSelection) {

  const std::string origin="posError_deg";
  if (m_indexErr < 0) {
    if (m_numRows < 0)
      printWarn(origin, "must first use one 'import' method");
    else
      printWarn(origin, "missing generic position error quantity");
    return BAD_QUANT_NAME;
  }
  int num;
  if (!inSelection) {
    num=checkSize_row(origin, row);
    if (num <= IS_VOID) return num;
    num=m_quantities[m_indexErr].m_index;
    *realVal=m_numericals[num].at(row)/m_posErrFactor;
  }
  else {
    num=checkSel_row(origin, row);
    if (num <= IS_VOID) return num;
    long tot=0;
    num=m_quantities[m_indexErr].m_index;
    // first bit indicates global selection
    for (long k=0; k<m_numRows; k++) if (m_rowIsSelected[0].at(k) & 1) {
      if (tot == row) {
        *realVal=m_numericals[num].at(k)/m_posErrFactor;
        break;
      }
      tot++;
    }
    if (tot >= m_numSelRows) return IS_VOID; // should not happen
  }
  return IS_OK;
}

/**********************************************************************/
// 6 methods returning the name of generic quantities
std::string Catalog::getNameRA() {
  if (m_indexRA < 0) {
    if (m_numRows < 0)
      printWarn("getNameRA", "must first use one 'import' method");
    else
      printWarn("getNameRA", "missing generic RA position quantity");
    return "";
  }
  return m_quantities[m_indexRA].m_name;
}
std::string Catalog::getNameDEC() {
  if (m_indexDEC < 0) {
    if (m_numRows < 0)
      printWarn("getNameDEC", "must first use one 'import' method");
    else
      printWarn("getNameDEC", "missing generic DEC position quantity");
    return "";
  }
  return m_quantities[m_indexDEC].m_name;
}
std::string Catalog::getNamePosErr() {
  if (m_indexErr< 0) {
    if (m_numRows < 0)
      printWarn("getNamePosErr", "must first use one 'import' method");
    else
      printWarn("getNamePosErr", "missing generic position error quantity");
    return "";
  }
  return m_quantities[m_indexErr].m_name;
}

std::string Catalog::getNameL() {
  const std::string origin="getNameL";
  int quantSize=checkImport(origin, true);
  if (quantSize < IS_VOID) return "";
  int i;
  for (i=0; i<quantSize; i++) {
    if ((m_quantities[i].m_isGeneric) &&
        (m_quantities[i].m_ucd == Catalog::s_genericL))
      return m_quantities[i].m_name;
  }
  return "";
}
std::string Catalog::getNameB() {
  const std::string origin="getNameB";
  int quantSize=checkImport(origin, true);
  if (quantSize < IS_VOID) return "";
  int i;
  for (i=0; i<quantSize; i++) {
    if ((m_quantities[i].m_isGeneric) &&
        (m_quantities[i].m_ucd == Catalog::s_genericB))
      return m_quantities[i].m_name;
  }
  return "";
}

std::string Catalog::getNameObjName() {
  const std::string origin="getNameObjName";
  int quantSize=checkImport(origin, true);
  if (quantSize < IS_VOID) return "";
  int i;
  for (i=0; i<quantSize; i++) {
    if ((m_quantities[i].m_isGeneric) &&
        (m_quantities[i].m_type == Quantity::STRING))
      return m_quantities[i].m_name;
  }
  return "";
}

} // namespace catalogAccess
