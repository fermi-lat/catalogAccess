/**
 * @file   catalog.cxx
 * @brief  Basic routines for Catalog class.
 * The 3 static members should be read in configuration files in the future.
 * The methods give information on: these static members, the few members
 * that describe the catalog itself, the quantity data without selection.
 *
 * @author A. Sauvageon
 *
 * $Header $
 */

#include "catalog.h"

namespace catalogAccess {

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

// information on catalog size laste updated on: JUL 2004
const char *Catalog::s_CatalogList[2*MAX_CAT]={
      "EGRET3 sources", "J/ApJS/123/79/3eg",    //   271 rows (18 columns)
      "EGRET3 fluxes",  "J/ApJS/123/79/fluxes", //  5245 rows (10 columns)
      "EGRET3 periods", "J/ApJS/123/79/table1", //   169 rows ( 6 columns)
      "ROSAT 1RXS",     "IX/10A/1rxs",          // 18806 rows (28 columns)
      "Veron (11th) quasar", "VII/235/table1",  // 48921 rows (25 columns)
      "Veron (11th) BL Lac", "VII/235/table2",  //   876 rows (25 columns)
      "Veron (11th) AGN",    "VII/235/table3"   // 15069 rows (25 columns)
      };


const char *Catalog::s_CatalogGeneric[MAX_CAT][MAX_GEN]={
     {"3EG", "RAJ2000", "DEJ2000", "theta95", "GLON", "GLAT"},
     {"3EG", "", "", "", "", ""},
     {"",    "", "", "", "GLON", "GLAT"},
     {"1RXS", "RAJ2000", "DEJ2000", "PosErr", "+", "+"},
     {"Name", "+", "+", "", "+", "+"},   // original format of RA/DEC
     {"Name", "+", "+", "", "+", "+"},   // is in sexagesimal
     {"Name", "+", "+", "", "+", "+"}    // ==> need to be added in decimal
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
  m_URL="";
  m_catName="";
  m_catRef="";
  m_tableName="";
  m_tableRef="";
  m_quantities.clear();
  m_selection="";
  m_selRegion=false;
  m_selEllipse.clear();
}
/**********************************************************************/
// Copy constructor needed to allocate arrays in copy
Catalog::Catalog(const Catalog & myCat) {

  #ifdef DEBUG_CAT
  std::cout << "!! DEBUG Catalog COPY constructor on: "
            << myCat.m_tableName << std::endl;
  #endif  
  // copying values
  m_code=myCat.m_code;
  m_URL =myCat.m_URL;
  m_catName   =myCat.m_catName;
  m_catRef    =myCat.m_catRef;
  m_tableName =myCat.m_tableName;
  m_tableRef  =myCat.m_tableRef;
  m_numRows   =myCat.m_numRows;
  m_selection =myCat.m_selection;
  m_criteriaORed=myCat.m_criteriaORed;
  // following three data members needed for efficient selection
  m_numSelRows=myCat.m_numSelRows;
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
  std::string errText;
  // following data member m_selEllipse[] needed for efficient selection
  try {
    vecSize=myCat.m_selEllipse.size();
    for (j=0; j<vecSize; j++) m_selEllipse.push_back(myCat.m_selEllipse.at(j));
  }
  catch (std::exception &err) {
    errText=std::string("EXCEPTION on m_selEllipse[]: ")+err.what();
    printErr("Catalog copy constructor", errText);
    throw;
  }
  try {
    std::vector<Quantity>::const_iterator itQ;
    for (itQ=myCat.m_quantities.begin(); itQ!=myCat.m_quantities.end(); itQ++)
      m_quantities.push_back(*itQ);
  }
  catch (std::exception &err) {
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
    catch (std::exception &err) {
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
    catch (std::exception &err) {
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
    catch (std::exception &err) {
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
  catch (std::exception &prob) {
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
  catch (std::exception &prob) {
    text=std::string("EXCEPTION filling names: ")+prob.what();
    printErr("getWebList", text);
    throw;
  }
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
  catch (std::exception &err) {
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
  catch (std::exception &err) {
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
  catch (std::exception &err) {
    std::string errText;
    errText=std::string("EXCEPTION on ucds: ")+err.what();
    printErr(origin, errText);
    throw;
  }
  return quantSize;
}
/**********************************************************************/
// get only Quantity member m_type
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
  catch (std::exception &err) {
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
  if (statName == "") {;
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
                           double *realValStat) {

  *realValStat = -1.0;
  const std::string origin="getSysError";
  int num=checkSize_row(origin, row);
  if (num <= IS_VOID) return num;
  num=checkQuant_name(origin, name);
  if (num < 0) return num;
  std::string sysName=m_quantities.at(num).m_sysError;
  if (sysName == "") {
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
  *realValStat=m_numericals[num].at(row);
  return IS_OK;
}

/**********************************************************************/
// get the values of the statistical error of given vector in given row

/**********************************************************************/
// get the values of the systematic error of given vector in given row

/**********************************************************************/
// access to generic quantity for object name

/**********************************************************************/



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
  catch (std::exception &err) {
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

} // namespace catalogAccess
