/**
 * @file   catalog_ioText.cxx
 * @brief  Read/write (or import/export) routines for CDS text file.
 *
 * @author A. Sauvageon
 *
 * $Header $
 *
 */

#include "catalog.h"

namespace catalogAccess {

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
      m_numericals[j].at(m_numRows)=MissNAN;
  }

}

/**********************************************************************/
/* PRIVATE METHOD is only called by: load in "catalog_io.cxx"
   read the catalog header from CDS text file
*/
int Catalog::analyze_head(unsigned long *tot, int *what, bool *testCR,
                          std::fstream *myFile) {

  std::string text, mot;
  unsigned int pos;
  char line[MAX_LINE];
  int  i, last,
       found=0,
       err=IS_OK;

  // must use good instead eof, because eof is still false
  // if getline reads MAX_LINE char ==> infinite loop
  m_catName="";   m_catRef ="";
  m_tableName=""; m_tableRef ="";
  myFile->clear();  
  // to reset the state flags (checked by the previous load call)
  while ( myFile->good() ) {

    // extract line with delimiter \n which is discarded
    myFile->getline(line, MAX_LINE);
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
#blabla(CoosysG:galactic)
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
/* PRIVATE METHOD is called by: load in "catalog_io.cxx"
                                getMaxNumRows in "catalog.cxx"
   read catalog data from text file (read only column/quantity description
   if getDescr is true, otherwise read row data)
*/
int Catalog::analyze_body(unsigned long *tot, int *what, const bool testCR,
                     const bool getDescr, std::fstream *myFile, long *maxRows){

  std::string  origin, text, mot;
  unsigned int pos;
  char sep=';',
       line[MAX_LINE];
  std::ostringstream sortie;
  bool foundColumn=false,
       lineSkipped=false;
  int  i, last, err=IS_OK,
       found=0,
       nbQuantAscii=0,
       maxLine=MAX_LINE-1; // to avoid computation each line
  int (*pfunc)(int)=toupper; // function used by transform

  if (getDescr) origin="importDescription"; else origin="import";
  while ( myFile->good() ) {

    // extract line with delimiter \n which is discarded
    myFile->getline(line, MAX_LINE);
    (*tot)++;
    switch (found) {
    case 0:
      mot="#Column";
      text=line; /* convert C string to C++ string */
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
        catch (const std::exception &prob) {
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
      text=line; /* convert C string to C++ string */
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
      last=strlen(line);
      if (testCR) line[--last]='\0';
      if (last > 0) {
        found++;
        text=line; /* convert C string to C++ string */
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
          catch (const std::exception &prob) {
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
      last=strlen(line);
      if (testCR) line[--last]='\0';
      if ((last > 0) && (line[0] == '-')) {
        found++;
        m_numRows=0;
        m_filePos=myFile->tellg();
        if (getDescr) break; // must NOT read all file and create tables
        if ( !*maxRows ) {
          // read the total number of rows to have a maximal value
          // to allocate m_strings, m_numericals buffers.
          while ( myFile->good() ) {
            myFile->getline(line, MAX_LINE);
            if (strlen(line) < 2) break; // 1 CR for WINDOWS
            (*maxRows)++;
          }
          // go back to initial position printf("%ld ReadRows\n", *maxRows);
          myFile->clear(); // needed to reset flags before seekg
          myFile->seekg(m_filePos);
          m_numOriRows=*maxRows; // used by importSelected();
        }
        create_tables(nbQuantAscii, *maxRows);
      }
      break;

    default:
      last=strlen(line);
      if ((last == 0) || (line[0] == 0x0D)) {
        lineSkipped=true; // to have Warning messages below
        break;
      }
      // string max size is MAX_LINE-1;
      if (last >= maxLine) {
        sortie << "line #" << *tot << " exceeds maximal size ("
               << MAX_LINE << ")";
        printErr(origin, sortie.str());
        sortie.str(""); // Will empty the string.
        err=BAD_FILELINE;
        break;
      }
      i=strncmp(line, "#Table", 6);
      if (i == 0) {
        sortie << "line #" << *tot << ": second table start (not read)";
        printWarn(origin, sortie.str());
        sortie.str(""); // Will empty the string.
        err=BAD_ROW;
        break;
      }
      if (testCR) line[--last]='\0';
      text=line; /* convert C string to C++ string */
      pos=text.find(sep);
      if (pos == std::string::npos) {
        sortie << "line #" << *tot << " without separator, line skipped";
        printWarn(origin, sortie.str());
        sortie.str(""); // Will empty the string.
        break;
      }
      if ((m_numRows == *maxRows) || lineSkipped) {
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
        if (*what >= 2) { found=4; break; }
                       // found changed, no error on META ALL
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
/* PRIVATE METHOD is called by: importSelected() in "catalog_io.cxx"  */
int Catalog::loadSelected(unsigned long *tot, std::fstream *myFile,
                          long *maxRows) {

  const std::string origin="importSelected";
  int  i, last,
       maxLine=MAX_LINE-1, // to avoid computation each line
       err=0;
  char sep=';',
       line[MAX_LINE];

  if ( !m_numOriRows ) {
    // read the total number of rows to have a maximal value
    // to allocate m_strings, m_numericals buffers.
    while ( myFile->good() ) {
      myFile->getline(line, MAX_LINE);
      if (strlen(line) < 2) break; // 1 CR for WINDOWS
      m_numOriRows++;
    }
    // go back to initial position
    myFile->clear(); // needed to reset flags before seekg
    myFile->seekg(m_filePos);
  }
  last=m_quantities.size();
  for (i=0; i<last; i++)
    if (m_quantities[i].m_type == Quantity::STRING) err++;
//std::cout << m_numOriRows << " OriRows" << std::endl;
  m_numRows=0;
  if ( !*maxRows ) *maxRows=m_numOriRows;
  if ( !m_numOriRows ) return IS_OK;

  create_tables(err, *maxRows);
  bool testCR=false,
       first=true,
       lineSkipped=false;
  unsigned int pos;
  std::ostringstream sortie;
  std::string text, mot;

  err=IS_OK;
  while ( myFile->good() ) {

    // extract line with delimiter \n which is discarded
    myFile->getline(line, MAX_LINE);
    (*tot)++;
    last=strlen(line);
    if ((last == 0) || (line[0] == 0x0D)) {
      lineSkipped=true; // to have Warning messages below
      continue;
    }
    // string max size is MAX_LINE-1;
    if (last >= maxLine) {
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
    }
    else if (testCR) line[--last]='\0';
    i=strncmp(line, "#Table", 6);
    if (i == 0) {
      sortie << "data line #" << *tot << ": second table start (not read)";
      printWarn(origin, sortie.str());
      sortie.str(""); // Will empty the string.
      break;
    }
    text=line; /* convert C string to C++ string */
    pos=text.find(sep);
    if (pos == std::string::npos) {
      sortie << "data line #" << *tot << " without separator, line skipped";
      printWarn(origin, sortie.str());
      sortie.str(""); // Will empty the string.
      continue;
    }
    i=0;
    if (lineSkipped) break;
    err=m_loadQuantity.size();
    last=0;
    do {
      mot=text.substr(0, pos);
      if (i >= err) {
        sortie << "data line #" << *tot << " contains too many quantities";
        printWarn(origin, sortie.str());
        sortie.str(""); // Will empty the string.
        break;
      }
      else {
        if (m_loadQuantity[i]) translate_cell(mot, i-last);
        else last++;
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
    m_numRows++;
  }
  return err;
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
  m_URL=urlCode;
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
  catch (const std::exception &prob) {
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
  catch (const std::exception &prob) {
    std::string text;
    text=std::string("EXCEPTION filling m_rowIsSelected: ")+prob.what();
    printErr("importWeb", text);
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
      /* precision is 1/28 arcsec, */
/*
      if (fabs(m_selEllipseCentDEC_deg) <= Min_Axis) {
        /* the ellipse SAO region is on 2D point, must calculate on sphere */
        /* can only use it if DEC = 0 */
/*        filter="(circle("; //filter="(ellipse(";
        sprintf(value, "%9.5f", m_selEllipseCentRA_deg);
        filter+=value;;
        filter+=",0.0,";
        sprintf(value, "%8.5f", m_selEllipseMajAxis_deg+Min_Axis);
        filter+=value; filter+=',';
        sprintf(value, "%8.5f", m_selEllipseMinAxis_deg);
        filter+=value; filter+=',';
        sprintf(value, "%8.4f", m_selEllipseRot_deg);
        filter+=value; filter+=',';
        filter+=m_quantities[m_indexRA].m_name+',';
        filter+=m_quantities[m_indexDEC].m_name+')';
      }
*/
} // namespace catalogAccess
