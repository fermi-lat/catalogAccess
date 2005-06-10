/**
 * @file   quantity.cxx
 * @brief  Implementation for the Quantity class.
 * Contains only the copy constructor definition and the 3 methods
 * to print results (error, warning or log) on screen (via cout
 * for log, or cerr).
 *
 * @author A. Sauvageon
 *
 * $Header $
 */

#include "quantity.h"

namespace catalogAccess {

/**********************************************************************/
/*  GLOBAL FUNCTION for catalogAccess                                 */
/**********************************************************************/
int verbosity;

void printErr(const std::string origin, const std::string text) {
  std::cerr << "ERROR catalogAccess (IN " << origin << ") "
            << text << std::endl;
}
void printWarn(const std::string origin, const std::string text) {
  if (verbosity > 0)
  std::cerr << "WARNING catalogAccess (IN " << origin << "): "
            << text << std::endl;
}
void printLog(const int level, const std::string text) {
  if (verbosity+level >= 4)
  std::cout << "LOG_" << (int)level << " (catalogAccess): "
            << text << std::endl;
}

/**********************************************************************/
// Copy constructor needed to allocate arrays in copy
Quantity::Quantity(const Quantity & q) {

  #ifdef DEBUG_CAT
  std::cout << "!! DEBUG Quantity COPY constructor on: "
            << q.m_name <<" (ucd="<< q.m_ucd <<")"<< std::endl;
  #endif
//try {
  m_name   =q.m_name;
  m_comment=q.m_comment;
  m_ucd    =q.m_ucd;
  m_format =q.m_format;
  m_type   =q.m_type;
  m_unit   =q.m_unit;
  m_index  =q.m_index;
  m_isGeneric =q.m_isGeneric;
  m_statError =q.m_statError;
  m_sysError  =q.m_sysError;
  int vecSize, i;
  std::string errText;
  try {
    vecSize=q.m_vectorQs.size();
    for (i=0; i<vecSize; i++) m_vectorQs.push_back(q.m_vectorQs.at(i));
  }
  catch (const std::exception &err) {
    errText=std::string("EXCEPTION on m_vectorQs[]: ")+err.what();
    printErr("Quantity copy constructor", errText);
    throw;
  }
  // selection criteria
  m_lowerCut    =q.m_lowerCut;
  m_upperCut    =q.m_upperCut;
  m_precision   =q.m_precision;
  m_rejectNaN   =q.m_rejectNaN;
  m_cutORed     =q.m_cutORed;
  m_excludeList =q.m_excludeList;
  try {
    vecSize=q.m_listValS.size();
    for (i=0; i<vecSize; i++) m_listValS.push_back(q.m_listValS.at(i));
    vecSize=q.m_listValN.size();
    for (i=0; i<vecSize; i++) m_listValN.push_back(q.m_listValN.at(i));
  }
  catch (const std::exception &err) {
    errText=std::string("EXCEPTION on selection list: ")+err.what();
    printErr("Quantity copy constructor", errText);
    throw;
  }
/* line is commented on purpose to TEMINATE the program on EXCEPTION */
//} catch (...) { printErr("copy constructor", ""); }
}


} // namespace catalogAccess
