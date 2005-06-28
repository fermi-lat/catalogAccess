/**
 * @file   catalog_io.cxx
 * @brief  Read/write (or import/export) routines for Catalog class.
 * Methods exist to access both files: ASCII or fits (writing only BINARY table,
 * reading any table format).
 * Methods to load catalog through web are placeholders.
 * Methods not coded: READ only selected data from file.
 *
 * @author A. Sauvageon
 *
 * $Header $
 *
 */

#include "catalog.h"

namespace catalogAccess {
  using namespace tip;
/**********************************************************************/
/*  DEFINING CLASS CONSTANTS                                          */
/**********************************************************************/

// BEWARE: the two constant below are UCD1 standard
//         need to be changed for UCD1+

// the order is important and must be compatible with s_CatalogGeneric
static const char *UCD_List[MAX_GEN]={
       "ID_MAIN", "POS_EQ_RA_MAIN", "POS_EQ_DEC_MAIN",
        "",       "POS_GAL_LON",    "POS_GAL_LAT"};
// "ERROR", "POS_GAL_LON", "POS_GAL_LAT"
// ERROR do not only refer to the position error
// ERROR is for any "Uncertainty in Measurements"

static const char *UCD_Added[MAX_GEN]={
       "", "_RAJ2000", "_DEJ2000", "", "_Glon", "_Glat"};

// the 2 constants above are only defined in this file,
// need to set catalog static members for global availability

const std::string Catalog::s_genericL = UCD_List[4];
const std::string Catalog::s_genericB = UCD_List[5];
const std::string KeyCatal[2]={"CDS-CAT",
                               "Catalogue designation in CDS nomenclature"};
const std::string KeyTable[2]={"CDS-NAME", "Table used in a VizieR Query"};
const std::string Key_UCD="TBUCD";

/**********************************************************************/
/*  METHODS for IMPORTING, SAVING, LOADING                            */
/**********************************************************************/

// set m_posErrFactor, suppose that Quantity index exist (private method)
void Catalog::setPosErrFactor(const int index) {

  std::string text=m_quantities[index].m_unit;
  if (text == "") {
    std::ostringstream sortie;
    sortie << "Generic position error (" << m_quantities[index].m_name
           << ") has no unit, by default multiply by: 1/" << m_posErrFactor;
    printWarn("private setGeneric", sortie.str());
  }
  else {
    // use explicit cast to be sure compiler choose the right tolower()
    transform(text.begin(), text.end(), text.begin(), (int(*)(int)) tolower);
    if (text == "deg") m_posErrFactor=1.0;
    else if (text == "arcsec") m_posErrFactor=3600.0;
    else if (text == "arcmin") m_posErrFactor=60.0;
    else if (text == "rad") m_posErrFactor=1.0/Angle_Conv;
    else {
      std::ostringstream sortie;
      sortie << "Generic position error (" << m_quantities[index].m_name
             << ") has unknown unit, by default multiply by: 1/" << m_posErrFactor;
      printWarn("private setGeneric", sortie.str());
    }
  }
}
// search for generic quantities, private: suppose import done (private method)
void Catalog::setGeneric(const int whichCat) {

  const std::string epoch="J2000";
  int max=m_quantities.size();
  int j;
  std::string text, name;
  std::vector<Quantity>::iterator itQ=m_quantities.begin();
  for (int i=0; itQ != m_quantities.end(); ++itQ, ++i) {

    if ((whichCat >= 0) && (whichCat < MAX_CAT)) {
      // if it is a known catalog, the generic are defined
      text=itQ->m_name;
      for (j=0; j<MAX_GEN; j++) {
        name=Catalog::s_CatalogGeneric[whichCat][j];
        if (name == "+") name=UCD_Added[j];
        if (text == name) {
          itQ->m_isGeneric=true;
          switch (j) {
            case 1: m_indexRA=i;
            break;
            case 2: m_indexDEC=i;
            break;
            case 3: m_indexErr=i;
                    setPosErrFactor(i);
            break;
            default: break;
          }
        }
      }// loop on generic
      text=itQ->m_ucd;
    }

    else {
      // otherwise, take first matching UCD
      text=itQ->m_ucd;
      if (text != "") for (j=0; j<MAX_GEN; j++) { 
        if (text == UCD_List[j]) {
          itQ->m_isGeneric=true;
          if ((j == 1) || (j == 2)) {
            // check that epoch is J2000
            name=itQ->m_name;
            name.erase(0, name.length()-epoch.length());
            // format contains at least 1 char
            if ((name == epoch) && (itQ->m_type == Quantity::NUM)) {
              if (j == 1) m_indexRA=i;
              else        m_indexDEC=i;
            }
            else itQ->m_isGeneric=false;
          }
          break;
        }
      }// loop on generic
    }

    // flag error quantities, useless for already found generic
    if ((text == "ERROR") && (!itQ->m_isGeneric)) {
      // error column name is e_"associated column" except for position error
      text=itQ->m_name;
      if (text.find("e_") == 0) {
        text.erase(0, 2);
        #ifdef DEBUG_CAT
        std::cout << "ERROR column (" << text << ")" << std::endl;
        #endif
        for (j=0; j<max; j++) if (m_quantities.at(j).m_name == text) {
          itQ->m_statError=text;
          break;
        }
      }
      else if ((text == "PosErr") || (text == "ErrorRad")) {
        itQ->m_isGeneric=true;
        m_indexErr=i;
        setPosErrFactor(i);
      }
    }

  }// loop on quantities

}

/**********************************************************************/
// return the number of RAM bytes needed for numRows catalog rows
long Catalog::getRAMsize(const long numRows, const bool writeLog) {

  std::string mot, text;
  int quantSize=m_quantities.size();
  if (quantSize == 0) return IMPORT_NEED;

  int nD=0, nS=0, nchar=0;
  int i, j;
  std::vector<Quantity>::iterator itQ=m_quantities.begin();
  for (; itQ != m_quantities.end(); ++itQ) {
    if (itQ->m_type == Quantity::NUM) nD++;
    else if  (itQ->m_type == Quantity::STRING) {
      nS++;
      text=itQ->m_format;
      i=text.length();
      if (i > 1) {
        if  (m_URL == "CDS") text.erase(0, 1);
        else text.erase(i-1);
        j=std::atoi(text.c_str());
        if (j > 0) nchar+=j;
      }
    }
  }
  char buffer[50];
  long sizeD=nD*sizeof(double)*numRows;
  long sizeS=nchar*sizeof(char)*numRows;
  i=1+(quantSize+1)/(sizeof(long)*8);
  long sizeB=i*sizeof(long)*numRows;
  if (writeLog && (m_numOriRows > 0)) {
    sprintf(buffer, "%6ld", m_numOriRows);
    mot=buffer; /* convert C string to C++ string */
    text="Original whole catalog number of rows = "+mot;
    printLog(1, text);
  }
  if (numRows <= 0) return 0;
  if (writeLog) {
    sprintf(buffer, "%6ld", numRows);
    mot=buffer; /* convert C string to C++ string */
    text="Needed RAM space (MB) for "+mot;
    sprintf(buffer, "%5.1f", (sizeD+sizeS+sizeB)/(1024.*1024.));
    mot=buffer;
    text=text+" data rows = "+mot+"\n";
    sprintf(buffer, "%5.0f kB for numericals (%3d double per row)",
                   sizeD/1024., nD);
    mot=buffer;
    text=text+mot+"\n";
    sprintf(buffer, "%5.0f kB for %2d strings (%3d char per row)",
                   sizeS/1024., nS, nchar);
    mot=buffer;
    text=text+mot+"\n";
    sprintf(buffer, "%5.0f kB for select bits (%2d long per row)",
                   sizeB/1024., i);
    mot=buffer;
    text=text+mot;
    printLog(1, text);
  }
  return (sizeD+sizeS+sizeB);
}


/**********************************************************************/
// creates a new column in m_strings, m_numericals (private method)
void Catalog::create_tables(const int nbQuantAscii, const long maxRows) {

  int vecSize;
  std::string errText;
  if (nbQuantAscii > 0) {
    try {
      m_strings.resize(nbQuantAscii);
    }
    catch (const std::exception &err) {
      errText=std::string("EXCEPTION on m_strings: ")+err.what();
      printErr("private create_tables", errText);
      throw;
    }
  }
  vecSize=m_quantities.size()-nbQuantAscii;
// printf("sizes = %d , %d\n", nbQuantAscii, vecSize);
  if (vecSize > 0) {
    try {
      m_numericals.resize(vecSize);
    }
    catch (const std::exception &err) {
      errText=std::string("EXCEPTION on m_numericals: ")+err.what();
      printErr("private create_tables", errText);
      throw;
    }
  }
  if (maxRows > 0) add_rows(maxRows);

}
/**********************************************************************/
// creates a new row in m_strings, m_numericals (private method)
void Catalog::add_rows(const long maxRows) {

  int i, vecSize;
  std::string errText;
//printf("!! %ld / %ld \n", m_numRows, maxRows);
  vecSize=m_strings.size();
  if (vecSize > 0 ) {
    try {
      for (i=0; i<vecSize; i++) {
        m_strings[i].resize(maxRows);
//        for (j=0; j<maxRows; j++) m_strings[i].at(j).resize(20);
      }
    }
    catch (const std::exception &err) {
      errText=std::string("EXCEPTION on m_strings: ")+err.what();
      printErr("private add_rows", errText);
      throw;
    }
  }
  vecSize=m_numericals.size();
  if (vecSize > 0 ) {
    try {
      for (i=0; i<vecSize; i++) m_numericals[i].resize(maxRows);
    }
    catch (const std::exception &err) {
      errText=std::string("EXCEPTION on m_numericals: ")+err.what();
      printErr("private add_rows", errText);
      throw;
    }
  } 

}


/**********************************************************************/
// read the catalog from fits file (only description if getDescr is true)
int Catalog::analyze_fits(const Table *myDOL, const bool getDescr,
                          const std::string origin, long *maxRows) {
  std::string text, mot;
  int  i, j, max,
       nbQuantAscii=0;
  char name[9]; /* 8 char maximum for header key */
  bool binary=true;
  unsigned int pos;
  int (*pfunc)(int)=toupper; // function used by transform
  const Header &header=myDOL->getHeader();

  try {
    text=myDOL->getName();
    header.getKeyword("XTENSION", mot);
  /* text.rfind('_');
    since '/' are replaced by '_' in EXTNAME
    and  is ambiguous with '_' in catalog name:
    must search for keywords CDS-CAT  CDS-NAME
  */
  }
  catch (const TipException &x) {
    text=": fits EXTENSION, cannot get name";
    printErr(origin, text);
    return BAD_FITS;
  }
  std::ostringstream sortie;
  sortie << "fits extension " << mot << " name = " << text;
  printLog(1, sortie.str());
  sortie.str(""); // Will empty the string.

  m_catName="";   m_catRef="";
  m_tableName=""; m_tableRef="";
  m_URL="";
  if (mot == "TABLE") {
    binary=false;
    m_URL="CDS"; /* to know that format string need to be changed for fits */
  }
  try {
    header.getKeyword(KeyTable[0], text);
    m_tableName=text;
    header.getKeyword(KeyCatal[0], mot);
    m_catName=mot;

  }
  catch (const TipException &x) {
    if (binary) {
      printWarn(origin, "fits EXTENSION, cannot get CDS header keys");
    }
    else {
      printErr(origin, ": fits EXTENSION, cannot get CDS header keys");
      return BAD_FITS;
    }
  }
  try {
    text=header.getKeyComment(KeyTable[0]);
    mot =header.getKeyComment(KeyCatal[0]);
    // have to use long comment
  }
  catch (const TipException &x) {
    printWarn(origin, "fits EXTENSION, cannot get CDS header comments");
  }
//  const Table::FieldCont &allCol=myDOL->getValidFields();
  try {
    max=myDOL->getValidFields().size();
    m_numOriRows=myDOL->getNumRecords();
  }
  catch (const TipException &x) {
    printErr(origin, ": fits EXTENSION, cannot get number of rows or columns");
    return BAD_FITS;
  }
  if (max < 1) {
    printErr(origin, ": fits EXTENSION, need at least 1 column");
    return BAD_FILETYPE;
  }
  text="";
  std::vector<double> colNull(max, 0.0);
  try {
    const IColumn *myCol = 0;
    for (i=0; i < max; i++) {
      text="";
      myCol=myDOL->getColumn(i);
      Quantity readQ;
      readQ.m_name=myCol->getId();
      readQ.m_unit=myCol->getUnits();
      sprintf(name, "TFORM%d", i+1);
      mot=name; /* convert C string to C++ string */
      header.getKeyword(mot, text);
      transform(text.begin(), text.end(), text.begin(), pfunc);
      j=text.length();
      readQ.m_format=text;
      text="";
      if (binary) {
        if ( !myCol->isScalar() ) {
          text="unauthorized FITS TABLE vector";
          sortie << ": VECTOR not supported, column#" << i+1;
          throw std::runtime_error(text);
        }
        if (j == 0) {
          text="unauthorized FITS empty format ";
          sortie << ": FITS format is empty, column#" << i+1;
          throw std::runtime_error(text);
        }
        if (readQ.m_format[j-1] == 'A') readQ.m_type=Quantity::STRING;
        else readQ.m_type=Quantity::NUM;
        sprintf(name, "TNULL%d", i+1); mot=name;
        try { header.getKeyword(mot, readQ.m_null); }
        catch (const TipException &x) {}
        colNull.at(i)=atof(readQ.m_null.c_str());
//printf("'%s'\n",  readQ.m_null.c_str() );
        sprintf(name, "%s%d", Key_UCD.c_str(), i+1); mot=name;
        try {
          readQ.m_comment=header.getKeyComment(mot);
          header.getKeyword(mot, text);
        }
        catch (const TipException &x) {
          sortie << "missing keyword " << mot << " for column#" << i+1;
          printWarn(origin, sortie.str() );
          sortie.str(""); // Will empty the string.
        }
      }
      else {
        if (readQ.m_format[0] == 'A') readQ.m_type=Quantity::STRING;
        else readQ.m_type=Quantity::NUM;
        sprintf(name, "TBCOL%d", i+1); mot=name;
        text=header.getKeyComment(mot);
        j=4;
        if (text.substr(0,j) == "UCD=") text.erase(0,j);
        pos=text.find('.');
        if (pos != std::string::npos) text.erase(pos);;
        /* ONLY  readQ.m_comment  IS NOT SET */
      }
      transform(text.begin(), text.end(), text.begin(), pfunc);
      readQ.m_ucd=text;
      if (readQ.m_type == Quantity::STRING) {
        readQ.m_index=nbQuantAscii;
        nbQuantAscii++;
      }
      else if  (readQ.m_type == Quantity::NUM) {
        readQ.m_index=m_quantities.size()-nbQuantAscii;
        readQ.m_type=Quantity::NUM;
      }
      try { m_quantities.push_back(readQ); }
      catch (const std::exception &prob) {
        text=std::string("EXCEPTION filling m_quantities: ")+prob.what();
        sortie << text;
        throw;
      }
    }/* loop on columns */
  }
  catch (...) {
    m_quantities.clear();
    j=BAD_FILETYPE;
    m_numRows=j;
    if ( text.empty() ) {
      sortie << ": fits EXTENSION, error reading TABLE column#";
      sortie << i+1 << " description";
      printErr(origin, sortie.str() );
    }
    else {
      printErr(origin, sortie.str() );
      // exception sent only for vector allocation
      if (text == sortie.str() ) throw;
    }
    return j;
  }
//printf("numRows=%ld\n", m_numOriRows );
  m_numRows=0;
  if ( !*maxRows ) *maxRows=m_numOriRows;
  if ((getDescr) || (m_numOriRows == 0)) return IS_OK;

  create_tables(nbQuantAscii, *maxRows);
  try {
    double rowVal;
    std::vector<Quantity>::iterator itQ;
    // Loop over all records (rows) and extract values
    for (Table::ConstIterator itor=myDOL->begin(); itor != myDOL->end();
         ++itor) {
      i=0;
      for (itQ=m_quantities.begin(); itQ != m_quantities.end(); ++itQ, ++i) {
        j=itQ->m_index;
        // double variable to hold the value of all the numeric fields
        if (itQ->m_type == Quantity::NUM) {
          rowVal=(*itor)[itQ->m_name].get();
          if (itQ->m_null == "") m_numericals[j].at(m_numRows)=rowVal;
          else {
            if (rowVal == colNull.at(i)) m_numericals[j].at(m_numRows)=MissNAN;
            else m_numericals[j].at(m_numRows)=rowVal;
          }
        }
        else (*itor)[itQ->m_name].get(m_strings[j].at(m_numRows) );
      }
      if (++m_numRows == *maxRows) break;
    }/* loop on rows*/
  }
  catch (const TipException &x) {
    sortie << ": fits EXTENSION, cannot read after row#" << m_numRows+1;
    printErr(origin, sortie.str() );
    return BAD_ROW;
  }
  return IS_OK;
}
/**********************************************************************/
/* PRIVATE METHODS analyze_head, analyze_body are in file "catalog_ioText.cxx"
/**********************************************************************/
// common code between import and importDescription (private method)
int Catalog::load(const std::string &fileName, const std::string ext,
                  const bool getDescr, long *maxRows) {

  int i, err=IS_OK;
  std::string origin, text;
  if (getDescr) origin="importDescription"; else origin="import";

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
      // or is not a table, or is not either Root nor Fits format. 
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
    }
    catch (const TipException &x) {
      sortie << ": FITS is TABLE, but cfitsio returned error=" << x.code();
      printErr(origin, sortie.str());
      err=BAD_FITS;
      m_numRows=err;
      delete myDOL;
      return err;
    }
    const char lf = 0x0A;
    m_filename=fileName+lf+ext;
    err=analyze_fits(myDOL, getDescr, origin, maxRows);
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
      sortie << ": FILENAME \"" << fileName << "\" is fits without extension[] "
             << "specified";
      printErr(origin, sortie.str());
      sortie.str(""); // Will empty the string.
      text="input file";
      if (err == IS_OK) err=BAD_FILENAME;
      // in case, for strange reason, file.good() fails at first time
    }
    else if (err < IS_OK) {
      sortie << ": FILENAME \"" << fileName << "\" is empty or "
             << "has unknown structure (stopped step " << -1*err << ")";
      printErr(origin, sortie.str());
      sortie.str(""); // Will empty the string.
      text="input file";
      if (err == 0) err=BAD_FILENAME; /* nothing is read */
      else err=BAD_FILETYPE; /* can search if catalog name exist */
    }
    else {

      //what is 1 or 2, then negate if TSV type
      if (what == 1) text="input text file";
      else text="input META file";

      // get columns description and units, data
      err=analyze_body(&tot, &what, testCR, getDescr, &myFile, maxRows);
      if (err == BAD_FILELINE) {
        // stopped before reading needed stuff (with getDescr false)
        text=": FILENAME \""+fileName+"\" couldn't be read (line too long)";
        printErr(origin, text);
      }
      else if (err < IS_OK) {
        sortie << ": FILENAME \"" << fileName
             << "\" has wrong type (stopped step " << 5-1*err << ")";
        printErr(origin, sortie.str());
        sortie.str(""); // Will empty the string.
        err=BAD_FILETYPE;
      }
      else {
        if (what > 0) sortie << text << " is CSV type (; separator)";
        else sortie << text << " is TSV type (Tab=0x09 separator)";
        printLog(1, sortie.str());
        sortie.str(""); // Will empty the string.
      }

    }
    myFile.close();
    sortie << text << ": " << tot << " lines read";
    printLog(0, sortie.str());
    m_filename=fileName;
    m_URL="CDS"; /* to know that format string need to be changed for fits */

  }// file is read
  if (err < 0) {
    deleteContent(); // to erase data already loaded in memory
    m_numRows=err;
    // exit only if nothing is read
    if ((err == BAD_FILENAME) || (err == BAD_FITS)) return err;
  }
  // m_quantities vector maybe filled, MUST fill m_selEllipse, m_loadQuantity
  try {
    m_selEllipse.assign(7, 0.0);
    m_loadQuantity.assign(m_quantities.size(), true);
  }
  catch (const std::exception &prob) {
    text=std::string("EXCEPTION on creating m_selEllipse, m_loadQuantity: ")
        +prob.what();
    printErr(origin, text);
    throw;
  }
  // check if tableName is known to set generic
  for (i=0; i<MAX_CAT; i++) {
    if (Catalog::s_CatalogList[2*i+1] == m_tableName) break;
  }
  if (i == MAX_CAT) {
    text="Unknown table name, all generic quantities may be not found";
    printWarn(origin, text);
  }
  else m_code=Catalog::s_CatalogList[2*i];
  setGeneric(i);
  return err;
}
/**********************************************************************/
// read only the catalog description from file
int Catalog::importDescription(const std::string &fileName,
                               const std::string ext) {
  int err;
  err=checkImport("importDescription", false);
  // must be called before load() which set m_numRows to 0
  if (err < IS_VOID) return err;

  long maxRows=0l;
  err=load(fileName, ext, true, &maxRows);
  if (err < IS_OK) return err;

  return m_quantities.size();
}
/**********************************************************************/
// read from file an entire catalog without selection
int Catalog::import(const std::string &fileName, const long maxRows,
                    const std::string ext) {
  int err;
  long maxR=maxRows;
  err=checkImport("import", false);
  if (err < IS_VOID) return err;

  if (maxR <= 0) {
    printWarn("import", "trying to get whole catalog file");
    maxR=0;
  }

  err=load(fileName, ext, false, &maxR);
  if (err < IS_OK) return err;
  getRAMsize(m_numRows, true);
  try {
    if (m_numRows < maxR) {
      int i;
      err=m_strings.size();
      // erase unused memory  printf("ERASING\n");
      for (i=0; i<err; i++) m_strings[i].resize(m_numRows);
      err=m_numericals.size();
      for (i=0; i<err; i++) m_numericals[i].resize(m_numRows);
    }
    err=m_quantities.size()+2;
    // number of required bits including global and region
    err=1+(err-1)/(sizeof(long)*8);
    m_rowIsSelected.resize(err);
    #ifdef DEBUG_CAT
    std::cout << "Number of unsigned long required for m_rowIsSelected = "
              << err << std::endl;
    #endif
    if (m_numRows)
      for (int j=0; j<err; j++) m_rowIsSelected[j].assign(m_numRows, 0);
    // above lines can be commented to test the try catch mechanism
  }
  catch (const std::exception &prob) {
    std::string text;
    text=std::string("EXCEPTION filling m_rowIsSelected: ")+prob.what();
    printErr("import", text);
    throw;
  }
  return m_numRows;
}


