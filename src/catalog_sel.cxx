/**
 * @file   catalog_sel.cxx
 * @brief  Selection routines for Catalog class.
 * Provide methods to READ selected data or MAKE the selection.
 * Selection can be defined with no row, it will be used with
 * importSelected() method.
 *
 * @author A. Sauvageon
 *
 * $Header $
 */

#include "catalog.h"

namespace catalogAccess {

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
  }
  if (tot < m_numSelRows) return IS_OK;
  return IS_VOID; // should not happen
}

/**********************************************************************/
// get the values of given vector quantity in given selected row

/**********************************************************************/
// get value of the statistical error of given quantity in given selected row

/**********************************************************************/
// get value of the systematic error of given quantity in given selected row

/**********************************************************************/
// get the values of the statistical error of given vector in given selected row

/**********************************************************************/
// get the values of the systematic error of given vector in given selected row

/**********************************************************************/
// access to generic quantity for object name in given selected row

/**********************************************************************/



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
  catch (std::exception &err) {
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
/*  METHODS for SELECTING DATA (BEFORE or AFTER IMPORT)               */
/**********************************************************************/
// private method, suppose given row exist
bool Catalog::checkRegion(const long row, const int nRA, const int nDEC) {

  /* for the moment, only circle
    if angle phi=RA and t=PI/2 - DEC then:
    OM postion is x= (sin t <=> cos DEC) * cos phi
                  y= (sin t <=> cos DEC) * sin phi
                  z= (cos t <=> sin DEC)
   and the circular region around OA is defined by
   its scalar product with OM > cos desired_angle
  */
  double myRA=m_numericals[m_indexRA].at(row);
  double myDEC=m_numericals[m_indexDEC].at(row);
  if (isnan(myRA) && m_quantities[nRA].m_rejectNaN) return false;
  if (isnan(myDEC) && m_quantities[nDEC].m_rejectNaN) return false;
  double obj_cosP=cos(myRA * Angle_Conv);
  double obj_sinP=sin(myRA * Angle_Conv);
  double obj_sinT=cos(myDEC * Angle_Conv);
  double obj_cosT=sin(myDEC * Angle_Conv);
  double myAngle=obj_sinT*obj_cosP*m_selEllipse.at(2)*m_selEllipse.at(0)
                +obj_sinT*obj_sinP*m_selEllipse.at(2)*m_selEllipse.at(1)
                +obj_cosT*m_selEllipse.at(3);
/*std::cout << "cos(angle)*1E6 = " << myAngle*1E6 << " ? "
            <<  m_selEllipse.at(4)*1E6 << std::endl;*/
  if (myAngle >= m_selEllipse.at(4)) return true;
  return false;
}

/**********************************************************************/
// private method, suppose given quantity index exist
bool Catalog::checkNUM(const double r, const int index, const bool reject) {

  bool isNull=isnan(r);
  int i, vecSize=m_quantities[index].m_listValN.size();
  for (i=0; i<vecSize; i++) {
  }
  double myLimit=m_quantities[index].m_lowerCut;
  if (myLimit < NO_SEL_CUT) {
    if (isNull && reject) return false;
    else if (r < myLimit) return false;
  }

  myLimit=m_quantities[index].m_upperCut;
  if (myLimit < NO_SEL_CUT) {
    if (isNull && reject) return false;
    else if (r > myLimit) return false;
  } 
  return true;
}
/**********************************************************************/
// private method, suppose given quantity index exist
bool Catalog::checkSTR(const std::string s, const int index, const int code) {

  int i, vecSize=m_quantities[index].m_listValS.size();
  bool check=(code & 1);
  // true for equality, false otherwise
  for (i=0; i<vecSize; i++) {
    if (s == m_quantities[index].m_listValS[i]) return check;
  }
  return !check;
}

/**********************************************************************/
// private method, suppose given row exist and m_rowIsSelected set
bool Catalog::rowSelect(const long row, const std::vector<bool> &quantSel) {

  int  numBit=sizeof(long)*8,
       vecSize=quantSel.size();
  bool check=true;
  if (m_criteriaORed) check=false;
  unsigned long currSel, test=2ul; // bit0 reserved for check
  #ifdef DEBUG_CAT
  std::cout << m_criteriaORed << "quantSize = " << vecSize;
  #endif
  int j=0;
  for (int i=1; i<=vecSize; ) {

    currSel=m_rowIsSelected[j].at(row);
    if (quantSel[i-1]) {
      #ifdef DEBUG_CAT
      std::cout << " (AND with " << currSel <<" & "<< test
                << ", pos[" << j <<"] )" << std::endl;
      #endif
      if (m_criteriaORed) {
        // OR  of bits, true --> useless to continue
        if ((currSel & test) == test) {check=true; break;}
      }
      else {
        // AND of bits, false--> useless to continue
        if ((currSel & test) == 0ul) {check=false; break;}
      }
    }
    if ((++i % numBit) == 0) {
      test=1ul; // first bit
      j++;      // of next vector index.
    }
    else test*=2ul;

  }
  if (check) // setting bit0 to 1
    m_rowIsSelected[0].at(row)|= 1ul;
  else       // setting bit0 to 0
    m_rowIsSelected[0].at(row)&= (Max_Test-1ul);

  return check;
}


/**********************************************************************/
// set and apply an elliptical selection region
int Catalog::setSelEllipse(const double centRA_deg, const double centDEC_deg,
                           const double majAxis_deg, const double minAxis_deg,
                           const double rot_deg) {

  const std::string origin="setSelEllipse";
  std::string text;
  std::ostringstream sortie;

  // first check that selection is possible
  int quantSize=checkImport(origin, true);
  if (quantSize < IS_VOID) return quantSize;
  if ((m_indexRA < 0) || (m_indexDEC < 0)) {
    text="missing generic position quantities (RA and DEC)"; 
    printWarn(origin, text);
    return NO_RA_DEC;
  }
  int numPb=0;
  //const std::_Ios_Fmtflags outDouble=std::ios::right | std::ios::scientific;
  if ((centRA_deg < 0.) || (centRA_deg >= 360.)) numPb=BAD_RA;
  else if ((centDEC_deg < -90.) || (centDEC_deg > 90.)) numPb=BAD_DEC;
  else if ((rot_deg < 0.) || (rot_deg >= 180.)) numPb=BAD_ROT;
  if (numPb < 0) {
    text="bad ellipse position (impossible RA, DEC or rotation)"; 
    printWarn(origin, text);
    return numPb;
  }
  if ((majAxis_deg < Min_Axis) || (majAxis_deg > 90.)) numPb=BAD_AXIS;
  else if ((minAxis_deg < Min_Axis) || (minAxis_deg > 90.)) numPb=BAD_AXIS;
  // size limited to one hemisphere
  if (numPb < 0) {
    sortie << "bad ellipse size, radius from " << std::setprecision(2)
           << Min_Axis*1000 << "E-3 to 90 (in RA or DEC)";
    printWarn(origin, sortie.str());
    return numPb;
  }
  if (rot_deg > 0.)
    printWarn(origin, "whatever orientation, using 0"); 
  if (fabs(majAxis_deg/minAxis_deg - 1.) > 10*Min_Prec)
    printWarn(origin, "axis sizes differ, taking only major axis");
  m_selRegion=true;
  m_selEllipseCentRA_deg=centRA_deg;
  m_selEllipseCentDEC_deg=centDEC_deg;
  m_selEllipseMajAxis_deg=majAxis_deg;
  m_selEllipseMinAxis_deg=majAxis_deg; //minAxis_deg;
  m_selEllipseRot_deg=0.; //rot_deg;
  // angles are phi=RA and theta=PI/2-DEC
  m_selEllipse.at(0)=cos(m_selEllipseCentRA_deg * Angle_Conv);
  m_selEllipse.at(1)=sin(m_selEllipseCentRA_deg * Angle_Conv);
  m_selEllipse.at(2)=cos(m_selEllipseCentDEC_deg* Angle_Conv);
  m_selEllipse.at(3)=sin(m_selEllipseCentDEC_deg* Angle_Conv);
  m_selEllipse.at(4)=cos(majAxis_deg * Angle_Conv);

  sortie << "selection ellipse center RA=" << std::setprecision(4)
         << m_selEllipseCentRA_deg << " , DEC="
         << m_selEllipseCentDEC_deg << " with radius "
         << m_selEllipseMajAxis_deg << " * "
         << m_selEllipseMinAxis_deg << " (degrees) orientated at "
         << m_selEllipseRot_deg << " (with respect to North pole)";
  printLog(1, sortie.str());
  /* if no data: exit */
  if (m_numRows == 0) return IS_OK;

  // now, apply the selection ellipse
  std::vector<bool> isSelected;
  existCriteria(&isSelected); // returns true (m_selRegion is true)
  bool check;
  int j, nRA=0, nDEC=0; // should be useless to initialize 
  unsigned long currSel;
  for (j=0; j<quantSize; j++) {
    if (m_quantities[j].m_index == m_indexRA) nRA=j;
    else if (m_quantities[j].m_index == m_indexDEC) nDEC=j;
  }
  m_numSelRows=0;
  for (long i=0; i<m_numRows; i++) {

    check=checkRegion(i, nRA, nDEC);
    currSel=m_rowIsSelected[0].at(i);
    if (check) // setting 2nd bit to 1
      m_rowIsSelected[0].at(i)=currSel | 2ul;
    else       // setting 2nd bit to 0
      m_rowIsSelected[0].at(i)=currSel & (Max_Test-2ul);
    if (rowSelect(i, isSelected) == true) m_numSelRows++;

  }// loop on all rows
  return IS_OK;
}
/**********************************************************************/
// remove the effects of the ellipse selection
int Catalog::unsetSelEllipse() {

  int quantSize=checkImport("unsetSelEllipse", true);
  if (quantSize < IS_VOID) return quantSize;

  // if region was already unset: do nothing
  if (!m_selRegion) return IS_OK;
  m_selRegion=false;
  /* if no data: exit */
  if (m_numRows == 0) return IS_OK;

  // now, must check criteria on other quantities
  std::vector<bool> isSelected;
  bool oneCriteria=existCriteria(&isSelected);
  if (oneCriteria) {
    m_numSelRows=0;
    for (long i=0; i<m_numRows; i++) {

      // setting 2nd bit to 0
      m_rowIsSelected[0].at(i)&= (Max_Test-2ul);
      if (rowSelect(i, isSelected) == true) m_numSelRows++;
 
    }// loop on rows
  }
  else {
    if (m_numSelRows > 0) {
      quantSize=m_rowIsSelected.size();
      for (int j=0; j<quantSize; j++) m_rowIsSelected[j].assign(m_numRows, 0);
    }
    m_numSelRows=0;
    printLog(0, "All rows unselected");
  }
  return IS_OK;
}


/**********************************************************************/
// unset all cuts on all quantities except the selection ellipse
int Catalog::unsetCuts() {

  int quantSize=checkImport("unsetCuts", true);
  if (quantSize < IS_VOID) return quantSize;
  int j;
  /*for (j=0; j<quantSize; j++) unsetCuts(m_quantities.at(j).m_name);*/
  //quicker to just check for selection ellipse
  for (j=0; j<quantSize; j++) {
    m_quantities.at(j).m_listValS.clear();
    m_quantities.at(j).m_lowerCut=NO_SEL_CUT;
    m_quantities.at(j).m_upperCut=NO_SEL_CUT;
    m_quantities.at(j).m_listValN.clear();
  }
  /* if no data: exit */
  if (m_numRows == 0) return IS_OK;

  // now, must check criteria on region
  quantSize=m_rowIsSelected.size();
  if (m_selRegion) {
    unsigned long currSel;
    m_numSelRows=0;
    for (long i=0; i<m_numRows; i++) {

      currSel=m_rowIsSelected[0].at(i);
      if (currSel & 2ul) {
        m_rowIsSelected[0].at(i)=3ul;
        m_numSelRows++;
      }
      else m_rowIsSelected[0].at(i)=0ul;
      // setting all bits to 0 except first two
      for (j=0; j<quantSize; j++) m_rowIsSelected[j].at(i)=0ul;

    }// loop on rows
  }
  else {
    if (m_numSelRows > 0)
      for (j=0; j<quantSize; j++) m_rowIsSelected[j].assign(m_numRows, 0);
    m_numSelRows=0;
    printLog(0, "All rows unselected");
  }
  return IS_OK;
}
/**********************************************************************/
int Catalog::unsetCuts(const std::string name) {

  int quantSize=checkImport("unsetCuts", true);
  if (quantSize < IS_VOID) return quantSize;
  int index=checkQuant_name("unsetCuts", name);
  if (index < 0) return index;
  m_quantities.at(index).m_listValS.clear();
  m_quantities.at(index).m_lowerCut=NO_SEL_CUT;
  m_quantities.at(index).m_upperCut=NO_SEL_CUT;
  m_quantities.at(index).m_listValN.clear();
  /* if no data: exit */
  if (m_numRows == 0) return IS_OK;

  // now, must check criteria on region and other quantities
  std::vector<bool> isSelected;
  bool oneCriteria=existCriteria(&isSelected);
  if (oneCriteria) {
    int k;
    unsigned long test=bitPosition(index, &k);
    m_numSelRows=0;
    for (long i=0; i<m_numRows; i++) {

      // setting required bit to 0
      m_rowIsSelected[k].at(i)&= (Max_Test-test);
      if (rowSelect(i, isSelected) == true) m_numSelRows++;

    }// loop on rows
  }
  else {
    if (m_numSelRows > 0) {
      quantSize=m_rowIsSelected.size();
      for (index=0; index<quantSize; index++)
        m_rowIsSelected[index].assign(m_numRows, 0);
    }
    m_numSelRows=0;
    printLog(0, "All rows unselected");
  }
  return IS_OK;
}


/**********************************************************************/
// set and apply a cut on given quantity such that all values >= cutVal pass
// double is not const because it can be locally modified to NO_SEL_CUT
int Catalog::setLowerCut(const std::string name, double cutVal) {

  const std::string origin="setLowerCut";
  int quantSize=checkImport(origin, true);
  if (quantSize < IS_VOID) return quantSize;
  int index=checkQuant_name(origin, name);
  if (index < 0) return index;
  if (m_quantities.at(index).m_type != Quantity::NUM) {  
    std::string errText;
    errText="given Quantity name ("+name+") is not of NUM type";
    printWarn(origin, errText);
    return BAD_QUANT_TYPE;
  }
  // if cut is the same: do nothing
  if (cutVal >= NO_SEL_CUT) cutVal=NO_SEL_CUT;
  if (cutVal == m_quantities.at(index).m_lowerCut) return IS_OK;
  m_quantities.at(index).m_lowerCut=cutVal;

  /* if no data: exit */
  if (m_numRows == 0) return IS_OK;

  std::vector<bool> isSelected;
  bool oneCriteria=existCriteria(&isSelected);
  bool check=true;
  if (cutVal >= NO_SEL_CUT) {
    printLog(1, "Disabling lower cut (on "+name+")");
    if (!oneCriteria) {
      if (m_numSelRows > 0) {
        quantSize=m_rowIsSelected.size();
        for (index=0; index<quantSize; index++)
          m_rowIsSelected[index].assign(m_numRows, 0);
      }
      m_numSelRows=0;
      check=false;
      printLog(0, "All rows unselected");
    }
  }
  else {
    std::ostringstream sortie;
    sortie << "Enabling lower cut (" <<cutVal<<" on "<<name<< ")";
    printLog(1, sortie.str());  
  }
  
  if (check) {
    bool reject=m_quantities[index].m_rejectNaN;
    int  k,
         pos=m_quantities[index].m_index;
    unsigned long test=bitPosition(index, &k);
    m_numSelRows=0;
    for (long i=0; i<m_numRows; i++) {

      check=checkNUM(m_numericals[pos].at(i), index, reject);
      // setting required bit
      if (check)
        m_rowIsSelected[k].at(i)|= test;
      else
        m_rowIsSelected[k].at(i)&= (Max_Test-test);
      if (rowSelect(i, isSelected) == true) m_numSelRows++;
 
    }// loop on rows
  }

  return IS_OK;
}
/**********************************************************************/
// set and apply a cut on given quantity such that all values <= cutVal pass
// double is not const because it can be locally modified to NO_SEL_CUT
int Catalog::setUpperCut(const std::string name, double cutVal) {

  const std::string origin="setUpperCut";
  int quantSize=checkImport(origin, true);
  if (quantSize < IS_VOID) return quantSize;
  int index=checkQuant_name(origin, name);
  if (index < 0) return index;
  if (m_quantities.at(index).m_type != Quantity::NUM) {  
    std::string errText;
    errText="given Quantity name ("+name+") is not of NUM type";
    printWarn(origin, errText);
    return BAD_QUANT_TYPE;
  }
  // if cut is the same: do nothing
  if (cutVal >= NO_SEL_CUT) cutVal=NO_SEL_CUT;
  if (cutVal == m_quantities.at(index).m_upperCut) return IS_OK;
  m_quantities.at(index).m_upperCut=cutVal;

  /* if no data: exit */
  if (m_numRows == 0) return IS_OK;

  std::vector<bool> isSelected;
  bool oneCriteria=existCriteria(&isSelected);
  bool check=true;
  if (cutVal >= NO_SEL_CUT) {
    printLog(1, "Disabling upper cut (on "+name+")");
    if (!oneCriteria) {
      if (m_numSelRows > 0) {
        quantSize=m_rowIsSelected.size();
        for (index=0; index<quantSize; index++)
          m_rowIsSelected[index].assign(m_numRows, 0);
      }
      m_numSelRows=0;
      check=false;
      printLog(0, "All rows unselected");
    }
  }
  else {
    std::ostringstream sortie;
    sortie << "Enabling upper cut (" <<cutVal<<" on "<<name<< ")";
    printLog(1, sortie.str());  
  }
  
  if (check) {
    bool reject=m_quantities[index].m_rejectNaN;
    int  k,
         pos=m_quantities[index].m_index;
    unsigned long test=bitPosition(index, &k);
    m_numSelRows=0;
    for (long i=0; i<m_numRows; i++) {

      check=checkNUM(m_numericals[pos].at(i), index, reject);
      // setting required bit
      if (check)
        m_rowIsSelected[k].at(i)|= test;
      else
        m_rowIsSelected[k].at(i)&= (Max_Test-test);
      if (rowSelect(i, isSelected) == true) m_numSelRows++;
 
    }// loop on rows
  }

  return IS_OK;
}


/**********************************************************************/
// erase all non-selected rows from memory
int Catalog::eraseNonSelected() {

  const std::string origin="eraseNonSelected";
  if (m_numRows <= 0) {
    printWarn(origin, "catalog is empty");
    return IS_VOID; 
  }
  // test below avoid deleting NOTHING
  if (m_numRows == m_numSelRows) {
    printWarn(origin, "all rows selected, nothing done");
    return IS_OK; 
  }
  // log if ALL is deleted
  std::string text;
  if (m_numSelRows == 0) {
    text="no row selected, calling deleteContent()";
    printLog(2, text);
    deleteContent();
    return IS_OK;
  }
  try {
    long i, tot=0;
    int j;
    int sizeS=m_strings.size();
    int sizeN=m_numericals.size();
    int vecSize=m_rowIsSelected.size();
    // to speed-up, will not change (avoid reading size in loop)

    std::vector<std::vector<double> > myNum;
    myNum.resize(sizeN);
    for (j=0; j<sizeN; j++) myNum[j].assign(m_numSelRows, 0.0);
    std::vector<std::vector<std::string> > myStr;
    myStr.resize(sizeS);
    for (j=0; j<sizeS; j++) myStr[j].assign(m_numSelRows, "");
    std::vector<std::vector<unsigned long> > myBits;
    myBits.resize(vecSize);
    for (j=0; j<vecSize; j++) myBits[j].assign(m_numSelRows, 0ul);
    for (i=0; i<m_numRows; i++) {
      if (m_rowIsSelected[0].at(i) & 1) {
        for (j=0; j<sizeN; j++) myNum[j].at(tot)=m_numericals[j].at(i);
        for (j=0; j<sizeS; j++) myStr[j].at(tot)=m_strings[j].at(i);
        for (j=0; j<vecSize; j++) myBits[j].at(tot)=m_rowIsSelected[j].at(i);
        if (++tot == m_numSelRows) break; // to speed up
      }
    }
    for (j=0; j<sizeN; j++) {
      m_numericals[j].insert(m_numericals[j].begin(),
                             myNum[j].begin(), myNum[j].end());
      myNum[j].clear();
    }
    myNum.clear();
    for (j=0; j<sizeS; j++) {
      m_strings[j].insert(m_strings[j].begin(),
                          myStr[j].begin(), myStr[j].end());
      myStr[j].clear();
    }
    myStr.clear();
    for (j=0; j<vecSize; j++)  {
      m_rowIsSelected[j].insert(m_rowIsSelected[j].begin(),
                                myBits[j].begin(), myBits[j].end());
      myBits[j].clear();
    }
    myBits.clear();
/* if (!keepCriteria) {
    for (j=0; j<vecSize; j++) m_rowIsSelected[j].assign(m_numSelRows, 0);
    vecSize=m_quantities.size();
    for (i=0; i<vecSize; i++) {
      m_quantities.at(i).m_listValS.clear();
      m_quantities.at(i).m_lowerCut=NO_SEL_CUT;
      m_quantities.at(i).m_upperCut=NO_SEL_CUT;
      m_quantities.at(i).m_listValN.clear();
    }
    m_selRegion=false;
*/
/*  THIS METHOD IS VERY VERY LONG 
    std::vector< std::vector<std::string>::iterator > stringIter;
    std::vector< std::vector<double>::iterator > doubleIter;
    for (j=0; j<sizeS; j++) stringIter.push_back(m_strings[j].begin());
    for (j=0; j<sizeN; j++) doubleIter.push_back(m_numericals[j].begin());
    if (keepCriteria) {

      std::vector< std::vector<unsigned long>::iterator > myIter;
      for (j=0; j<vecSize; j++) myIter.push_back(m_rowIsSelected[j].begin());
      for (i=0; i<m_numRows; i++) {
        if (!(*myIter[0] & 1)) {
          for (j=0; j<sizeS; j++) m_strings[j].erase(stringIter[j]);
          for (j=0; j<sizeN; j++) m_numericals[j].erase(doubleIter[j]);
          for (j=0; j<vecSize; j++) m_rowIsSelected[j].erase(myIter[j]);
        }
        else {
          for (j=0; j<sizeS; j++) stringIter.at(j)++;
          for (j=0; j<sizeN; j++) doubleIter.at(j)++;
          for (j=0; j<vecSize; j++) myIter.at(j)++;
        }
      }
      stringIter.clear();
      doubleIter.clear();
      myIter.clear();
*/
  }
  catch (std::exception &prob) {
    text="EXCEPTION erasing m_strings, m_numericals or m_rowIsSelected: ";
    text=text+prob.what();
    printErr(origin, text);
    throw;
  }
  std::ostringstream sortie;
  sortie << m_numRows-m_numSelRows << " row(s) deleted";
  printLog(0, sortie.str());
  m_numRows=m_numSelRows;
  //if (!keepCriteria) m_numSelRows=0;
  return IS_OK;
}
/**********************************************************************/
// erase all selected rows from memory
int Catalog::eraseSelected() {

  const std::string origin="eraseSelected";
  if (m_numRows <= 0) {
    printWarn(origin, "catalog is empty");
    return IS_VOID; 
  }
  // test below avoid deleting NOTHING
  if (m_numSelRows == 0) {
    printWarn(origin, "no row selected, nothing done");
    return IS_OK; 
  }
  // log if ALL is deleted
  std::string text;
  if (m_numRows == m_numSelRows) {
    text="all rows selected, calling deleteContent()";
    printLog(2, text);
    deleteContent();
    return IS_OK;
  }
  try {
    long i, tot=0,
         numRows=m_numRows-m_numSelRows;
    int j;
    int sizeS=m_strings.size();
    int sizeN=m_numericals.size();
    int vecSize=m_rowIsSelected.size();
    // to speed-up, will not change (avoid reading size in loop)

    std::vector<std::vector<double> > myNum;
    myNum.resize(sizeN);
    for (j=0; j<sizeN; j++) myNum[j].assign(numRows, 0.0);
    std::vector<std::vector<std::string> > myStr;
    myStr.resize(sizeS);
    for (j=0; j<sizeS; j++) myStr[j].assign(numRows, "");
    std::vector<std::vector<unsigned long> > myBits;
    myBits.resize(vecSize);
    for (j=0; j<vecSize; j++) myBits[j].assign(numRows, 0ul);
    for (i=0; i<m_numRows; i++) {
      if (!(m_rowIsSelected[0].at(i) & 1)) {
        for (j=0; j<sizeN; j++) myNum[j].at(tot)=m_numericals[j].at(i);
        for (j=0; j<sizeS; j++) myStr[j].at(tot)=m_strings[j].at(i);
        for (j=0; j<vecSize; j++) myBits[j].at(tot)=m_rowIsSelected[j].at(i);
        if (++tot == numRows) break; // to speed up
      }
    }
    for (j=0; j<sizeN; j++) {
      m_numericals[j].insert(m_numericals[j].begin(),
                             myNum[j].begin(), myNum[j].end());
      myNum[j].clear();
    }
    myNum.clear();
    for (j=0; j<sizeS; j++) {
      m_strings[j].insert(m_strings[j].begin(),
                          myStr[j].begin(), myStr[j].end());
      myStr[j].clear();
    }
    myStr.clear();
    for (j=0; j<vecSize; j++)  {
      m_rowIsSelected[j].insert(m_rowIsSelected[j].begin(),
                                myBits[j].begin(), myBits[j].end());
      myBits[j].clear();
    }
    myBits.clear();
/* if (!keepCriteria) {
    for (j=0; j<vecSize; j++) m_rowIsSelected[j].assign(numRows, 0);
    vecSize=m_quantities.size();
    for (i=0; i<vecSize; i++) {
      m_quantities.at(i).m_listValS.clear();
      m_quantities.at(i).m_lowerCut=NO_SEL_CUT;
      m_quantities.at(i).m_upperCut=NO_SEL_CUT;
      m_quantities.at(i).m_listValN.clear();
    }
    m_selRegion=false;
*/
  }
  catch (std::exception &prob) {
    text="EXCEPTION erasing m_strings, m_numericals or m_rowIsSelected: ";
    text=text+prob.what();
    printErr(origin, text);
    throw;
  }
  std::ostringstream sortie;
  sortie << m_numSelRows << " row(s) deleted";
  printLog(0, sortie.str());
  m_numRows=m_numRows-m_numSelRows;
  m_numSelRows=0;
  return IS_OK;
}

/**********************************************************************/
// select rows depending on the given string list
int Catalog::doSelS(const std::string name, const std::string origin,
                    const std::vector<std::string> &stringList, bool exact) {

  int quantSize=checkImport(origin, true);
  if (quantSize < IS_VOID) return quantSize;
  std::ostringstream sortie;
  int code,
      index=checkQuant_name(origin, name);
  if (index < 0) return index;
  if (m_quantities.at(index).m_type != Quantity::STRING) {  
    std::string errText;
    errText="given Quantity name ("+name+") is not of STRING type";
    printWarn(origin, errText);
    return BAD_QUANT_TYPE;
  }
  if (origin == "useOnlyS") {
    m_quantities[index].m_includeList=true;
    code=1;
    sortie << "Include rows";
  }
  else {
    m_quantities[index].m_includeList=false;
    code=0;
    sortie << "Exclude rows";
  }
  //m_quantities[index].m_caselessList=-exact; //how to negate ?
  m_quantities[index].m_listValS.clear();
  quantSize=stringList.size();
  int j;
  int (*pfunc)(int)=tolower; // function used by transform
  std::string mot;
  if (exact) {
    m_quantities[index].m_caselessList=false;
    for (j=0; j<quantSize; j++)
      m_quantities[index].m_listValS.push_back(stringList.at(j));
  }
  else {
    m_quantities[index].m_caselessList=true;
    for (j=0; j<quantSize; j++) {
      mot=stringList.at(j);
      transform(mot.begin(), mot.end(), mot.begin(), pfunc);
      m_quantities[index].m_listValS.push_back(mot);
    }
  }
  #ifdef DEBUG_CAT
  for (j=0; j<quantSize; j++)
    std::cout << m_quantities[index].m_listValS[j] << "|";
  std::cout <<"caseless="<< m_quantities[index].m_caselessList << std::endl;
  #endif

  /* if no data: exit */
  if (m_numRows == 0) return IS_OK;

  std::vector<bool> isSelected;
  bool oneCriteria=existCriteria(&isSelected);
  bool check=true;
  if (quantSize == 0) {
    printLog(1, "Disabling list selection (on "+name+")");
    if (!oneCriteria) {
      if (m_numSelRows > 0) {
        code=m_rowIsSelected.size(); // to avoid compiler Warning
        for (j=0; j<code; j++)
          m_rowIsSelected[j].assign(m_numRows, 0);
      }
      m_numSelRows=0;
      check=false;
      printLog(0, "All rows unselected");
    }
  }
  else {
    sortie << " with \"" << name <<"\" string in list ("
           << quantSize << " elements, ";
    if (exact) sortie << "exact match)"; else sortie << "caseless match)"; 
    printLog(1, sortie.str());
  }
  
  if (check) {
    int pos=m_quantities[index].m_index;
    unsigned long test=bitPosition(index, &j);
    m_numSelRows=0;
    for (long i=0; i<m_numRows; i++) {

      mot=m_strings[pos].at(i);
      if (!exact) transform(mot.begin(), mot.end(), mot.begin(), pfunc);
      check=checkSTR(mot, index, code);
      // setting required bit
      if (check)
        m_rowIsSelected[j].at(i)|= test;
      else
        m_rowIsSelected[j].at(i)&= (Max_Test-test);
      if (rowSelect(i, isSelected) == true) m_numSelRows++;
 
    }// loop on rows
  }

  return IS_OK;
}
/**********************************************************************/
// only include rows which have string value in the given list
int Catalog::useOnlyS(const std::string name,
                      const std::vector<std::string> &stringList, bool exact) {

  int done=doSelS(name, "useOnlyS", stringList, exact);
  return done;
}
/**********************************************************************/
// exclude all rows which have string value in the given list
int Catalog::excludeS(const std::string name,
                      const std::vector<std::string> &stringList, bool exact) {

  int done=doSelS(name, "excludeS", stringList, exact);
  return done;
}


/**********************************************************************/
} // namespace catalogAccess
