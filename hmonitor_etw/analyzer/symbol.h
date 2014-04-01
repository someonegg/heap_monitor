/*
 *  File   : analyzer/symbol.h
 *  Author : Zhuyie
 *  Date   : 2014/03/30 22:02:03
 *  Brief  :
 *
 *  $Id: $
 */
#ifndef __ANALYZER_SYMBOL_H__
#define __ANALYZER_SYMBOL_H__


bool initSymbol();
void termSymbol();
bool symLoadModule(const char* dllName);
unsigned int getFuncInfo(const char* moduleName, unsigned int offset, char info[512]);


#endif /* __ANALYZER_SYMBOL_H__ */