/**********************************************************************/
int Catalog::loadSelectFits(const std::string &fileName, const std::string ext,
                            long *maxRows)
{
  const std::string  origin="importSelected";
  std::ostringstream sortie;
  unsigned int       i, max;
  const Table *myDOL = 0;

  try {
    myDOL=IFileSvc::instance().readTable(fileName, ext);
  }
  catch (const TipException &x) {
    sortie << ": FITS is TABLE, but cfitsio returned error=" << x.code();
    printErr(origin, sortie.str());
    delete myDOL;
    return BAD_FITS;
  }
  try {
    max=myDOL->getValidFields().size();
    m_numOriRows=myDOL->getNumRecords();
  }
  catch (const TipException &x) {
    printErr(origin, ": fits EXTENSION, cannot get number of rows or columns");
    delete myDOL;
    return BAD_FITS;
  }
  if (max < 1) {
    printErr(origin, ": fits EXTENSION, need at least 1 column");
    delete myDOL;
    return BAD_FILETYPE;
  }
  /* beware some initial qunatities can be skipped */
  if (m_loadQuantity.size() != max) {
    sortie << ": fits EXTENSION, number of fields (" << max;
    sortie << ") differ from previous import call (";
    sortie << m_loadQuantity.size() << ")";
    printErr(origin, sortie.str());
    delete myDOL;
    return BAD_FILETYPE;
  }
  const Header &header=myDOL->getHeader();
  std::string text, unit, mot, form;
  int  err=0;
  char name[9]; /* 8 char maximum for header key */
  std::vector<double> colNull(max, 0.0);
  std::vector<Quantity>::iterator itQ=m_quantities.begin();
  try {
    const IColumn *myCol = 0;
    for (i=0; i < max; i++) {
      text="start";
      myCol=myDOL->getColumn(i);
      text=myCol->getId();
      unit=myCol->getUnits();
      sprintf(name, "TNULL%d", i+1); mot=name;
      try { header.getKeyword(mot, form); }
      catch (const TipException &x) {}
      colNull.at(i)=atof(form.c_str());
      sprintf(name, "TFORM%d", i+1);
      mot=name; /* convert C string to C++ string */
      header.getKeyword(mot, form);
      if (m_loadQuantity[i]) {
        if ((text!=itQ->m_name)||(unit!=itQ->m_unit)||(form!=itQ->m_format)) {
          text="";
          throw std::runtime_error("fileDiff");
        }
        if (itQ->m_type == Quantity::STRING) err++;
        itQ++;
      }
    }/* loop on columns */
  }
  catch (...) {
    if ( text.empty() ) {
      sortie << ": fits EXTENSION, column#";
      sortie << i+1 << " differ from previous import call";
      printErr(origin, sortie.str() );
      err=BAD_FILETYPE;
    }
    else {
      sortie << ": fits EXTENSION, error reading TABLE column#";
      sortie << i+1 << " description";
      printErr(origin, sortie.str() );
      err=BAD_FITS;
    }
    delete myDOL;
    return err;
  }
//printf("%ld OriRows\n", m_numOriRows);
  m_numRows=0;
  if ( !*maxRows ) *maxRows=m_numOriRows;
  if ( !m_numOriRows ) return IS_OK;

  create_tables(err, *maxRows);
  try {
    double rowVal;
    // max=m_quantities.size();
    for (Table::ConstIterator itor=myDOL->begin(); itor != myDOL->end();
         ++itor, ++m_numRows) {
      err=0;
      for (itQ=m_quantities.begin(); itQ != m_quantities.end(); ++itQ, ++err) {
        i=itQ->m_index;
        if (itQ->m_type == Quantity::NUM) {
          rowVal=(*itor)[itQ->m_name].get();
          if (itQ->m_null == "") m_numericals[i].at(m_numRows)=rowVal;
          else {
            if (rowVal == colNull[err]) m_numericals[i].at(m_numRows)=MissNAN;
            else m_numericals[i].at(m_numRows)=rowVal;
          }
        }
        else (*itor)[itQ->m_name].get(m_strings[i].at(m_numRows) );
      }
    }/* loop on rows*/
  }
  catch (const TipException &x) {
    sortie << ": fits EXTENSION, cannot read after row#" << m_numRows+1;
    printErr(origin, sortie.str() );
    return BAD_ROW;
  }
  delete myDOL;
  return IS_OK;
}
/**********************************************************************/
/* PRIVATE METHOD loadSelected is in file "catalog_ioText.cxx"
/**********************************************************************/
// if a catalog description was already loaded, this method does
// the same as import() adding selection criteria for loading
int Catalog::importSelected() {

  const std::string origin="importSelected";
  int quantSize=checkImport(origin, true);
  if (quantSize < IS_VOID) return quantSize;

  if (m_numRows > 0) {
    printWarn(origin, "call 'deleteContent' before 'importSelected'");
    return IMPORT_BIS;
  }
  int i, err=0,
      maxSize=m_loadQuantity.size();
  for (i=0; i<maxSize; i++) if (m_loadQuantity[i]) err++;
  if (err < 2) {
    printErr(origin, "at least 2 quantities must be selected for import");
    return BAD_SEL_QUANT;
  }
  std::ostringstream sortie; 
  sortie << err << " quantities (over " << maxSize << ") selected for import";
  printLog(1, sortie.str());
  sortie.str(""); // Will empty the string
  if (err != quantSize) {
    // in fact, due to select*Quantities() restrictions,
    // quantSize can only be greater than err
    printLog(2, "Erasing unwanted quantities for importSelected()");
    deleteQuantities();
  }
  err=IS_VOID;
  std::string text;
  long maxRows=0l;
  unsigned int pos=m_filename.find(0x0A);
  if (pos == std::string::npos) {

    if ( m_filename.empty() ) {
      // data must be querried via web


    }
    else {
      if (!m_filePos) {
        printErr(origin, "file import was not succesful");
        return IMPORT_NEED;
      }
      std::fstream myFile (m_filename.c_str(), std::ios::in);
      // file can be opened ?
      if ( !myFile.is_open() ) {
        text=": FILENAME \""+m_filename+"\" cannot be opened";
        printErr(origin, text);
        err=BAD_FILENAME;
        m_numRows=err;
        return err;
      }
      // go back to initial position
      myFile.clear(); // needed to reset flags before seekg
      myFile.seekg(m_filePos);
      unsigned long tot=0ul; // number of data lines read
      err=loadSelected(&tot, &myFile, &maxRows);
      if (err == IS_OK) {
        sortie << tot << " data lines read for importSelected()";
        printLog(0, sortie.str());
      }
      myFile.close();
    }

  }
  else { //fits file

    text=m_filename;
    std::string fileName=text.substr(0, pos);
    text.erase(0, pos+1);
    const Extension *myEXT = 0;
    // cannot use readTable because FITS IMAGE returns also error 1
    try {
      myEXT=IFileSvc::instance().readExtension(fileName, text);
    }
    catch (const TipException &x) {
      err=x.code();
      if (err == 1) {
        // This non-cfitsio error number means the file does not exist
        // or is not a table, or is not either Root nor Fits format. 
        sortie << ": FILENAME \"" << fileName;
        sortie << "\" cannot be opened or is NOT fits";
        err=BAD_FILENAME;
      }
      else {
        // Other errors come from cfitsio, but apply to a file
        // which is in FITS format but has some sort of format error.
        sortie << ": FILENAME is FITS, but cfitsio returned error=" << err;
        err=BAD_FITS;
      }
    }
    if (err >= IS_VOID) {
      if ( !myEXT->isTable() ) {
        sortie << ": FILENAME is FITS, but NOT a TABLE";
        err=BAD_FITS;
      }
    }
    delete myEXT;
    if (err < IS_VOID) {
      printErr(origin, sortie.str());
      m_numRows=err;
      return err;
    }
    err=loadSelectFits(fileName, text, &maxRows);

  }
  printWarn(origin, "function only implemented for column selection");
  if (err < IS_VOID) {
    deleteContent(); // to erase data already loaded in memory
    m_numRows=err;
    return err;
  }
  try {
    if (m_numRows < maxRows) {
      int i;
      err=m_strings.size();printf("ERASING\n");
      // erase unused memory  
      for (i=0; i<err; i++) m_strings[i].resize(m_numRows);
      err=m_numericals.size();
      for (i=0; i<err; i++) m_numericals[i].resize(m_numRows);
    }
    err=m_quantities.size()+2;
    // number of required bits including global and region
    err=1+(err-1)/(sizeof(long)*8);
    m_rowIsSelected.resize(err);
    #ifdef DEBUG_CAT
    std::cout << "Number of unsigned long required for m_rowIsSelected = "
              << err << std::endl;
    #endif
    if (m_numRows)
      for (int j=0; j<err; j++) m_rowIsSelected[j].assign(m_numRows, 0);
    // above lines can be commented to test the try catch mechanism
  }
  catch (const std::exception &prob) {
    std::string text;
    text=std::string("EXCEPTION filling m_rowIsSelected: ")+prob.what();
    printErr("import", text);
    throw;
  }
  return m_numRows;
}


