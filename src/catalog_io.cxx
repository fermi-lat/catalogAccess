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
// BEWARE: the two constant below are UCD1 standard
//         need to be changed for UCD1+

// the order is important and must be compatible with s_CatalogGeneric
static const char *UCD_List[MAX_GEN]={
       "ID_MAIN", "POS_EQ_RA_MAIN", "POS_EQ_DEC_MAIN",
        "", "POS_GAL_LON", "POS_GAL_LAT"};
// "ERROR", "POS_GAL_LON", "POS_GAL_LAT"
// ERROR do not only refer to the position error
// ERROR is for any "Uncertainty in Measurements"

static const char *UCD_Added[MAX_GEN]={
       "", "_RAJ2000", "_DEJ2000", "", "_Glon", "_Glat"};

/**********************************************************************/
/*  METHODS for IMPORTING, SAVING, LOADING                            */
/**********************************************************************/
// search for generic quantities, private: suppose import done
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
          if (j == 1)      m_indexRA=m_quantities[i].m_index;
          else if (j == 2) m_indexDEC=m_quantities[i].m_index;
          break;
        }
      }// loop on generic
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
            if (name == epoch) {
              if (j == 1) m_indexRA=m_quantities[i].m_index;
              else        m_indexDEC=m_quantities[i].m_index;
            }
            //else m_quantities.at(i).m_isGeneric= keep it generic ?
          }
          break;
        }
      }// loop on generic
    }

  }// loop on quantities

}

/**********************************************************************/
// output the estimation of RAM needed per data row
void Catalog::showRAMsize() {

  std::string mot, text;
  int quantSize=m_quantities.size();
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
  long sizeD=nD*sizeof(double)*44000;
  long sizeS=nchar*sizeof(char)*44000;
  i=1+(quantSize+1)/(sizeof(long)*8);
  long sizeB=i*sizeof(long)*44000;
  sprintf(buffer, "%5.1f" , (sizeD+sizeS+sizeB)/(1024.*1024.));
  mot=buffer; /* convert C string to C++ string */
  text="Needed RAM space (Mo) for 44000 data rows = "+mot+"\n";
  sprintf(buffer, "%5.0f Ko for numericals (%3d double per row)",
                 sizeD/1024., nD);
  mot=buffer;
  text=text+mot+"\n";
  sprintf(buffer, "%5.0f Ko for %2d strings (%3d char per row)",
                 sizeS/1024., nS, nchar);
  mot=buffer;
  text=text+mot+"\n";
  sprintf(buffer, "%5.0f Ko for select bits (%2d long per row)",
                 sizeB/1024., i);
  mot=buffer;
  text=text+mot;
  printLog(1, text);
}

/**********************************************************************/
// creates a new column in m_strings, m_numericals
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

}
/**********************************************************************/
// creates a new row in m_strings, m_numericals
void Catalog::add_row() {

  int i, vecSize;
  std::string errText;
  m_numRows++;
  vecSize=m_strings.size();
  if (vecSize > 0 ) {
    try {
      for (i=0; i<vecSize; i++) m_strings[i].resize(m_numRows);
    }
    catch (std::exception &err) {
      errText=std::string("EXCEPTION on m_strings: ")+err.what();
      printErr("private add_row", errText);
      throw;
    }
  }
  vecSize=m_numericals.size();
  if (vecSize > 0 ) {
    try {
      for (i=0; i<vecSize; i++) m_numericals[i].resize(m_numRows);
    }
    catch (std::exception &err) {
      errText=std::string("EXCEPTION on m_numericals: ")+err.what();
      printErr("private add_row", errText);
      throw;
    }
  } 

}

