/**
 * @file   catalog_io.cxx
 * @brief  Read/write (or import/export) routines for Catalog class.
 * The fits access methods are not coded. Only methods to read or write
 * ASCII file are coded. Methods to load catalog through web are placeholders.
 * Methods to read or write selected data are not coded.
 *
 * @author A. Sauvageon
 *
 * $Header $
 *
 */

#include "catalog.h"

namespace catalogAccess {

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
  for (int i=0; i<max; i++)  {

    if ((whichCat >= 0) && (whichCat < MAX_CAT)) {
      // if it is a known catalog, the generic are defined
      text=m_quantities.at(i).m_name;
      for (j=0; j<MAX_GEN; j++) {
        name=Catalog::s_CatalogGeneric[whichCat][j];
        if (name == "+") name=UCD_Added[j];
        if (text == name) {
          m_quantities.at(i).m_isGeneric=true;
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
      text=m_quantities.at(i).m_ucd;
    }

    else {
      // otherwise, take first matching UCD
      text=m_quantities.at(i).m_ucd;
      if (text != "") for (j=0; j<MAX_GEN; j++) { 
        if (text == UCD_List[j]) {
          m_quantities.at(i).m_isGeneric=true;
          if ((j == 1) || (j == 2)) {
            // check that epoch is J2000
            name=m_quantities.at(i).m_name;
            name.erase(0, name.length()-epoch.length());
            // format contains at least 1 char
            if ((name == epoch) && (m_quantities.at(i).m_format[0] == 'F')) {
              if (j == 1) m_indexRA=i;
              else        m_indexDEC=i;
            }
            else m_quantities.at(i).m_isGeneric=false;
          }
          break;
        }
      }// loop on generic
    }

    // flag error quantities, useless for already found generic
    if ((text == "ERROR") && (!m_quantities.at(i).m_isGeneric)) {
      // error column name is e_"associated column" except for position error
      text=m_quantities.at(i).m_name;
      if (text.find("e_") == 0) {
        text.erase(0, 2);
        #ifdef DEBUG_CAT
        std::cout << "ERROR column (" << text << ")" << std::endl;
        #endif
        for (j=0; j<max; j++) if (m_quantities.at(j).m_name == text) {
          m_quantities.at(i).m_statError=text;
          break;
        }
      }
      else if ((text == "PosErr") || (text == "ErrorRad")) {
        m_quantities.at(i).m_isGeneric=true;
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
  for (i=0; i<quantSize; i++) {
    if (m_quantities[i].m_type == Quantity::NUM) nD++;
    else if  (m_quantities[i].m_type == Quantity::STRING) {
      nS++;
      text=m_quantities[i].m_format;
      if (text.length() > 1) {
        text.erase(0, 1);
        j=std::atoi(text.c_str());
        if (j <= 0) continue;        
        nchar+=j;
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
void Catalog::create_tables(const int nbQuantAscii) {

  int vecSize;
  std::string errText;
  if (nbQuantAscii > 0) {
    try {
      m_strings.resize(nbQuantAscii);
    }
    catch (std::exception &err) {
      errText=std::string("EXCEPTION on m_strings: ")+err.what();
      printErr("private create_tables", errText);
      throw;
    }
  }
  vecSize=m_quantities.size()-nbQuantAscii;
  if (vecSize > 0) {
    try {
      m_numericals.resize(vecSize);
    }
    catch (std::exception &err) {
      errText=std::string("EXCEPTION on m_numericals: ")+err.what();
      printErr("private create_tables", errText);
      throw;
    }
  }
  if (m_numReadRows > 0) add_rows();

}
/**********************************************************************/
// creates a new row in m_strings, m_numericals (private method)
void Catalog::add_rows() {

  int i, vecSize;
  std::string errText;
//printf("!! %ld / %ld \n", m_numRows, m_numReadRows);
  vecSize=m_strings.size();
  if (vecSize > 0 ) {
    try {
      for (i=0; i<vecSize; i++) {
        m_strings[i].resize(m_numReadRows);
//        for (j=0; j<m_numReadRows; j++) m_strings[i].at(j).resize(20);
      }
    }
    catch (std::exception &err) {
      errText=std::string("EXCEPTION on m_strings: ")+err.what();
      printErr("private add_rows", errText);
      throw;
    }
  }
  vecSize=m_numericals.size();
  if (vecSize > 0 ) {
    try {
      for (i=0; i<vecSize; i++) m_numericals[i].resize(m_numReadRows);
    }
    catch (std::exception &err) {
      errText=std::string("EXCEPTION on m_numericals: ")+err.what();
      printErr("private add_rows", errText);
      throw;
    }
  } 

}

/**********************************************************************/
// loads Ascii input in m_quantities (private method)
// suppose that index really exists: 0 <= index < m_quantities.size()
void Catalog::translate_cell(std::string mot, const int index) {

  int  j, last;
  char form; 

  // remove trailing spaces
  last=mot.length();
/* useless Warning, happen many times for last column of 3EG objects (EGRET)
  if (last == 0) {
    std::ostringstream sortie;
    sortie << "one quantity has no character (row #" << m_numRows+1 << ")";
    printWarn("private translate_cell", sortie.str());
  }
  else {*/
  if (last) {
    do last--;
    while ( (last >= 0) && (mot.at(last) == ' ') );
  }
  last++;
  j=m_quantities[index].m_index;
  form=m_quantities[index].m_format.at(0);
  if (form == 'A') {
    m_strings[j].at(m_numRows)=mot.substr(0, last);
  }
  else {
    if (last) {
      m_numericals[j].at(m_numRows)=std::atof(mot.c_str());
    }
    else
      m_numericals[j].at(m_numRows)=0./0.;
  }

}

/**********************************************************************/
// read the catalog from fits file (only description if getDescr is true)
int Catalog::analyze_fits(const std::string &fileName, const bool getDescr,
                          const std::string origin) {

/*TO BE DONE   use tip to open ASCII fits */

  return IS_OK;
}


/**********************************************************************/
// read the catalog header from text file (private method)
int Catalog::analyze_head(unsigned long *tot, int *what, bool *testCR) {

  std::string text, mot;
  unsigned int pos;
  char line[MAX_LINE];
  int  i, last,
       found=0,
       err=IS_OK;

  // must use good instead eof, because eof is still false
  // if getline reads MAX_LINE char ==> infinite loop
  myFile.clear();  
  // to reset the state flags (checked by the previous load call)
  while ( myFile.good() ) {

    // extract line with delimiter \n which is discarded
    myFile.getline(line, MAX_LINE);
    text=line; /* convert C string to C++ string */
    if (*tot == 0) {
      pos=text.find(' ');
      if (pos != std::string::npos) {
        mot=text.substr(0, pos);
        // fits starts with "SIMPLE  ="
        if (mot == "SIMPLE") return BAD_FILENAME;
      }
    }
    (*tot)++;

    /* should find something like:
#RESOURCE=21230079
#Name: J/ApJS/123/79
#Title: Third EGRET catalog (3EG) (Hartman+, 1999)
#Table  J_ApJS_123_79_3eg:
#Name: J/ApJS/123/79/3eg
#Title: Third EGRET Source Catalog (table 4)
#blabla(Coosys	G:	galactic)
#Column
// Column must be followed by at least one separation line 

    OR (-meta.all for description)

#RESOURCE=J/ApJS/123/79/3eg
#INFO   =271Total number of tuples (rows) in the table
#INFO   =CGRO
#INFO   =Gamma-ray
#    Third EGRET catalog (3EG) (Hartman+, 1999)
#    Third EGRET Source Catalog (table 4)
#
*/
    switch (found) {
    case 0:
      pos=text.find("#RESOURCE=");
      if (pos == 0) {
        found++;
        last=text.length();
        // test if last char is CR from WINDOWS
        if (text[last-1] == 0x0D) {
          *testCR=true;
          last--;
          text.erase(last);
        }
        else *testCR=false;
        mot=text.substr(10);
        pos=mot.find('/');
        if (pos == std::string::npos) {
          // standard desciption  printf("|%s|\n", mot.c_str());
          *what=1;
        }
        else {
          // -meta.all OR -meta QUERY
          *what=2;
          pos=mot.rfind('/');
          m_catName=mot.substr(0,pos);
          m_tableName=mot;
        }
      }
      break;

    case 1: case 3:
      if (*what < 2) mot="#Name:";  else mot="#INFO"; 
      pos=text.find(mot);
      if (pos == 0) {
        found++;
        last=text.length();
        if (*testCR) text.erase(--last);
        // remove heading spaces
        for (i=mot.length(); i<last; i++) {
          if ((text[i] != ' ') && (text[i] != 0x09)) break;
        }
        if (i == last) mot="";
        else {
          mot=text.substr(i, last-i);
          // remove trailing spaces
          pos=mot.length();
          do pos--;
          while ((mot[pos] == ' ') || (mot[pos] == 0x09));
          mot.erase(pos+1);
        }
        if (*what < 2) {
          if (found > 2) m_tableName=mot; else m_catName=mot;
        }
        else if (found == 2) {
          if ((!m_numOriRows) && ((last=mot.length()) > 1)) {
            if (mot[0] == '=') mot[0]=' ';
            for (i=1; i<last; i++) { if (!isdigit(mot[i])) break; } 
            mot.erase(i);
            m_numOriRows=atol(mot.c_str());
          }
        }
      }
      break;

    case 2: case 4:
      if (*what < 2) mot="#Title:";  else mot="#INFO";
      pos=text.find(mot);
      if (pos == 0) {
        if (*what >= 2) break; // read lines until do not start with #INFO
        found++;
        last=text.length();
        if (*testCR) text.erase(--last);
        // remove heading spaces
        for (i=mot.length(); i<last; i++) {
          if ((text[i] != ' ') && (text[i] != 0x09)) break;
        }
        if (i == last) mot="";
        else {
          mot=text.substr(i, last-i);
          // remove trailing spaces
          pos=mot.length();
          do pos--;
          while ((mot[pos] == ' ') || (mot[pos] == 0x09));
          mot.erase(pos+1);
        }
        if (found > 3) m_tableRef=mot; else m_catRef=mot;
      }
      else if (*what >= 2) {
        // separation '#' found,
        if ((last=text.length()) < 3) { found=5; break;}
        found++;
        if (*testCR) text.erase(--last);
        // remove heading spaces
        for (i=1; i<last; i++) {
          if ((text[i] != ' ') && (text[i] != 0x09)) break;
        }
        if (i == last) mot="";
        else {
          mot=text.substr(i, last-i);
          // remove trailing spaces
          pos=mot.length();
          do pos--;
          while ((mot[pos] == ' ') || (mot[pos] == 0x09));
          mot.erase(pos+1);
        }
        if (found > 3)  m_tableRef=mot; 
        else { found=4; m_catRef=mot; }
      }
      break;

    default:
      // case only for META file, read until #RESOURCE= or # [ucd=]
      if ((last=text.length()) > 7) {
        if (*testCR) text.erase(--last);
        // for META file with several tables, start of 2nd table
        mot="#RESOURCE=";
        if (text.find(mot) == 0) { found++; break; }
        // otherwise, remove heading spaces
        for (i=1; i<last; i++) {
          if ((text[i] != ' ') && (text[i] != 0x09)) break;
        }
        mot=text.substr(i, last-i);
        // before #Column, metaALL must end with following string
        if (mot == "[ucd=]") found++;
      }
      break;
    }
    #ifdef DEBUG_CAT
    std::cout << *tot <<",";
    if (*tot < 60ul) std::cout << line <<"|\n";
    #endif
    err=-1*found;
    if (*what < 2) {
      if (found == 5) { err=IS_OK; break; }
    }
    else if (found > 5) { err=IS_OK; break; }

  }// loop on inFile lines
  #ifdef DEBUG_CAT
  std::cout << std::endl;
  #endif

  return err;
}

/**********************************************************************/
// read catalog data from text file (private method);
// read only column/quantity description if getDescr is true
int Catalog::analyze_body(unsigned long *tot, int *what, const bool testCR,
                          const bool getDescr) {

  std::string origin, text, mot;
  unsigned int pos;
  char sep=';',
       line[MAX_LINE];
  std::ostringstream sortie;
  bool foundColumn=false,
       lineSkipped=false;
  int  i, last, err=IS_OK,
       found=0,
       nbQuantAscii=0;
  int (*pfunc)(int)=toupper; // function used by transform

  if (getDescr) origin="importDescription"; else origin="import";
  while ( myFile.good() ) {

    // extract line with delimiter \n which is discarded
    myFile.getline(line, MAX_LINE);
    text=line; /* convert C string to C++ string */
    (*tot)++;
    switch (found) {
    case 0:
      mot="#Column";
      pos=text.find(mot);
      if (pos == 0) {
        Quantity readQ;
        foundColumn=true;
      do {
        // column NAME, FORMAT, DESCR, UCD separated by only 1 TAB
        last=text.length();
        for (i=mot.length(); i<last; i++) {
          if ((text[i] != ' ') && (text[i] != 0x09)) break;
        }
        if (i == last) break; //no NAME found
        mot=text.substr(i, last-i);
        pos=mot.find(0x09);
        if (pos == std::string::npos) break;  // lack end of data

        readQ.m_name=mot.substr(0, pos);
        text=mot;
        text.erase(0, pos+1);
        pos=text.find(0x09);
        if ((pos == std::string::npos) || (pos < 3)) break;

        readQ.m_format=text.substr(1, pos-2); // disregard ( ) 
        text.erase(0, pos+1);
        last=text.length();
        for (i=0; i<last; i++) {
          if ((text[i] != ' ') && (text[i] != 0x09)) break;
        }
        if (i == last) break; // no DESCR found
        mot=text.substr(i, last-i);
        pos=mot.find(0x09);
        if (pos == std::string::npos) break;  // lack end of data
 
        text=mot;
        text.erase(0, pos+1);
        // remove trailing blanks
        do pos--; while (mot[pos] == ' ');
        readQ.m_comment=mot.substr(0, pos+1);
        mot="[ucd=";
        pos=text.find(mot);
        if (pos == std::string::npos) break;  // lack end of data
        text.erase(0, mot.length());
        pos=text.find(']');
        if (pos == std::string::npos) break;  // lack last ]

        mot=text.substr(0, pos);
        transform(mot.begin(), mot.end(), mot.begin(), pfunc);
        readQ.m_ucd=mot;
        if (readQ.m_format[0] == 'A') {
          readQ.m_index=nbQuantAscii;
          nbQuantAscii++;
          readQ.m_type=Quantity::STRING;
        }
        else {
          readQ.m_index=m_quantities.size()-nbQuantAscii;
          readQ.m_type=Quantity::NUM;
        }
        try { m_quantities.push_back(readQ); }
        catch (std::exception &prob) {
          text=std::string("EXCEPTION filling m_quantities: ")+prob.what();
          printErr(origin, text);
          throw;
        }
      }while(0);
        if (readQ.m_type == Quantity::VECTOR) {
          sortie << "line #" << *tot << " wrong column description";
          printErr(origin, sortie.str());
          sortie.str(""); // Will empty the string.
          m_quantities.clear();
          found++; break; // to stop reading file
        }
      }
      else if (foundColumn) found++;
      // skip lines before #Column (foundColumn is false);
      // then: at least one line without any information
      break;

    case 1:
      // If lines are separated by  ;  ==> CSV (what unchanged)
      // If lines are separated by TAB ==> TSV (what * -1)
      // Check that first word match first quantity,
      // if not: considered as separation line and loop on case 1.
      pos=text.find(sep);
      if (pos != std::string::npos) {
        mot=text.substr(0, pos);
        if (mot == m_quantities.at(0).m_name) found++;
      }
      else if ((pos=text.find(0x09)) != std::string::npos) {
        // suppose there is at least 2 columns
        mot=text.substr(0, pos);
        if (mot == m_quantities.at(0).m_name) {
          (*what)*=-1;
          found++;
          sep=0x09;
        }
      }
      break;

    case 2:
      // most of decription is read, units on separate line
      last=text.length();
      if (testCR) text.erase(--last);
      if (last > 0) {
        found++;
        m_filePos=myFile.tellg();
        i=0;
        last=text.find(sep);
        do {
          pos=last;
          mot=text.substr(0, pos);
          if (pos != std::string::npos) {
            text.erase(0, pos+1); // pos); to test exception below
            last=text.find(sep);
          }
          try { m_quantities.at(i).m_unit=mot; }
          catch (std::exception &prob) {
            if (i == m_quantities.size())
              text="more units than quantities, ignoring last unit(s)";
            else
              text=std::string("EXCEPTION setting m_unit in m_quantities ")
                   +prob.what();
            printWarn(origin, text);
            break;
          }
          i++;
        }
        while (pos != std::string::npos);
        if (i < m_quantities.size())
          printWarn(origin, "less units than quantities !");
      }
      break;

    case 3:
      // decription is read, separation line starting with ---
      last=text.length();
      if (testCR) text.erase(--last);
      if ((last > 0) && (text[0] == '-')) {
        found++;
        m_numRows=0;
        m_filePos=myFile.tellg();
        if (getDescr) break; // must NOT read all file and create tables
        if ( !m_numReadRows ) {
          // read the total number of rows to have a maximal value
          // to allocate m_strings, m_numericals buffers.
          while ( myFile.good() ) {
            myFile.getline(line, MAX_LINE);
            if (strlen(line) == (size_t)testCR) break;
            m_numReadRows++;
          }
          // go back to initial position printf("%ld ReadRows\n", m_numReadRows);
          myFile.clear(); // needed to reset flags before seekg
          myFile.seekg(m_filePos);
        }
        create_tables(nbQuantAscii);
      }
      break;

    default:
      last=text.length();
      if ((last == 0) || (text[0] == 0x0D)) {
        lineSkipped=true;
        break;
      }
      // string max size is MAX_LINE-1;
      if (last >= MAX_LINE-1) {
        sortie << "line #" << *tot << " exceeds maximal size ("
               << MAX_LINE << ")";
        printErr(origin, sortie.str());
        sortie.str(""); // Will empty the string.
        err=BAD_FILELINE;
        break;
      }
      if (testCR) text.erase(--last);
      mot="#Table";
      pos=text.find(mot);
      if (pos == 0) {
        sortie << "line #" << *tot << ": second table start (not read)";
        printWarn(origin, sortie.str());
        sortie.str(""); // Will empty the string.
        err=BAD_ROW;
        break;
      }
      pos=text.find(sep);
      if (pos == std::string::npos) {
        sortie << "line #" << *tot << " without separator, line skipped";
        printWarn(origin, sortie.str());
        sortie.str(""); // Will empty the string.
        break;
      }
      if ((m_numRows == m_numReadRows) || lineSkipped) {
        err=BAD_ROW;
        break;
      }
      i=0;
      err=m_quantities.size();
      do {
        mot=text.substr(0, pos);
        if (i < err) translate_cell(mot, i);
        else {
          sortie << "line #" << *tot << " contains too many quantities";
          printWarn(origin, sortie.str());
          sortie.str(""); // Will empty the string.
          break;
        }
        i++;
        if (pos != std::string::npos) {
          text.erase(0, pos+1);
          pos=text.find(sep);
        }
        else break;
      }
      while (1);
      if (i <  err) {
        sortie << "line #" << *tot << " does not contain all quantities";
        printWarn(origin, sortie.str());
        sortie.str(""); // Will empty the string.
      }
      err=IS_OK;
      m_numRows++;
      //found++;
      break;
    }
    // to have only 1 test when reading all file
    if (found < 4) {
      // when separation line after Column is found,
      // Quantities must be set.
      if (found == 1) {
        if (m_quantities.size() == 0) { found--; break; }
        if (*what >= 2) { found=4; break; }// found changed, no error on META ALL
      }
    }
    else if (getDescr) break;
    else if (err == BAD_ROW) break; // just stop, error not taken into account

  }// loop on file lines
  //only possible errors: BAD_FILELINE or BAD_ROW
  if (err == BAD_ROW) err=IS_OK;
  else if (err == BAD_FILELINE) return err;
  else if (found < 4) return (-1*found);
  
  return err;
}

/**********************************************************************/
// common code between import and importDescription (private method)
int Catalog::load(const std::string &fileName, const bool getDescr) {

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
  if (fileName.at(i-1) == ']') {
    // file should be a fits
    err=analyze_fits(fileName, getDescr, origin);
 
    text=": fits FILE not handled now";
    printErr(origin, text);
    err=BAD_FILETYPE;
    m_numRows=err;
    return err;
  }
  else {

    std::ostringstream sortie;
    unsigned long tot=0ul; // number of lines read
    bool testCR=false; // true if lines ends with CR (on windows)
    int what=0;        // 0 for unkwown, >0 for csv, <0 to tsv
                       // fabs()=1 for standard, =2 for meta QUERY
    if ( myFile.is_open() ) myFile.close();
    myFile.open(fileName.c_str(), std::ios::in);
    // file can be opened ?
    if ( !myFile.is_open() ) {
      text=": FILENAME \""+fileName+"\" cannot be opened";
      printErr(origin, text);
      err=BAD_FILENAME;
      m_numRows=err;
      return err;
    }
    DIR *testDir=opendir(fileName.c_str());
    if (testDir != NULL) {
      closedir(testDir);
      myFile.close();
      text=": FILENAME \""+fileName+"\" is a directory";
      printErr(origin, text);
      err=BAD_FILETYPE;
      m_numRows=err;
      return err;
    }
    err=analyze_head(&tot, &what, &testCR);
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
      err=BAD_FILETYPE;
    }
    else {

      //what is 1 or 2, then negate if TSV type
      if (what == 1) text="input text file";
      else text="input META file";

      // get columns description and units, data
      err=analyze_body(&tot, &what, testCR, getDescr);
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
    // myFile.close();  to be commented to let file opened
    sortie << text << ": " << tot << " lines read";
    printLog(0, sortie.str());

  }// file is read
  if (err < 0) {
    deleteContent(); // to erase data already loaded in memory
    m_numRows=err;
    m_numReadRows=0;
    // exit only if nothing is read
    if (err == BAD_FILENAME) return err;
  }
  // m_quantities vector maybe filled, MUST fill m_selEllipse, m_loadQuantity
  try {
    m_selEllipse.assign(7, 0.0);
    m_loadQuantity.assign(m_quantities.size(), true);
  }
  catch (std::exception &prob) {
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
int Catalog::importDescription(const std::string &fileName) {

  int err;
  err=checkImport("importDescription", false);
  // must be called before load() which set m_numRows to 0
  if (err < IS_VOID) return err;

  err=load(fileName, true);
  if (err < IS_OK) return err;

  return m_quantities.size();
}
/**********************************************************************/
// read from file an entire catalog without selection
int Catalog::import(const std::string &fileName, const long maxRow) {

  int err;
  err=checkImport("import", false);
  // after this check, m_numReadRows is 0
  if (err < IS_VOID) return err;

  if (maxRow <= 0) {
    printWarn("import", "trying to get whole catalog file");
    m_numReadRows=0;
  }
  else m_numReadRows=maxRow;

  err=load(fileName, false);
  if (err < IS_OK) return err;
  getRAMsize(m_numRows, true);
  try {
    if (m_numRows < m_numReadRows) {
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
  catch (std::exception &prob) {
    std::string text;
    text=std::string("EXCEPTION filling m_rowIsSelected: ")+prob.what();
    printErr("import", text);
    throw;
  }
  return m_numRows;
}


/**********************************************************************/
// common code between importWeb and importDescriptionWeb (private method)
int Catalog::loadWeb(const std::string catName, const std::string urlCode,
                     const std::string &fileName, const long maxRow) {

  std::string origin, text, web;
  int i, err;
  unsigned int pos;
  if (maxRow >= 0) origin="importWeb"; else origin="importDescriptionWeb";

  err=BAD_URL;
  m_numRows=err;
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
  if (iCat < 0) {
    m_numRows=iCat;
    return iCat;
  }
  err=checkImport(origin, false);
  // after this check, m_numOriRows is 0
  if (err < IS_VOID) return err;

  if (maxRow == 0) printWarn(origin, "trying to query whole catalog");
  m_numRows=0;
/* now, ASCII query must be created
  and sent, using CDS package to given URL
*/
  err=-9;
  text="Web query not implemented";
  printErr(origin, text);


  if (err < 0) {
    deleteContent(); // to erase data already loaded in memory
    m_numRows=err;
    return err;
  }
  try { m_selEllipse.assign(7, 0.0); }
  catch (std::exception &prob) {
    text=std::string("EXCEPTION on creating m_selEllipse: ")+prob.what();
    printErr(origin, text);
    throw;
  }
  m_code=catName;
  setGeneric(iCat);
  return IS_OK;
}
/**********************************************************************/
// load only the catalog description from CDS web site
int Catalog::importDescriptionWeb(const std::string catName,
                                  const std::string urlCode,
                                  const std::string &fileName) {

  int err;
  err=loadWeb(catName, urlCode, fileName, -1);
  if (err < IS_OK) return err;

  return m_quantities.size();
}
/**********************************************************************/
// load from CDS web site an entire catalog without selection
int Catalog::importWeb(const std::string catName,
                       const std::string urlCode, const long maxRow,
                       const std::string &fileName) {

  int err;
  long limitRow=maxRow;
  if (limitRow < 0) limitRow=0; // to avoid confusion with importDescriptionWeb
  err=loadWeb(catName, urlCode, fileName, limitRow);
  if (err < IS_OK) return err;
  getRAMsize(m_numRows, true);

  try {
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
  catch (std::exception &prob) {
    std::string text;
    text=std::string("EXCEPTION filling m_rowIsSelected: ")+prob.what();
    printErr("importWeb", text);
    throw;
  }
  return m_numRows;
}


/**********************************************************************/
int Catalog::loadSelected(unsigned long *tot) {

  const std::string origin="importSelected";
  char sep=';',
       line[MAX_LINE];
  bool testCR=false,
       first=true;
  int  i, last,
       err=IS_OK;
  unsigned int pos;
  std::ostringstream sortie;
  std::string text, mot;
  while ( myFile.good() ) {

    // extract line with delimiter \n which is discarded
    myFile.getline(line, MAX_LINE);
    (*tot)++;
    last=strlen(line);
    if ((last == 0) || (line[0] == 0x0D)) continue;
    // string max size is MAX_LINE-1;
    if (last >= MAX_LINE-1) {
      sortie << "data line #" << *tot << " exceeds maximal size ("
             << MAX_LINE << ")";
      printErr(origin, sortie.str());
      err=BAD_FILELINE;
      break;
    }
    if (first) {
      first=false;
      // test if last char is CR from WINDOWS
      if (line[last-1] == 0x0D) testCR=true;
      char *posC=strchr(line, sep);
      // if separator isn't the default ; search for TAB
      // suppose there is at least 2 columns
      if (posC == NULL) {
        posC=strchr(line, 0x09);
        if (posC != NULL) sep=0x09;
        else {
          printErr(origin, "first data line has no separator");
          err=BAD_FILETYPE;
          break;
        }
      }
/*   if (pos == std::string::npos) {
       if ((pos=text.find(0x09)) != std::string::npos) {
        // suppose there is at least 2 columns
          found++;
          sep=0x09;
        }*/
    }
    else if (testCR) line[--last]='\0';
    text=line; /* convert C string to C++ string */
    mot="#Table";
    pos=text.find(mot);
    if (pos == 0) {
      sortie << "data line #" << *tot << ": second table start (not read)";
      printWarn(origin, sortie.str());
      sortie.str(""); // Will empty the string.
      break;
    }
    pos=text.find(sep);
    if (pos == std::string::npos) {
      sortie << "data line #" << *tot << " without separator, line skipped";
      printWarn(origin, sortie.str());
      sortie.str(""); // Will empty the string.
      continue;
    }
    i=0;
/*    err=m_quantities.size();
    do {
      mot=text.substr(0, pos);
      if (i < err) translate_cell(mot, i);
      else {
        sortie << "data line #" << *tot << " contains too many quantities";
        printWarn(origin, sortie.str());
        sortie.str(""); // Will empty the string.
        break;
      }
      i++;
      if (pos != std::string::npos) {
        text.erase(0, pos+1);
        pos=text.find(sep);
      }
      else break;
    }
    while (1);
    if (i <  err) {
      sortie << "data line #" << *tot << " does not contain all quantities";
      printWarn(origin, sortie.str());
      sortie.str(""); // Will empty the string.
    }
    err=IS_OK;
    m_numRows++;*/
  }
  return err;
}
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
  int i, err=0;
  for (i=0; i<quantSize; i++) if (m_loadQuantity.at(i)) err++;
  if (err < 2) {
    printWarn(origin, "at least 2 quantities must be selected for import");
    return IS_VOID;
  }
  std::ostringstream sortie; 
  sortie << err << " quantities (over " << quantSize << ") selected for import";
  printLog(1, sortie.str());
  sortie.str(""); // Will empty the string

  err=IS_VOID; //IS_OK;
  if ( myFile.is_open() ) { // data must be read from file

    if (!m_filePos) {
      printErr(origin, "file import was not succesful");
      return IMPORT_NEED;
    }
    // go back to initial position
    myFile.clear(); // needed to reset flags before seekg
    myFile.seekg(m_filePos);
    unsigned long tot=0ul; // number of data lines read
    err=loadSelected(&tot);
    if (err == IS_OK) {
      sortie << tot << " data lines read for importSelected()";
      printLog(0, sortie.str());
      err=m_numRows;
    }

  }
  else { // data must be querried via web


  }
  printWarn(origin, "function not implemented");
  return err;
}


/**********************************************************************/
// save whole catalog from memory to a FITS file
int Catalog::save(const std::string fileName, bool no_replace) {

  const std::string origin="save";
  std::string text;
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
  if (fileName.at(err-1) == ']') {
    // file should be a fits
 
    text=": fits FILE not handled now";
    printErr(origin, text);
    err=BAD_FILETYPE;
    return err;
  }

  std::fstream file;
  // overwrite existing file ?
  if (no_replace) {
    file.open(fileName.c_str(), std::ios::in);
    if (file) {
      file.close();
      text=": FILENAME \""+fileName+"\" exist (no_replace option)";
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
  unsigned long tot=0ul;
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
    tot+=m_numRows;
    if (buffer != NULL) free(buffer);
  }
  catch (std::exception &prob) {
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
// save selected rows of catalog from memory to a FITS file



} // namespace catalogAccess