/**********************************************************************/
// create catalog header from memory to a text file
int Catalog::createText(const std::string &fileName, bool clobber,
                        const std::string origin) {
  std::string  text;
  std::fstream file;
  int err;

  err=checkImport(origin, true);
  if (err < IS_VOID) return err;

  err=fileName.length();
  if (err == 0) {
    text=": FILENAME is EMPTY";
    printErr(origin, text);
    err=BAD_FILENAME;
    return err;
  }

  // overwrite existing file ?
  if (!clobber) {
    file.open(fileName.c_str(), std::ios::in);
    if (file) {
      file.close();
      text=": FILENAME \""+fileName+"\" exist (clobber=no)";
      printErr(origin, text);
      return BAD_FILENAME;
    }
    file.clear();  // clears all flags associated with the current stream
  }
  // open in write mode, if the file already existed it is erased
  file.open(fileName.c_str(), std::ios::out | std::ios::trunc);
  if (!file) {
    text=": FILENAME \""+fileName+"\" cannot be written";
    printErr(origin, text);
    return BAD_FILENAME;
  }

  char tab=0x09, sep=';';
  int j, vecSize;
  long tot=0l, totData=0l;
  bool saveAll=(origin == "saveText");
  if (m_numRows == m_numSelRows) saveAll=true;

  file << "#RESOURCE=catalogAccess(" << m_code << ")" << std::endl;
  file << "#Name: " << m_catName << std::endl;
  file << "#Title:" << tab <<  m_catRef << std::endl;
  file << "#Name: " << m_tableName << std::endl;
  file << "#Title:" << tab <<  m_tableRef << std::endl;
  tot+=5;
  vecSize=m_quantities.size();
  for (j=0; j<vecSize; j++) {
    file << "#Column" << tab << m_quantities[j].m_name << tab << "("
         << m_quantities[j].m_format << ")" << tab;
    if (m_quantities[j].m_name.length() < 8) file << "        ";
    file << m_quantities[j].m_comment << tab << "[ucd="
         << m_quantities[j].m_ucd << "]"
         << std::endl;
  }
  file << std::endl;
  tot+=vecSize+1;
  // line do NOT end with separator
  for (j=0; j<vecSize-1; j++) {
    file << m_quantities[j].m_name << sep;
  }
  file << m_quantities[j].m_name << std::endl;
  for (j=0; j<vecSize-1; j++) {
    file << m_quantities[j].m_unit << sep;
  }
  file << m_quantities[j].m_unit << std::endl;
  file << "---" << std::endl;
  tot+=3;
  try {
    int i, len, bufSize=0;
    char first, *buffer=NULL;
    std::vector<std::string> formats(vecSize, "%s");
    std::vector<int>         lengths(vecSize, 0);
    std::ostringstream sortie;
    double r;
    for (i=0; i<vecSize; i++) { 
      text=m_quantities[i].m_format;
      j=text.length();
      if (j == 0) continue;
      first=text[0];
      text.erase(0, 1);
      j=std::atoi(text.c_str());
      if (j <= 0) continue;
      lengths[i]=j;
      if (j > bufSize) {
        bufSize=j;
        buffer=(char *)realloc(buffer, bufSize*sizeof(char));
        if (buffer == NULL)
          throw std::runtime_error("Cannot allocate string buffer");
      }
      switch (first) {
      case 'A':
        sortie << "%" << j << "s";
        formats[i]=sortie.str();
        break;
      case 'I':
        sortie << "%" << j << ".0f";
        formats[i]=sortie.str();
        break;
      case 'F':  // number of decimals is needed from CSV/TSV
        if (i == m_indexRA)
          sortie << "%0" << text << "f";
        else if (i == m_indexDEC)
          sortie << "%+0" << text << "f";
        else sortie << "%" << text << "f";
        formats[i]=sortie.str();
        break;
      default :  // number of decimals is needed from CSV/TSV
        sortie << "%" << text << "e";
        formats[i]=sortie.str();
        break;
      }
      sortie.str(""); // Will empty the string
    }
    // all the quantities have their sprintf format
    // IF their lengths[] is positive
    if (saveAll) {

      for (long k=0; k<m_numRows; k++) for (j=0; j<vecSize; ) {
        if (m_quantities[j].m_type == Quantity::NUM) {
          r=m_numericals[m_quantities[j].m_index].at(k);
          if (isnan(r)) {
            len=lengths[j];
            if (len == 0) len=1;
            file << std::setw(len+1) << std::setfill(' ');
          }
          else if (lengths[j] > 0) {
            sprintf(buffer, formats[j].c_str(), r);
            file.write(buffer, lengths[j]);
          }
          else file << r;
        }
        else if (m_quantities[j].m_type == Quantity::STRING) {
          text=m_strings[m_quantities[j].m_index].at(k);
          if (lengths[j] > 0) {
            sprintf(buffer, formats[j].c_str(), text.c_str());
            file.write(buffer, lengths[j]);
          }
          else file << text;
        }
        if (++j == vecSize) file << std::endl; else file << sep;
      } // loop on rows and quantities

    }
    else for (long k=0; k<m_numRows; k++) if (m_rowIsSelected[0].at(k) & 1) {

      for (j=0; j<vecSize; ) {
        if (m_quantities[j].m_type == Quantity::NUM) {
          r=m_numericals[m_quantities[j].m_index].at(k);
          if (isnan(r)) {
            len=lengths[j];
            if (len == 0) len=1;
            file << std::setw(len+1) << std::setfill(' ');
          }
          else if (lengths[j] > 0) {
            sprintf(buffer, formats[j].c_str(), r);
            file.write(buffer, lengths[j]);
          }
          else file << r;
        }
        else if (m_quantities[j].m_type == Quantity::STRING) {
          text=m_strings[m_quantities[j].m_index].at(k);
          if (lengths[j] > 0) {
            sprintf(buffer, formats[j].c_str(), text.c_str());
            file.write(buffer, lengths[j]);
          }
          else file << text;
        }
        if (++j == vecSize) file << std::endl; else file << sep;
      }
      totData++;
      if (totData == m_numSelRows) break;

    } // loop on rows and quantities, selected branch

    if (saveAll) tot+=m_numRows; else tot+=m_numSelRows;
    if (buffer != NULL) free(buffer);
  }
  catch (const std::exception &prob) {
    text=std::string("EXCEPTION writing rows in file: ")+prob.what();
    printErr(origin, text);
    throw;
  }
  file << std::endl;
  tot++;
  file.close();
  std::ostringstream sortie; 
  sortie << "output text file is closed ( " << tot << " lines written)";
  printLog(0, sortie.str());
  return IS_OK;  
}
/**********************************************************************/
// save whole catalog from memory to a text file
int Catalog::saveText(const std::string &fileName, bool clobber) {

  const std::string origin="saveText";
  int err;
  err=createText(fileName, clobber, origin);
  if (err < IS_VOID) return err;

  return IS_OK;
}
/**********************************************************************/
// save selected rows of catalog from memory to a text file
int Catalog::saveSelectedText(const std::string &fileName, bool clobber) {

  const std::string origin="saveSelectedText";
  int err;
  err=createText(fileName, clobber, origin);
  if (err < IS_VOID) return err;

  return IS_OK;
}