/**********************************************************************/
// loads Ascii input in m_quantities, suppose that index really exists:
// 0 <= index < m_quantities.size()
void Catalog::translate_cell(std::string mot, const int index) {

  int i, last;
  std::string form;
  std::ostringstream sortie; 

  // remove trailing spaces
  i=mot.length();
  if (i == 0) {
    sortie << "one quantity has no character (row #" << m_numRows << ")";
    printWarn("private translate_cell", sortie.str());
  }
  else {
    do i--;
    while (mot[i] == ' ');
    mot.erase(i+1);
  }
  form=m_quantities[index].m_format;
  if (form[0] == 'A') {
    i=m_quantities[index].m_index;
    m_strings[i].at(m_numRows-1)=mot;
  }
  else {
    last=mot.length();
    for (i=0; i<last; i++) {
      if (mot[i] != ' ') break;
    }
    mot.erase(0, i);  
    i=m_quantities[index].m_index;
    if (mot == "")
      m_numericals[i].at(m_numRows-1)=0./0.;
      //nan(""); does not compile under Sun/Solaris
    else  m_numericals[i].at(m_numRows-1)=std::atof(mot.c_str());
  }

}

/**********************************************************************/
// read the catalog from fits file (only description if getAll is false)
int Catalog::analyze_fits(const std::string &fileName, const bool getAll,
                          const std::string origin) {

/*TO BE DONE   use tip to open ASCII fits */

  return IS_OK;
}
/**********************************************************************/
// read the catalog from text file (only description if getAll is false)
int Catalog::analyze_text(const std::string &fileName, const bool getAll,
                          const std::string origin) {

  std::string text, mot;
  unsigned int pos;

  // file can be opened ?
  std::ifstream file(fileName.c_str());
  //if ( !file.is_open() ) {
  if (!file) {
    text=": FILENAME \""+fileName+"\" cannot be opened";
    printErr(origin, text);
    return BAD_FILENAME;
  }
  DIR *testDir=opendir(fileName.c_str());
  pos=(testDir != NULL);
  if (pos) {
    closedir(testDir);
    file.close();
    text=": FILENAME \""+fileName+"\" is a directory";
    printErr(origin, text);
    return BAD_FILETYPE;
  }

  std::ostringstream sortie;
  int  i, last, err=0,
       nbQuantAscii=0,
       found=0,
       what=0; /* error if stays 0 ==> fits or unknown type of file */
  bool testCR=false;  // true if lines ends with CR (on windows)
  char line[MAX_LINE], sep=';';
  unsigned long tot=0ul;
  int (*pfunc)(int)=toupper; // function used by transform

  // must use good instead eof, because eof is still false
  // if getline reads MAX_LINE char ==> infinite loop
  while ( file.good() ) {

    // extract line with delimiter \n which is discarded
    file.getline(line, MAX_LINE);
    text=line; /* convert C string to C++ string */
    if (!tot) {
      pos=text.find(' ');
      if (pos != std::string::npos) {
        mot=text.substr(0, pos);
        // fits starts with "SIMPLE  ="
        if (mot == "SIMPLE") break;
      }
    }
    tot++;
    /* should find something like:
#RESOURCE=21230079
#Name: J/ApJS/123/79
#Title: Third EGRET catalog (3EG) (Hartman+, 1999)
#Table  J_ApJS_123_79_3eg:
#Name: J/ApJS/123/79/3eg
#Title: Third EGRET Source Catalog (table 4)
#Column
*/
// Column must follow Title without separation line
// Column must be followed by at least one separation line 
    switch (found) {
    case 0:
      pos=text.find("#RESOURCE");
      if (pos == 0) found++;
      break;

    case 1: case 3:
      mot="#Name:";
      pos=text.find(mot);
      if (pos == 0) {
        found++;
        last=text.length();
        if (text[last-1] == 0x0D) {
          testCR=true;
          last--;
          text.erase(last);
        }
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
        if (found > 2) m_tableName=mot; else m_catName=mot;
      }
      break;


    case 2: case 4:
      mot="#Title:";
      pos=text.find(mot);
      if (pos == 0) {
        found++;
        last=text.length();
        if (testCR) text.erase(--last);
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
      break;

    case 5:
      mot="#Column";
      pos=text.find(mot);
      if (pos == 0) {
        // column NAME, FORMAT, DESCR, UCD separated by only 1 TAB
        last=text.length();
        for (i=mot.length(); i<last; i++) {
          if ((text[i] != ' ') && (text[i] != 0x09)) break;
        }
        if (i == last) break; //no NAME found
        mot=text.substr(i, last-i);
        pos=mot.find(0x09);
        if (pos == std::string::npos) break;  // lack end of data

        Quantity readQ;
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
      }
      else found++;
      //at least one line without any information
      break;

    case 6:
      // If lines are separated by  ;  ==> CSV (what = 1)
      // If lines are separated by TAB ==> TSV (what = -1)
      // Check that first word match first quantity,
      // if not: considered as separation line and loop on case 6.
      pos=text.find(sep);
      if (pos != std::string::npos) {
        mot=text.substr(0, pos);
        if (mot == m_quantities.at(0).m_name) {
          what=1;
          found++;
        }
      }
      else if ((pos=text.find(0x09)) != std::string::npos) {
        // suppose there is at least 2 columns
        mot=text.substr(0, pos);
        if (mot == m_quantities.at(0).m_name) {
          what=-1;
          found++;
          sep=0x09;
        }
      }
      break;

    case 7:
      // most of decription is read, units follow Quantity names
      last=text.length();
      if (testCR) text.erase(--last);
      if (last > 0) {
        found++;
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
      }
      break;

    case 8:
      // decription is read, only used if getAll is true
      // separation line starting with ---
      last=text.length();
      if (testCR) text.erase(--last);
      if ((last > 0) && (text[0] == '-')) {
        found++;
        m_numRows=0;
        create_tables(nbQuantAscii);
      }
      break;

    default:
      last=text.length();
      if ((last == 0) || (text[0] == 0x0D)) break;
      // string max size is MAX_LINE-1;
      if (last >= MAX_LINE-1) {
        sortie << "line #" << tot << " exceeds maximal size ("
               << MAX_LINE << ")";
        printErr(origin, sortie.str());
        sortie.str(""); // Will empty the string.
        err=BAD_FILELINE;
        break;
      }
      if (testCR) text.erase(--last);
      mot="#Table";
      pos=text.find(mot);
      if (pos != std::string::npos) {
        sortie << "line #" << tot << ": second table start (not read)";
        printWarn(origin, sortie.str());
        sortie.str(""); // Will empty the string.
        err=BAD_ROW;
        break;
      }
      pos=text.find(sep);
      if (pos == std::string::npos) {
        sortie << "line #" << tot << " without separator, line skipped";
        printWarn(origin, sortie.str());
        sortie.str(""); // Will empty the string.
        break;
      }
      i=0;
      add_row();
      err=m_quantities.size();
      last=text.find(sep);
      do {
        pos=last;
        mot=text.substr(0, pos);
        if (pos != std::string::npos) {
          text.erase(0, pos+1);
          last=text.find(sep);
        }
        if (i >= err) {
          sortie << "line #" << tot << " contains too many quantities";
          printWarn(origin, sortie.str());
          sortie.str(""); // Will empty the string.
          break;
        }
        else translate_cell(mot, i);
        i++;
      }
      while (pos != std::string::npos);
      if (i <  err) {
        sortie << "line #" << tot << " does not contain all quantities";
        printWarn(origin, sortie.str());
        sortie.str(""); // Will empty the string.
      }
      err=0;
      found++;
      break;
    }
    // to have only 1 test when reading all file
    if (found < 9) {
      // when separation line after Column is found,
      // Quantities must be set.
      if ((found==6) && (m_quantities.size() == 0)) break;
      // all description is read, now read data (if required)
      if ((found==8) && (!getAll)) break;
    }
    else if (err == BAD_ROW) break; // just stop, error not taken into account
    #ifdef DEBUG_CAT
    std::cout << tot <<",";
    if (tot < 60ul) std::cout << line <<"|\n";
    #endif

  }// loop on file lines
  #ifdef DEBUG_CAT
  std::cout << std::endl;
  #endif
  file.close();
  if (!tot) {
    text=": FILENAME \""+fileName+"\" is fits without extension[] specified";
    printErr(origin, text);
    return BAD_FILENAME;
  }
  if (err == BAD_FILELINE) {
    // stopped before reading needed stuff (all or description)
    text=": FILENAME \""+fileName+"\" couldn't be read (line too long)";
    printErr(origin, text);
    return err;
  }
  if (what == 0) {
    sortie << ": FILENAME \"" << fileName
           << "\" is empty or has unknown type (stopped step " << found << ")";
    printErr(origin, sortie.str());
    return BAD_FILETYPE;
  }
  //what is -1 or 1
  sortie << "input text file is closed ( " << tot << " lines read)";
  printLog(0, sortie.str());
  if (what == 1) text="input text file is CSV type (; separator)";
  else text="input text file is TSV type (Tab=0x09 separator)";
  printLog(1, text);

  return IS_OK;
}


/**********************************************************************/
// common code between import and importDescription
int Catalog::load(const std::string &fileName, const bool getAll) {

  std::string origin, text;
  int err;
  if (getAll) origin="import"; else origin="importDescription";

  int i=checkImport(origin, false);
  if (i < IS_VOID) return i;

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
    err=analyze_fits(fileName, getAll, origin);
 
    text=": fits FILE not handled now";
    printErr(origin, text);
    err=BAD_FILETYPE;
    m_numRows=err;
    return err;
  }
  else err=analyze_text(fileName, getAll, origin);
  if (err < 0) {
    m_numRows=err;
    return err;
  }
  try { m_selEllipse.assign(7, 0.0); }
  catch (std::exception &prob) {
    text=std::string("EXCEPTION on creating m_selEllipse: ")+prob.what();
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
  showRAMsize();
  return IS_OK;
}
/**********************************************************************/
// read only the catalog description from file
int Catalog::importDescription(const std::string &fileName) {

  int err;
  err=load(fileName, false);
  if (err < IS_OK) return err;

  m_numRows=0;
  return m_quantities.size();
}
/**********************************************************************/
// read from file an entire catalog without selection
int Catalog::import(const std::string &fileName) {

  int err;
  err=load(fileName, true);
  if (err < IS_OK) return err;

  try {
    err=m_quantities.size()+2;
    // number of required bits including global and region
    err=1+(err-1)/(sizeof(long)*8);
    m_rowIsSelected.resize(err);
    #ifdef DEBUG_CAT
    std::cout << "Number of unsigned long required for m_rowIsSelected = "
              << err << std::endl;
    #endif
    for (int j=0; j<err; j++) m_rowIsSelected[j].assign(m_numRows, 0);
    // above line can be commented to test the try catch mechanism
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
// common code between importWeb and importDescriptionWeb
int Catalog::loadWeb(const std::string catName, const std::string urlCode,
                     const std::string &fileName, const long maxRow) {

  std::string origin, text, web;
  int err;
  if (maxRow >= 0) origin="importWeb"; else origin="importDescriptionWeb";

  int iCat=checkImport(origin, false);
  if (iCat < IS_VOID) return iCat;
  iCat=checkCatName(origin, catName);
  if (iCat < 0) { m_numRows=iCat; return iCat; }

  if (urlCode.length() == 0) {
    text=": CODE for URL (web http address) is empty";
    printErr(origin, text);
    err=BAD_URL;
    m_numRows=err;
    return err;
  }
  unsigned int pos;
  int i;
  for (i=0; i<MAX_URL; i++) {
    text=Catalog::s_CatalogURL[i]; /* convert C string to C++ string */
    pos=text.find(' ');
    if (pos == std::string::npos) continue;
    if (text.substr(0, pos) == urlCode) break;
  }
  if (i == MAX_URL) {
    text=": CODE for URL (web http address) do not exist";
    printErr(origin, text);
    err=BAD_URL;
    m_numRows=err;
    return err;
  }
  pos=text.rfind(' ');
  if (pos == std::string::npos) web=text;
  else web=text.substr(pos+1, text.length()-pos);

/* now, ASCII query must be created
  and sent, using CDS package to given URL
*/
  err=-9;

  if (err < 0) {
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
  showRAMsize();
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

  m_numRows=0;
  return m_quantities.size();
}
/**********************************************************************/
// load from CDS web site an entire catalog without selection
int Catalog::importWeb(const std::string catName,
                       const std::string urlCode, const long maxRow,
                       const std::string &fileName) {

  int err;
  long limitRow=maxRow;
  if (limitRow <= 0) {
    printWarn("importWeb", "trying to query whole catalog");
    limitRow=0;
  }
  err=loadWeb(catName, urlCode, fileName, limitRow);
  if (err < IS_OK) return err;

  try {
    err=m_quantities.size()+2;
    // number of required bits including global and region
    err=1+(err-1)/(sizeof(long)*8);
    m_rowIsSelected.resize(err);
    #ifdef DEBUG_CAT
    std::cout << "Number of unsigned long required for m_rowIsSelected = "
              << err << std::endl;
    #endif
    for (int j=0; j<err; j++) m_rowIsSelected[j].assign(m_numRows, 0);
    // above line can be commented to test the try catch mechanism
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


  return m_numRows;
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
  file.open(fileName.c_str(), std::ios::out | std::ios::trunc);
  if (!file) {
    text=": FILENAME \""+fileName+"\" cannot be written";
    printErr(origin, text);
    return BAD_FILENAME;
  }

  char sep=0x09;
  int i, j, vecSize;
  unsigned long tot=0ul;
  file << "#RESOURCE=catalogAccess(" << m_code << ")" << std::endl;
  file << "#Name: " << m_catName << std::endl;
  file << "#Title:" << sep <<  m_catRef << std::endl;
  file << "#Name: " << m_tableName << std::endl;
  file << "#Title:" << sep <<  m_tableRef << std::endl;
  tot+=5;
  vecSize=m_quantities.size();
  for (i=0; i<vecSize; i++) {
    file << "#Column" << sep << m_quantities[i].m_name << sep << "("
         << m_quantities[i].m_format << ")" << sep << "       "
         << m_quantities[i].m_comment << "     " << sep << "[ucd="
         << m_quantities[i].m_ucd << "]"
         << std::endl;
  }
  file << std::endl;
  tot+=vecSize+1;
  // line do NOT end with separator
  for (i=0; i<vecSize-1; i++) {
    file << m_quantities[i].m_name << sep;
  }
  file << m_quantities[i].m_name << std::endl;
  for (i=0; i<vecSize-1; i++) {
    file << m_quantities[i].m_unit << sep;
  }
  file << m_quantities[i].m_unit << std::endl;
  file << "---" << std::endl;
  tot+=3;
  try {
    int len, bufSize=0;
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
        if (m_quantities[i].m_index == m_indexRA)
          sortie << "%0" << text << "f";
        else if (m_quantities[i].m_index == m_indexDEC)
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
    for (j=0; j<m_numRows; j++) for (i=0; i<vecSize; ) {   

      if (m_quantities[i].m_type == Quantity::NUM) {
        r=m_numericals[m_quantities[i].m_index].at(j);
        if (isnan(r)) {
          len=lengths[i];
          if (len == 0) len=1;
          file << std::setw(len+1) << std::setfill(' ');
        }
        else if (lengths[i] > 0) {
          sprintf(buffer, formats[i].c_str(), r);
          file.write(buffer, lengths[i]);
        }
        else file << r;
      }
      else if (m_quantities[i].m_type == Quantity::STRING) {
        text=m_strings[m_quantities[i].m_index].at(j);
        if (lengths[i] > 0) {
          sprintf(buffer, formats[i].c_str(), text.c_str());
          file.write(buffer, lengths[i]);
        }
        else file << text;
      }
      if (++i == vecSize) file << std::endl; else file << sep;

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
