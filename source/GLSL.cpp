///                                                                           
/// Langulus::Module::Assets::Materials                                       
/// Copyright (c) 2016 Dimo Markov <team@langulus.com>                        
/// Part of the Langulus framework, see https://langulus.com                  
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "GLSL.hpp"
#include <Anyness/Edit.hpp>
#include <Flow/Verbs/Catenate.hpp>

#define GLSL_VERBOSE(a) LANGULUS(NOOP)


/// Create GLSL from shader template                                          
///   @param stage - the shader stage to use as template                      
///   @return the GLSL code                                                   
GLSL GLSL::Template(Offset stage) {
   LANGULUS_ASSUME(DevAssumes, stage < ShaderStage::Counter, "Bad index");
   return Templates[stage];
}

/// Check if a character is blank space                                       
///   @param c - the character to test                                        
///   @return true if character is space                                      
bool GLSL::IsOperator(char c) {
   switch (c) {
   case ';': case '.': case ',': case '!': case ':': case '?':
   case '+': case '-': case '*': case '/': case '=': case '|':
   case '[': case ']': case '(': case ')': case '{': case '}':
   case '<': case '>': case '~': case '"': case '\'':
      return true;
   default:
      return false;
   }
}

/// Check if a #define exists for a symbol                                    
///   @param symbol - the definition to search for                            
///   @return true if definition exists                                       
bool GLSL::IsDefined(const Token& symbol) const {
   return FindKeyword(Text {"#define ", symbol}) != IndexNone;
}

/// Find an isolated token                                                    
///   @param symbol - the definition to search for                            
///   @return the index of the first match, or IndexNone if not found         
Index GLSL::FindKeyword(const Text& symbol) const {
   if (not symbol)
      return IndexNone;

   Index progress;
   while ((progress = FindBlock(symbol, progress))) {
      // Match found                                                    
      // For it to be keyword, its left side must be either the start,  
      // isspace, an operator, or:                                      
      //  - a number, if the symbol starts with a letter                
      if (progress == 0) {
         // LHS is valid, lets check RHS                                
         // For it to be keyword, its right must be either the end,     
         // isspace, or an operator                                     
         if (symbol.GetCount() >= GetCount()) {
            // Found                                                    
            return progress;
         }
         else {
            const char rhs = (*this)[symbol.GetCount()];
            if (IsSpace(rhs) or IsOperator(rhs))
               return progress;
         }
      }
      else {
         const char lhs = (*this)[progress - 1];
         if (IsSpace(lhs) or IsOperator(lhs) or (IsAlpha(symbol[0]) and IsDigit(lhs))) {
            // LHS is valid, lets check RHS                             
            // For it to be keyword, its right must be either the end,  
            // isspace, or an operator                                  
            if (progress + symbol.GetCount() >= GetCount()) {
               // Found                                                 
               return progress;
            }
            else {
               const char rhs = (*this)[progress + symbol.GetCount()];
               if (IsSpace(rhs) or IsOperator(rhs))
                  return progress;
            }
         }
      }

      progress += symbol.GetCount();
   }

   return IndexNone;
}

/// Generate a log-friendly pretty version of the code, with line             
/// numbers, colors, highlights                                               
///   @return the pretty GLSL code                                            
Text GLSL::Pretty() const {
   if (IsEmpty())
      return {};

   const auto linescount = GetLineCount();
   const auto linedigits = CountDigits(linescount);
   Text result {"\n"};
   Count line = 1;
   Offset lstart = 0;
   Offset lend = 0;

   for (Offset i = 0; i <= mCount; ++i) {
      if (i == mCount or (*this)[i] == '\n') {
         const auto size = lend - lstart;

         // Insert line number                                          
         auto segment = result.Extend(size + linedigits + 3);

         Offset lt = 0;
         for (; lt < linedigits - CountDigits(line); ++lt)
            segment.GetRaw()[lt] = ' ';

         ::std::to_chars(segment.GetRaw() + lt, segment.GetRaw() + linedigits, line);
         segment.GetRaw()[linedigits] = '|';
         segment.GetRaw()[linedigits + 1] = ' ';

         // Insert line text                                            
         if (size)
            ::std::memcpy(segment.GetRaw() + linedigits + 2, GetRaw() + lstart, size);

         // Add new-line character at the end                           
         segment.GetRaw()[size + linedigits + 2] = '\n';

         ++line;
         lstart = lend = i + 1;
      }
      else if ((*this)[i] == '\0')
         break;
      else
         ++lend;
   }

   result.Last() = '\0';
   return result;
}

/// Add a definition to code                                                  
///   @param definition - the definition to add                               
///   @return a reference to this code                                        
GLSL& GLSL::Define(const Token& definition) {
   const Text defined {"#define ", definition, '\n'};
   if (FindKeyword(defined))
      return *this;

   Edit(this).Select(ShaderToken::Defines) >> defined;
   return *this;
}

/// Set GLSL version for the code                                             
///   @param version - the version string                                     
///   @return a reference to this code                                        
GLSL& GLSL::SetVersion(const Token& version) {
   const Text defined {"#version ", version, '\n'};
   if (FindKeyword(defined))
      return *this;

   Edit(this).Select(ShaderToken::Version) >> defined;
   return *this;
}