/**********************************************************************/
// create catalog header from memory to a FITS file
int Catalog::createFits(const std::string &fileName, const std::string &extName,
                        bool clobber, bool append, const std::string origin,
                        tip::Table **ptrTable) {

  char name[9]; /* 8 char maximum for header key */
  std::string text, newName;
  int  j, err;
  bool create=true;

  err=checkImport(origin, true);
  if (err < IS_VOID) return err;

  err=fileName.length();
  if (err == 0) {
    text=": FILENAME is EMPTY";
    printErr(origin, text);
    return BAD_FILENAME;
  }
  if (extName.length() > 68) {
    text=": EXTENSION name is TOO long (limited to 68 characters)";
    printErr(origin, text);
    return BAD_FILENAME;
  }
  // overwrite existing file ?
  if (!clobber) {
    std::fstream file;
    file.open(fileName.c_str(), std::ios::in);
    if (file) {
      file.close();
      create=false;
      if (!append) {
        text=": FILENAME \""+fileName+"\" exist (clobber=no, append=no)";
        printErr(origin, text);
        return BAD_FILENAME;
      }
      else {
        text="appending extension to existing FILENAME (method "+origin+")";
        printLog(1, text);
      }
    }
  }
  if ( extName.empty() ) {
    /* max length is 68, as keyword value starts/ends with ' in 11/80 */
    newName=m_tableName.substr(0,68);
    /* replace forbidden char. */
    err=newName.length();
    for (j=0; j < err; j++) {
      if (newName[j] == '/') newName[j]='_';
    }
    text="EXTENSION name set to '"+newName+"'";
    printWarn(origin, text); 
    err=IS_OK;
  }
  else newName=extName;

  try {
    if (create) IFileSvc::instance().createFile(fileName);
    IFileSvc::instance().appendTable(fileName, newName);
    (*ptrTable)=IFileSvc::instance().editTable(fileName, newName);
  }
  catch (const TipException &x) {
    text=": cannot append and edit fits EXTENSION";
    printErr(origin, text);
    return BAD_FITS;
  }
  Header &header=(*ptrTable)->getHeader();
  try {
    header.setKeyword(KeyCatal[0], m_catName);
    header.setKeyComment(KeyCatal[0], KeyCatal[1]);
    header.setKeyword(KeyTable[0], m_tableName);
    header.setKeyComment(KeyTable[0], KeyTable[1]);
    /* test 
    header.setKeyUnit(KeyTable[0], "deg");*/
  }
  catch (const TipException &x) {
    text="fits EXTENSION, cannot add CDS header keys";
    printWarn(origin, text);
  }
  try {
    err=m_quantities.size();
    for (j=0; j < err; j++) {
      const Quantity &readQ=m_quantities[j];
      if (readQ.m_type == Quantity::NUM) {
        if ( !readQ.m_null.empty() ) {
          sprintf(name, "TNULL%d", j+1);
          newName=name; /* convert C string to C++ string */
          text=readQ.m_format;
          // to be improved for integers
        }
        else text="1D";
      }
      else {
        text=readQ.m_format;
        if ( isalpha(readQ.m_format[0]) ) {
          text.erase(0,1);
          text=text+"A";
        }
      }
      (*ptrTable)->appendField(readQ.m_name, text);
      if ( !readQ.m_null.empty() ) {
        header.setKeyword(newName, atoi(readQ.m_null.c_str()) );
        header.setKeyComment(newName, "Undefined value of field");
      }
      text=readQ.m_unit;
      if ( !text.empty() ) {
        sprintf(name, "TUNIT%d", j+1);
        newName=name; /* convert C string to C++ string */
        header.setKeyword(newName, text);
        header.setKeyComment(newName, "physical unit of field");
      }
      sprintf(name, "%s%d", Key_UCD.c_str(), j+1);
      newName=name; /* convert C string to C++ string */
      header.setKeyword(newName, readQ.m_ucd);
      header.setKeyComment(newName, readQ.m_comment);
    }/* loop on quantities */
  }
  catch (const TipException &x) {
    text=": fits EXTENSION, cannot add column";
    printErr(origin, text);
    return BAD_FITS;
  }
/* NEED new tip methods
  const IColumn *myCol = 0;
  try {
    for (j=0; j < err; j++) {
      const Quantity &readQ=m_quantities[j];
      myCol=(*ptrTable)->getColumn(j);

    }
  }
  catch (const TipException &x) {
    text=": fits EXTENSION, cannot modify column description";
    printErr(origin, text);
    return BAD_FITS;
  }
*/
  return IS_OK;
}
/**********************************************************************/
// save whole catalog from memory to a FITS file
int Catalog::saveFits(const std::string &fileName, const std::string &extName,
                      bool clobber, bool append) {

  const std::string origin="saveFits";
  Table *myDOL=0;
  int err;
  err=createFits(fileName, extName, clobber, append, origin, &myDOL);
  if (err < IS_VOID) return err;

  std::ostringstream sortie;
  long tot=0l;
  int  i, j;

  if (m_numRows > 0) {
  try {
    myDOL->setNumRecords(m_numRows);
    Quantity readQ;
    double rowVal;
    err=m_quantities.size();
    // Loop over all records (rows) and set values
    for (Table::Iterator itor=myDOL->begin(); itor != myDOL->end(); ++itor) {
      // double variable to hold the value of all the numeric fields
      for (j=0; j < err; j++) {
        readQ=m_quantities[j];
        i=readQ.m_index;
        if (readQ.m_type == Quantity::NUM) {
          rowVal= m_numericals[i].at(tot);
          if ( readQ.m_null.empty() ) (*itor)[readQ.m_name].set(rowVal);
          else {
            if ( isnan(rowVal) ) (*itor)[readQ.m_name].set(readQ.m_null);
            else (*itor)[readQ.m_name].set(rowVal);
            // to be improved for integers
          }
        }
        else (*itor)[readQ.m_name].set( m_strings[i].at(tot) );
      }
      tot++;
    }/* loop on rows */
  }
  catch (const TipException &x) {
    sortie << ": fits EXTENSION, cannot write at row#" << tot;
    printErr(origin, sortie.str() );
    return BAD_ROW;
  }/* end of try */
  }/* at least 1 row */

  delete myDOL; myDOL=0;
  sortie << "output fits is closed ( " << tot << " rows written)";
  printLog(0, sortie.str());
  return IS_OK;
}
/**********************************************************************/
// save selected rows of catalog from memory to a FITS file
int Catalog::saveSelectedFits(const std::string &fileName,
                              const std::string &extName,
                              bool clobber, bool append) {

  const std::string origin="saveSelectedFits";
  Table *myDOL=0;
  int err;
  err=createFits(fileName, extName, clobber, append, origin, &myDOL);
  if (err < IS_VOID) return err;

  std::ostringstream sortie;
  long tot=0l;
  int  i, j;

  if (m_numSelRows > 0) {
  try {
    myDOL->setNumRecords(m_numSelRows);
    Quantity readQ;
    double rowVal;
    err=m_quantities.size();
    Table::Iterator itor=myDOL->begin();
    // Loop over all selected records (rows) and set values
    // first bit indicates global selection
    for (long k=0; k<m_numRows; k++) if (m_rowIsSelected[0].at(k) & 1) {
      for (j=0; j < err; j++) {
        readQ=m_quantities[j];
        i=readQ.m_index;
        if (readQ.m_type == Quantity::NUM) {
          rowVal= m_numericals[i].at(tot);
          if ( readQ.m_null.empty() ) (*itor)[readQ.m_name].set(rowVal);
          else {
            if ( isnan(rowVal) ) (*itor)[readQ.m_name].set(readQ.m_null);
            else (*itor)[readQ.m_name].set(rowVal);
            // to be improved for integers
          }
        }
        else (*itor)[readQ.m_name].set( m_strings[i].at(tot) );
      }
      tot++;
      itor++;
      if (tot == m_numSelRows) break;
    }
  }
  catch (const TipException &x) {
    sortie << ": fits EXTENSION, cannot write at row#" << tot;
    printErr(origin, sortie.str() );
    return BAD_ROW;
  }/* end of try */
  }/* at least 1 selected row */

  delete myDOL; myDOL=0;
  sortie << "output fits is closed ( " << tot << " rows written)";
  printLog(0, sortie.str());
  return IS_OK;
}

} // namespace catalogAccess
